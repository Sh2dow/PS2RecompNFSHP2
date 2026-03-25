# Debug bOpen 0xAAAAAAAA after input init

## Summary
- Game stuck in initialization loop - loads files, runs eFixUpTables, GenerateEventsFromScanners, then bOpen with 0xAAAAAAAA every frame
- Bad pointer rejection in bOpen works (returns null), but game still loops

## Reason
- Need to trace where 0xAAAAAAAA is coming from after GenerateEventsFromScanners
- The game expects joystick/input initialization to complete, but alpha build has incomplete input system

## Files Changed
- `ps2xRuntime/src/lib/game_overrides.cpp` - bOpen bad pointer rejection active

## Build / Runtime Impact
- Current: bOpen returns null for 0xAAAAAAAA, game loops
- Need to find what's triggering bOpen with bad path after input init

## Validation
- Log shows: `bypassed GenerateEventsFromScanners joystick=0x2a1850 requestedConfig=0x0`
- Then immediately: `bOpen:rejected-bad-path pathAddr=0xaaaaaaaa ra=0x1f95a0`
- ra=0x1f95a0 is return address from bOpen wrapper

## Follow-up
- Add more tracing to find what's calling bOpen with bad path
- Consider making game proceed past this point by mocking more state
