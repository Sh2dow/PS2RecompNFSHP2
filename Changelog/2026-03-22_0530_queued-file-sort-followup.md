# Queued File Sort Follow-up

## Summary
- Confirmed the async-entry queue repair worked and moved the runtime past the old `bServiceFileSystem` head-link failure.
- Added ALPHA diagnostics/repair for the queued-file list in `ServiceQueuedFiles`.
- Replaced the remaining queue/file delete TODO stubs with no-op delete handlers.

## Reason
- The new `log.log` shows `AsyncRealCDRead` now runs for `GLOBAL\\DYNTEX.BIN`, `GLOBAL\\HUDTEX.BIN`, and `CARS\\TEXTURES.BIN`, so the previous async queue bug is fixed.
- The run now parks at `0x182260` (`SortByPriority__10QueuedFileP10QueuedFileT1`), indicating the next likely corruption boundary is the queued-file list/sort path.
- The runtime was also still hitting noisy unimplemented delete stubs for `AsyncEntry`, `bFile`, and `QueuedFile`.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Runtime only.
- `ServiceQueuedFiles` now normalizes the queued-file sentinel at `gp-0x57E0` before entering generated code and logs the active head node when present.
- Delete stubs for `AsyncEntry`, `bFile`, and `QueuedFile` now behave as safe no-op frees instead of emitting TODO warnings.

## Validation
- Reviewed `output/Debug/log.log` and confirmed:
  - async queue repair fired
  - `AsyncRealCDRead` executed
  - the new parked PC is `0x182260`
- Read generated files:
  - `output/ServiceQueuedFiles__Fv_0x181e10.cpp`
  - `output/SortByPriority__10QueuedFileP10QueuedFileT1_0x182260.cpp`
  - `output/ps2__GLOBAL__I_QueuedFileSlotPool_0x182238.cpp`
- Did not rebuild in this step.

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime -OutputBackend ninja`.
- Rerun and inspect `output/Debug/log.log` for:
  - `[nfs-hp2-alpha] repaired queued-file queue before ServiceQueuedFiles ...`
  - `[nfs-hp2-alpha] ServiceQueuedFiles head=... prio=...`
- If the queued-file list is intact, the next target is a bad priority/list cycle inside the sort path itself.
