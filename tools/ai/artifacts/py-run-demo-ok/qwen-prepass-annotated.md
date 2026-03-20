# Annotated Prompt

[role] qwen-prepass
[phase] PHASE_RUNTIME_BUILD
[blocker] Trace unresolved sceCdRead request
[command] cmake --build output/build
[relevant_files] ps2xRuntime/src/lib/game_overrides.cpp, ps2xRuntime/src/lib/ps2_stubs.cpp

## State Excerpt
<!-- ⛔ RULES — Re-read this section EVERY TIME you open this file. -->
<!-- These rules also appear in the SKILL.md. Redundancy is intentional. -->

# ⛔ QUICK RULES (mandatory re-read)

1. **Build:** `cmake --build <build_dir>` — NEVER add `--clean-first`, `--target clean`, or delete the build directory.
2. **Files:** NEVER modify `runner/*.cpp`. Fix in `src/lib/` or game overrides.
3. **Headers:** NEVER modify `.h` without asking user. Triggers mass rebuild.
4. **Git:** NEVER use destructive git commands (`checkout`, `clean`, `reset`, `stash`, `pull`).
5. **Verify:** NEVER assume file names/paths. Use tools (`list_dir`, `find_by_name`, `grep_search`).

---

# PS2 Recomp — Project State
> Auto-maintained by agent. DO NOT DELETE. Read at session start, update after every major action.

## Boot Status
- [x] Loaded `03-ps2recomp-pipeline.md`
- [x] Loaded `04-runtime-syscalls-stubs.md`
- [x] Verified comprehension (3 questions answered)

## Game Info
- **Title**: Need for Speed: Hot Pursuit 2 Alpha
- **Region**: NTSC-U
- **Has Symbols**: partial

## Workspace Paths
- **PS2Recomp Repo**: `D:/Repos/Recomp/PS2Recomp`
- **Game Workspace**: `F:/Games/ISO/PS2/ALPHA`
- **ISO Path**: `F:/Games/ISO/PS2/ALPHA`
- **Output Dir**: `D:/Repos/Recomp/PS2Recomp/output`

## Binaries
| Role | File Name | Type | Path | TOML Config | Status |
|------|-----------|------|------|-------------|--------|
| Main | SLUS_203.62 | ELF | `F:/Games/ISO/PS2/ALPHA/SLUS_203.62` | `F:/Games/ISO/PS2/ALPHA/SLUS_203.62.toml` | recompiled + built |
| Fallback analysis | SLUS_203.62_ps2xAnalyzer.toml | TOML | `F:/Games/ISO/PS2/ALPHA/SLUS_203.62_ps2xAnalyzer.toml` | `F:/Games/ISO/PS2/ALPHA/SLUS_203.62_ps2xAnalyzer.toml` | reference only |

## Environment Setup
- **Ghidra Install Path**: `D:/Development/Ghidra`

## Current Phase
PHASE_RUNTIME_BUILD

## Build Configuration
- **CMake Generator**: Visual Studio 18 2026
- **C++ Compiler**: MSVC 14.50.35717
- **Build Type**: Debug
- **Ghidra CSV Path**: `F:/Games/ISO/PS2/ALPHA/SLUS_203.62_cm.csv`
- **single_file_output**: false

## Active Runner Command
<!-- ACTIVE RUNNER COMMAND: -->
`D:/Repos/Recomp/PS2Recomp/output/build/Debug/ps2_game.exe "F:/Games/ISO/PS2/ALPHA/SLUS_203.62"`

## Unique Crashes & Subsystem Map
| Crash Address/PC | Subsystem | Callstack/Context | Proposed Fix Type | Regression Status | Resolution |
| ---------------- | --------- | ----------------- | ----------------- | ----------------- | ---------- |
| 0x125AE0 / 0x125C40 | bootstrap / kernel syscalls | Legacy monitor syscall `0x63` (`$a0=0x15`) triggered during platform probe | Runtime C++ | fixed in current build | handled as unavailable / returns 0 |
| 0x230210 / 0x234F90 / 0x230218 | libc heap stubs | `__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock` fired during allocator init | Runtime C++ | fixed in current build | no-op triage handlers added |
| 0x231348 | stdio | `vfprintf` received guest pointer-like file handle during shutdown/logging | Runtime C++ | fixed in current build | guest-handle fallback to `stdout` |
| 0x1FA7A0 | generated control flow | `RecurseDirectory` recursive tail path returned to internal label `0x1FA7A0`, which dispatcher treated as missing function | Game Override | fixed in current build | ALPHA-only continuation handlers bound for leaked internal PCs |
| 0x2264C0 / 0x1FA4D0 | CD streaming | `sceCdRead` receives unresolved sector request and game loops in fatal CD retry | Runtime C++ / game data-flow | active | current primary blocker |

## Resolved Stubs
| Address | Handler | Binding Method | Status | Notes |
| ------- | ------- | -------------- | ------ | ----- |
| export-derived | 395 named bindings | `F:/Games/ISO/PS2/ALPHA/SLUS_203.62.toml` stubs array | active | ExportPS2Functions.java classification is the authoritative baseline |

## Resolved Syscalls
| Syscall ID | Implementation | File | Status |
| ---------- | -------------- | ---- | ------ |
| 0x40 | CreateSema | `ps2xRuntime/src/lib/ps2_syscalls.cpp` | observed working |

## Temporary Triage Stubs
| Address | Triage Type | Caller Context | Priority | Notes |
| ------- | ----------- | -------------- | -------- | ----- |
| 0x223C08 | named stub | `InitAlarm` via syscall override path | high | still logs as unimplemented during boot |
| 0x125BF0 | named stub | `__main` | medium | boot continues past warning, but startup still logs it |
| 0x1F8530 | named stub | `_str_out_str__FPCciPi` | medium | repeated formatting helper warning during boot text |

## Unhandled Opcodes
| Address | Opcode | Type | Resolution |
| ------- | ------ | ---- | ---------- |
| none | none | scan result | `rg "Unhandled opcode" D:/Repos/Recomp/PS2Recomp/output` returned no matches after ALPHA recompilation |

## Known Issues
- [x] ALPHA export TOML was patched to emit generated C++ into `D:/Repos/Recomp/PS2Recomp/output` instead of `F:/Games/ISO/PS2/ALPHA/output`.
- [x] `output/main.cpp` now defaults to `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`.
- [x] `ps2_recomp` successfully regenerated 8,704 functions into repo `output/`.
- [x] `output/build/Debug/ps2_game.exe` was rebuilt at `2026-03-16 17:52:59`.
- [x] Runtime bootstrap no longer dies at internal PC `0x1FA7A0`; ALPHA game override continuation handlers are active.
- [x] `__18TempDirectoryEntryiiPCcT3` is no longer an unimplemented stub; ALPHA override now allocates/copies the guest path string and stores entry metadata.
- [x] `output/main.cpp` now pins the ALPHA prototype disc image: `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`.
- [x] `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` now has a runtime constructor implementation in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- [ ] Boot still enters repeated CD-error recovery because `sceCdRead` at `0x2264C0` receives an unresolved request (`ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`).
- [ ] Need to trace the directory/file-state path feeding `ReadCDSectorASync__FPvii_0x1fa4a0`; `bFile` ctor fix, `AsyncEntry` ctor fix, and explicit ALPHA ISO binding did not correct the bogus LBN path.
- [ ] Build wrappers left an idle `MSBuild` process after link; artifact output is valid, but exit-code confirmation from the wrapper remains noisy.

## Learned Patterns (Auto-growing)
- For this repo, the safest “from scratch” path is to keep the exported game TOML as the source of truth and only redirect its `output` path into the repo build target.
- A clean ALPHA baseline means: `ps2_recomp` emits 8,704 functions, `register_functions.cpp` is regenerated, no `Unhandled opcode` markers remain, and `output/build/Debug/ps2_game.exe` relinks against the new export.
- The first executable boot proves the rebuild path is valid even before gameplay works; the next blockers are runtime-layer issues, not recompiler codegen failures.
- For NFS HP2 Alpha, one generated recursive function (`RecurseDirectory`) leaks internal resume PCs to the dispatcher. The safe fix is an ELF-specific game override in `game_overrides.cpp`, not editing generated output.
- `TempDirectoryEntry` in this build needs game-specific construction logic: guest string duplication plus metadata writes. Leaving the export stub in place corrupts later file-system state.
- The default CD auto-detection can select the wrong parent-directory image. For this title, `paths.cdImage` must be pinned to the ALPHA prototype ISO instead of relying on auto-detect.
- The async filesystem queue uses a real `AsyncEntry` object with a stored worker proc at offset `0x34` and a file reference at `0x08`; leaving that constructor as a TODO stub is unsafe even if boot appears to progress.
- The unresolved CD read survives the `AsyncEntry` constructor fix, which points back to directory/path data-flow rather than the queue object alone.

## Session Log
### 2026-03-16
- **Actions**: Verified `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`, `SLUS_203.62.toml`, and `SLUS_203.62_ps2xAnalyzer.toml`; patched the exported TOML output path to repo `output/`; updated the runner default ELF path; re-ran `ps2_recomp`; scanned for unhandled opcodes; rebuilt `output/build`; smoke-tested the fresh executable against the ALPHA ELF.
- **Discovered**: The exported TOML is richer than the analyzer fallback and is the right baseline. Recompilation succeeded with 8,704 functions and no unhandled opcodes. The rebuilt executable initializes raylib, loads the ELF, creates semaphores, and reaches the guest entry point before stalling.
- **Current Blocker**: Runtime bootstrap is blocked by unknown syscall `0x63`, heap-related stubs (`__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock`), and an invalid `vfprintf` file handle during shutdown.
- **Actions**: Added runtime handling for syscall `0x63`, triaged malloc lock/mallinfo stubs, added `vfprintf` guest-handle fallback, raised guest heap hard limit to full RAM, then added ALPHA-specific game overrides in `ps2xRuntime/src/lib/game_overrides.cpp` for leaked `RecurseDirectory` continuation PCs and `TempDirectoryEntry` construction. Rebuilt both `out/build` and `output/build` under `vcvars64` after each patch and reran `ps2_game.exe`.
- **Discovered**: The previous crash at `0x1FA7A0` is resolved. The `TempDirectoryEntry` constructor stub warning is gone. Boot now progresses into CD/file streaming, where `sceCdRead` still receives an unresolved request (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`) and the game loops in fatal CD-error handling.
- **Current Blocker**: Trace the file-object sector-base path that feeds `ReadCDSectorASync__FPvii_0x1fa4a0` / `sceCdRead_0x2264C0`. Current evidence says the remaining failure is in CD/file-state decoding, not recompiler codegen or the previous recursive dispatch bug.
- **Actions**: Implemented the missing `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` runtime constructor in `ps2xRuntime/src/lib/ps2_stubs.cpp`, pinned `paths.cdImage` in `output/main.cpp` to `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`, rebuilt `out/build` and `output/build` under `vcvars64`, and reran the ALPHA executable with live process capture.
- **Discovered**: The game now uses the ALPHA prototype ISO instead of auto-detecting the retail disc image, and the async queue object is no longer a TODO stub. Despite that, the same unresolved CD request persists (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`). Live tracing also shows watched path-buffer memory around `0x1efffa0` being allocator-poisoned (`0xEE` then `0xAA`) immediately before `Loading entire directory...`, which strengthens the case that the remaining bug is in directory/path construction rather than the raw `sceCdRead` implementation.
- **Current Blocker**: Trace the directory-load path from `MakeSureDirectoryIsLoaded` / `LoadDirectory` / `RecurseDirectory` into `OpenCDFile` and confirm whether the directory-entry metadata or caller path hash is malformed before `sceCdRead` is reached.

## Reduced Failure Artifact
`D:/Repos/Recomp/PS2Recomp/output/build/Debug/ps2_game.exe "F:/Games/ISO/PS2/ALPHA/SLUS_203.62"`

## Unique Crashes & Subsystem Map
| Crash Address/PC | Subsystem | Callstack/Context | Proposed Fix Type | Regression Status | Resolution |
| ---------------- | --------- | ----------------- | ----------------- | ----------------- | ---------- |
| 0x125AE0 / 0x125C40 | bootstrap / kernel syscalls | Legacy monitor syscall `0x63` (`$a0=0x15`) triggered during platform probe | Runtime C++ | fixed in current build | handled as unavailable / returns 0 |
| 0x231348 | stdio | `vfprintf` received guest pointer-like file handle during shutdown/logging | Runtime C++ | fixed in current build | guest-handle fallback to `stdout` |
| 0x1FA7A0 | generated control flow | `RecurseDirectory` recursive tail path returned to internal label `0x1FA7A0`, which dispatcher treated as missing function | Game Override | fixed in current build | ALPHA-only continuation handlers bound for leaked internal PCs |
| 0x2264C0 / 0x1FA4D0 | CD streaming | `sceCdRead` receives unresolved sector request and game loops in fatal CD retry | Runtime C++ / game data-flow | active | current primary blocker |

## Resolved Stubs
| Address | Triage Type | Caller Context | Priority | Notes |
| ------- | ----------- | -------------- | -------- | ----- |
| 0x223C08 | named stub | `InitAlarm` via syscall override path | high | still logs as unimplemented during boot |
| 0x125BF0 | named stub | `__main` | medium | boot continues past warning, but startup still logs it |
| 0x1F8530 | named stub | `_str_out_str__FPCciPi` | medium | repeated formatting helper warning during boot text |
- [x] `output/build/Debug/ps2_game.exe` was rebuilt at `2026-03-16 17:52:59`.
- [x] Runtime bootstrap no longer dies at internal PC `0x1FA7A0`; ALPHA game override continuation handlers are active.
- [x] `__18TempDirectoryEntryiiPCcT3` is no longer an unimplemented stub; ALPHA override now allocates/copies the guest path string and stores entry metadata.
- [x] `output/main.cpp` now pins the ALPHA prototype disc image: `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`.
- [x] `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` now has a runtime constructor implementation in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- [ ] Boot still enters repeated CD-error recovery because `sceCdRead` at `0x2264C0` receives an unresolved request (`ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`).
- [ ] Need to trace the directory/file-state path feeding `ReadCDSectorASync__FPvii_0x1fa4a0`; `bFile` ctor fix, `AsyncEntry` ctor fix, and explicit ALPHA ISO binding did not correct the bogus LBN path.
- [ ] Build wrappers left an idle `MSBuild` process after link; artifact output is valid, but exit-code confirmation from the wrapper remains noisy.
- **Current Blocker**: Runtime bootstrap is blocked by unknown syscall `0x63`, heap-related stubs (`__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock`), and an invalid `vfprintf` file handle during shutdown.
- **Actions**: Added runtime handling for syscall `0x63`, triaged malloc lock/mallinfo stubs, added `vfprintf` guest-handle fallback, raised guest heap hard limit to full RAM, then added ALPHA-specific game overrides in `ps2xRuntime/src/lib/game_overrides.cpp` for leaked `RecurseDirectory` continuation PCs and `TempDirectoryEntry` construction. Rebuilt both `out/build` and `output/build` under `vcvars64` after each patch and reran `ps2_game.exe`.
- **Discovered**: The previous crash at `0x1FA7A0` is resolved. The `TempDirectoryEntry` constructor stub warning is gone. Boot now progresses into CD/file streaming, where `sceCdRead` still receives an unresolved request (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`) and the game loops in fatal CD-error handling.
- **Current Blocker**: Trace the file-object sector-base path that feeds `ReadCDSectorASync__FPvii_0x1fa4a0` / `sceCdRead_0x2264C0`. Current evidence says the remaining failure is in CD/file-state decoding, not recompiler codegen or the previous recursive dispatch bug.
- **Actions**: Implemented the missing `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` runtime constructor in `ps2xRuntime/src/lib/ps2_stubs.cpp`, pinned `paths.cdImage` in `output/main.cpp` to `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`, rebuilt `out/build` and `output/build` under `vcvars64`, and reran the ALPHA executable with live process capture.

## Unhandled Opcodes
| Address | Opcode | Type | Resolution |
| ------- | ------ | ---- | ---------- |
| none | none | scan result | `rg "Unhandled opcode" D:/Repos/Recomp/PS2Recomp/output` returned no matches after ALPHA recompilation |

## Known Issues
- [x] ALPHA export TOML was patched to emit generated C++ into `D:/Repos/Recomp/PS2Recomp/output` instead of `F:/Games/ISO/PS2/ALPHA/output`.
- [x] `output/main.cpp` now defaults to `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`.
- [x] `ps2_recomp` successfully regenerated 8,704 functions into repo `output/`.
- [x] `output/build/Debug/ps2_game.exe` was rebuilt at `2026-03-16 17:52:59`.
- [x] Runtime bootstrap no longer dies at internal PC `0x1FA7A0`; ALPHA game override continuation handlers are active.
- [x] `__18TempDirectoryEntryiiPCcT3` is no longer an unimplemented stub; ALPHA override now allocates/copies the guest path string and stores entry metadata.
- [x] `output/main.cpp` now pins the ALPHA prototype disc image: `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`.
- [x] `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` now has a runtime constructor implementation in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- [ ] Boot still enters repeated CD-error recovery because `sceCdRead` at `0x2264C0` receives an unresolved request (`ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`).
- [ ] Need to trace the directory/file-state path feeding `ReadCDSectorASync__FPvii_0x1fa4a0`; `bFile` ctor fix, `AsyncEntry` ctor fix, and explicit ALPHA ISO binding did not correct the bogus LBN path.
- [ ] Build wrappers left an idle `MSBuild` process after link; artifact output is valid, but exit-code confirmation from the wrapper remains noisy.

## Learned Patterns (Auto-growing)
- For this repo, the safest “from scratch” path is to keep the exported game TOML as the source of truth and only redirect its `output` path into the repo build target.
- A clean ALPHA baseline means: `ps2_recomp` emits 8,704 functions, `register_functions.cpp` is regenerated, no `Unhandled opcode` markers remain, and `output/build/Debug/ps2_game.exe` relinks against the new export.
- The first executable boot proves the rebuild path is valid even before gameplay works; the next blockers are runtime-layer issues, not recompiler codegen failures.
- For NFS HP2 Alpha, one generated recursive function (`RecurseDirectory`) leaks internal resume PCs to the dispatcher. The safe fix is an ELF-specific game override in `game_overrides.cpp`, not editing generated output.
- `TempDirectoryEntry` in this build needs game-specific construction logic: guest string duplication plus metadata writes. Leaving the export stub in place corrupts later file-system state.
- The default CD auto-detection can select the wrong parent-directory image. For this title, `paths.cdImage` must be pinned to the ALPHA prototype ISO instead of relying on auto-detect.
- The async filesystem queue uses a real `AsyncEntry` object with a stored worker proc at offset `0x34` and a file reference at `0x08`; leaving that constructor as a TODO stub is unsafe even if boot appears to progress.
- The unresolved CD read survives the `AsyncEntry` constructor fix, which points back to directory/path data-flow rather than the queue object alone.

## Session Log
### 2026-03-16
- **Actions**: Verified `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`, `SLUS_203.62.toml`, and `SLUS_203.62_ps2xAnalyzer.toml`; patched the exported TOML output path to repo `output/`; updated the runner default ELF path; re-ran `ps2_recomp`; scanned for unhandled opcodes; rebuilt `output/build`; smoke-tested the fresh executable against the ALPHA ELF.
- **Discovered**: The exported TOML is richer than the analyzer fallback and is the right baseline. Recompilation succeeded with 8,704 functions and no unhandled opcodes. The rebuilt executable initializes raylib, loads the ELF, creates semaphores, and reaches the guest entry point before stalling.
- **Current Blocker**: Runtime bootstrap is blocked by unknown syscall `0x63`, heap-related stubs (`__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock`), and an invalid `vfprintf` file handle during shutdown.
- **Actions**: Added runtime handling for syscall `0x63`, triaged malloc lock/mallinfo stubs, added `vfprintf` guest-handle fallback, raised guest heap hard limit to full RAM, then added ALPHA-specific game overrides in `ps2xRuntime/src/lib/game_overrides.cpp` for leaked `RecurseDirectory` continuation PCs and `TempDirectoryEntry` construction. Rebuilt both `out/build` and `output/build` under `vcvars64` after each patch and reran `ps2_game.exe`.
- **Discovered**: The previous crash at `0x1FA7A0` is resolved. The `TempDirectoryEntry` constructor stub warning is gone. Boot now progresses into CD/file streaming, where `sceCdRead` still receives an unresolved request (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`) and the game loops in fatal CD-error handling.
- **Current Blocker**: Trace the file-object sector-base path that feeds `ReadCDSectorASync__FPvii_0x1fa4a0` / `sceCdRead_0x2264C0`. Current evidence says the remaining failure is in CD/file-state decoding, not recompiler codegen or the previous recursive dispatch bug.
- **Actions**: Implemented the missing `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` runtime constructor in `ps2xRuntime/src/lib/ps2_stubs.cpp`, pinned `paths.cdImage` in `output/main.cpp` to `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`, rebuilt `out/build` and `output/build` under `vcvars64`, and reran the ALPHA executable with live process capture.
- **Discovered**: The game now uses the ALPHA prototype ISO instead of auto-detecting the retail disc image, and the async queue object is no longer a TODO stub. Despite that, the same unresolved CD request persists (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`). Live tracing also shows watched path-buffer memory around `0x1efffa0` being allocator-poisoned (`0xEE` then `0xAA`) immediately before `Loading entire directory...`, which strengthens the case that the remaining bug is in directory/path construction rather than the raw `sceCdRead` implementation.
- **Current Blocker**: Trace the directory-load path from `MakeSureDirectoryIsLoaded` / `LoadDirectory` / `RecurseDirectory` into `OpenCDFile` and confirm whether the directory-entry metadata or caller path hash is malformed before `sceCdRead` is reached.

## Prompt Body
You are a PS2Recomp build and runtime triage assistant working under a strict skill contract.

Your role is preprocessing only. You do not make final engineering decisions.

Given the provided build log, runtime trace, diff summary, and blocker notes:

Identify only:
1. earliest likely root-cause errors
2. repeated downstream or duplicate noise
3. files most likely requiring inspection
4. the smallest candidate fix directions
5. anything that looks dangerous to hand to a weak model

Rules:
- Return JSON only.
- Do not invent APIs, file paths, or symbols.
- Do not propose editing runner/*.cpp.
- Do not propose header changes unless the evidence explicitly requires it.
- Prefer runtime/TOML/override classification language when possible.
- If uncertain, say uncertain.
- Keep messages compact and exact.

Context:
- Current phase: PHASE_RUNTIME_BUILD
- Current blocker: Trace unresolved sceCdRead request
- Build command or run command: cmake --build output/build
- Reduced log/trace:
`D:/Repos/Recomp/PS2Recomp/output/build/Debug/ps2_game.exe "F:/Games/ISO/PS2/ALPHA/SLUS_203.62"`

## Unique Crashes & Subsystem Map
| Crash Address/PC | Subsystem | Callstack/Context | Proposed Fix Type | Regression Status | Resolution |
| ---------------- | --------- | ----------------- | ----------------- | ----------------- | ---------- |
| 0x125AE0 / 0x125C40 | bootstrap / kernel syscalls | Legacy monitor syscall `0x63` (`$a0=0x15`) triggered during platform probe | Runtime C++ | fixed in current build | handled as unavailable / returns 0 |
| 0x231348 | stdio | `vfprintf` received guest pointer-like file handle during shutdown/logging | Runtime C++ | fixed in current build | guest-handle fallback to `stdout` |
| 0x1FA7A0 | generated control flow | `RecurseDirectory` recursive tail path returned to internal label `0x1FA7A0`, which dispatcher treated as missing function | Game Override | fixed in current build | ALPHA-only continuation handlers bound for leaked internal PCs |
| 0x2264C0 / 0x1FA4D0 | CD streaming | `sceCdRead` receives unresolved sector request and game loops in fatal CD retry | Runtime C++ / game data-flow | active | current primary blocker |

## Resolved Stubs
| Address | Triage Type | Caller Context | Priority | Notes |
| ------- | ----------- | -------------- | -------- | ----- |
| 0x223C08 | named stub | `InitAlarm` via syscall override path | high | still logs as unimplemented during boot |
| 0x125BF0 | named stub | `__main` | medium | boot continues past warning, but startup still logs it |
| 0x1F8530 | named stub | `_str_out_str__FPCciPi` | medium | repeated formatting helper warning during boot text |
- [x] `output/build/Debug/ps2_game.exe` was rebuilt at `2026-03-16 17:52:59`.
- [x] Runtime bootstrap no longer dies at internal PC `0x1FA7A0`; ALPHA game override continuation handlers are active.
- [x] `__18TempDirectoryEntryiiPCcT3` is no longer an unimplemented stub; ALPHA override now allocates/copies the guest path string and stores entry metadata.
- [x] `output/main.cpp` now pins the ALPHA prototype disc image: `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`.
- [x] `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` now has a runtime constructor implementation in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- [ ] Boot still enters repeated CD-error recovery because `sceCdRead` at `0x2264C0` receives an unresolved request (`ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`).
- [ ] Need to trace the directory/file-state path feeding `ReadCDSectorASync__FPvii_0x1fa4a0`; `bFile` ctor fix, `AsyncEntry` ctor fix, and explicit ALPHA ISO binding did not correct the bogus LBN path.
- [ ] Build wrappers left an idle `MSBuild` process after link; artifact output is valid, but exit-code confirmation from the wrapper remains noisy.
- **Current Blocker**: Runtime bootstrap is blocked by unknown syscall `0x63`, heap-related stubs (`__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock`), and an invalid `vfprintf` file handle during shutdown.
- **Actions**: Added runtime handling for syscall `0x63`, triaged malloc lock/mallinfo stubs, added `vfprintf` guest-handle fallback, raised guest heap hard limit to full RAM, then added ALPHA-specific game overrides in `ps2xRuntime/src/lib/game_overrides.cpp` for leaked `RecurseDirectory` continuation PCs and `TempDirectoryEntry` construction. Rebuilt both `out/build` and `output/build` under `vcvars64` after each patch and reran `ps2_game.exe`.
- **Discovered**: The previous crash at `0x1FA7A0` is resolved. The `TempDirectoryEntry` constructor stub warning is gone. Boot now progresses into CD/file streaming, where `sceCdRead` still receives an unresolved request (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`) and the game loops in fatal CD-error handling.
- **Current Blocker**: Trace the file-object sector-base path that feeds `ReadCDSectorASync__FPvii_0x1fa4a0` / `sceCdRead_0x2264C0`. Current evidence says the remaining failure is in CD/file-state decoding, not recompiler codegen or the previous recursive dispatch bug.
- **Actions**: Implemented the missing `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` runtime constructor in `ps2xRuntime/src/lib/ps2_stubs.cpp`, pinned `paths.cdImage` in `output/main.cpp` to `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`, rebuilt `out/build` and `output/build` under `vcvars64`, and reran the ALPHA executable with live process capture.

## Unhandled Opcodes
| Address | Opcode | Type | Resolution |
| ------- | ------ | ---- | ---------- |
| none | none | scan result | `rg "Unhandled opcode" D:/Repos/Recomp/PS2Recomp/output` returned no matches after ALPHA recompilation |

## Known Issues
- [x] ALPHA export TOML was patched to emit generated C++ into `D:/Repos/Recomp/PS2Recomp/output` instead of `F:/Games/ISO/PS2/ALPHA/output`.
- [x] `output/main.cpp` now defaults to `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`.
- [x] `ps2_recomp` successfully regenerated 8,704 functions into repo `output/`.
- [x] `output/build/Debug/ps2_game.exe` was rebuilt at `2026-03-16 17:52:59`.
- [x] Runtime bootstrap no longer dies at internal PC `0x1FA7A0`; ALPHA game override continuation handlers are active.
- [x] `__18TempDirectoryEntryiiPCcT3` is no longer an unimplemented stub; ALPHA override now allocates/copies the guest path string and stores entry metadata.
- [x] `output/main.cpp` now pins the ALPHA prototype disc image: `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`.
- [x] `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` now has a runtime constructor implementation in `ps2xRuntime/src/lib/ps2_stubs.cpp`.
- [ ] Boot still enters repeated CD-error recovery because `sceCdRead` at `0x2264C0` receives an unresolved request (`ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`).
- [ ] Need to trace the directory/file-state path feeding `ReadCDSectorASync__FPvii_0x1fa4a0`; `bFile` ctor fix, `AsyncEntry` ctor fix, and explicit ALPHA ISO binding did not correct the bogus LBN path.
- [ ] Build wrappers left an idle `MSBuild` process after link; artifact output is valid, but exit-code confirmation from the wrapper remains noisy.

## Learned Patterns (Auto-growing)
- For this repo, the safest “from scratch” path is to keep the exported game TOML as the source of truth and only redirect its `output` path into the repo build target.
- A clean ALPHA baseline means: `ps2_recomp` emits 8,704 functions, `register_functions.cpp` is regenerated, no `Unhandled opcode` markers remain, and `output/build/Debug/ps2_game.exe` relinks against the new export.
- The first executable boot proves the rebuild path is valid even before gameplay works; the next blockers are runtime-layer issues, not recompiler codegen failures.
- For NFS HP2 Alpha, one generated recursive function (`RecurseDirectory`) leaks internal resume PCs to the dispatcher. The safe fix is an ELF-specific game override in `game_overrides.cpp`, not editing generated output.
- `TempDirectoryEntry` in this build needs game-specific construction logic: guest string duplication plus metadata writes. Leaving the export stub in place corrupts later file-system state.
- The default CD auto-detection can select the wrong parent-directory image. For this title, `paths.cdImage` must be pinned to the ALPHA prototype ISO instead of relying on auto-detect.
- The async filesystem queue uses a real `AsyncEntry` object with a stored worker proc at offset `0x34` and a file reference at `0x08`; leaving that constructor as a TODO stub is unsafe even if boot appears to progress.
- The unresolved CD read survives the `AsyncEntry` constructor fix, which points back to directory/path data-flow rather than the queue object alone.

## Session Log
### 2026-03-16
- **Actions**: Verified `F:/Games/ISO/PS2/ALPHA/SLUS_203.62`, `SLUS_203.62.toml`, and `SLUS_203.62_ps2xAnalyzer.toml`; patched the exported TOML output path to repo `output/`; updated the runner default ELF path; re-ran `ps2_recomp`; scanned for unhandled opcodes; rebuilt `output/build`; smoke-tested the fresh executable against the ALPHA ELF.
- **Discovered**: The exported TOML is richer than the analyzer fallback and is the right baseline. Recompilation succeeded with 8,704 functions and no unhandled opcodes. The rebuilt executable initializes raylib, loads the ELF, creates semaphores, and reaches the guest entry point before stalling.
- **Current Blocker**: Runtime bootstrap is blocked by unknown syscall `0x63`, heap-related stubs (`__malloc_lock`, `__malloc_update_mallinfo`, `__malloc_unlock`), and an invalid `vfprintf` file handle during shutdown.
- **Actions**: Added runtime handling for syscall `0x63`, triaged malloc lock/mallinfo stubs, added `vfprintf` guest-handle fallback, raised guest heap hard limit to full RAM, then added ALPHA-specific game overrides in `ps2xRuntime/src/lib/game_overrides.cpp` for leaked `RecurseDirectory` continuation PCs and `TempDirectoryEntry` construction. Rebuilt both `out/build` and `output/build` under `vcvars64` after each patch and reran `ps2_game.exe`.
- **Discovered**: The previous crash at `0x1FA7A0` is resolved. The `TempDirectoryEntry` constructor stub warning is gone. Boot now progresses into CD/file streaming, where `sceCdRead` still receives an unresolved request (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`) and the game loops in fatal CD-error handling.
- **Current Blocker**: Trace the file-object sector-base path that feeds `ReadCDSectorASync__FPvii_0x1fa4a0` / `sceCdRead_0x2264C0`. Current evidence says the remaining failure is in CD/file-state decoding, not recompiler codegen or the previous recursive dispatch bug.
- **Actions**: Implemented the missing `__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i` runtime constructor in `ps2xRuntime/src/lib/ps2_stubs.cpp`, pinned `paths.cdImage` in `output/main.cpp` to `F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso`, rebuilt `out/build` and `output/build` under `vcvars64`, and reran the ALPHA executable with live process capture.
- **Discovered**: The game now uses the ALPHA prototype ISO instead of auto-detecting the retail disc image, and the async queue object is no longer a TODO stub. Despite that, the same unresolved CD request persists (`pc=0x2264C0`, `ra=0x1FA4D0`, `a0=0x1fe8caa`, `a1=0x1`, `a2=0x1fe8520`). Live tracing also shows watched path-buffer memory around `0x1efffa0` being allocator-poisoned (`0xEE` then `0xAA`) immediately before `Loading entire directory...`, which strengthens the case that the remaining bug is in directory/path construction rather than the raw `sceCdRead` implementation.
- **Current Blocker**: Trace the directory-load path from `MakeSureDirectoryIsLoaded` / `LoadDirectory` / `RecurseDirectory` into `OpenCDFile` and confirm whether the directory-entry metadata or caller path hash is malformed before `sceCdRead` is reached.
