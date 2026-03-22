# Duplicate Address Override Replacement

## Summary
- Patched `PS2Runtime::registerFunction()` to replace an existing same-address entry instead of always appending a duplicate.

## Reason
- The logs showed `[game_overrides] applied 1 matching override(s).` for `SLUS_203.62`, but runtime still executed the old `TempDirectoryEntry` stub and never reached the new `LoadDirectory` remap log.
- The cause was duplicate address registration: override callbacks registered addresses such as `0x1FA548` and `0x1FA888` after earlier generated/stub entries already existed.
- Because lookup uses `lower_bound()`, duplicate entries at the same address could still resolve to the older entry first.

## Files Changed
- `ps2xRuntime/src/lib/ps2_runtime.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Same-address override registrations now take precedence deterministically.
- ALPHA override bindings for `TempDirectoryEntry` and `LoadDirectory` should now replace the prior generated/stub handlers instead of competing with them.

## Validation
- Source-level verification only in this step.
- The issue is grounded in observed logs:
  - override match was present
  - `TempDirectoryEntry` still ran through the old stub path
  - no `remapped=` startup marker appeared
- Did not run a rebuild here; this patch is prepared for the next local runtime/output build.

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- Confirm startup now shows:
  - `[game_overrides] applied ...`
  - `[nfs-hp2-alpha] loaded DIR.BIN override entries=... remapped=...`
- Confirm `__18TempDirectoryEntryiiPCcT3` no longer appears as an unimplemented stub in `stderr.log`.
