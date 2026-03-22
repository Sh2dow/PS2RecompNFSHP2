# Live Run Tick Sampling

## Summary
- Updated `ps2xRuntime/src/lib/ps2_runtime.cpp` so `[run:tick]` prefers live `m_cpuContext` register reads for `pc`, `ra`, `sp`, and `gp`.
- Kept the earlier `workerPcs=` thread snapshot summary in place.

## Reason
- The first worker-PC diagnostic showed only `#1:pc=0x125ae0`, which is the `_start` entry PC.
- That did not mean the game was truly parked at `_start`; it meant the previous sampling path was still reading stale pre-dispatch state while a long nested guest call chain was active.

## Files Changed
- `ps2xRuntime/src/lib/ps2_runtime.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- The next `output/Debug/log.txt` should show the actual live guest PC on `[run:tick]`, not just the bootstrap entrypoint.
- This should make the remaining black-screen/live-thread blocker diagnosable without editing generated output.

## Validation
- Source patch applied successfully with `apply_patch`.
- No build was run in this step.
- Next validation command:
  - `.\build.ps1 -Action runtime`
  - `.\build.ps1 -Action run`

## Follow-up
- Inspect the updated `[run:tick]` lines in `output/Debug/log.txt`.
- Use the first non-`0x125AE0` live PC to choose the next runtime trace target.
