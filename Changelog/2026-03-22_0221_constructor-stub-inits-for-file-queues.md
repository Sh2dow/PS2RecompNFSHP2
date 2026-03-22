# Constructor Stub Inits For File Queues

## Summary
- Replaced runtime TODO stubs for `QueuedFile`, `ResourceFile`, and `TexturePack` constructors with concrete field initialization in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- Added bounded guest-string copying for constructor-owned name/path buffers.
- Initialized queue callback/state slots that are later executed by `ServiceQueuedFiles`.

## Reason
- Startup had moved past the joystick-event hang and into file/resource loading.
- Generated constructors for `__10QueuedFilePvPCciiPFii_vi`, `__12ResourceFilePCc16ResourceFileTypeiPFi_vi`, and `__11TexturePackPCc` were still TODO stubs.
- The queue runner at `ServiceQueuedFiles__Fv_0x181e10` executes the callback stored at `QueuedFile + 0x50`; leaving that object uninitialized is consistent with the observed bad-PC jump through ASCII-like data (`0x4e49422e`, `.BIN`).

## Files Changed
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `QueuedFile` objects now start with valid list links, id/state fields, callback pointer, callback user data, path buffer, and offset/size state instead of allocator garbage.
- `ResourceFile` objects now start with sane list links, type, flags, filename buffer, and zeroed async/data pointers.
- `TexturePack` objects now start with sane node links, filename buffer, and sentinel state fields aligned with the safe defaults already used by `StreamingTexturePack`.
- This should remove one major source of bad indirect calls during async file/resource boot.

## Validation
- Static inspection only in this step.
- Used generated callsites and consumers to derive the required layouts:
  - `AddQueuedFile__FPvPCciiPFii_vi_0x182020.cpp`
  - `BeginRead__10QueuedFile_0x181d20.cpp`
  - `ServiceQueuedFiles__Fv_0x181e10.cpp`
  - `FindResourceFile__F16ResourceFileType_0x170800.cpp`
  - `FindResourceFile__FPCc_0x170798.cpp`
  - `Init__20StreamingTexturePackPCcPFi_vi_0x181618.cpp`
- No build or runtime command was executed in this patch step.

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- The follow-up run already showed that `Function at address 0x4e49422e not found` disappeared.
- The new active blocker is an async-file completion spin: `bIsAsyncDone__FP5bFile_0x1f9938` polls `bFile+0x74`, and the generated logic expects that field to be a pending-operation count which reaches `0`.
- `ps2_stubs.cpp` was adjusted in the same patch series so the `bFile` constructor now seeds `0x74` with `0` instead of the queue sentinel. The next rerun should confirm whether `GLOBAL\DYNTEX.BIN` advances beyond the poll loop.
