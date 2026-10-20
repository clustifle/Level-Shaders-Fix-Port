param(
    [ValidateSet('2.2081','2.209','2.2091')][string]$GameVersion = '2.2091',
    [ValidateSet('Win64','Android32','Android64')][string]$Target = 'Win64',
    [string]$AndroidNdk = $env:ANDROID_NDK_HOME,
    [string]$BindingsPath = $env:GEODE_BINDINGS_REPO_PATH
)
$ErrorActionPreference = 'Stop'
foreach ($tool in @('node','cmake','geode')) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Install $tool before building." }
}
if (-not $env:GEODE_SDK) { throw 'Set GEODE_SDK to the Geode 5.11.2 SDK directory.' }
if (-not $BindingsPath) { throw 'Provide -BindingsPath pointing to the pinned bindings checkout described in README.md.' }
$portRoot = Split-Path $PSScriptRoot -Parent
$bindingsFile = Join-Path $BindingsPath "bindings/$GameVersion/GeometryDash.bro"
if (-not (Test-Path -LiteralPath $bindingsFile)) { throw "Missing $bindingsFile" }
$stage = & node (Join-Path $PSScriptRoot 'stage.mjs') $GameVersion
if ($LASTEXITCODE -ne 0) { throw 'Source staging failed.' }
$build = Join-Path $portRoot "build/$GameVersion/$Target"
$configure = @('-S', $stage, '-B', $build, '-G', 'Ninja',
    '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_C_COMPILER=clang',
    '-DCMAKE_CXX_COMPILER=clang++', "-DLSF_TARGET_GD=$GameVersion",
    '-DLSF_ACKNOWLEDGE_EXPERIMENTAL=ON', "-DGEODE_TARGET_PLATFORM=$Target",
    "-DGEODE_BINDINGS_REPO_PATH=$BindingsPath",
    '-DGEODE_CODEGEN_CMAKE_ARGS=-DCMAKE_C_COMPILER=clang;-DCMAKE_CXX_COMPILER=clang++;-G Ninja')
if ($Target -like 'Android*') {
    if (-not $AndroidNdk) { throw 'Supply -AndroidNdk or ANDROID_NDK_HOME.' }
    $abi = if ($Target -eq 'Android64') { 'arm64-v8a' } else { 'armeabi-v7a' }
    $configure += @("-DCMAKE_TOOLCHAIN_FILE=$AndroidNdk/build/cmake/android.toolchain.cmake",
        "-DANDROID_ABI=$abi", '-DANDROID_PLATFORM=android-23', '-DANDROID_STL=c++_shared')
}
& cmake @configure
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed. No working binary has been produced.' }
& cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Compilation failed. See compiler output.' }
Get-ChildItem -LiteralPath $build -Filter '*.geode' -Recurse | Select-Object FullName,Length
