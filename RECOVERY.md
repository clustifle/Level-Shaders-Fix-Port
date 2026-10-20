# Recovery from alpha.1

Version 0.1.0-alpha.1 passed compilation but failed the user's Windows 2.2081
runtime test: a cropped upper-right scene, black remainder and a large FPS drop.

## Findings

- Runtime logs contained 179 custom render-target configuration messages in the
  inspected session, including many configurations within a single second.
- GD 2.2081 ShaderLayer::update compares m_targetTextureSize with m_screenSize
  and calls native setupShader when they differ.
- Alpha.1 set the target to logical visible dimensions while retaining the native
  physical screen dimensions. The game and mod repeatedly replaced each other's
  target.
- Native visit sets the director content scale from m_scaleFactor internally.
  The alpha.1 visit wrapper's temporary director scale was overwritten.
- Native 2.2081 setup already creates a rectangular target. The assumptions from
  the original 2.206 mod cannot be transplanted into that pipeline unchanged.

These findings were checked against the user's 2.2081 executable (known PE
timestamp 1768959392) at the function locations recorded in the pinned bindings.
No new native addresses were guessed.

## Recovery behavior

Alpha.2 removes the entire custom renderer, shader remapping and render hooks.
It emits one load-time status message and otherwise leaves the game renderer
intact. It performs no per-frame GL checks, allocations, pixelation or camera
transform changes, and sets no FPS or VSync options.

This is a rollback/recovery package, not a completed performance fix.
Removing/disable alpha.1 also restores the native renderer without installing
this package. Alpha.2 is supplied to replace the faulty experimental version
under the same mod ID and make its inactive renderer status explicit.

## Install and verify

1. Close Geometry Dash.
2. Replace the old .geode under the same ID with the alpha.2 package for your ABI,
   or disable/remove alpha.1 in Geode.
3. Restart the game. Check that alpha.2 is the loaded version.
4. Repeat the same scene with identical game/display settings.
5. Confirm the crop is gone and repeated custom target messages no longer occur.
6. Compare FPS with the mod disabled. A precise 60 FPS plateau needs a separate
   refresh/VSync/frame-limit check; no FPS recovery is claimed before retesting.

2.209/2.2091 remain blocked by Geode 5.11.2 SDK interface incompatibilities.
