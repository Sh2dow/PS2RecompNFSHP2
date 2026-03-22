# Runtime Action Wrapper Fix

## Summary
- Fixed `build.ps1` so `-Action runtime` builds `ps2_runtime` only.
- The old behavior incorrectly rebuilt `output` as well.

## Reason
- After a successful `.\build.ps1 -Action smart -OutputBackend ninja`, running `.\build.ps1 -Action runtime -OutputBackend ninja` was still triggering another full `ps2_game` rebuild.
- This was a wrapper bug, not expected Ninja behavior: the `runtime` action was wired to `Invoke-RuntimeAndOutputBuild`.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `-Action runtime` now only builds `out/build` target `ps2_runtime`.
- `-Action smart` remains the route that can decide `runtime + output` when timestamps require it.
- The interactive menu text now reflects the corrected behavior.

## Validation
- Inspected `build.ps1` control flow and confirmed the previous runtime branch called `Invoke-RuntimeAndOutputBuild`.
- Patched the script to add `Invoke-RuntimeBuild` and route `-Action runtime` through it.
- Did not run a build in this step.

## Follow-up
- Re-run:
  - `.\build.ps1 -Action runtime -OutputBackend ninja`
- It should now rebuild only `ps2_runtime`.
- Use `.\build.ps1 -Action smart -OutputBackend ninja` when you want automatic `runtime + output` routing.
