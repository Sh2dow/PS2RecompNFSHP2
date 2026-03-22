# VSync Worker Dispatch Diagnostics

## Summary
- Added logs for VSync worker startup and tick delivery.
- Added logs for `sceGsSyncVCallback` dispatch attempts, skip reasons, and callback invocation.
- Updated persistent project state with the new hypothesis.

## Reason
- The previous run showed `sceGsSyncVCallback` registering `PSX2VBlank` at `0x100038`.
- The same run then stayed on black frames with an active guest thread, but there were no callback-dispatch logs.
- That makes the VSync worker / callback-delivery path the next most likely blocker.

## Files Changed
- `ps2xRuntime/src/lib/syscalls/ps2_syscalls_interrupt.inl`
- `ps2xRuntime/src/lib/stubs/ps2_stubs_gs.inl`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- The runtime will now log when the interrupt worker starts and when it emits VSync ticks.
- The runtime will now log whether GS sync callback dispatch is skipped, missing, or invoked.
- No generated output files were edited.

## Validation
- Source inspection only in this step; no rebuild was run here.
- Baseline evidence came from `output/Debug/log.log`, which showed callback registration but no follow-up dispatch evidence.

## Follow-up
- Rebuild with `./build.ps1 -Action runtime` and rerun.
- Check for:
  - `[vsync-worker:start]`
  - `[vsync-worker:tick]`
  - `[sceGsSyncVCallback:dispatch]`
  - `[sceGsSyncVCallback:dispatch-skip]`
- If the worker runs and dispatches, the next fix is inside the callback itself or the handler it drives.
- If the worker never starts, the start path or worker lifetime logic is wrong.
