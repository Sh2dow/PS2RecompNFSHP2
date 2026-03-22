# Formatter String Output Stubs

## Summary
- Implemented `_str_out_char__FcPi` and `_str_out_str__FPCciPi` in the runtime stub layer.
- The new handlers write into the guest output buffer and advance the mutable output-pointer cell used by `_bOutput`.

## Reason
- Boot was no longer blocked on directory bootstrap. `DIR.BIN` remap was active, but the next failure was `Reboot failed!`.
- Tracing showed that the reboot/module-load path hits `_str_out_str__FPCciPi` repeatedly just before `sceSifLoadModule` failures.
- `sceSifLoadModule` only returns `-1` in the current runtime when it receives an empty or unusable path, so the formatter stubs were the highest-probability cause of malformed IRX module strings.

## Files Changed
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- `%c` and `%s` output through the string formatter path should now populate guest buffers instead of no-oping through `TODO_NAMED`.
- This directly affects reboot/module path construction and any other in-game `bSPrintf` / `bVSPrintf` callers that rely on the string-output callbacks.

## Validation
- Source-level validation only in this step.
- Traced call flow:
  - `bRebootPS2__FPcT0PPci_0x1fa1e8`
  - `bSPrintf__FPcPCce_0x1f64c8`
  - `bVSPrintf__FPcPCcT0_0x1f6570`
  - `_bOutput__FcPCcPc_0x1f6620`
  - `_str_out_char__FcPi` / `_str_out_str__FPCciPi`
- Did not run a rebuild here; this patch is prepared for the next local `runtime` build and run.

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- Verify:
  - `_str_out_str__FPCciPi` warnings disappear from `stderr.log` / `raw_log.txt`
  - `sceSifLoadModule` starts seeing non-empty IRX paths
  - `Reboot failed!` disappears or narrows to a specific module-load issue
