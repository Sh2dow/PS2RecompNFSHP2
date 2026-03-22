# Smart Incremental Build Routing

## Summary
- Added a `smart` build mode to [build.ps1](/D:/Repos/Recomp/PS2Recomp/build.ps1).
- The wrapper now inspects source timestamps and chooses one of:
- no build
- `output` only
- `ps2_runtime` plus `output`
- Updated the interactive menu so smart incremental build is the first option.

## Reason
- The manual choice between `output` and `runtime` rebuilds was slowing the iteration loop.
- Most current work alternates between runtime-only edits under `ps2xRuntime\` and generated/output-side edits under `output\`.
- The wrapper already knew how to build the narrow targets; it just did not know when to skip work or when a runtime rebuild was actually required.

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- `.\build.ps1 -Action smart` now compares:
- latest `ps2xRuntime` source timestamps against `out\build\ps2xRuntime\<Config>\ps2_runtime.lib`
- latest generated/output source timestamps against `output\<Config>\ps2_game.exe`
- If only `output` sources changed, it runs the cheaper `ps2_game` target build.
- If runtime sources changed, it rebuilds `ps2_runtime` and then relinks `ps2_game`.
- If nothing relevant changed, it exits without building.

## Validation
- Parsed [build.ps1](/D:/Repos/Recomp/PS2Recomp/build.ps1) with the PowerShell parser:
```powershell
[System.Management.Automation.Language.Parser]::ParseFile(...)
```
- No parse errors were reported.

## Follow-up
- Use:
```powershell
.\build.ps1 -Action smart
```
- Keep `-Action output` and `-Action runtime` for forced/manual staging when needed.
- If source classification still feels too broad or too narrow in practice, the next refinement is a path allowlist for specific runtime subtrees or generated files rather than a full recursive scan.
