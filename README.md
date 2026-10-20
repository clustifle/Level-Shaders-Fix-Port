# Level Shaders Fix — GD 2.209 / 2.2091 port work

20 October update: an additional experimental **2.2081** profile is available.
Use `./tools/build.ps1 -GameVersion 2.2081 -Target Win64` after installing
the development prerequisites below. Its source audit passed, but the local
CMake attempt stopped because no C++ compiler is installed. See
BUILD_ATTEMPT_22081.md. The existing 2.209/2.2091 profiles remain available.

**Status: experimental source, not a compiled or runtime-verified mod.**
Targets: Windows Steam x64, Android arm64-v8a and armeabi-v7a.
Baseline: Geode SDK/loader 5.11.2. There is no installable .geode in this package.

## Changes implemented

- Retains ConfiG's fragment and vertex shaders and native-resolution rectangular target.
- Uses the current named objectParent/inShaderParent fields. Parent selection follows
  the current resetTargetContainer binding's minBlendingLayer rule.
- Moves the camera and temporary pixelation correction to the visit scope.
  This avoids the missing Windows prepareTargetContainer address in both profiles.
- Calls native init first and then replaces its framebuffer transactionally.
  It does not hook setupShader, whose Windows address is absent in the reviewed 2.209 profile.
- Keeps native initialization of newer ShaderLayer scalars instead of guessing them.
- Validates dimensions/GPU limits and shader compilation/linking before replacing
  live resources. Failed initial setup leaves the native renderer in place.
- Clamps the pixelation cell divisor to at least one and restores temporary values with RAII.
- Detects changed framebuffer/program/physical frame dimensions on visit and attempts
  reconfiguration. If a resize allocation fails while an old custom target is valid,
  it keeps that target; this is degraded rendering, not successful recovery.
- Uses a distinct mod ID and blocks coexistence with the original.
- Provides separate exact-version builds. Unknown runtime versions disable all hooks.

## Source compatibility versus loader compatibility

Geode 5.11.2's stock SDK defaults to GD 2.2081. Simply declaring gd=2.2091
does not change generated addresses or field layouts. This project sets
GEODE_GD_VERSION to the selected target before adding the SDK, using the pinned
2.209/2.2091 bindings. This is an experimental SDK override, not evidence that
the installed loader or its linked game libraries support every target.

The 5.11.2 binary libraries and runtime exports still require a real build/load
check against these generated bindings. On Android, exported ShaderLayer symbol
availability must also be checked for both ABIs. On Windows, only hooks with
known addresses are used. No addresses were guessed.

Forward compatibility in Geode is not enough to make later GD versions safe.
Each new version needs a reviewed binding profile, a separately compiled package
and in-game testing. There is deliberately no wildcard GD support.

## Build prerequisites

Install Git, Node.js 22+, CMake 3.25+, Ninja, LLVM/Clang with C++23,
the Geode CLI and the Geode 5.11.2 SDK plus the platform development binaries.
Windows additionally needs Visual Studio C++ build tools and Windows SDK.
Android needs the NDK and Geode's libraries for the chosen ABI.
Use the Geode SDK documentation for SDK installation:
https://docs.geode-sdk.org/getting-started/sdk/

Pin your SDK to v5.11.2. Pin a separate geode-sdk/bindings checkout to:
587989d2b95d8f0e943e73b5a8fee352ab2ac3c4
Point GEODE_SDK and GEODE_BINDINGS_REPO_PATH to those checkouts.
The commit records and selected class declarations are bundled for review.

### Windows PowerShell

Open a terminal with clang, Ninja and the Windows C++ toolchain available:

```powershell
./tools/build.ps1 -GameVersion 2.209 -Target Win64 -BindingsPath C:/dev/bindings
./tools/build.ps1 -GameVersion 2.2091 -Target Win64 -BindingsPath C:/dev/bindings
```

### Android from PowerShell

```powershell
./tools/build.ps1 -GameVersion 2.2091 -Target Android64 -BindingsPath C:/dev/bindings -AndroidNdk C:/Android/ndk
./tools/build.ps1 -GameVersion 2.2091 -Target Android32 -BindingsPath C:/dev/bindings -AndroidNdk C:/Android/ndk
```

Each command stages a version-specific manifest without modifying the source.
No files are automatically installed into your game.
Outputs, if compilation succeeds, are under build/<GD>/<Target>/.

For Android on Linux, stage using node tools/stage.mjs 2.2091, then configure
the staged folder with the NDK toolchain, ANDROID_ABI, ANDROID_PLATFORM=android-23,
ANDROID_STL=c++_shared, GEODE_TARGET_PLATFORM=Android64 or Android32,
LSF_TARGET_GD=2.2091 and LSF_ACKNOWLEDGE_EXPERIMENTAL=ON.
The bundled manual GitHub Actions workflow expresses all six target combinations.
It has not been executed here and does not publish releases.

## Checks

```text
node tools/check-source.mjs
cmake -S tests -B build-math
cmake --build build-math
ctest --test-dir build-math --output-on-failure
```

The Node audit checks field names and Windows hook availability against the
bundled declarations. It cannot prove ABI compatibility or rendering correctness.
The C++ math tests exercise invalid dimensions, extreme pixelation and nested
state restoration. They require a C++ compiler.

## Runtime testing still required

See TESTING.md. The visit-scope substitution is the principal behavioral risk:
it holds zero camera rotation and remapped pixelation for longer than the
original prepareTargetContainer hook. Shader updates or other render mods may
observe these temporary values. Newer native shader calculations also use
screenSize/aspect/scale fields absent from the original mod, so effect parity
must be checked rather than assumed.

The existence of the old performance bug on 2.209/2.2091 is not yet established.
Measure vanilla/native rendering before claiming that this port fixes a current
regression. Native-resolution rendering can cost more than the original target
on high-resolution displays. No FPS improvement is claimed.

Android background/foreground GL context restoration and framebuffer/NPOT UV
behavior need physical-device tests. The retained upstream shader may also
differ from newer game effects. Do not describe this as a finished port.

Original author, source pins and redistribution status: THIRD_PARTY.md.
