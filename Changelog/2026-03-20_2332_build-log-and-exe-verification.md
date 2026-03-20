# Build Log And Exe Verification

## Summary
- Updated `build.ps1` to capture `cmake --build` output into timestamped log files.
- Added post-build verification so `Build output only` now fails if `output\Debug\ps2_game.exe` was not actually produced.

## Reason
- The wrapper could previously report a successful build while no fresh `ps2_game.exe` existed.
- That made it hard to distinguish MSBuild up-to-date behavior from an incomplete or broken link step.

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- Build logs are now written under `tools\ai\artifacts\build-logs\`.
- `.\build.ps1 -Action output` now errors if the target exe is still missing after a nominally successful build.

## Validation
- Logic updated to tee build output into a per-run log file.
- Logic updated to verify `output\Debug\ps2_game.exe` after the `ps2_game` target build path completes.

## Follow-up
- Run `.\build.ps1 -Action output` again.
- If it fails, inspect the emitted build log under `tools\ai\artifacts\build-logs\` for the actual linker/MSBuild failure text.
