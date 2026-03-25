# 2026-03-24_2225_bOpen-bad-path-rejection

## Summary
- Added early rejection for bad pointer values (0xAAAAAAAA, 0xEEEEEEEE, 0x0) in `bOpen` file open handler
- Changed from logging-only to returning failure (null handle) when bad paths are detected

## Reason
- The game was attempting to open files with sentinel/invalid pointer values (0xaaaaaaaa) from uninitialized joystick/input system state
- Previous behavior logged the issue but continued, potentially causing downstream corruption
- New behavior gracefully rejects invalid paths and returns failure, allowing game to continue

## Files Changed
- `ps2xRuntime\src\lib\game_overrides.cpp` - Modified `nfsHp2AlphaBOpen()` function

## Build / Runtime Impact
- Runtime: Graceful handling of invalid file open requests from uninitialized subsystems
- No impact on valid file operations
- Allows game to progress past joystick initialization issues that were previously causing cascading failures

## Validation
- Built with: `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Ran game: `.\build.ps1 -Action run -OutputBackend ninja -RunTimeoutSeconds 20`
- Observed in log: `[nfs-hp2-alpha] bOpen:rejected-bad-path pathAddr=0xaaaaaaaa` (changed from `caught-bad-path`)
- Game continues execution past the rejection point
- Texture loading, sound loading, and file system operations all working correctly

## Follow-up
- Joystick initialization remains an issue but is now gracefully handled
- The root cause is game data initialization (joystick handler list not populated for this alpha build)
- This is a data/initialization problem specific to the alpha build, not a runtime bug
- Next steps could include:
  - Investigating joystick handler registration path
  - Adding stub joystick handlers for alpha builds
  - Further tracing of input system initialization
