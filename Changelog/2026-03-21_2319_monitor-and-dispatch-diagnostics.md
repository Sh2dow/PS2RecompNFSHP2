# Monitor And Dispatch Diagnostics

## Summary
- Added targeted diagnostics for the legacy monitor syscall `0x63`.
- Added richer stable-PC dispatch logging in the runtime loop.
- Updated persistent state to reflect that reboot and `DIR.BIN` remap are now past blockers.

## Reason
- The current ALPHA run no longer dies in CD bootstrap or IOP reboot.
- The remaining issue is a live-thread blank-frame state where the host window keeps rendering while the guest thread appears active and the last debug PC remains `0x125AE0`.
- The previous logs were not sufficient to tell whether the thread was spinning in guest code, blocked in a host callback, or only publishing ignored monitor packets.

## Files Changed
- `ps2xRuntime/src/lib/ps2_syscalls.cpp`
- `ps2xRuntime/src/lib/ps2_runtime.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `LegacyMonitorSyscall` now logs decoded packet words, payload bytes, and a short printable payload preview for `op=0x15` calls instead of only dumping raw register values.
- `dispatchLoop` now logs `ra`, `sp`, `gp`, and dispatch history when the guest PC remains stable for a long interval.
- No generated runner files were edited.

## Validation
- Source inspection only in this step; no rebuild was run here.
- Baseline evidence for the change came from:
  - `output/Debug/log.log`
  - `output/bSendPacket__FPCvlT0l_0x200b78.cpp`
  - `output/bFunkCallASync__FPCcUcPCvl_0x200c18.cpp`
  - `output/AllocateMemory__10MemoryPooliiPi_0x1fb900.cpp`

## Follow-up
- Rebuild with `./build.ps1 -Action runtime` and rerun.
- Check whether monitor packet logs appear beyond the initial probe.
- If the guest thread is blocked outside normal dispatch, trace the host callback or worker-thread path that keeps `activeThreads=1` while the window remains black.
