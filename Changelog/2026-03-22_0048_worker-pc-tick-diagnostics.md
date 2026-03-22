# Worker PC Tick Diagnostics

## Summary
- Added per-dispatch-thread runtime snapshots in `ps2xRuntime/src/lib/ps2_runtime.cpp`.
- `dispatchLoop()` now records each live guest thread's `pc`, `ra`, `sp`, `gp`, and stable-iteration count.
- `[run:tick]` now emits a `workerPcs=` summary so the log shows live guest worker PCs instead of only the bootstrap `_start` context.

## Reason
- The latest `output/Debug/log.txt` showed a black screen while `activeThreads=1`, but the sampled tick PC stayed at `_start` (`0x125AE0`).
- VSync callback dispatch is already proven alive, so the next useful signal is where the surviving guest worker thread is actually parked.

## Files Changed
- `ps2xRuntime/src/lib/ps2_runtime.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Runtime logging is more descriptive during black-screen/live-thread states.
- No generated output files were edited.
- The next `log.txt` should include `workerPcs=...` on each sampled `[run:tick]` line.

## Validation
- Source patch applied successfully with `apply_patch`.
- No build was run in this step.
- Next validation command:
  - `.\build.ps1 -Action runtime`
  - `.\build.ps1 -Action run`

## Follow-up
- Inspect `output/Debug/log.txt` for the new `workerPcs=` field.
- Use the reported parked worker PC(s) to identify the next runtime blocker.
