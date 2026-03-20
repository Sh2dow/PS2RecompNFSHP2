# Crash Dialog Suppression And Register Trace

## Summary
- Added runner-side MSVC/Windows crash-report suppression so debug aborts can be routed to `stderr` instead of a modal dialog.
- Added temporary bootstrap and registration trace logging to narrow the current startup blocker.

## Reason
- `ps2_game.exe` was stopping on a blocking Visual C++ Runtime Library dialog, which breaks automated run/trace workflow.
- The active runtime blocker now reproduces during `registerAllFunctions(runtime)`, so startup-stage tracing was needed.

## Files Changed
- `output\main.cpp`
- `ps2xRuntime\src\lib\ps2_runtime.cpp`
- `ps2xRuntime\include\ps2_runtime.h`

## Build / Runtime Impact
- Once rebuilt into `output\Debug\ps2_game.exe`, debug CRT aborts should stop showing the modal dialog and instead report through standard error.
- Temporary trace files currently used:
- `bootstrap_log.txt`
- `ps2_init_trace.txt`
- `ps2_register_trace.txt`

## Validation
- Runtime tracing confirmed `PS2Runtime::initialize()` completes.
- Bootstrap tracing confirmed the next failure boundary is `registerAllFunctions(runtime)`.
- Registration tracing currently stops on the first `registerFunction()` insertion.
- The updated runner suppression code is patched but not yet verified in a rebuilt executable because the `output` relink is still taking too long / timing out in the current shell runs.

## Follow-up
- Complete a clean relink of `output\Debug\ps2_game.exe` and verify the abort dialog no longer appears.
- Continue isolating why the first function-table insert fails during `registerAllFunctions(runtime)`.
