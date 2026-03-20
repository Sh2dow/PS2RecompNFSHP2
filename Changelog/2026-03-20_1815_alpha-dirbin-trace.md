# Alpha DIR.BIN Trace Prep

## Summary
- Added ALPHA-only trace wrappers for `OpenCDFile` and `ReadCDSectorASync`.
- Added broader `DIR.BIN` fallback logic in the ALPHA `LoadDirectory` override.
- Rebuilt incrementally and reran the executable to validate the new trace path.

## Reason
- The active blocker was upstream of `sceCdRead`: we needed visibility into guest path and directory-entry metadata before the unresolved CD request.
- The rerun exposed an earlier blocker: the override still resolves `DIR.BIN` to a stale prototype folder path.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Incremental build still succeeds.
- The runtime now has ALPHA-specific hooks ready for `OpenCDFile` and `ReadCDSectorASync`.
- Those hooks are currently gated by `DIR.BIN` resolution, which fails before the deeper trace path becomes active.

## Validation
- Build command:
  - `cmd.exe /c "call ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"" && cmake --build output\build"`
- Run command:
  - `D:\Repos\Recomp\PS2Recomp\output\Debug\ps2_game.exe F:\Games\ISO\PS2\ALPHA\SLUS_203.62`
- Observed result:
  - build succeeded
  - runtime still reports `DIR.BIN missing at F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (Mar 6, 2002 prototype)/DIR.BIN`
  - actual on-disk file is `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002/DIR.BIN`

## Follow-up
- Inspect the effective `cdRoot` / `cdImage` values seen by the ALPHA `LoadDirectory` override.
- Fix media-path normalization first.
- Then rerun and use the new wrappers to continue tracing the original `OpenCDFile -> ReadCDSectorASync -> sceCdRead` corruption path.
