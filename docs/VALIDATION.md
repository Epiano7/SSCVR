# Validation record

This record separates checks that can be automated from comfort and visual checks that require a physical headset.

## Steam build 24877062 compatibility

Validated on August 24, 2026:

- Executable size: `15,171,072` bytes
- Executable SHA-256: `3FE88430ADFD97C5F32ED2BFC4612FF918C2FE856BB57A3BB72B3BAB7C0CDA1E`
- Gameplay-render RVA: `0x1EACE0`
- World-draw RVA: `0x316B30`
- Culling-bounds RVA: `0xF8B060`

The two function prologues and their surrounding implementations were compared with the frozen v17 build before the new RVAs were added. The runtime checks both prologues before writing either hook. The launcher and installer also require the exact executable size and SHA-256; unknown builds are rejected.

## Automated results

- The exact-build test accepted both known executables and rejected altered hook signatures.
- The camera, stereo reprojection, and HUD geometry test passed.
- The network behavior test passed with normal game network access unchanged.
- The release ZIP's double-click installer payload copied the current Steam build into an isolated directory and selected `Steam build 24877062`.
- All 2,164 original Steam game files matched the isolated copy byte-for-byte; the only seven additional files were the expected SSCVR runtime and generated configuration files.
- Every installed runtime file matched its packaged source by SHA-256.
- The installed VR configuration and calibration files matched the v17 defaults byte-for-byte.
- The copied system OpenGL library matched the Windows System32 source by SHA-256.
- A recursive before/after hash inventory found zero changes in the Steam game directory.
- The installed launcher verified the copied executable successfully.
- The corrected `sscvr-install.json` manifest and installed uninstall entry points were present. The manifest-guarded uninstaller removed only the isolated validation copy; a second full Steam inventory still matched byte-for-byte afterward.
- A no-input game smoke test selected the current layout and installed the gameplay and peripheral-culling hooks. It initialized both eye swapchains, the interface swapchain, WGL/D3D interop, HUD and cursor compositors, reported 72 Hz, remained responsive, and closed through the test marker.
- A 3,600-render-frame Meta XR Simulator regression published 1,800 coherent stereo pairs and 1,300 heavy-interface frames with zero matching XR, device-removal, or interop errors.
- A clean GitHub source checkout rebuilt successfully. Its package repeated the zero-change Steam inventory and byte-for-byte runtime-install checks, then selected the current layout, installed both live hooks, reported 72 Hz, remained responsive, and closed cleanly in Meta XR Simulator.

## Still requires a headset

Simulator and desktop checks cannot establish comfort, perceived depth, text readability, or whether every live game interface is visible in both eyes. The current Steam build still needs a physical Quest Link singleplayer pass before it should be treated as a fully accepted build.
