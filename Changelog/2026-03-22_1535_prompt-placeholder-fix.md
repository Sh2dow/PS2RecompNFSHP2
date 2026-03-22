# Prompt Placeholder Fix

## Summary
- Fixed `tools/ai/prompt_builder.py` so `{{PROMPT_TEXT}}` and `{{PROMPT_PATH}}` are substituted independently.

## Reason
- The OpenClaw preset uses `--message {{PROMPT_TEXT}}`, but the helper was first replacing the user-selected placeholder with the prompt file path.
- That caused OpenClaw to receive the prompt filename instead of the actual prompt content.

## Files Changed
- `tools/ai/prompt_builder.py`
- `PS2_PROJECT_STATE.md`

## Build / Runtime Impact
- No PS2 runtime/build behavior changed.
- `run-qwen` now produces a correct OpenClaw command for the local embedded agent flow.

## Validation
- Ran `python -m py_compile tools/ai/prompt_builder.py`.
- Confirmed from the prior `run-qwen` summary that the broken command shape passed the prompt file path to `--message`.

## Follow-up
- Re-run the interactive `run-qwen` flow once so the fresh `qwen-stdout.txt` / `qwen-triage.json` are based on the real prompt body.
