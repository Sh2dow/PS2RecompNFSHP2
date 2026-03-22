# Reconnect Guards And Sound Init

## Summary
- Added ALPHA-specific guards for model, animation, AFX, and sound reconnect/update paths in `ps2xRuntime/src/lib/game_overrides.cpp`.
- Completed the host-backed texture-table fast path so `TexturePack` metadata and payload windows load from unpacked `ZZDATA` instead of falling back to broken prototype state.
- Added a sound common-region fallback that now disables malformed prototype common-region data and marks sound init ready instead of leaving `InitializeSoundDriver` in an endless empty-queue poll.

## Reason
- Startup had moved past earlier CD, reboot, VSync, and queue issues but kept getting trapped by poisoned prototype lists and malformed sound-region data.
- Generated reconnect code assumed valid linked-list sentinels and well-formed region chunks; the prototype build does not meet those assumptions consistently.
- The sound common-region override previously prevented bad reads, but it also removed the callback path that flips the sound-init completion bit.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Runtime behavior for `SLUS_203.62` now favors bounded host-side recovery over infinite guest loops in:
- model reconnect
- animation reconnect
- AFX texture refresh
- sound common-region startup
- The game should no longer stall on the earlier texture/model/animation loops, and sound init can now advance even when the prototype common-region chunk table is malformed.

## Validation
- Rebuilt and relinked incrementally with:
- `.\build.ps1 -Action runtime-output -OutputBackend ninja`
- Ran timed runtime captures with:
- `.\build.ps1 -Action run`
- Verified in `output\Debug\raw_log.txt` that:
- texture tables load through the host fast path from `ZZDATA`
- reconnect/list guard logs replace prior infinite-loop PCs
- malformed sound common-region data is detected and disabled
- the new active boundary moved to `InitializeSoundDriver -> bServiceFileSystem`

## Follow-up
- Verify that the new sound-init completion write removes the `pc=0x1f8ae8/0x1f8b58/0x1f8b04 ra=0x183f70` poll loop.
- If the loop persists, trace the remaining sound-init completion path through `SoundDriverCallback__FUiUiUi_0x183c70` and `SetSignal__6bSoundUiUi_0x20d5d8`.
