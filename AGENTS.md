# AGENTS

## Purpose

This repository uses a dual-model helper workflow for large PS2Recomp tasks.

Agents working here should:

- use the repo tooling instead of inventing ad hoc prompt packs
- keep AI artifacts separate from recomp/build output
- record major source/build changes in markdown under `Changelog\`
- keep durable project state in `PS2_PROJECT_STATE.md`

## Tool Usage

### Primary helper

Use the unified helper script:

```powershell
python tools/ai/prompt_builder.py
```

This launches the interactive menu and asks what to do.

### Built-in helper modes

```powershell
python tools/ai/prompt_builder.py explain
python tools/ai/prompt_builder.py qwen
python tools/ai/prompt_builder.py run-qwen
python tools/ai/prompt_builder.py codex
```

### What each mode is for

- `explain`
  - prints the embedded workflow notes, default paths, and optional prompt templates
- `qwen`
  - builds reduced-log and prompt artifacts for a local Qwen pre-pass
- `run-qwen`
  - builds the Qwen artifacts and runs the local Qwen command
- `codex`
  - builds the Codex handoff from a verified `qwen-triage.json`

### Important paths

- recomp/build output: `output\`
- AI helper artifacts: `tools\ai\artifacts\latest\`
- persistent project state: `PS2_PROJECT_STATE.md`
- major change log entries: `Changelog\`

Do not mix AI helper artifacts into `output\`.

## Change Tracking Policy

Agents must create a markdown changelog entry for major source/build changes in:

```text
Changelog\
```

### When to write a changelog entry

Create an entry when any of the following happens:

- runtime source changes under `ps2xRuntime\`
- recompiler configuration changes that affect output/build behavior
- build system or toolchain configuration changes
- generated output handling changes that affect the workflow
- game override behavior changes
- major debugging milestones that change current direction or unblock progress

### Suggested filename format

```text
YYYY-MM-DD_HHMM_short-title.md
```

Example:

```text
2026-03-20_1810_runtime-cd-read-triage.md
```

### Required changelog sections

Each changelog entry should include:

- `Summary`
- `Reason`
- `Files Changed`
- `Build / Runtime Impact`
- `Validation`
- `Follow-up`

## Recommended Changelog Content

Use this structure:

```md
# Short Title

## Summary
- What changed.

## Reason
- Why the change was made.

## Files Changed
- `path\to\file`

## Build / Runtime Impact
- What this affects in recompilation, runtime behavior, or build output.

## Validation
- Commands run, artifacts produced, and observed result.

## Follow-up
- Remaining blocker, risk, or next step.
```

## Additional Recommendations

- Prefer incremental builds only. Do not clean the build unless the user explicitly asks.
- Treat `output\` as generated/build territory, not as a scratchpad for helper notes.
- Keep `PS2_PROJECT_STATE.md` updated after major diagnosis, patch, rebuild, or runtime findings.
- Do not edit generated runner files directly.
- Avoid header changes unless explicitly necessary and approved.
- When changing build or runtime behavior, record the exact command used and the result.
- When a helper or automation changes workflow behavior, document it in `Changelog\` before relying on it.
