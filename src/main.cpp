// Experimental GD 2.209 / 2.2091 adaptation of ConfiG's Level Shaders Fix.
// Upstream: https://github.com/cgytrus/LevelShadersFix
#include <Geode/Geode.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include "RenderMath.hpp"
#include <cmath>
#include <numbers>
#include <string_view>

using namespace geode::prelude;

#ifndef LSF_TARGET_GD
#error "Select an explicit LSF_TARGET_GD build profile."
#endif

class $modify(LSFShaderLayer, ShaderLayer) {
    struct Fields {
        Ref<CCGLProgram> program;
        CCRenderTexture* configuredTexture = nullptr; // borrowed; ShaderLayer retains it
        GLint cameraRotationUniform = -1;
        bool failed = false;
        bool inVisit = false;
        int width = 0;
        int height = 0;
    };

    static void onModify(auto& self) {
        // Forward compatibility in the loader is not a layout/ABI guarantee.
        // Do not register active detours for an unreviewed game version.
        if (Loader::get()->getGameVersion() != std::string(LSF_TARGET_GD)) {
            for (auto* hook : self.getAllHooks()) hook->setAutoEnable(false);
            log::warn("Level Shaders Fix: build targets GD {}, running {}. Hooks disabled.",
                LSF_TARGET_GD, Loader::get()->getGameVersion());
        }
    }

    bool init() {
        if (!ShaderLayer::init()) return false;
        // Native init supplies the current game's scalar fields and defaults.
        // No setupShader hook is required (its Windows address is missing in 2.209).
        configureNativeTarget();
        return true;
    }

    bool configureNativeTarget() {
        if (m_fields->failed) return false;
        auto* director = CCDirector::get();
        if (!director || !director->getOpenGLView() || !m_sprite || !m_renderTexture)
            return false;
        auto visible = director->getVisibleSize();
        auto frame = director->getOpenGLView()->getFrameSize() * utils::getDisplayFactor();
        auto dimensions = lsf::dimensions(frame.width, frame.height);
        if (!dimensions || !lsf::positive(visible.width) || !lsf::positive(visible.height))
            return false;
        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (dimensions->width > maxTextureSize || dimensions->height > maxTextureSize) {
            log::warn("Native shader target {}x{} exceeds GPU limit {}; retaining game renderer.",
                dimensions->width, dimensions->height, maxTextureSize);
            m_fields->failed = true;
            return false;
        }

        if (!m_fields->program || !glIsProgram(m_fields->program->getProgram())) {
            auto vertex = file::readString(Mod::get()->getResourcesDir() / "pp-vert.glsl");
            auto fragment = file::readString(Mod::get()->getResourcesDir() / "pp-frag.glsl");
            if (!vertex || !fragment) {
                log::error("Level Shaders Fix: shader resources unavailable; retaining game renderer.");
                m_fields->failed = true;
                return false;
            }
            auto* candidate = new CCGLProgram();
            // Compile and link before changing any live renderer state.
            if (!candidate->initWithVertexShaderByteArray(
                    vertex.unwrap().c_str(), fragment.unwrap().c_str())) {
                candidate->release();
                m_fields->failed = true;
                log::error("Level Shaders Fix: shader compilation failed; retaining game renderer.");
                return false;
            }
            candidate->addAttribute("a_position", 0);
            candidate->addAttribute("a_color", 1);
            candidate->addAttribute("a_texCoord", 2);
            if (!candidate->link()) {
                candidate->release();
                m_fields->failed = true;
                log::error("Level Shaders Fix: shader link failed; retaining game renderer.");
                return false;
            }
            candidate->updateUniforms();
            m_fields->program = candidate;
            candidate->release(); // Fields::program owns a reference
        }

        CCRenderTexture* replacement = nullptr;
        {
            lsf::RestoreValue scale(director->m_fContentScaleFactor, 1.f);
            replacement = CCRenderTexture::create(dimensions->width, dimensions->height,
                kCCTexture2DPixelFormat_RGBA8888);
        }
        if (!replacement || !replacement->getSprite() ||
            !replacement->getSprite()->getTexture()) {
            m_fields->failed = true;
            log::error("Level Shaders Fix: render target allocation failed; retaining game renderer.");
            return false;
        }
        // All fallible work is finished. Keep the existing sprite and its ownership.
        replacement->retain();
        auto* previous = m_renderTexture;
        m_renderTexture = replacement;
        m_shader = m_fields->program;
        m_sprite->setTexture(replacement->getSprite()->getTexture());
        m_sprite->setTextureRect(CCRect(-1.f, -1.f, visible.width + 2.f, visible.height + 2.f));
        m_sprite->setPosition(ccp(0.f, 0.f));
        m_sprite->setAnchorPoint(ccp(0.f, 0.f));
        m_sprite->setFlipY(true);
        m_sprite->setShaderProgram(m_shader);
        previous->release();

        // Preserve the upstream shader's square logical coordinate space while
        // allocating a rectangular framebuffer. The shaders perform the UV remap.
        float diagonal = std::floor(std::hypot(visible.width, visible.height));
        m_textureContentSize = CCSize(diagonal, diagonal);
        m_targetTextureSize = visible;
        m_targetTextureSizeExtra = CCSize(0.f, 0.f);
        auto screenScale = director->getScreenScaleFactorW();
        m_shockWaveTimeMult = visible.width / diagonal;
        m_shockWaveTimeMult /= 480.f / std::hypot(480.f, 320.f);
        m_shockWaveTimeMult /= lsf::positive(screenScale) ? screenScale : 1.f;

        bindPortUniforms();
        auto remap = m_shader->getUniformLocationForName("_robHack");
        m_shader->setUniformLocationWith2f(remap, visible.width / diagonal, visible.height / diagonal);
        m_fields->cameraRotationUniform = m_shader->getUniformLocationForName("_camRot");
        m_shader->setUniformLocationWith2f(m_fields->cameraRotationUniform, 1.f, 0.f);
        m_configuredAntiAlias = !GameManager::get()->getGameVariable("0155");
        toggleAntiAlias(m_configuredAntiAlias);
        m_fields->configuredTexture = m_renderTexture;
        m_fields->width = dimensions->width;
        m_fields->height = dimensions->height;
        log::info("Level Shaders Fix: experimental native target {}x{} (GD {})",
            dimensions->width, dimensions->height, LSF_TARGET_GD);
        return true;
    }

    void visit() {
        if (m_fields->inVisit) {
            ShaderLayer::visit();
            return;
        }
        lsf::RestoreValue visiting(m_fields->inVisit, true);
        auto* director = CCDirector::get();
        if (!director || !director->getOpenGLView()) {
            ShaderLayer::visit();
            return;
        }
        auto frame = director->getOpenGLView()->getFrameSize() * utils::getDisplayFactor();
        auto size = lsf::dimensions(frame.width, frame.height);
        bool changed = m_renderTexture != m_fields->configuredTexture ||
            !m_fields->program || m_shader != m_fields->program.data() ||
            !size || size->width != m_fields->width || size->height != m_fields->height ||
            !glIsProgram(m_fields->program->getProgram());
        if (changed && !configureNativeTarget() &&
            (!m_fields->configuredTexture || m_renderTexture != m_fields->configuredTexture ||
                m_shader != m_fields->program.data())) {
            ShaderLayer::visit();
            return;
        }
        if (!m_renderTexture || !m_shader || !m_sprite) {
            ShaderLayer::visit();
            return;
        }
        auto target = m_renderTexture->getSprite()->getTexture()->getContentSizeInPixels();
        auto window = director->getWinSize();
        if (!lsf::positive(window.height)) {
            ShaderLayer::visit();
            return;
        }

        // Widen the original prepareTargetContainer correction to the visit scope.
        // This avoids a missing Windows function binding. It requires visual QA,
        // especially when other rendering mods hook this same visit.
        auto* parent = m_gameLayer ?
            (m_state.m_minBlendingLayer < 2 ? m_gameLayer->m_objectParent :
                m_gameLayer->m_inShaderParent) : nullptr;
        float rotation = parent ? parent->getRotation() : 0.f;
        struct RestoreRotation {
            CCNode* node;
            float rotation;
            ~RestoreRotation() { if (node) node->setRotation(rotation); }
        } restoreRotation{parent, rotation};
        if (parent) parent->setRotation(0.f);
        float radians = rotation * (std::numbers::pi_v<float> / 180.f);
        m_shader->setUniformLocationWith2f(m_fields->cameraRotationUniform,
            std::cos(radians), std::sin(radians));

        // Recompute scales after a resize or native renderer recreation.
        prePixelateShader();
        float nativeScale = director->getContentScaleFactor();
        lsf::RestoreValue pixelX(m_state.m_pixelateTargetX,
            lsf::pixelSize(target.width, m_textureContentSize.width,
                m_state.m_pixelateTargetX) / (lsf::positive(nativeScale) ? nativeScale : 1.f));
        lsf::RestoreValue pixelY(m_state.m_pixelateTargetY,
            lsf::pixelSize(target.height, m_textureContentSize.height,
                m_state.m_pixelateTargetY) / (lsf::positive(nativeScale) ? nativeScale : 1.f));
        lsf::RestoreValue scale(director->m_fContentScaleFactor, target.height / window.height);
        ShaderLayer::visit();
    }

    void prePixelateShader() {
        // Before configuration, or after a failed setup, preserve native behavior.
        if (!m_fields->configuredTexture || m_renderTexture != m_fields->configuredTexture ||
            m_shader != m_fields->program.data()) {
            ShaderLayer::prePixelateShader();
            return;
        }
        bool tweening = m_state.m_tweenActions.contains(9) ||
            m_state.m_tweenActions.contains(10) ||
            getActionByTag(9) || getActionByTag(10);
        bool enabled = m_state.m_pixelateTargetX > 1.f ||
            m_state.m_pixelateTargetY > 1.f || tweening;
        m_state.m_pixelatePixelating = enabled;
        if (enabled) m_state.m_usesShaders = true;
        if (m_state.m_pixelateTargetX <= 1.f && m_state.m_pixelateTargetY <= 1.f) {
            toggleAntiAlias(m_configuredAntiAlias);
            m_state.m_textureScaleX = 1.f;
            m_state.m_textureScaleY = 1.f;
            m_renderTexture->updateInternalScale(0.f, 0.f);
            m_sprite->setScaleX(1.f);
            m_sprite->setScaleY(1.f);
            return;
        }
        toggleAntiAlias(!m_state.m_pixelateHardEdges);
        auto target = m_renderTexture->getSprite()->getTexture()->getContentSizeInPixels();
        float zoom = m_state.m_pixelateRelative && m_gameLayer && m_gameLayer->m_objectLayer ?
            std::abs(m_gameLayer->m_objectLayer->getScale()) : 1.f;
        float x = std::max(1.f, zoom * lsf::pixelSize(target.width,
            m_textureContentSize.width, m_state.m_pixelateTargetX));
        float y = std::max(1.f, zoom * lsf::pixelSize(target.height,
            m_textureContentSize.height, m_state.m_pixelateTargetY));
        m_state.m_textureScaleX = 1.f / x;
        m_state.m_textureScaleY = 1.f / y;
        m_renderTexture->updateInternalScale(1.f / x, 1.f / y);
        m_sprite->setScaleX(x);
        m_sprite->setScaleY(y);
    }

    void bindPortUniforms() {
        // setupCommonUniforms
        m_textureScaleUniform = m_shader->getUniformLocationForName("_textureScale");
        m_textureScaleInvUniform = m_shader->getUniformLocationForName("_textureScaleInv");
        m_shaderPositionUniform = m_shader->getUniformLocationForName("_shaderPosition");
        m_blurRefColorUniform = m_shader->getUniformLocationForName("_blurRefColor");
        m_blurUseRefUniform = m_shader->getUniformLocationForName("_blurUseRef");
        m_blurIntensityUniform = m_shader->getUniformLocationForName("_blurIntensity");
        m_blurFadeUniform = m_shader->getUniformLocationForName("_blurFade");
        m_blurOnlyEmptyUniform = m_shader->getUniformLocationForName("_blurOnlyEmpty");

        // setupShockWaveUniforms
        m_shockWaveTimeUniform = m_shader->getUniformLocationForName("_shockWaveTime");
        m_shockWaveTime1Uniform = m_shader->getUniformLocationForName("_shockWaveTime1");
        m_shockWaveTime2Uniform = m_shader->getUniformLocationForName("_shockWaveTime2");
        m_shockWaveTime3Uniform = m_shader->getUniformLocationForName("_shockWaveTime3");
        m_shockWaveTime4Uniform = m_shader->getUniformLocationForName("_shockWaveTime4");
        m_shockWaveStrengthUniform = m_shader->getUniformLocationForName("_shockWaveStrength");
        m_shockWaveWavesUniform = m_shader->getUniformLocationForName("_shockWaveWaves");
        m_shockWaveCenterUniform = m_shader->getUniformLocationForName("_shockWaveCenter");
        m_shockWaveInvertUniform = m_shader->getUniformLocationForName("_shockWaveInvert");
        m_shockWaveMinSizeUniform = m_shader->getUniformLocationForName("_shockWaveMinSize");
        m_shockWaveMaxSizeUniform = m_shader->getUniformLocationForName("_shockWaveMaxSize");
        m_shockWaveMaxDistValUniform = m_shader->getUniformLocationForName("_shockWaveMaxDistVal");

        // setupShockLineUniforms
        m_shockLineTimeUniform = m_shader->getUniformLocationForName("_shockLineTime");
        m_shockLineTime1Uniform = m_shader->getUniformLocationForName("_shockLineTime1");
        m_shockLineTime2Uniform = m_shader->getUniformLocationForName("_shockLineTime2");
        m_shockLineTime3Uniform = m_shader->getUniformLocationForName("_shockLineTime3");
        m_shockLineTime4Uniform = m_shader->getUniformLocationForName("_shockLineTime4");
        m_shockLineAxisUniform = m_shader->getUniformLocationForName("_shockLineAxis");
        m_shockLineDirectionUniform = m_shader->getUniformLocationForName("_shockLineDirection");
        m_shockLineDualUniform = m_shader->getUniformLocationForName("_shockLineDual");
        m_shockLineWavesUniform = m_shader->getUniformLocationForName("_shockLineWaves");
        m_shockLineStrengthUniform = m_shader->getUniformLocationForName("_shockLineStrength");
        m_shockLineCenterUniform = m_shader->getUniformLocationForName("_shockLineCenter");
        m_shockLineMaxDistValUniform = m_shader->getUniformLocationForName("_shockLineMaxDistVal");

        // setupGlitchUniforms
        m_glitchBotUniform = m_shader->getUniformLocationForName("_glitchBot");
        m_glitchTopUniform = m_shader->getUniformLocationForName("_glitchTop");
        m_glitchXOffsetUniform = m_shader->getUniformLocationForName("_glitchXOffset");
        m_glitchColOffsetUniform = m_shader->getUniformLocationForName("_glitchColOffset");
        m_glitchRndUniform = m_shader->getUniformLocationForName("_glitchRnd");

        // setupChromaticUniforms
        m_chromaticXOffUniform = m_shader->getUniformLocationForName("_chromaticXOff");
        m_chromaticYOffUniform = m_shader->getUniformLocationForName("_chromaticYOff");

        // setupChromaticGlitchUniforms
        m_cGRGBOffsetUniform = m_shader->getUniformLocationForName("_cGRGBOffset");
        m_cGYOffsetUniform = m_shader->getUniformLocationForName("_cGYOffset");
        m_cGTimeUniform = m_shader->getUniformLocationForName("_cGTime");
        m_cGStrengthUniform = m_shader->getUniformLocationForName("_cGStrength");
        m_cGHeightUniform = m_shader->getUniformLocationForName("_cGHeight");
        m_cGLineThickUniform = m_shader->getUniformLocationForName("_cGLineThick");
        m_cGLineStrengthUniform = m_shader->getUniformLocationForName("_cGLineStrength");

        // setupLensCircleShader
        m_lensCircleOriginUniform = m_shader->getUniformLocationForName("_lensCircleOrigin");
        m_lensCircleStartUniform = m_shader->getUniformLocationForName("_lensCircleStart");
        m_lensCircleEndUniform = m_shader->getUniformLocationForName("_lensCircleEnd");
        m_lensCircleStrengthUniform = m_shader->getUniformLocationForName("_lensCircleStrength");
        m_lensCircleTintUniform = m_shader->getUniformLocationForName("_lensCircleTint");
        m_lensCircleAdditiveUniform = m_shader->getUniformLocationForName("_lensCircleAdditive");

        // setupRadialBlurShader
        m_radialBlurCenterUniform = m_shader->getUniformLocationForName("_radialBlurCenter");
        m_radialBlurValueUniform = m_shader->getUniformLocationForName("_radialBlurValue");

        // setupBulgeShader
        m_bulgeValueUniform = m_shader->getUniformLocationForName("_bulgeValue");
        m_bulgeValue2Uniform = m_shader->getUniformLocationForName("_bulgeValue2");
        m_bulgeOriginUniform = m_shader->getUniformLocationForName("_bulgeOrigin");
        m_bulgeRadiusUniform = m_shader->getUniformLocationForName("_bulgeRadius");

        // setupBulgeShader
        m_pinchValueUniform = m_shader->getUniformLocationForName("_pinchValue");
        m_pinchCenterPosUniform = m_shader->getUniformLocationForName("_pinchCenterPos");
        m_pinchCalcUniform = m_shader->getUniformLocationForName("_pinchCalc1");
        m_pinchRadiusUniform = m_shader->getUniformLocationForName("_pinchRadius");

        // setupMotionBlurShader
        m_motionBlurValueUniform = m_shader->getUniformLocationForName("_motionBlurValue");
        m_motionBlurMultUniform = m_shader->getUniformLocationForName("_motionBlurMult");
        m_motionBlurDualUniform = m_shader->getUniformLocationForName("_motionBlurDual");

        // setupGrayscaleShader
        m_grayscaleValueUniform = m_shader->getUniformLocationForName("_grayscaleValue");
        m_grayscaleTintUniform = m_shader->getUniformLocationForName("_grayscaleTint");
        m_grayscaleUseLumUniform = m_shader->getUniformLocationForName("_grayscaleUseLum");

        // setupSepiaShader
        m_sepiaValueUniform = m_shader->getUniformLocationForName("_sepiaValue");

        // setupInvertColorShader
        m_invertColorValueUniform = m_shader->getUniformLocationForName("_invertColorValue");

        // setupHueShiftShader
        m_hueShiftCosAUniform = m_shader->getUniformLocationForName("_hueShiftCosA");
        m_hueShiftSinAUniform = m_shader->getUniformLocationForName("_hueShiftSinA");

        // setupColorChangeShader
        m_colorChangeCUniform = m_shader->getUniformLocationForName("_colorChangeC");
        m_colorChangeBUniform = m_shader->getUniformLocationForName("_colorChangeB");

        // setupSplitScreenShader
        m_rowmodUniform = m_shader->getUniformLocationForName("_rowmod");
        m_colmodUniform = m_shader->getUniformLocationForName("_colmod");
        m_rowmodCalcUniform = m_shader->getUniformLocationForName("_rowmodCalc");
        m_colmodCalcUniform = m_shader->getUniformLocationForName("_colmodCalc");
        m_splitXStartUniform = m_shader->getUniformLocationForName("_splitXStart");
        m_splitXRangeUniform = m_shader->getUniformLocationForName("_splitXRange");
        m_splitXRangeMultUniform = m_shader->getUniformLocationForName("_splitXRangeMult");
        m_splitYStartUniform = m_shader->getUniformLocationForName("_splitYStart");
        m_splitYRangeUniform = m_shader->getUniformLocationForName("_splitYRange");
        m_splitYRangeMultUniform = m_shader->getUniformLocationForName("_splitYRangeMult");
        m_screenAspectUniform = m_shader->getUniformLocationForName("_screenAspect");
        m_screenAspectINVUniform = m_shader->getUniformLocationForName("_screenAspectINV");
    }
};
