# Acceptance checklist — no tests below have been run in-game

Use backed-up saves and a separate development profile. Record GD version,
loader version, mod build profile, ABI, GPU/driver, resolution and other mods.
Keep the original Level Shaders Fix disabled.

## First establish the baseline

1. Run the same shader-heavy level with vanilla/stock Geode rendering.
2. Record actual render-target dimensions, GPU frame time and visual output.
3. Check whether the old oversized-square target issue still exists.
4. Compare the port with identical graphics settings; do not infer performance
   from allocation size alone.

## Platform matrix

| Game | Windows x64 | Android arm64-v8a | Android armeabi-v7a |
| --- | --- | --- | --- |
| 2.209 | Build/load/visual/performance pending | Pending | Pending |
| 2.2091 | Build/load/visual/performance pending | Pending | Pending |

All six native builds must compile and link. Confirm the .geode manifest and
binary architecture match the target. Reaching the main menu is not sufficient.

## Visual and lifecycle cases

- Every shader family alone, then combined triggers and animated transitions.
- Invert RGB/clamping, split screen, grayscale tint and blur blending layers.
- Camera rotation 0, 45, 90, 180 and 360 degrees, with zoom and shader targets.
- Below/above shader layers, particles, editor playtest and platformer camera.
- Pixelate X/Y alone and together; hard edges, relative zoom, snap grid,
  values below one and values larger than the logical texture dimension.
- Level start, death, practice checkpoints, restart, pause, resume and completion.
- Window resize, fullscreen change, low/medium/high quality and 16:9/ultrawide.
- Android background/foreground, lock/unlock, GL context loss and orientation.
- Missing shaders, compile/link failure and GPU allocation failure: initial
  failures should retain the native renderer; active-target recovery needs review.
- Other shader/rendering mods: compare hook ordering and shared temporary state.
- Unknown future GD version: hooks should remain disabled. Do not bypass this
  guard merely to make the mod appear compatible.

## Report a failure

Include the Geode crash log or compiler output, exact build profile, ABI, level
ID or reproducible local level, trigger values, screenshot/video and whether the
same issue occurs without the port. A successful source audit does not replace
any of these tests.
