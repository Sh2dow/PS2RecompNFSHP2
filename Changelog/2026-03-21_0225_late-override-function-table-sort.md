# Late Override Function Table Sort

## Summary
- Updated `ps2xRuntime/src/lib/ps2_runtime.cpp` so `registerFunction()` re-sorts the function table when a late registration appends an address lower than the current tail.

## Reason
- `raw_log.txt` proved the `SLUS_203.62` ALPHA override was applied, yet dispatch still failed on `0x1FA7A0`.
- The function table is finalized before `loadELF()`, but game overrides are applied inside `loadELF()`. That means override handlers like `0x1FA7A0` were being appended after sorting, which breaks `lower_bound`-based lookup.

## Files Changed
- `ps2xRuntime/src/lib/ps2_runtime.cpp`

## Build / Runtime Impact
- Late game-override registrations now stay visible to `hasFunction()` and `lookupFunction()`.
- This specifically targets continuation PCs registered during `ps2_game_overrides::applyMatching()` after the initial function-table finalization.

## Validation
- Confirmed from `output\Debug\raw_log.txt` that:
  - `[game_overrides] applied 1 matching override(s).`
  - dispatch still reported `Function at address 0x1FA7A0 not found`
- Confirmed the root ordering issue from code flow:
  - `output/main.cpp` calls `registerAllFunctions(runtime); runtime.finalizeFunctionTable();`
  - `PS2Runtime::loadELF()` then calls `ps2_game_overrides::applyMatching(*this, elfPath, m_cpuContext.pc);`

## Follow-up
- Rebuild runtime and output, then rerun.
- Check `ps2_register_trace.txt` for the new `resorted function table after late registration` line.
- If `0x1FA7A0` still misses after this, inspect duplicate-address handling or any thread-resume code that bypasses the registered handler entirely.
