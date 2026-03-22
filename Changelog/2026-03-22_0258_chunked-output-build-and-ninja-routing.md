# Chunked Output Build And Ninja Routing

## Summary
- Split the generated `output` runner build into chunked object-library targets in [output/CMakeLists.txt](/D:/Repos/Recomp/PS2Recomp/output/CMakeLists.txt).
- Added smart build routing and output-backend selection to [build.ps1](/D:/Repos/Recomp/PS2Recomp/build.ps1).
- Added optional Ninja Multi-Config configure/build support for `output` via `output/build-ninja`.

## Reason
- The biggest iteration cost is the `output` build, even when almost all generated sources are unchanged.
- The previous setup treated all generated `output/*.cpp` as one monolithic executable target.
- A chunked target layout gives the build system a narrower invalidation boundary.
- A Ninja backend is likely preferable for the generated-source runner, but the wrapper needed a clean way to configure and select it.

## Files Changed
- `output/CMakeLists.txt`
- `build.ps1`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `output/CMakeLists.txt` now:
- sorts generated source files
- groups them into object-library chunks
- links `ps2_game` from those chunk targets
- preserves per-config output locations under `output\Debug`, `output\Release`, etc.
- `build.ps1` now:
- supports `-Action smart`
- supports `-OutputBackend auto|vs|ninja`
- auto-selects Ninja for `output` builds when `ninja` is installed
- configures `output/build-ninja` with `Ninja Multi-Config` on first use
- still keeps runtime builds on the existing `out\build` path

## Validation
- Parsed [build.ps1](/D:/Repos/Recomp/PS2Recomp/build.ps1) with the PowerShell parser; no parse errors were reported.
- Checked local tool availability:
```powershell
Get-Command ninja
```
- Result: Ninja is not installed in this environment right now, so only the wrapper logic was locally validated, not the Ninja configure path.

## Follow-up
- Use:
```powershell
.\build.ps1 -Action smart
```
- If Ninja is installed later, use:
```powershell
.\build.ps1 -Action smart -OutputBackend ninja
```
- Next practical check is a real local `output` rebuild to confirm the chunked target layout materially reduces incremental build cost.
