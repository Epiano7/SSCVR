# Validation record

This record separates checks that can be automated from comfort and visual checks that require a physical headset.

## Skillshot City v1.990 revision 8 compatibility

Validated statically and in an isolated installer copy on August 26, 2026:

- Internal Steam build: `24945856`
- Executable size: `15,173,632` bytes
- Executable SHA-256: `B5AFB4081D8F13733DA55BBB10DC0751E1D4F4F1C4F3B15EC11308A6CDE4C3A2`
- Gameplay-render RVA: `0x1EAD40`
- World-draw RVA: `0x316D70`
- Culling-bounds RVA: `0xF8B0E8`

The gameplay and world functions retain their verified prologues and corresponding implementations. The relocated world function reads the four culling floats at `0xF8B0E8` through `0xF8B0F4` in the same coordinate-comparison patterns as the earlier build. The exact executable was then accepted by a freshly rebuilt launcher and isolated installer.

## Skillshot City v1.990 revision 5 compatibility

Validated on August 24, 2026:

- Internal Steam build: `24877062`
- Executable size: `15,171,072` bytes
- Executable SHA-256: `3FE88430ADFD97C5F32ED2BFC4612FF918C2FE856BB57A3BB72B3BAB7C0CDA1E`
- Gameplay-render RVA: `0x1EACE0`
- World-draw RVA: `0x316B30`
- Culling-bounds RVA: `0xF8B060`

The two function prologues and their surrounding implementations were compared with the frozen v17 build before the new RVAs were added. The runtime checks both prologues before writing either hook. The launcher and installer also require the exact executable size and SHA-256; unknown builds are rejected.

## Automated results

- The exact-build test accepted all three known executables and rejected altered hook signatures.
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
- For `v1.990` revision 8, all 2,164 original Steam files matched the new isolated copy byte-for-byte. Its only seven extra game-directory files were the expected SSCVR runtime and generated configuration files. The launcher selected the new exact hash, and the normal-network regression test remained enabled.

## Still requires a headset

Simulator and desktop checks cannot establish comfort, perceived depth, text readability, or whether every live game interface is visible in both eyes. Skillshot City `v1.990` revision 8 still needs a physical Quest Link singleplayer pass before it should be treated as a fully accepted build.
