# Contributing

Keep changes small enough to test and explain. For VR behavior changes, include the headset, OpenXR runtime, refresh rate, camera settings, game state, test duration, and whether the result came from a simulator or physical headset.

Before opening a pull request:

1. Run `runtime/build.ps1` and the geometry tests.
2. Check the repository against `docs/CONTENT_POLICY.md`.
3. Make sure logs, captures, binaries, game files, and personal paths are not staged.
4. Describe which README checklist item the change addresses.

Please do not test automation against casual, ranked, matchmaking, or multiplayer. The current automation boundary is singleplayer only.
