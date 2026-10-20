// Recovery build: the legacy replacement is incompatible with GD 2.2081.
// This mod intentionally installs no rendering hooks. See RECOVERY.md.
#include <Geode/Geode.hpp>
#include <string>

using namespace geode::prelude;

#ifndef LSF_TARGET_GD
#error "Select an explicit LSF_TARGET_GD build profile."
#endif

$execute {
    log::info("Level Shaders Fix recovery alpha.2: native renderer retained; no shader/rendering hooks installed.");
    auto gameVersion = Loader::get()->getGameVersion();
    if (gameVersion != std::string(LSF_TARGET_GD)) {
        log::warn("Recovery package targets GD {}; detected {}. Native renderer remains unchanged.",
            LSF_TARGET_GD, gameVersion);
    }
}
