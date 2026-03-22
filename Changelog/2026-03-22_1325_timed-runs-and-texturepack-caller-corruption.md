# Timed Runs And TexturePack Caller Corruption

## Summary
- Verified from `raw_log.txt` that the ALPHA `GetLoadedTexture` override is active.
- Added a default timed run window to `build.ps1` so local test runs terminate automatically after 15 seconds unless `-NoWait` is used.

## Reason
- The latest runtime evidence shows the texture-lookup override is being entered, but with invalid caller state.
- Repeated manual game-window closure was slowing the debug loop, so the run wrapper now needs to stop test runs on its own.

## Files Changed
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `.\build.ps1 -Action run` now prints and uses `timeoutSeconds=15` by default.
- Timed runs kill `ps2_game.exe` automatically after the timeout and still write redirected logs.
- The current runtime blocker is upstream of `GetLoadedTexture`: the caller is passing garbage `TexturePack` / texture-id state.

## Validation
- Reviewed `output\Debug\raw_log.txt`.
- Confirmed:
  - `[game_overrides] alpha registrations ready ...`
  - `[nfs-hp2-alpha] enter GetLoadedTexture override ...`
- Observed bad caller values such as:
  - `pack=0x0`
  - `pack=0x2cce48`
  - `texture=0x8868656f`
  - `table=0x74736f68`

## Follow-up
- Trace `GetTextureInfo__FUii_0x180650` and the `TexturePackList` root at `gp-0x5808`.
- The next fix should target the caller/list state feeding `GetLoadedTexture`, not the lookup body itself.
