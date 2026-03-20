# Output Include Order Fix

## Summary
- Reordered include directories in `output\CMakeLists.txt` so `ps2xRuntime\include` is searched before `output\`.

## Reason
- `output\ps2_runtime.h` is a stale compatibility copy and was overriding the real runtime header during `ps2_game` compilation.
- That caused `output\main.cpp` to fail with `error C2039: 'finalizeFunctionTable': is not a member of 'PS2Runtime'` even though the real runtime header declared it.

## Files Changed
- `output\CMakeLists.txt`

## Build / Runtime Impact
- The next `Build output only` should regenerate the Visual Studio project and compile `ps2_game` against the real runtime API from `ps2xRuntime\include`.

## Validation
- Confirmed stale header exists at `output\ps2_runtime.h`.
- Confirmed generated `ps2_game.vcxproj` previously listed `output\` before `ps2xRuntime\include` in `AdditionalIncludeDirectories`.
- Patched include order in CMake so future regeneration prefers the runtime header.

## Follow-up
- Run `.\build.ps1 -Action output`.
- If a build error remains, inspect the newest log under `tools\ai\artifacts\build-logs\`.
