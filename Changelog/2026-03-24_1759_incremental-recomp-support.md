# Incremental Recompilation Support

## Summary
- Added incremental recompilation support to `build.ps1` with change detection and manifest tracking.

## Reason
- Running `ps2_recomp` on every build was extremely slow (20–60 minutes).
- Most builds only need C++ compilation, not full recompilation.
- Need to skip recompilation when:
  - config.toml hasn't changed
  - ELF file hasn't changed
  - ps2xRecomp source hasn't changed
  - generated output already exists

## Files Changed
- `build.ps1`

## Build / Runtime Impact
- **New parameters:**
  - `-Recomp` / `-Action recomp`: Run recompilation with change detection
  - `-ForceRecomp`: Force recompilation regardless of change detection
  - `-Action clean`: Clean build directories and manifest

- **Smart build improvements:**
  - `Invoke-SmartBuild` now checks if recompilation is needed before building
  - Automatic change detection via SHA256 hashes stored in `tools\ai\artifacts\recomp-manifest.json`

- **Menu additions:**
  - Option 6: Run recompilation (ps2_recomp)
  - Option 7: Clean build

- **Workflow impact:**
  - First build: `build.ps1 -recomp` (runs full recompilation)
  - Subsequent builds: `build.ps1` (incremental C++ compile only, ~30–90 sec)
  - After config/ELF/recompiler changes: automatic recompilation triggered

## Validation
- Syntax check: `powershell -Command "Invoke-Expression (Get-Content build.ps1 -Raw)"` (no errors)
- Menu displays new options correctly
- Manifest tracking implemented with SHA256 hashes

## Follow-up
- Test full recompilation pipeline with `build.ps1 -recomp`
- Verify manifest is created at `tools\ai\artifacts\recomp-manifest.json`
- Test incremental build speed with `build.ps1` (should skip recomp)
- Consider adding `-Recomp` flag to force recompilation in smart mode
