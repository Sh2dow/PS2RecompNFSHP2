# Unpacked ZZDATA DIR Remap

## Summary
- Added a host-file registration hook in `ps2_stubs.cpp` so runtime can assign pseudo-LBNs directly to extracted files.
- Updated CD path resolution helpers to prefer an auto-detected `ZZDATA` unpacked root for host-file registration.
- Updated the ALPHA `LoadDirectory` override to rewrite `DIR.BIN` entries onto pseudo-LBNs backed by files from the unpacked `ZZDATA` tree when hashes can be resolved.

## Reason
- The current runtime got past the `0x1FA7A0` continuation issue but still failed in CD loading with `sceCdGetError() == -1`.
- The fallback `sceCdRead` heuristics were becoming too speculative.
- The repository already had a largely extracted HP2 asset tree, so the safer approach was to map directory entries onto real host files instead of inferring more raw `DATA.BIN` read contracts.

## Files Changed
- `ps2xRuntime/src/lib/ps2_stubs.cpp`
- `ps2xRuntime/src/lib/stubs/helpers/ps2_stubs_helpers.inl`
- `ps2xRuntime/src/lib/game_overrides.cpp`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- Path-based file opens can now prefer the unpacked `ZZDATA` tree before falling back to the configured CD root.
- The ALPHA `LoadDirectory` override can now replace original `DIR.BIN` LBN/size pairs with pseudo-LBNs and actual host sizes for hashes resolved from the unpacked asset tree.
- If the remap works, subsequent `sceCdRead` requests should flow through existing pseudo-LBN sector reads instead of the previous raw-image fallback path.

## Validation
- Inspected `NFS.Unpacker` source to confirm the uppercase rolling hash function and `ZDIR.BIN`-driven archive model.
- Verified the unpacked asset tree at `F:\Games\ISO\PS2\HP2_A098_PS2_07_MAR_2002_OUT_Unpacker\ZZDATA`.
- Verified `__Unknown_Resolved\inventory.csv` exposes hash-to-path mappings for unresolved assets.
- Did not run a rebuild in this step; this patch is prepared for the next local `.\build.ps1 -Action runtime` and `.\build.ps1 -Action run`.

## Follow-up
- Rebuild runtime and output locally, then inspect `output\Debug\raw_log.txt` for the `loaded DIR.BIN override ... remapped=... unpackedRoot=...` line.
- If CD failure persists, capture the remaining unresolved hashes/paths and narrow the gap between `DIR.BIN` entries and extracted assets instead of broadening `sceCdRead` argument fallbacks.
