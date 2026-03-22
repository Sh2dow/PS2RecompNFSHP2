# Async Filesystem Queue Diagnostics

## Summary
- Added ALPHA-only diagnostics around the async file-service path.
- Fixed the runtime build break by declaring the generated `BeginRead__10QueuedFile_0x181d20` and `bServiceFileSystem__Fv_0x1f8ae8` entry points used by the new overrides.
- Rebuilt `ps2_runtime`, relinked `ps2_game`, and captured a timed rerun against `SLUS_203.62`.

## Reason
- The game no longer fails in boot, CD remap, IOP reboot, or VBlank dispatch.
- The current visible stall is in `bIsAsyncDone -> bServiceFileSystem` after `OpenPlatformFile("GLOBAL\\DYNTEX.BIN")`.
- The next fix depends on seeing the actual async queue links and entry state instead of guessing from the top-level poll loop.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `ps2_runtime` now compiles with the async queue diagnostics in scope.
- The ALPHA override path logs `AsyncEntry` creation and `bServiceFileSystem` queue state for the `GLOBAL\\DYNTEX.BIN` stall.
- The latest timed rerun shows the async read entry is created with `proc=0x1FADC0`, but the queue-service dump still needs one more correction to log the sentinel links accurately when the head pointer is corrupted or zero.

## Validation
- Built with:
  - `.\build.ps1 -Action runtime -OutputBackend ninja`
- Runtime artifacts:
  - `tools/ai/artifacts/build-logs/20260322_034242_debug_ps2_runtime.log`
  - `tools/ai/artifacts/build-logs/20260322_034247_debug_ps2_game.log`
- Timed run executed with redirected output and forced stop after 25 seconds.
- Observed in `output/Debug/raw_log.txt`:
  - `OpenPlatformFile("GLOBAL\\DYNTEX.BIN")`
  - `[AsyncEntry:ctor] entry=0x1fea6cc file=0x1fea640 proc=0x1fadc0 ...`
  - repeated `bServiceFileSystem` diagnostics, confirming the new runtime binary is in use

## Follow-up
- Rebuild once more with the corrected sentinel-link logging in `nfsHp2AlphaServiceFileSystem`.
- Confirm whether the queue head is truly corrupted/zero or whether the service loop is parked on a valid `AsyncRealCDRead` entry.
- Patch the next fault at the concrete async entry/state boundary instead of widening generic filesystem fallbacks.
