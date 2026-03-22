# Runtime Output Action

## Summary
- Added `-Action runtime-output` to `build.ps1`.
- Kept `-Action runtime` as runtime-library-only.

## Reason
- Runtime changes in `ps2xRuntime` do not affect `ps2_game.exe` until the runner is relinked.
- Using `-Action runtime` alone was leaving the executable stale, which made the latest runtime patches invisible in `log.log`.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `-Action runtime` now clearly means compile-only for `ps2_runtime.lib`.
- `-Action runtime-output` explicitly rebuilds `ps2_runtime` and then relinks `ps2_game.exe`.
- The interactive menu now exposes both choices separately.

## Validation
- Patched the action enum and menu routing in `build.ps1`.
- Did not run a build in this step.

## Follow-up
- For runtime patches that must show up in logs immediately, use:
  - `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Then rerun the game and confirm the new log markers are present.
