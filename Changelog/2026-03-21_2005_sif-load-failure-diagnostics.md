# SIF Load Failure Diagnostics

## Summary
- Added explicit `SifLoadModule` failure diagnostics for empty guest paths and rejected normalized paths.

## Reason
- The formatter stub fix removed `_str_out_str__FPCciPi` warnings, but the reboot path still failed with repeated `gave error -1 (unknown)` messages and `Reboot failed!`.
- Existing logging only showed successful module loads. Failed `sceSifLoadModule` calls returned `-1` without preserving the actual guest path state that caused the rejection.

## Files Changed
- `ps2xRuntime/src/lib/syscalls/ps2_syscalls_rpc.inl`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- The next runtime build will report whether `sceSifLoadModule` is failing because:
  - the guest path buffer is empty at the syscall boundary, or
  - the path string is present but normalizes to an unusable key
- This does not change successful load behavior; it only improves diagnostics on failure.

## Validation
- Source-level validation only in this step.
- Confirmed from the latest `output/Debug/log.log` that:
  - `_str_out_str__FPCciPi` warnings are gone
  - `Reboot failed!` still occurs
  - there are still no normal `[SIF module] load ...` lines before failure

## Follow-up
- Rebuild with `.\build.ps1 -Action runtime`.
- Rerun with `.\build.ps1 -Action run`.
- Inspect the new `[SIF module] load failed: ...` line in `log.log` / `raw_log.txt` and patch the actual module-path issue from there.
