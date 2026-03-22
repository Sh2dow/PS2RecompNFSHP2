# Formatter Output Pointer Sync

## Summary
- Updated `_str_out_char__FcPi` and `_str_out_str__FPCciPi` to keep the runtime's global string-output pointer in sync with the local callback state.

## Reason
- The SIF load diagnostics showed repeated `empty path` failures at `pathAddr=0x1ffbe00`, but the preview bytes were `00 48 00 ...`.
- That pattern means formatting was producing output bytes while the final NUL terminator was still being written at the beginning of the buffer.
- In this runtime, `bVSPrintf` keeps a global “current output pointer” at `gp-0x47FC`. The string-output callbacks were only advancing their local state cell, leaving the global pointer stale.

## Files Changed
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- String-format callbacks now advance both:
  - the local callback state cell passed to `%c` / `%s`
  - the runtime-global current output pointer used by `bVSPrintf` for final termination
- This should prevent formatted strings from being nulled at byte zero after successful writes.

## Validation
- Source-level validation only in this step.
- Grounded in observed log evidence from `output/Debug/log.log`:
  - `[SIF module] load failed: empty path ... preview=0 48 0 ...`
  - which strongly implies a trailing-NUL placement bug rather than a pure missing-format-output bug

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- Verify that `SifLoadModule` no longer reports `empty path`.
- If reboot still fails, use the next emitted non-empty module path to fix translation or loader handling directly.
