# Staged Build Wrappers

## Summary
- Added explicit PowerShell wrappers for `output`-only builds, `ps2xRuntime + output` builds, and game launch with default log discovery.
- Kept `build.ps1` as the compatibility entry point for `output`-only rebuilds.

## Reason
- The repo was using a one-line `build.ps1` that only rebuilt `output\build`.
- Runtime edits under `ps2xRuntime\` require rebuilding `out\build` first, then relinking `output\build`.
- The current workflow needed a single obvious runner for `ps2_game.exe` and `ps2_log.txt`.

## Files Changed
- `build.ps1`
- `build-output.ps1`
- `build-runtime-and-output.ps1`
- `run-game.ps1`

## Build / Runtime Impact
- `build.ps1` now forwards to `build-output.ps1`.
- `build-output.ps1` builds only `output\build`.
- `build-runtime-and-output.ps1` rebuilds `out\build` first, then `output\build`.
- `run-game.ps1` launches `output\Debug\ps2_game.exe` by default and reports the active `ps2_log.txt` path.

## Validation
- Script contents were added under the repo root and configured to use `vswhere.exe` plus `vcvars64.bat`.
- Default runner paths match the current Debug executable layout and `ps2_log.h` path behavior.

## Follow-up
- Add these script entry points to repo docs once the current runtime probes are cleaned up.
- If needed, a later pass can factor the duplicated `Get-VcVarsBat` helper into one shared script.
