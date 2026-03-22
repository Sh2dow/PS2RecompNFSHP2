# Smart Build Ninja Scan Fix

## Summary
- Fixed `build.ps1` smart-build source scanning so Ninja build directories are excluded from `output` timestamp checks.

## Reason
- `smart -OutputBackend ninja` could decide `output` was dirty even after a prior Ninja build because `output/build-ninja-debug` contained generated probe files like `CMakeCXXCompilerId.cpp`.
- Those files are build artifacts, not source inputs, and should not participate in incremental routing decisions.

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- `.\build.ps1 -Action smart -OutputBackend ninja` should now avoid unnecessary output rebuilds caused by files under `output/build-ninja-*`.

## Validation
- Confirmed `output/build-ninja-debug` contains generated source-like files:
  - `CMakeFiles/4.2.1/CompilerIdCXX/CMakeCXXCompilerId.cpp`
  - `CMakeFiles/ShowIncludes/foo.h`
- Added exclusions for:
  - `output/build-ninja-debug`
  - `output/build-ninja-release`
  - `output/build-ninja-relwithdebinfo`
  - `output/build-ninja-minsizerel`

## Follow-up
- If `ps2_runtime.lib` is newer than `ps2_game.exe`, `smart` will still rebuild output intentionally. That is correct and separate from this scan bug.
