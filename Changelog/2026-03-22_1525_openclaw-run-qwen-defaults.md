# OpenClaw Run-Qwen Defaults

## Summary
- Updated `tools/ai/prompt_builder.py` so `run-qwen` defaults to the local OpenClaw embedded-agent flow.
- Added inline prompt replacement support via `{{PROMPT_TEXT}}`.
- Removed the misleading interactive default that pushed users toward a prompt-file placeholder for OpenClaw.

## Reason
- The installed local Qwen runner is `openclaw`, not a generic prompt-file CLI.
- `openclaw` expects `agent --local --json --agent main --message ...`, so the old `{{PROMPT_PATH}}` flow was failing with `FileNotFoundError` or unusable parameter prompts.

## Files Changed
- `tools/ai/prompt_builder.py`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- No PS2 runtime or build behavior changed.
- The helper workflow for dual-model triage is now aligned with the actual local OpenClaw command shape.

## Validation
- Ran `openclaw --help`.
- Ran `openclaw agent --help`.
- Ran `openclaw agents list` and confirmed local agent `main`.
- Ran `openclaw agent --local --json --agent main --message 'Return JSON only: {"ok":true}' --timeout 20`.
- Ran `python -m py_compile tools/ai/prompt_builder.py`.

## Follow-up
- If long prompts approach Windows command-line limits, add a dedicated OpenClaw file/stdin adapter mode instead of relying on `--message`.
- For immediate use, the updated helper should be tested with the current `PHASE_RUNTIME_BUILD` camera-callback blocker handoff.
