# TexturePack Override Entry Probe

## Summary
- Added explicit ALPHA override startup confirmation logging.
- Added a one-shot entry probe inside the `GetLoadedTexture` override body.

## Reason
- The fresh runtime logs still showed execution inside the generated `0x180B30` scan path, but the latest `ps2_register_trace.txt` already contained replacement entries for `0x180B30` and `0x181E10`.
- That means the next missing piece is direct proof of whether the runtime ever enters the override body.

## Files Changed
- `ps2xRuntime\src\lib\game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Startup now emits:
  - `[game_overrides] alpha registrations ready getLoadedTexture=0x180b30 serviceQueuedFiles=0x181e10`
- The first few calls into the ALPHA `GetLoadedTexture` override now emit:
  - `[nfs-hp2-alpha] enter GetLoadedTexture override ...`

## Validation
- Reviewed `output\Debug\ps2_register_trace.txt` and confirmed current replacement records exist for:
  - `0x180B30`
  - `0x181E10`
- Build/test not run in this step.

## Follow-up
- Rebuild and rerun with:
  - `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Inspect `output\Debug\log.log` for the new startup and entry-probe markers.
- If the startup marker appears but the entry probe does not, inspect the `GetTextureInfo` call-site dispatch path rather than the override registration path.
