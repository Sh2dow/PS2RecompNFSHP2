# Sound Probe And List Guards

## Summary
- Tightened the ALPHA `AddTexturesSorted` override so scratchpad/empty-list cases now skip the old `AddTail` fallback.
- Removed ctor-side `NisManager` list insertion and added a `DrawNisObjects` guard that resets or trims poisoned NIS list state.
- Added targeted probes for `BeginLoad__6bSound` and `Open__12cBaseSpeaker` to identify which later sound/speech path feeds poisoned `bOpen(FPCc)` pointers.

## Reason
- Fresh timed runs showed the old scratchpad `TextureDownloader` fallback was still mutating a scratchpad destination list.
- The render path moved to `RenderClans -> GetPixelSize`, but a later `bOpen(FPCc)` still receives `pathAddr=0xAAAAAAAA`, so the next step had to rule out the early sound-header/data load path.

## Files Changed
- `ps2xRuntime\src\lib\game_overrides.cpp`
- `ps2xRuntime\src\lib\ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `AddTexturesSorted` no longer manufactures guest list state from a scratchpad downloader with an empty source list.
- The runtime now logs `BeginLoad__6bSound` input paths and `Open__12cBaseSpeaker` inputs when those codepaths are reached.
- The latest timed run confirms `BeginLoad__6bSound` receives valid `SOUND\SNDHDR.BIN` and `SOUND\SNDDATA.BIN` pointers, so the later poisoned `bOpen` comes from a different downstream caller.

## Validation
- `powershell -ExecutionPolicy Bypass -File .\build.ps1 -Action runtime-output -OutputBackend ninja`
- `powershell -ExecutionPolicy Bypass -File .\build.ps1 -Action run`
- Observed in `output\Debug\raw_log.txt`:
- `AddTexturesSorted skip fallback ... downloaderScratch=1 ... source-empty`
- `BeginLoad__6bSound ... pathAText="SOUND\SNDHDR.BIN" pathBText="SOUND\SNDDATA.BIN"`
- later `bOpen:return-null pathAddr=0xAAAAAAAA ... flags=0x1`

## Follow-up
- Continue from the later bad-open path, not from `BeginLoad__6bSound`.
- Strongest remaining suspects are `Open__12cBaseSpeaker`, `Initialize__9cSEDWedge`, and related speech/sample loaders that still pass a poisoned path pointer into `bOpen(FPCc)`.
