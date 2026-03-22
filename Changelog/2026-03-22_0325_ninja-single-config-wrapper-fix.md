# Ninja Single-Config Wrapper Fix

## Summary
- Updated `build.ps1` so the Ninja backend uses single-config build directories instead of `Ninja Multi-Config`.
- Made the wrapper prefer Visual Studio's bundled `ninja.exe` over the Chocolatey shim.
- Tightened configure validation so a Ninja build directory is considered usable only when `build.ninja` exists.

## Reason
- The previous Ninja path left `output\build-ninja\` in a half-configured state with `CMakeCache.txt` but no `build.ninja`.
- Because the wrapper only checked for `CMakeCache.txt`, every later Ninja build skipped configure and failed immediately with `ninja: error: loading 'build.ninja'`.
- A single-config Ninja directory per build configuration is simpler and more reliable for this repo's standalone `output\` target.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `.\build.ps1 -Action smart -OutputBackend ninja` now targets per-config directories such as `output\build-ninja-debug`.
- Ninja configure should now be re-run automatically if the directory contains only partial CMake state.
- The wrapper now uses the Visual Studio bundled Ninja first, which avoids depending on external PATH ordering or shim behavior.

## Validation
- Read the failed Ninja log at `tools\ai\artifacts\build-logs\20260322_030158_debug_ps2_game.log`.
- Inspected `output\build-ninja\` and confirmed it had only `CMakeCache.txt` and `CMakeFiles\` with no `build.ninja`.
- Parsed the updated `build.ps1` successfully with PowerShell's scriptblock parser.

## Follow-up
- Run `.\build.ps1 -Action smart -OutputBackend ninja`.
- Confirm that `output\build-ninja-debug\build.ninja` is created and that `ps2_game.exe` builds from that directory.
- If Ninja still fails, use the new build log under `tools\ai\artifacts\build-logs\` to diagnose the next configure/build error.
