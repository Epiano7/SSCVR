# Building SSCVR

The current build targets 64-bit Windows. It has been tested with a MinGW-w64 UCRT compiler and OpenXR SDK 1.1.60.

## Requirements

- Windows 10 or 11
- A 64-bit MinGW-w64 GCC toolchain
- The [Khronos OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK), including `openxr.h` and a 64-bit `openxr_loader.dll`
- PowerShell 7 or Windows PowerShell 5.1

Set `SSCVR_GCC` to the full path of the compiler and `OPENXR_SDK_ROOT` to the SDK or prebuilt package root. You can also pass both paths directly:

```powershell
.\runtime\build.ps1 -Compiler C:\path\to\x86_64-w64-mingw32-gcc.exe -OpenXRRoot C:\path\to\OpenXR-SDK
```

The output is placed in `runtime/build`. The build copies Windows' own OpenGL DLL into that untracked directory for local forwarding tests; it must not be committed or added to a release.

## Tests

`build.ps1` always runs the geometry test. The build directory also contains:

- `guard_test.exe`, which checks that ordinary Winsock traffic is not intercepted
- `game_build_layout_test.exe`, which checks both exact executable layouts and fail-closed hook signatures
- `test_gl.exe`, a small OpenGL hook smoke test
- `integration/openxr_bridge_integration_test.exe`, which needs an active OpenXR runtime

The simulator is useful for structural testing, but perceived depth, comfort, physical Link interop, and headset-only interface behavior still require a real headset.

### Hidden interface regression

The integration test can run without showing a game or simulator window. With
a local Meta XR Simulator runtime and a batch configuration that disables its
compositor, run from the repository root:

```powershell
.\runtime\tools\Invoke-InterfaceRegression.ps1 `
  -BuildRoot '.\runtime\build' `
  -RunRoot 'D:\SSCVR-LocalTests\ui-run-01' `
  -RuntimeJson 'D:\MetaXRSimulator\meta_openxr_simulator.json' `
  -SimulatorConfig 'D:\MetaXRSimulator\config\batch.json'
```

Use a new output directory for every run. The runner sets only process-local
runtime variables, restores them afterward, enforces a deadline, and closes
its own test if the simulator hangs during teardown after writing a result.
Python 3 is required; the image validators use its standard library only.
`-Frames 12000` exercises 55 full menu cycles after startup. The runner checks
fresh-pair cadence as well as the result counters and final submitted pixels.

The synthetic scene tests inherited scissor clipping, disabled alpha writes,
clear-state restoration, translucent panels, opaque colors, edge placement,
left/right alignment, and two distinct captures. For a local archived game UI
replay, add `-ReplayDirectory 'D:\SSCVR-LocalCaptures\menu'`. That directory must
contain `SkillshotCityVR-interface-alpha.bmp`, its `-mask.bmp` companion, and
`SkillshotCityVR-stereo-diagnostics.txt` from a completed capture. No game
process is launched. Replay checks compare the resampled captured RGB/alpha
and opaque pixels submitted to both eyes. Captures stay outside the repository
and must not be included in a release.

Replay verifies the compositor with real UI pixels; it cannot verify whether
the live game's menu classification/capture timing selects every overlay.
Level-up, results, game transitions, physical lens readability, and Link
transport still require live Singleplayer and physical-headset acceptance.

## Local game copy

Do not copy game files into this repository. A developer can point the launcher at an existing isolated copy with `SSCVR_GAME_DIR`. The directory must contain the supported `SkillshotCity.exe` and its ordinary locally installed data.

```powershell
$env:SSCVR_GAME_DIR = 'D:\Games\SSCVR-Test\game'
.\runtime\build\SkillshotCityVRLauncher.exe --verify
```

The normal Steam installation should remain untouched.
