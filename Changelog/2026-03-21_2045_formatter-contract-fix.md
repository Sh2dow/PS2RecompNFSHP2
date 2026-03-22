# Formatter Contract Fix

## Summary
- Corrected the runtime implementations of `_str_out_char__FcPi` and `_str_out_str__FPCciPi` to use the global current-output pointer at `gp-0x47FC`.
- The `Pi` argument is now treated as a count accumulator, not as an output-pointer cell.

## Reason
- The prior pointer-sync patch did not change the observed failure: `sceSifLoadModule` still saw an empty path with preview bytes `00 48 00 ...`.
- Inspection of `_bOutput__FcPCcPc_0x1f6620.cpp` showed the actual contract:
  - `_str_out_char__FcPi` is called with just `(char, int*)`
  - `_str_out_str__FPCciPi` is called with `(const char*, int, int*)`
  - the real destination pointer is stored globally at `gp-0x47FC`
- Treating the `Pi` parameter like an output-pointer cell was therefore wrong and could not fix the string clobber.

## Files Changed
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- String-format output now follows the generated formatter’s actual ABI.
- This should allow `bVSPrintf` / `_bOutput` to build non-empty module paths for `sceSifLoadModule` during reboot.

## Validation
- Source-level validation only in this step.
- Grounded in observed runtime evidence:
  - `[SIF module] load failed: empty path ... preview=0 48 0 ...`
- Grounded in generated control-flow evidence from `_bOutput__FcPCcPc_0x1f6620.cpp`:
  - write destination maintained at `gp-0x47FC`
  - `Pi` parameters used as mutable counters

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- Verify whether `sceSifLoadModule` now receives a non-empty path.
- If reboot still fails, use the first concrete non-empty module path in the log to fix path translation or loader behavior.
