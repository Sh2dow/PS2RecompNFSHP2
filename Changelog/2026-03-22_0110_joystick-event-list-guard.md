# Joystick Event List Guard

## Summary
- Mapped the live black-screen PC sequence to `FindEventNode__8Joystick8JoyEvent_0x140208`.
- Added an ALPHA override for `0x140208` in `ps2xRuntime/src/lib/game_overrides.cpp`.
- The override searches the joystick event list normally, but now detects circular-node revisits and runaway traversal and returns `null` instead of spinning forever.

## Reason
- Updated `run:tick` sampling showed the active guest thread cycling at `0x14021C`, `0x140228`, and `0x14022C`.
- Those PCs are inside the circular list walk in `FindEventNode__8Joystick8JoyEvent`.
- The list was not reaching its sentinel, which explains the black-screen/live-thread hang.

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- The next run should either:
  - log a joystick list cycle/runaway and continue past the old hang, or
  - expose the next startup blocker after `AddJoyHandler`.
- No generated output files were edited.

## Validation
- Source patch applied successfully with `apply_patch`.
- No build was run in this step.
- Next validation command:
  - `.\build.ps1 -Action runtime`
  - `.\build.ps1 -Action run`

## Follow-up
- Inspect `output/Debug/log.txt` for:
  - `[nfs-hp2-alpha] joystick event list cycle detected ...`
  - `[nfs-hp2-alpha] joystick event list runaway ...`
- If the run moves forward, use the next live `run:tick` PC or new error log as the next target.
