# Dual-Model Workflow Spec for `ps2-recomp-Agent-SKILL`

## Purpose

This document defines a concrete operating model for using:

- `Codex` as the primary PS2 recompilation agent
- local `Qwen3-Coder-Next-UD-IQ1_M.gguf` as a cheap sidecar worker
- `PS2_PROJECT_STATE.md` as the persistent source of truth

This spec does not replace the skill. It constrains how two unequal models should be used under the skill.

## Core Policy

Use the skill as the workflow contract.

- `Codex` owns diagnosis, patch decisions, code edits, rebuild decisions, and state conclusions.
- `Qwen` owns compression, classification, grep-heavy triage, and narrowly scoped mechanical transforms.
- `PS2_PROJECT_STATE.md` is updated only after a meaningful observation, fix attempt, verification result, or blocker change.

## Hard Division of Labor

### Codex owns

- Boot sequence compliance from the skill
- Reading and respecting `PS2_PROJECT_STATE.md`
- Mapping problems to the 4 fix tools
- Root-cause analysis across runtime, TOML, override, and recompiler layers
- All risky edits
- Any change involving behavior, architecture, hidden invariants, or multiple plausible causes
- Build execution and build-result verification
- Final decision on whether a Qwen suggestion is safe
- State-file updates after major actions

### Qwen owns

- Summarizing long compiler, linker, or runtime logs
- Grouping duplicate diagnostics
- Identifying the earliest likely root errors
- Listing likely touched files and symbols
- Producing constrained candidate fix strategies
- Performing narrow, repetitive transforms after Codex has already decided the pattern
- Converting noisy raw output into compact structured artifacts

### Qwen must not own

- Final patch selection
- Runtime semantics decisions
- Cross-file architectural refactors
- Game-override strategy without Codex review
- Header changes
- Any decision that can trigger a mass rebuild or incorrect PS2 behavior

## Operating Loop

### Phase 0: Skill bootstrap

Codex follows the skill boot sequence:

1. Locate skill resources.
2. Read `PS2_PROJECT_STATE.md`.
3. Load required PS2Recomp knowledge files.
4. Detect game, build configuration, and current phase.
5. Re-state the current blocker from the state file before taking action.

Qwen is not used during bootstrap unless Codex explicitly asks it to compress a very large pre-existing log.

### Phase 1: Local extraction

Before asking either model to reason deeply, collect only the smallest relevant artifact set:

- build command used
- current diff summary
- first failing translation units or linker symbols
- current crash PC / RA / subsystem notes
- exact file paths under consideration
- relevant state-file excerpts

Preferred artifacts:

- incremental build output
- single failing runtime trace
- targeted grep results
- one-file-at-a-time source excerpts

Avoid feeding either model giant raw context when a filtered artifact can be produced first.

### Phase 2: Qwen pre-pass

Use Qwen as a reducer, not as the main engineer.

Inputs:

- reduced build or runtime log
- current blocker summary from `PS2_PROJECT_STATE.md`
- narrowed file list
- optional diff summary

Outputs:

- root error cluster list
- downstream noise list
- likely affected files
- confidence estimate
- minimal candidate fix directions

Qwen output must be structured and compact enough for Codex to validate directly.

### Phase 3: Codex diagnosis and patching

Codex consumes:

- skill rules
- current state-file facts
- Qwen pre-pass artifact
- exact source files
- exact build or runtime failure

Codex then:

1. maps the issue to one of the skill's 4 fix tools
2. states the root-cause hypothesis
3. restricts edits to the smallest safe set
4. patches
5. rebuilds or reruns only the relevant verification step
6. decides whether the hypothesis was confirmed, weakened, or disproven

### Phase 4: State update

After every major action, Codex updates `PS2_PROJECT_STATE.md` with:

- action taken
- evidence observed
- result
- remaining blocker
- any new learned pattern worth preserving

Qwen does not directly update the state file. It may draft a candidate update block for Codex to review.

## Routing Rules

### Send to Qwen first

- 10k+ lines of noisy build output
- repeated compile errors with likely shared root cause
- repeated runtime warnings
- symbol-usage scans
- repetitive rename or wrapper generation after the pattern is fixed
- candidate clustering of unimplemented stubs

### Send directly to Codex

- ambiguity between runtime vs override vs TOML fix
- crashes with unclear ownership or lifetime implications
- linker or ABI issues
- anything touching `ps2xRuntime/src/lib/`
- anything that could tempt an edit to generated runner output
- any patch requiring final safety judgment

## Required Qwen Output Format

Qwen should return JSON only:

```json
{
  "task_type": "build-triage",
  "root_errors": [
    {
      "rank": 1,
      "file": "path/to/file.cpp",
      "line": 123,
      "message": "exact shortened diagnostic",
      "likely_causes": [
        "signature mismatch",
        "missing runtime stub"
      ],
      "confidence": 0.71
    }
  ],
  "downstream_noise": [
    {
      "pattern": "same undefined symbol repeated",
      "count": 14
    }
  ],
  "likely_files": [
    "ps2xRuntime/src/lib/example.cpp",
    "output/register_functions.cpp"
  ],
  "candidate_actions": [
    "inspect runtime binding for symbol X",
    "verify whether this belongs in TOML stub list"
  ],
  "do_not_do": [
    "do not edit runner files",
    "do not suggest header edits unless unavoidable"
  ],
  "uncertainties": [
    "missing direct callsite for symbol X"
  ]
}
```

## Qwen Pre-Pass Prompt

Use this prompt as the default pre-pass instruction:

```text
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
- Current phase: <phase>
- Current blocker: <blocker>
- Build command or run command: <command>
- Reduced log/trace:
<paste reduced artifact here>
```

## Codex Main-Agent Prompt

Use this as the default main-agent instruction for the active repair loop:

```text
You are the primary PS2Recomp repair agent operating under ps2-recomp-Agent-SKILL.

Goal:
- resolve the current blocker with the smallest safe change
- preserve behavior
- avoid broad refactors
- obey the skill's prohibitions and build gate

You own:
- root-cause diagnosis
- tool selection among TOML, Runtime C++, Game Override, or Re-run Recompiler
- code edits
- verification
- PS2_PROJECT_STATE.md updates

You must treat the Qwen artifact as triage only, not as truth.

Context:
- Current phase: <phase>
- Current blocker: <blocker>
- Build/run command: <command>
- Relevant files: <files>
- Current state excerpts:
<state excerpt>

- Qwen pre-pass JSON:
<json>

- Exact failure output:
<reduced failure output>

Rules:
- Never edit runner/*.cpp.
- Never modify headers without explicit user approval.
- Restrict edits to the smallest safe file set.
- State which of the 4 fix tools applies before patching.
- Explain the root-cause hypothesis briefly before editing.
- Rebuild or rerun only the relevant verification step.
- If the attempt fails, handle only the next exposed root blocker.
- Update PS2_PROJECT_STATE.md after the result is known.
```

## `PS2_PROJECT_STATE.md` Update Template

Use this template after each major action. Append to `Session Log`, and update any affected summary tables above it.

```md
### YYYY-MM-DD
- **Phase**: <PHASE_NAME>
- **Action Type**: <build-triage | runtime-fix | toml-fix | override-fix | recompile | verification>
- **Reason**: <why this action was taken>
- **Inputs Reviewed**: <build log, trace, files, qwen artifact, state sections>
- **Files Inspected**: `<path1>`, `<path2>`
- **Files Changed**: `<path1>`, `<path2>` or `none`
- **Tool Chosen**: <TOML Config | Runtime C++ | Game Override | Re-run Recompiler>
- **Hypothesis**: <one short root-cause statement>
- **Verification Command**: `<exact command>`
- **Observed Result**: <what happened, including key error/crash details>
- **Conclusion**: <confirmed / partially confirmed / disproven>
- **New Blocker**: <next root blocker or `none`>
- **Learned Pattern**: <portable fact worth preserving, or `none`>
```

## State-File Update Rules

- Update summary tables first when the blocker category changed.
- Append to `Session Log` second.
- Preserve old failed hypotheses; do not erase them.
- Record exact PCs, syscall IDs, stub names, and file paths when known.
- Use `confirmed`, `partially confirmed`, or `disproven` language for hypotheses.
- If a Qwen pre-pass was useful, capture only the verified conclusion, not the full Qwen reasoning.

## Minimal Artifact Contract Between Models

The handoff from Qwen to Codex should fit in one compact bundle:

1. `current_phase`
2. `current_blocker`
3. `build_or_run_command`
4. `root_errors`
5. `likely_files`
6. `candidate_actions`
7. `uncertainties`
8. `reduced_raw_output`

If the handoff grows beyond that, compress again before asking Codex to reason.

## Recommended File Layout

If automation is added later, keep artifacts in a narrow sidecar directory such as:

```text
tools/ai/
  prompt_builder.py
  artifacts/
    latest-build.log
    latest-runtime.log
    qwen-triage.json
    codex-handoff.md
```

This layout is optional. The workflow remains valid even when run manually.

## Recommended Local Commands

Use the Python helper as the primary interface.

Build an annotated Qwen prompt bundle:

```bash
python tools/ai/prompt_builder.py qwen \
  --phase PHASE_RUNTIME_BUILD \
  --blocker "Trace unresolved sceCdRead request" \
  --command-text "cmake --build output/build" \
  --log-path build.log \
  --relevant-files ps2xRuntime/src/lib/game_overrides.cpp ps2xRuntime/src/lib/ps2_stubs.cpp \
  --output-dir tools/ai/artifacts/latest
```

Run the Qwen pre-pass directly from the same tool:

```bash
python tools/ai/prompt_builder.py run-qwen \
  --phase PHASE_RUNTIME_BUILD \
  --blocker "Trace unresolved sceCdRead request" \
  --command-text "cmake --build output/build" \
  --log-path build.log \
  --relevant-files ps2xRuntime/src/lib/game_overrides.cpp ps2xRuntime/src/lib/ps2_stubs.cpp \
  --output-dir tools/ai/artifacts/latest \
  --executable openclaw \
  --prompt-mode file \
  --exec-arg --model \
  --exec-arg qwen \
  --exec-arg --prompt-file \
  --exec-arg {{PROMPT_PATH}}
```

Build the Codex handoff from the verified Qwen JSON:

```bash
python tools/ai/prompt_builder.py codex \
  --phase PHASE_RUNTIME_BUILD \
  --blocker "Trace unresolved sceCdRead request" \
  --command-text "cmake --build output/build" \
  --log-path build.log \
  --qwen-json-path tools/ai/artifacts/latest/qwen-triage.json \
  --relevant-files ps2xRuntime/src/lib/game_overrides.cpp ps2xRuntime/src/lib/ps2_stubs.cpp \
  --output-dir tools/ai/artifacts/latest
```

## Success Criteria

This workflow is working correctly when:

- Codex sees compressed, high-signal inputs instead of raw noise
- Qwen never makes the final architectural decision
- `PS2_PROJECT_STATE.md` stays current and trustworthy across sessions
- fixes are mapped cleanly to TOML, runtime, override, or recompiler actions
- rebuild loops stay incremental and tightly scoped
