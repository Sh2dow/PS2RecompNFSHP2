# sceCdRead Byte-Range Fallback

## Summary
- Added a guarded fallback in `ps2xRuntime/src/lib/stubs/ps2_stubs_ps2.inl` so `sceCdRead` can service requests that look like absolute byte-offset + byte-count reads instead of sector-aligned LBN + sector-count reads.

## Reason
- The active unresolved request in `output/Debug/raw_log.txt` is:
- `pc=0x2264c0 ra=0x1fa4d0 a0=0x1070036 a1=0x3181 a2=0x7282a0`
- That shape is inconsistent with the one-sector `AsyncRealCDRead` path and strongly suggests a wrapper leaking byte-oriented arguments into `sceCdRead`.
- The newly discovered `3AFE35FF.elf` is CD/DVD-heavy and supports the conclusion that the current blocker lives in the CD read contract rather than unrelated game logic.

## Files Changed
- `ps2xRuntime/src/lib/stubs/ps2_stubs_ps2.inl`

## Build / Runtime Impact
- Normal sector-based `sceCdRead` behavior remains first priority.
- If that fails, and the request has an unaligned `a0` with `a1 > 1`, runtime now tries:
- `a0` as absolute byte offset into the configured CD image
- `a1` as byte count
- `a2` as destination buffer

## Validation
- Code-path evidence from generated output:
- `ReadCDSectorASync__FPvii_0x1fa4a0` calls `sceCdRead_0x2264c0`
- `ReadSector__FPvii_0x1fa510` is fed by multiple callers, not only the one-sector async path
- Runtime evidence from `raw_log.txt`:
- `Loading entire directory...`
- `[sceCdRead] unresolved request pc=0x2264c0 ra=0x1fa4d0 a0=0x1070036 a1=0x3181 a2=0x7282a0`

## Follow-up
- Rebuild runtime/output and rerun.
- Check for:
- `[sceCdRead] recovered with byte-range fallback ...`
- If recovery works, continue tracing higher-level directory parsing and verify contents are sensible.
- If it does not, the next step is to instrument the direct `ReadSector__FPvii` callers to identify the exact argument contract per caller.
