# TexturePack Indexed Lookup

## Summary
- Added an ALPHA-specific override for `GetLoadedTexture__11TexturePackUi_0x180b30`.
- The override builds a host-side index for the attached `TexturePack` table and replaces the generated linear scan with an indexed lookup.
- The lookup preserves the original entry-validity checks based on fields at `+0x8C`, `+0x3C`, and `+0x90`.

## Reason
- The latest `output\Debug\log.log` shows startup spending seconds in `0x180B50` and `0x180688`, which map to `GetLoadedTexture` and its caller `GetTextureInfo`.
- After the earlier async and queued-file fixes, the generated O(n) scan over `0xA0`-byte texture entries became the next visible blocker.

## Files Changed
- `ps2xRuntime\src\lib\game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- A new ALPHA override is registered at guest PC `0x180B30`.
- Runtime now caches texture-entry addresses by texture id for the current `TexturePack` table instead of rescanning the full table on every lookup.
- The override validates `tableAddr` and `entryCount` before indexing and falls back to the generated scan if the table shape looks invalid.

## Validation
- Reviewed `output\Debug\log.log` and mapped the live PCs `0x180B50` and `0x180688` to:
  - `GetLoadedTexture__11TexturePackUi_0x180b30.cpp`
  - `GetTextureInfo__FUii_0x180650.cpp`
- Reviewed `LoadTextureTableFromFile__11TexturePackPCc_0x180c18.cpp` and `AttachTextureTable__11TexturePackP11TextureInfoi_0x1808e0.cpp` to confirm the relevant `TexturePack` fields:
  - table pointer at `this+0x60`
  - entry count at `this+0x64`
- Build/test not run in this step.

## Follow-up
- Rebuild and relink with:
  - `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Rerun and confirm the new log lines:
  - `[nfs-hp2-alpha] indexed TexturePack ...`
  - `[nfs-hp2-alpha] GetLoadedTexture ...`
- If startup still stalls after the indexed lookup is active, inspect `LoadTextureTableFromFile` / `AttachTextureTable` semantics instead of the lookup loop.
