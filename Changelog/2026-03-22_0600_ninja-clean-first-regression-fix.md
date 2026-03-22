# Ninja Clean-First Regression Fix

## Summary
- Fixed `build.ps1` so the Ninja backend no longer uses `--clean-first` for runtime-driven output relinks.
- Preserved the existing force-rebuild behavior only for the Visual Studio/MSBuild backend.

## Reason
- The output chunking work was being neutralized by the wrapper itself.
- When `ps2_runtime.lib` was newer than `ps2_game.exe`, the Ninja path still fell into a forced clean rebuild, recompiling all `ps2_game_chunk_*` targets from scratch.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `runtime-output` on Ninja should now behave like an incremental relink/build instead of a full clean output rebuild.
- Chunked output compilation can now actually pay off on repeated runtime iterations.

## Validation
- Inspected `build.ps1` output-build flow and confirmed the previous logic set `CleanFirst` whenever the runtime lib was newer than the exe.
- Patched the Ninja path to avoid deleting runner artifacts and avoid `--clean-first`.
- Did not run a build in this step.

## Follow-up
- Re-run:
  - `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Confirm that Ninja no longer recompiles all `ps2_game_chunk_*` targets when only a relink or small incremental update is needed.
