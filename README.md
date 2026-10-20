# Level Shaders Fix Port — recovery alpha.2

**The first experimental renderer (alpha.1) failed runtime testing. Do not use it.**

The user observed a cropped scene and severe FPS loss on GD 2.2081. Investigation
found an incompatible render-size/scale assumption that repeatedly rebuilt the
framebuffer. See [RECOVERY.md](RECOVERY.md).

## What alpha.2 does

**Alpha.2 is a recovery rollback. It installs no rendering hooks and retains the
native 2.2081 renderer. It does not implement a new performance fix.**
Disabling or removing alpha.1 is also sufficient to remove its replacement path.

The original 2.206 mod's square-target assumptions do not match the inspected
2.2081 pipeline, which already allocates a rectangular target. A future optimizer
needs a new measured baseline instead of importing that old replacement.

## Get a recovery package

Open [Actions](https://github.com/clustifle/Level-Shaders-Fix-Port/actions),
choose **Experimental port builds**, then select the newest alpha.2 build.
Packages are built for Windows x64 and Android armeabi-v7a/arm64-v8a with
**GD 2.2081 / Geode 5.11.2**. A successful compile does not establish runtime FPS.

Close GD, replace the alpha.1 .geode with alpha.2 for your platform, then restart.
Both versions use the same mod ID; install only one package.
See RECOVERY.md for the retest checklist.

## Newer versions

2.209 and 2.2091 source profiles remain blocked by missing/newer Cocos interfaces
in SDK 5.11.2. No compatible package for those versions is claimed.

## Development

The workflow defaults to 2.2081 and builds on pushes to main or manual dispatch.
No local Visual Studio installation is needed to use GitHub Actions.

Local development still requires the Geode SDK/CLI and a supported native
toolchain. Run tools/build.ps1 for Windows/Android after setting GEODE_SDK and
GEODE_BINDINGS_REPO_PATH. The reviewed bindings commit is in PROVENANCE.json.

The source audit checks that the recovery package has no renderer detours,
custom resources or frame-rate controls. The retained standalone math tests
cover historical render math only and cannot validate a shader pipeline.

Upstream author and source/licensing records: THIRD_PARTY.md.
