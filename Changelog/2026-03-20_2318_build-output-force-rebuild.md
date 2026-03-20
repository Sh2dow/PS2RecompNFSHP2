# Build Output Force Rebuild

## Summary
- Updated `build.ps1` so `Build output only` forces a real `ps2_game` rebuild when `ps2_game.exe` is missing or when `ps2_runtime.lib` is newer than the exe.

## Reason
- MSBuild was treating `ps2_game` as up to date based on project state even after the stale exe was removed.
- That caused successful output builds that did not recreate `output\Debug\ps2_game.exe`.

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- `.\build.ps1 -Action output` now uses `--target ps2_game`.
- When the exe is missing or stale relative to `out\build\ps2xRuntime\Debug\ps2_runtime.lib`, the wrapper now uses a clean-first target rebuild so the linker actually emits a fresh exe.

## Validation
- Confirmed generated `ps2_game.vcxproj` still points to `D:\Repos\Recomp\PS2Recomp\output\Debug\ps2_game.exe`.
- Confirmed prior wrapper behavior could delete the stale exe without triggering a relink.
- Added forced rebuild logic to close that gap.

## Follow-up
- Run `.\build.ps1 -Action output` once manually and verify that `output\Debug\ps2_game.exe` is recreated with a fresh timestamp.
- After the exe is restored, continue runtime testing on the `registerAllFunctions(runtime)` crash path.
