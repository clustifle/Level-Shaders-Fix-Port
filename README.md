# Level Shaders Fix — experimental port

Unofficial adaptation of [ConfiG's Level Shaders Fix](https://github.com/cgytrus/LevelShadersFix).
Uses native-resolution shader rendering with the original shader resources.

## Build status

- **GD 2.2081 / Geode 5.11.2:** Android 32-bit and 64-bit builds have passed.
  Windows x64 build is being checked.
- **GD 2.209 / 2.2091:** source profiles only. Builds against SDK 5.11.2 are blocked:
  the bindings require CCTouchClipper/CCKeyboardActionManager headers and newer
  virtual interfaces absent from that SDK. No working package for these versions
  is claimed.
- The math tests pass on GitHub Actions. **No in-game visual or performance test
  has been completed.** A compiled artifact is an experimental test build.

[Actions builds](https://github.com/clustifle/Level-Shaders-Fix-Port/actions)
provide per-platform artifact ZIPs. Failed matrix jobs do not invalidate the
separate successful artifacts. The initial full matrix is recorded in
[run 39372958420](https://github.com/clustifle/Level-Shaders-Fix-Port/actions/runs/39372958420).

## Build without installing local C++ tools

Open **Actions → Experimental port builds → Run workflow**. The default profile
is **2.2081**, built for Windows x64 and Android armeabi-v7a/arm64-v8a.
Pushes to main also build this default profile.

Download the artifact matching your game version and platform. Extract its ZIP
to obtain the .geode file. These are development artifacts, not published releases.
Selecting a 2.209+ profile currently stops with a clear SDK compatibility error;
a compatible SDK/loader must be reviewed before enabling those builds.

## Test installation

1. Back up saves and close Geometry Dash.
2. Disable the original Level Shaders Fix if installed.
3. For GD 2.2081 Windows, copy the extracted Windows .geode into that game's
   geode/mods folder. Do not use an Android package on Windows.
4. For Android, import the package matching your device ABI using Geode.
5. Start the game and confirm the experimental mod is enabled.
6. Follow TESTING.md before claiming rendering correctness or a performance gain.

Only install one package for this mod ID at a time. Packages are exact-version
builds; no automatic support for future GD updates is claimed.

## Implementation

The port retains upstream GLSL shaders, replaces the framebuffer only after
shader compilation/linking and allocation succeed, validates dimensions and GPU
limits, and restores temporary state with RAII. It uses current named game
fields and guards pixelation against a zero divisor.

Camera/pixelation correction is applied around visit() rather than hooking
prepareTargetContainer(). The latter has no Windows address in the reviewed
2.209/2.2091 bindings. Native init is preserved; setupShader() is not hooked.
Framebuffer/program/frame-size changes are checked during rendering.

The broader visit correction is experimental: other rendering hooks may observe
the temporary state. Newer shader calculations, camera rotation, split screen,
pixelation and Android context restoration need visual tests. The old oversized
render-target regression has not been verified on these game builds. Native
resolution can also cost more on high-resolution displays.

## Local builds (optional)

Install the [Geode development prerequisites](https://github.com/geode-sdk/docs/blob/main/getting-started/cpp-stuff.md),
Geode CLI, SDK 5.11.2, platform development binaries, CMake 3.25+ and Ninja.
Windows requires a compatible C++ compiler, MSVC libraries and Windows SDK.
Android requires the NDK.

Set GEODE_SDK and GEODE_BINDINGS_REPO_PATH to the SDK and bindings checkouts.
The bindings commit is pinned in PROVENANCE.json. Then run:

```powershell
./tools/build.ps1 -GameVersion 2.2081 -Target Win64 -BindingsPath C:/dev/bindings
./tools/build.ps1 -GameVersion 2.2081 -Target Android64 -BindingsPath C:/dev/bindings -AndroidNdk C:/Android/ndk
./tools/build.ps1 -GameVersion 2.2081 -Target Android32 -BindingsPath C:/dev/bindings -AndroidNdk C:/Android/ndk
```

The staging script writes a profile-specific manifest. SDK configuration sets
both GEODE_GD_VERSION and GEODE_COMP_GD_VERSION to select matching layouts.
No game files are automatically installed by the build scripts.

Checks:

```text
node tools/check-source.mjs
cmake -S tests -B build-math
cmake --build build-math
ctest --test-dir build-math --output-on-failure
```

The Node audit checks field names and hook addresses, not ABI or runtime safety.
Original author, source pins and upstream licensing status: THIRD_PARTY.md.
