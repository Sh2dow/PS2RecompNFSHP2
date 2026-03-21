# Run Log And Override Probe

## Summary
- Updated `build.ps1` so `-Action run` writes the raw command-line output to `output\Debug\raw_log.txt` automatically.
- Added startup probe logging in `ps2xRuntime/src/lib/game_overrides.cpp` to show the ELF name, entry, descriptor count, and whether any game override matched.

## Reason
- The current runtime blocker is visible in raw stdout/stderr rather than `ps2_log.txt`, so the unified runner needed to produce the same `raw_log.txt` artifact without manual shell redirection.
- `0x1FA7A0` is confirmed to be an internal `RecurseDirectory` continuation label, yet runtime still reports it as missing. The next run needs an explicit answer on whether the ALPHA override descriptor is being applied.

## Files Changed
- `build.ps1`
- `ps2xRuntime/src/lib/game_overrides.cpp`

## Build / Runtime Impact
- `.\build.ps1 -Action run` now emits `output\Debug\raw_log.txt` by default.
- `.\build.ps1 -Action run -NoWait` and `.\build.ps1 -Action run -TailLog` still launch the executable directly and do not redirect stdout/stderr.
- Startup output now reports game-override matching status, which should explain why the `0x1FA7A0` continuation handler is or is not active for `SLUS_203.62`.

## Validation
- Inspected `output\RecurseDirectory__FPt6bTList1Z18TempDirectoryEntryUiUiPCci_0x1fa648.cpp` and confirmed `0x1FA7A0` is an internal label target, not a normal top-level function entry.
- Verified `ps2xRuntime/src/lib/game_overrides.cpp` already registers `0x1FA7A0` to `nfsHp2AlphaRecurseDirectoryContinuation`.

## Follow-up
- Rebuild runtime and output, then run through `.\build.ps1 -Action run`.
- Check `output\Debug\raw_log.txt` for `[game_overrides] probe ...` and either `[game_overrides] applied ...` or `no matching overrides ...`.
- If no override matches, trace the ELF name / entry mismatch path; if it does match, continue diagnosing why dispatch still misses `0x1FA7A0`.
