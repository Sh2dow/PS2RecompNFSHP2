# Ignore Local Build Artifacts

## Summary
- Added root `.gitignore` rules for local Ninja/VS build directories, runtime output folders, AI helper artifacts, and Python cache files.

## Reason
- `output/build-ninja/` and `output/build-ninja-debug/` are local generated build trees and should not be committed.
- `output/Debug/` contains local executable/runtime logs.
- `tools/ai/artifacts/` and `tools/ai/__pycache__/` are helper/runtime byproducts, not source.

## Files Changed
- `.gitignore`

## Build / Runtime Impact
- No code or build behavior changes.
- Git status will stop surfacing local build/artifact noise from these directories.

## Validation
- Added ignore rules for:
  - `output/build/`
  - `output/build-ninja/`
  - `output/build-ninja-debug/`
  - `output/Debug/`
  - `tools/ai/artifacts/`
  - `tools/ai/__pycache__/`

## Follow-up
- If `output/build-ninja/` files were already staged before the ignore rule, unstage them once with `git restore --staged output/build-ninja`.
