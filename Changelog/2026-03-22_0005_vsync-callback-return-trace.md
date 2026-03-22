# VSync Callback Return Trace

## Summary
- Added return-state logging after each `sceGsSyncVCallback` dispatch.
- Updated persistent project state with the new runtime boundary.

## Reason
- The latest run proved that the VSync worker starts and repeatedly dispatches `PSX2VBlank`.
- That rules out the worker-start and callback-delivery path as the primary blocker.
- The next question is whether the callback itself returns cleanly or stalls inside guest code.

## Files Changed
- `ps2xRuntime/src/lib/stubs/ps2_stubs_gs.inl`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- The runtime now logs callback return state with step count and final `pc`, `ra`, `sp`, and `gp` for sampled VSync dispatches.
- No generated files were edited.

## Validation
- Source inspection only in this step; no rebuild was run here.
- Baseline evidence came from `output/Debug/log.txt`, which showed:
  - `[vsync-worker:start]`
  - `[vsync-worker:tick]`
  - `[sceGsSyncVCallback:dispatch]`

## Follow-up
- Rebuild with `./build.ps1 -Action runtime` and rerun.
- Inspect new `[sceGsSyncVCallback:return]` lines to decide whether to trace `PSX2VBlank__Fi_0x100038` directly or move to the callback’s callee chain.
