# Build Artifact Assertions

## Summary
- Hardened `build.ps1` so it fails if `ps2_game.exe` is missing after a supposedly successful output build.
- Added the same assertion to the `smart` no-op path.

## Reason
- `.\build.ps1 -Action smart -OutputBackend ninja` could still report completion when `output\Debug\ps2_game.exe` had been deleted.
- A successful backend exit code is not enough if the expected runner artifact does not exist.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Output builds now throw immediately if `ps2_game.exe` is absent after CMake/Ninja returns success.
- `smart` now refuses to print `no build needed` when the expected executable is missing.

## Validation
- Inspected the wrapper flow and replaced the soft post-build check with a hard `Assert-ArtifactExists`.
- Added the same assertion in the `smart` `none` branch.
- Did not run a build in this step.

## Follow-up
- Delete `output\Debug\ps2_game.exe` and re-run:
  - `.\build.ps1 -Action smart -OutputBackend ninja`
- Expected result: the script should now fail loudly instead of reporting a completed build without the executable.
