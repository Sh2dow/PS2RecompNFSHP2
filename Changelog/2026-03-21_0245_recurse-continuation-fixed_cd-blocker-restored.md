# Recurse Continuation Fixed, CD Blocker Restored

## Summary
- Verified that the late function-table re-sort fixed the `0x1FA7A0` continuation dispatch failure.
- The active runtime blocker is now back in the CD path: unresolved `sceCdRead` requests and repeated `sceCdGetError() == -1` / `FatalCDErrorFunction()` loops after `Loading entire directory...`.

## Reason
- The previous blocker was an internal `RecurseDirectory` continuation PC (`0x1FA7A0`) registered by a game override after the function table had already been finalized.
- Once that dispatch issue was fixed, runtime progressed further and exposed the next real blocker in disc I/O again.

## Files Changed
- `ps2xRuntime/src/lib/ps2_runtime.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `0x1FA7A0` is no longer reported as a missing function.
- Runtime now reaches the directory/CD loading stage and loops on CD errors instead of failing in continuation dispatch.

## Validation
- `output/Debug/raw_log.txt` shows:
- `[game_overrides] applied 1 matching override(s).`
- `Loading entire directory...`
- repeated `sceCdGetError() returned error code -1`
- `[sceCdRead] unresolved request pc=0x2264c0 ra=0x1fa4d0 a0=0x1070036 a1=0x3181 a2=0x7282a0`
- `output/Debug/ps2_register_trace.txt` shows:
- `resorted function table after late registration for address=0x1fa7a0`

## Follow-up
- Resume tracing the `MakeSureDirectoryIsLoaded -> LoadDirectory -> RecurseDirectory -> OpenCDFile -> ReadCDSectorASync -> sceCdRead` path.
- Focus on why the sector/LBN request is unresolved when `Loading entire directory...` begins.
- Use `stdout.log` and `stderr.log` for channel-specific inspection when chronology matters; `raw_log.txt` is currently a merged convenience artifact, not a true interleaved timeline.
