# Unified Build Script

## Summary
- Consolidated build and run helpers into a single `build.ps1` entry point.
- Removed the separate `build-output.ps1`, `build-runtime-and-output.ps1`, and `run-game.ps1` scripts.

## Reason
- The staged workflow was correct, but the repo still had multiple helper scripts to remember.
- The goal was one script that can build output only, build runtime plus output, or run the game with default paths.

## Files Changed
- `build.ps1`
- `Changelog/2026-03-20_2055_unified-build-script.md`

## Build / Runtime Impact
- `.\build.ps1` now opens a small menu if run without arguments.
- `.\build.ps1 -Action output` builds only `output\build`.
- `.\build.ps1 -Action runtime` builds `out\build` and then `output\build`.
- `.\build.ps1 -Action run` launches `output\Debug\ps2_game.exe` and reports the default `ps2_log.txt` path.

## Validation
- The new `build.ps1` preserves the existing Visual Studio discovery and `vcvars64.bat` setup flow.
- Syntax validation should be run after script edits before using it as the main entry point.

## Follow-up
- If the repo later needs more modes, add them here instead of creating new top-level helper scripts.
- The current runtime probes in `output/main.cpp` and `ps2_runtime.cpp` are still temporary and should be cleaned up separately.
