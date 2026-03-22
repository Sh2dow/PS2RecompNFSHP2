# Async Entry Queue Repair

## Summary
- Patched the ALPHA runtime wrappers to normalize the async-entry queue sentinel at `gp-0x4A98` before `BeginRead__10QueuedFile_0x181d20` and `bServiceFileSystem__Fv_0x1f8ae8`.
- Added one-shot repair logging so the next runtime log shows whether head/tail recovery actually fired.

## Reason
- The latest `log.log` showed the first async entry for `GLOBAL\\DYNTEX.BIN` was only linked from the tail side: `sentinelNext=0x0`, `sentinelPrev=entry`, `entry.next=sentinel`, `entry.prev=0x0`.
- `bServiceFileSystem` only consumes the queue from the head link at `gp-0x4A98`, so a zero head prevents `AsyncRealCDRead` from ever running.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Runtime only.
- The ALPHA-specific wrappers now repair broken zeroed sentinel links before the generated async queue code runs.
- The next run should either show explicit repair markers or prove the queue is arriving intact and the stall is deeper in `AsyncRealCDRead`.

## Validation
- Read generated queue code in:
  - `output/bReadAsync__FP5bFilePviPFP5bFilei_vi_0x1f9778.cpp`
  - `output/ps2__GLOBAL__I_bFileSlotPool_0x1f9d18.cpp`
  - `output/bServiceFileSystem__Fv_0x1f8ae8.cpp`
- Confirmed the generated logic expects the sentinel head and tail cells to be initialized to the sentinel address, not zero.
- Did not rebuild in this step.

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime -OutputBackend ninja`.
- Rerun and inspect `output/Debug/log.log` for:
  - `[nfs-hp2-alpha] repaired async-entry queue before BeginRead ...`
  - `[nfs-hp2-alpha] repaired async-entry queue before bServiceFileSystem ...`
- If the repair fires and `bServiceFileSystem` still stalls, trace `AsyncRealCDRead__FP5bFileP10AsyncEntry_0x1fadc0` directly.
