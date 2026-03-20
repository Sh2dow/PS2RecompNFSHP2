# Stale Output Runtime Header Removal

## Summary
- Updated `build.ps1` to remove `output\ps2_runtime.h` before `ps2_game` builds.

## Reason
- Generated sources in `output\` use `#include "ps2_runtime.h"`.
- Quoted includes search the source file directory first, so the stale compatibility header in `output\` overrides the real runtime header in `ps2xRuntime\include`.
- That caused build failures like `PS2Runtime::finalizeFunctionTable` not being found even after the real runtime header was updated.

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- `.\build.ps1 -Action output` now deletes the stale compatibility copy before compiling generated sources.
- Generated sources should then resolve `ps2_runtime.h` from `ps2xRuntime\include`.

## Validation
- Confirmed `output\ps2_runtime.h` exists.
- Confirmed thousands of generated source files in `output\` include `ps2_runtime.h` with quoted includes.
- Confirmed build log failure matched the stale-header symptom.

## Follow-up
- Run `.\build.ps1 -Action output` again.
- If the next failure moves, inspect the newest log under `tools\ai\artifacts\build-logs\`.
