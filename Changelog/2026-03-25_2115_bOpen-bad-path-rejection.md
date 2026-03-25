# 2026-03-25_2115_bOpen-bad-path-rejection

## Summary
- Added early rejection for bad pointer values (0xAAAAAAAA, 0xEEEEEEEE, 0x0) in `bOpen` file open handler
- Added fix for invalid joystick config (0xffffffff -> 0) in GenerateEventsFromScanners
- Fixed duplicate stub code in jukebox-related generated files

## Reason
- The game was attempting to open files with sentinel/invalid pointer values (0xaaaaaaaa) from uninitialized joystick/input system state
- Previous behavior logged the issue but continued, potentially causing downstream corruption
- New behavior gracefully rejects invalid paths and returns failure, allowing game to continue
- Invalid config (0xffffffff) was preventing joystick initialization

## Files Changed
- `ps2xRuntime\src\lib\game_overrides.cpp` - Modified `nfsHp2AlphaBOpen()` function
- `ps2xRuntime\src\lib\game_overrides.cpp` - Fixed `GenerateEventsFromScanners` config handling
- `output\Update__14cJukeboxPlayer_0x2120f0.cpp` - Fixed duplicate stub code
- `output\LoadCartridge__14cJukeboxPlayerPcb_0x211d88.cpp` - Fixed duplicate stub code

## Build / Runtime Impact
- Runtime: Graceful handling of invalid file open requests from uninitialized subsystems
- No impact on valid file operations
- Config now properly set to 0 instead of 0xffffffff

## Validation
- Built with: `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Ran game: `.\build.ps1 -Action run -OutputBackend ninja -RunTimeoutSeconds 20/45`
- Observed in log: `[nfs-hp2-alpha] bOpen:rejected-bad-path pathAddr=0xaaaaaaaa`
- Config now shows `requestedConfig=0x0` instead of `0xffffffff`

## Follow-up
- Game now makes progress through initialization (loads textures, sound files)
- Stuck in initialization loop - the joystick event list is empty/invalid
- The scePadRead keyboard mapper exists but isn't being called by this alpha build
- The game uses its own joystick event system (FindEventNode, AddJoyHandler) instead of scePadRead
- Need to either: (1) initialize joystick event list properly, or (2) make game call scePadRead
