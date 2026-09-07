# Validation record

This record separates checks that can be automated from comfort and visual checks that require a physical headset.

## Skillshot City v1.991 September 3 revision compatibility

Validated statically and in an isolated installer copy on September 3, 2026:

- Internal Steam build: `25098773`
- Executable size: `15,221,248` bytes
- Executable SHA-256: `487962A3A057A7DF6574DB98536518985AF18DA6A66C145FC23F6AC35F54D154`
- Gameplay-render RVA: `0x1EBC50`
- World-draw RVA: `0x31BDD0`
- Culling-bounds RVA: `0xF968B8`

The relocated gameplay and world functions retain their verified prologues and corresponding implementations. The world function reads the four culling floats at `0xF968B8` through `0xF968C4` in the same repeated coordinate-comparison patterns as the earlier supported builds.

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

- The exact-build test accepted all four known executables and rejected altered hook signatures.
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
- For `v1.991`, all 2,120 original Steam files matched the isolated installer copy byte-for-byte. Its only seven extra game-directory files were the expected SSCVR runtime and generated configuration files.
- Two consecutive cold Meta XR Simulator launches selected the `v1.991` layout, installed both live geometry hooks at a safe frame boundary, created both eye swapchains and the interface swapchain, reported a 72 Hz runtime cadence, remained responsive, and closed cleanly through SSCVR's test marker. No new Meta XR Simulator crash report or matching XR/device/interop error was produced.
- The final `0.1.2-alpha` ZIP contained no game files. A fresh extraction installed into a second isolated directory, reproduced all 2,120 Steam files with no missing or changed file, reproduced every packaged runtime binary byte-for-byte, selected the `v1.991` manifest entry, and passed another clean simulator launch with both hooks active.
- A 2,400-frame synthetic OpenXR integration run published 1,200 coherent stereo pairs and exercised 700 heavy-interface frames with no matching XR, device-removal, or interop error. Meta XR Simulator stalled during test-process teardown after the passing result was written; the real-game cold-launch and package-smoke cycles all closed cleanly.

## Still requires a headset

### September 5 interface clipping regression (unreleased)

A hidden simulator regression reproduced stale interface pixels when the game
entered capture with a small scissor rectangle. Before the fix, 1,399 of 2,400
capture frames failed the clear-pixel check despite zero alpha-write-mask
failures. The same inherited clip also restricted the transfer into the
OpenXR menu texture. Internal clears and transfers now temporarily disable
scissor clipping and enable all color channels, then restore the game state.

The corrected 2,400-frame test produced 1,200 stereo pairs and 700 heavy-menu
frames, with zero clear, mask, scissor-restoration, capture, publication, or
proof-archive failures. Submitted pixel checks passed for opaque RGB, partial
alpha, transparent borders, UI aspect/size, aligned left/right edges, and two
different menu captures.

The final candidate then completed a 12,000-render-frame hidden simulator soak:
6,000 stereo pairs, 5,500 heavy-interface frames, and 55 menu enter/exit cycles.
All capture-state counters remained zero and both submitted-image proofs
passed. The lowest logged coherent-pair rate was 34.5 pairs/s; the simulator
reported a 72 Hz XR cadence. Those rates are synthetic test measurements, not
a physical-game performance claim.

Archived Singleplayer Tab and Escape interfaces were replayed through the
current bridge over a synthetic stereo world without launching the game.
Each replay completed 2,400 frames and checked 7,788 reference points per
capture across two captures. Maximum captured RGB error against a two-stage
linear-resampling reference was 2/255 for Tab and 1/255 for Escape; alpha error
was at most 1/255. Sampled opaque submitted colors differed by at most 1/255
from the reference, with zero sampled left/right opaque-color difference.
These results test archived game pixels, not new live game/menu entry.

The gameplay hook now requests early capture for an interface classified as
heavy on the previous frame, covering the missing request for non-keyboard
overlays. Previously only explicit Tab/Escape requested that capture despite
a comment claiming level-up/results used it on the next frame. This timing
change still needs a live level-up/results regression; classification continues
to use the existing draw-count heuristic for those overlays.

Physical Link stability, a complete live Singleplayer round, and headset UI
comfort/readability remain unverified. No public release was made.

### September 3 in-round freeze/menu investigation (unreleased)

An actual isolated Singleplayer / Survival Challenge simulator run reproduced
a frozen stereo image while OpenXR continued submitting at 72 Hz. WGL interop
lock failures coincided with fresh stereo-pair production falling to zero.
Submission rate by itself therefore must not be used as a stability pass.

The desktop mirror now retains a normal OpenGL texture instead of reading old
contents through a WRITE_DISCARD shared-eye lock. Two subsequent real-game
warmup-map/menu runs retained fresh pairs for approximately 6.7 and 4.3 minutes,
with no matching interop/XR/device-removal error. These were menu stability
checks, not full combat or physical-Link acceptance.

Settled Tab and Escape captures also proved that menu RGB was present while
alpha was zero. The game's incoming color-write mask was `1/1/1/0`. Captured
interface rendering now enables alpha writes and restores the original mask
afterward. Both final per-eye screenshots then contained the actual in-round
menus. The integration test now reproduces disabled incoming alpha writes and
checks capture override and restoration. An additional sRGB conversion prevents
menu colors from being encoded twice on sRGB headset targets.

The final candidate completed 12,000 synthetic render frames, 6,000 published
stereo pairs and 55 menu cycles, with zero alpha-mask override/restoration
failures or matching interop/XR/device-removal errors. A subsequent actual
singleplayer warmup-map run stayed live for approximately 4.3 minutes, including
Escape/Tab screenshots from both submitted eyes, short player movement inputs,
and another Tab entry afterward. The menus remained visible with corrected
colors. Minor HUD artifacts and non-VR HUD framing remain; these tests are not
a full interface-quality or combat-performance sign-off.

The self-contained Shift+1 diagnostic generator was exercised twice in one
2,400-frame integration run. Each request produced submitted-eye, side-by-side,
overlay, red/cyan, amplified-difference, approximate-lens BMPs, and a text
report without blocking the OpenXR frame thread. The second set reported
1,536x1,608 per eye, a 1,536x864 interface rectangle, and 98.596% nearly equal
pixels in the synthetic stereo scene. The radial lens preview is intentionally
labelled approximate because it cannot reproduce Meta's private physical-lens
calibration.

The corrected repeated-capture path was then exercised in one actual isolated
Singleplayer / Survival Challenge session. Escape generated a fresh set at
16:30:18 with a 1,536x862 interface rectangle and 59.279% nearly equal pixels;
after returning to gameplay, Tab generated a different fresh set at 16:30:54
with the same interface rectangle and 46.213% nearly equal pixels. The submitted
left/right textures, overlays, and difference images showed both menus in both
eyes: head-locked menu edges aligned while the world retained its expected
stereo displacement. File hashes differed between the Escape and Tab sets,
confirming that the second request did not reuse the first capture. This is
useful software-side evidence, but still is not a through-lens Quest test.

No release containing these changes has been published. Physical-Link freeze
resolution, menu comfort/readability, and a full round still require acceptance.

Simulator and desktop checks cannot establish comfort, perceived depth, text readability, or whether every live game interface is visible in both eyes. Skillshot City `v1.991` still needs a physical Quest Link singleplayer pass before it should be treated as a fully accepted build.

### September 5 tournament follow-up (unreleased)

The tournament logs separated two failure classes. Recording increased Meta
Link encoder pressure, but the repeated headset freezes also contained USB
transport failures while the game continued rendering on the monitor. The
previous bridge did not stop its OpenXR worker or destroy its swapchains,
spaces, session, or instance when the game closed, which could leave the next
Link connection waiting on stale application state.

The proxy now intercepts `wglDeleteContext` and performs a bounded clean
shutdown before the owning OpenGL context disappears. It requests session
exit, services the STOPPING event, joins the frame worker, unregisters shared
WGL/D3D resources, and destroys the OpenXR objects. Shutdown is idempotent, so
a repeated call is harmless. If the worker cannot stop within two seconds,
cleanup fails closed instead of releasing resources beneath a live thread.

The round-complete path also no longer submits one identical desktop image
through two different eye projections. When gameplay geometry stops, stale
stereo/UI state is cleared and the completed menu is presented as the same
monoscopic quad used at startup. Tab and Escape remain in-round overlays and
continue to use the stereo-world plus HUD-safe interface path.

Fresh 2,400- and 4,800-render-frame simulator runs passed. The longer run
published 2,400 coherent pairs and exercised 1,900 heavy-interface frames with
zero capture, alpha-mask, scissor-restoration, publication, or proof-archive
failures. It observed the gameplay-to-menu monoscopic handoff, sustained a
72 Hz synthetic XR cadence with a minimum 36 coherent pairs/s, passed submitted
pixel checks, and completed two shutdown calls with `stopped cleanly` status.
This proves software lifecycle and submitted-texture behavior in the simulator;
it does not prove USB stability, through-lens comfort, or physical Quest Link
recovery.
