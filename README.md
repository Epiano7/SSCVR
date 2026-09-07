# SSCVR

SSCVR is an early VR mod for Skillshot City. It renders the world from two viewpoints, sends the result to an OpenXR headset, adds bounded head tracking, and expands the normal render area for a tabletop-style view.

This is a work in progress. Things WILL break, and crashes are expected, so I would recommend NOT playing ranked unless you understand these risks.

# Showcase and Game Page

This is a showcase that I filmed of an earlier build: https://www.youtube.com/watch?v=7HWS_Y2-Jr8

Download the game on Steam: https://skillshotcity.com/steam.php?r=Epiano7

<sub>Please note: the link above is a referral link, and I receive a small amount of in-game items per person who uses it.</sub>

## Installation

### Requirements

- Windows 10 or 11, 64-bit
- A legitimate Steam installation of Skillshot City
- Steam running while SSCVR is launched
- A working OpenXR PC runtime, such as Meta Quest Link with Meta set as the active OpenXR runtime

### Install the release

1. Download the newest `SSCVR-*.zip` from the [Releases page](https://github.com/Epiano7/SSCVR/releases).
2. Extract the complete ZIP to a normal folder. Do not run the installer from inside the ZIP preview.
3. Double-click `Install SSCVR.cmd` and read the verification output.
4. Start the mod with `Launch SSCVR.cmd` in `%LOCALAPPDATA%\SSCVR`.

The installer locates Skillshot City through Steam, verifies the exact supported executable, and copies the game into `%LOCALAPPDATA%\SSCVR\game` before adding SSCVR. The normal Steam installation is read but never modified. The copied game still requires Steam ownership and uses the game's normal network connection.

The alpha installer deliberately refuses to overwrite an existing SSCVR copy or patch an unknown game update. To reinstall or update, run `%LOCALAPPDATA%\SSCVR\Uninstall SSCVR.cmd` first. The uninstaller checks SSCVR's install manifest before removing the isolated copy and does not touch the Steam installation.

### Build from source

Follow [Building](docs/BUILDING.md), run `scripts/New-ReleasePackage.ps1`, and then use the installer from the generated package.

## Supported game versions

This release supports:

- Skillshot City `v1.990`
- Skillshot City `v1.991`

The installer still verifies the exact executable revision so that an unknown game update cannot receive unsafe hooks. Supported revisions passed the automated checks described in [the validation record](docs/VALIDATION.md); the newest `v1.991` revision's physical-headset check is still pending. Game versions not listed above may work but are not supported.

## What currently works

- True two-view world rendering through OpenXR
- Quest 3 through Meta Quest Link
- Adjustable tabletop camera angle, position, zoom, and eye alignment
- Bounded head rotation and translation
- Expanded world culling around the normal camera view
- A separate game copy, leaving the normal Steam installation untouched
- Keyboard and mouse gameplay

## Controls

SSCVR keeps Skillshot City's normal keyboard-and-mouse controls. The following temporary shortcuts control the VR camera without using the middle mouse button or interfering with ordinary gameplay inputs.

| Input | Action |
|---|---|
| `Ctrl` + `Shift` + `Alt` + move mouse | Pan the VR camera horizontally and vertically |
| `Ctrl` + `Shift` + `Alt` + mouse wheel | Zoom in or out |
| `Ctrl` + `Alt` + `Up` / `Down` | Increase or decrease the tabletop camera angle |
| `Ctrl` + `Alt` + `Page Up` / `Page Down` | Adjust the camera's vertical offset |
| `Ctrl` + `Alt` + `+` / `-` | Zoom in or out in smaller steps |
| `Ctrl` + `Alt` + `Home` | Reset the camera to SSCVR's defaults |
| `Ctrl` + `Alt` + `End` | Save changes made with the keyboard shortcuts |
| `Ctrl` + `Alt` + `F9` | Recenter the tabletop view around the headset's current pose |

Mouse-based camera adjustments save automatically when you release the key combination. Keyboard adjustments should be saved with `Ctrl` + `Alt` + `End`.

Saved settings are stored as `SkillshotCityVR-camera-live.ini` inside the isolated game's directory and are loaded automatically on later launches. The desktop window title displays the current angle, zoom, and pan values while you adjust the camera.

### Diagnostic shortcuts

| Input | Action |
|---|---|
| `F10` | Save an internal game-frame capture |
| `Shift` + `1` | Save both submitted eyes and a stereo diagnostic set |

`Shift` + `1` is self-contained and does not require the external capture watcher.
It saves the final left/right OpenXR projection textures plus side-by-side,
50/50 overlay, red/cyan alignment, four-times-amplified difference, and
approximate lens-preview BMPs in the isolated game's directory. A companion
`SkillshotCityVR-stereo-diagnostics.txt` records dimensions, the interface
rectangle, and pixel-difference statistics. The lens image is only a radial
visualization: OpenXR does not expose the Quest's private optical calibration,
so it is not a substitute for a physical-headset check.

## Current issues

These are the main things I am working on. If a fork fixes one, feel free to open a pull request and link the test results.

- [x] Keep the Tab and Escape interfaces visible, correctly colored, and aligned in both eyes; confirmed in a physical Quest 3 test and covered by simulator-submitted eye checks.
- [ ] Verify that the level-up interface and other full-screen overlays use the same fixed interface path.
- [ ] Verify in a physical headset that returning from a round uses the same monoscopic panel as the startup menu, without crossed-eye double vision.
- [x] Make `Shift+1` generate a self-contained two-eye alignment and approximate-lens diagnostic set without the external watcher.
- [x] Add deterministic, idempotent OpenXR shutdown and verify repeated session/instance teardown in Meta XR Simulator.
- [ ] Verify that Quest Link remains immediately usable after closing SSCVR on a physical headset.
- [ ] Complete a long physical-headset soak test after the WGL/D3D interop lock fix. Earlier builds could freeze the headset view after roughly one minute.
- [ ] Hold a stable 72 Hz during a full singleplayer round (this is dependent on your PC specs; read below for mine if you want to compare).
- [ ] Improve HUD and text clarity without increasing world-render cost.
- [ ] Finish HUD placement for the minimap, level points, edge indicators, and unusual round announcements.
- [ ] Make world-direction indicators react correctly when the player looks away from the neutral camera direction.
- [ ] Route audio to the active OpenXR/Quest Link device without requiring a Windows output-device change (there is currently no option in game to set an output audio device).
- [ ] Replace the temporary keyboard camera controls with an in-game settings screen/GUI.
- [x] Add and round-trip test a clean installer and manifest-guarded uninstaller for a verified, separate Steam copy.
- [x] Locate and verify the gameplay-render, world-draw, and culling addresses for supported Skillshot City `v1.990` and `v1.991` executable revisions (latest as of 9/3/2026).
- [x] Rebuild and install the package from a fresh GitHub clone, then confirm both live hooks in Meta XR Simulator.
- [ ] Test more aspect ratios, refresh rates, and OpenXR runtimes.
- [ ] Decide what online modes, if any, are safe to support. Multiplayer and ranked have not been extensively tested with this mod (from the few rounds I have played, I have placed top 3 every time).

## Important warning

This repository does not contain Skillshot City, its assets, or its Steam files. You need to own and install the game yourself. Development should happen on a separate copied instance, never directly in the normal Steam folder (the installer will do this automatically).

SSCVR intercepts OpenGL calls inside the copied game process. It has not been tested with ranked or casual multiplayer. Use singleplayer while the project is in this state.

## Test setup

The setup that I have used for development is as follows:

- Meta Quest 3 over a wired Meta Quest Link connection
- Meta OpenXR runtime and Oculus Virtual Audio Device
- Windows 11 Pro 64-bit, build `26200`
- AMD Ryzen 9 5950X; the test OS exposed 14 cores and 28 logical processors
- NVIDIA GeForce RTX 4060 Ti 8 GB
- NVIDIA driver `610.74`, OpenGL `4.6.0`
- 32 GB system memory
- Game window at `1536x864`
- OpenXR eye swapchains at `1536x1666` per eye, reduced from the runtime's `2272x2464` recommendation
- Meta Link render resolution commonly set to `4480x2400` (`1.1x`) during headset tests
- 72 Hz as the intended performance target

## How it works

The mod uses an `opengl32.dll` proxy to observe and adjust the game's OpenGL rendering. During gameplay, it renders the world once for each eye, transfers those images to an OpenXR compositor, and keeps interface elements on a separate near-field layer. Camera transforms add a bounded tabletop response to headset movement, while the culling adjustment asks the game to draw beyond the ordinary monitor view.

The source is specific to the currently verified game build. The launcher refuses to run against an executable with the wrong size or SHA-256 hash.

## Repository layout

- `runtime/src` contains the OpenGL proxy and OpenXR bridge.
- `runtime/launcher` contains the version-checking isolated launcher.
- `runtime/tests` contains exact-build, math, network-behavior, and OpenXR integration tests.
- `runtime/tools` contains optional capture and pose-analysis tools.
- `docs` contains build notes and the publication boundary for game content.

See [Building](docs/BUILDING.md) before compiling.
See [Validation](docs/VALIDATION.md) for the latest automated and headset-test boundary.

## Contributing

Bug fixes and focused experiments are welcome. Please describe the headset/runtime, refresh rate, game mode, and how long the test ran. Do not attach game binaries, assets, shader dumps, profiles, or copied data files to an issue or pull request.

This is an unofficial fan project and is not affiliated with or endorsed by the developer or publisher of Skillshot City, Valve, Meta, or Khronos.

# Credits

Bencelot - Developer of Skillshot City (you can check out his YouTube channel [here](https://www.youtube.com/@skillshotcity))

## License

SSCVR's original code is available under the [Apache License 2.0](LICENSE). That license does not apply to Skillshot City or any other third-party software.
