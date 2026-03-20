#!/usr/bin/env python3
"""Single-file prompt helper for the PS2Recomp dual-model workflow.

This script is intentionally self-contained:
- it embeds the default Qwen and Codex prompt templates
- it embeds the default path layout
- it can print an explanation of the helper flow
- it can build prompt bundles and run a local Qwen command

Default paths:
- state file: `PS2_PROJECT_STATE.md`
- artifact root: `tools/ai/artifacts/latest`
- embedded prompt roles:
  - `qwen`: preprocessing / triage only
  - `codex`: diagnosis / patch / verify
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STATE = ROOT / "PS2_PROJECT_STATE.md"
DEFAULT_OUTPUT = ROOT / "tools" / "ai" / "artifacts" / "latest"
DEFAULT_QWEN_PROMPT = """You are a PS2Recomp build and runtime triage assistant working under a strict skill contract.

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
"""

DEFAULT_CODEX_PROMPT = """You are the primary PS2Recomp repair agent operating under ps2-recomp-Agent-SKILL.

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
"""

HELPER_EXPLANATION = f"""PS2Recomp Dual-Model Helper

Default paths:
- repo root: {ROOT}
- state file: {DEFAULT_STATE}
- artifact output: {DEFAULT_OUTPUT}

Helper roles:
- qwen:
  - use for build-log reduction, duplicate clustering, likely-file extraction
  - output must be JSON only
  - never trusted as final truth
- codex:
  - use for root-cause diagnosis, safe patching, verification, and state updates
  - consumes the verified qwen JSON plus reduced artifacts

Typical flow:
1. Build Qwen prompt artifacts:
   python tools/ai/prompt_builder.py qwen --phase ... --blocker ... --command-text ... --log-path ...
2. Run local Qwen directly:
   python tools/ai/prompt_builder.py run-qwen --phase ... --blocker ... --command-text ... --log-path ... --executable ...
3. Build Codex handoff from qwen-triage.json:
   python tools/ai/prompt_builder.py codex --phase ... --blocker ... --command-text ... --log-path ... --qwen-json-path ...

Artifacts written by default:
- reduced-log.txt
- state-excerpt.md
- qwen-prepass-filled.txt
- qwen-prepass-annotated.md
- qwen-stdout.txt
- qwen-stderr.txt
- qwen-triage.json
- codex-handoff.md
- codex-handoff-annotated.md
"""

INTERACTIVE_MENU = """Choose an action:
1. explain
2. qwen
3. run-qwen
4. codex
"""


def normalize_paths(values: Iterable[str]) -> list[str]:
    result: list[str] = []
    for value in values:
        for part in value.split(","):
            part = part.strip()
            if part:
                result.append(part)
    return result


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def try_parse_json(text: str) -> tuple[bool, str | None]:
    stripped = text.strip()
    if not stripped:
        return False, None

    candidates = [stripped]

    fenced = re.search(r"```json\s*(.*?)\s*```", text, flags=re.IGNORECASE | re.DOTALL)
    if fenced:
        candidates.append(fenced.group(1).strip())

    first_brace = text.find("{")
    last_brace = text.rfind("}")
    if first_brace != -1 and last_brace > first_brace:
        candidates.append(text[first_brace : last_brace + 1].strip())

    for candidate in candidates:
        try:
            json.loads(candidate)
            return True, candidate
        except json.JSONDecodeError:
            continue

    return False, None


def get_state_excerpt(path: Path, max_lines: int = 140) -> str:
    lines = read_text(path).splitlines()
    return "\n".join(lines[:max_lines]) if len(lines) > max_lines else "\n".join(lines)


def get_reduced_log(path: Path, max_lines: int, context_lines: int) -> str:
    lines = read_text(path).splitlines()
    if not lines:
        return ""

    pattern = re.compile(
        r"(error|fatal|exception|crash|assert|undefined|unimplemented|failed|warning:|"
        r"linker|LNK\d+|C\d{4}|ninja: build stopped)",
        re.IGNORECASE,
    )

    selected: set[int] = set()
    for index, line in enumerate(lines):
        if pattern.search(line):
            start = max(0, index - context_lines)
            end = min(len(lines) - 1, index + context_lines)
            selected.update(range(start, end + 1))

    if not selected:
        return "\n".join(lines[-max_lines:])

    reduced = [lines[i] for i in sorted(selected)[:max_lines]]
    if len(reduced) < min(max_lines, 40):
        tail_budget = min(40, max_lines - len(reduced))
        reduced.extend(lines[-tail_budget:])
    return "\n".join(reduced)


def fill_template(template: str, mapping: dict[str, str]) -> str:
    for key, value in mapping.items():
        template = template.replace(key, value)
    return template


def annotate_prompt(
    role: str,
    phase: str,
    blocker: str,
    command_text: str,
    relevant_files: list[str],
    state_excerpt: str,
    reduced_log: str,
    body: str,
    qwen_json: str | None = None,
) -> str:
    files_text = ", ".join(relevant_files) if relevant_files else "<none provided>"
    sections = [
        "# Annotated Prompt",
        "",
        f"[role] {role}",
        f"[phase] {phase}",
        f"[blocker] {blocker}",
        f"[command] {command_text}",
        f"[relevant_files] {files_text}",
        "",
        "## State Excerpt",
        state_excerpt.strip(),
        "",
        "## Reduced Failure Artifact",
        reduced_log.strip(),
        "",
    ]

    if qwen_json is not None:
        sections.extend(["## Qwen JSON", qwen_json.strip(), ""])

    sections.extend(["## Prompt Body", body.strip(), ""])
    return "\n".join(sections)


def build_qwen(args: argparse.Namespace) -> dict[str, str]:
    qwen_template = DEFAULT_QWEN_PROMPT
    state_excerpt = get_state_excerpt(args.state_path)
    reduced_log = get_reduced_log(args.log_path, args.max_log_lines, args.context_lines)

    filled = fill_template(
        qwen_template,
        {
            "<phase>": args.phase,
            "<blocker>": args.blocker,
            "<command>": args.command_text,
            "<paste reduced artifact here>": reduced_log,
        },
    )

    annotated = annotate_prompt(
        role="qwen-prepass",
        phase=args.phase,
        blocker=args.blocker,
        command_text=args.command_text,
        relevant_files=args.relevant_files,
        state_excerpt=state_excerpt,
        reduced_log=reduced_log,
        body=filled,
    )

    outputs = {
        "reduced_log": str(args.output_dir / "reduced-log.txt"),
        "state_excerpt": str(args.output_dir / "state-excerpt.md"),
        "qwen_prompt": str(args.output_dir / "qwen-prepass-filled.txt"),
        "qwen_prompt_annotated": str(args.output_dir / "qwen-prepass-annotated.md"),
    }
    write_text(Path(outputs["reduced_log"]), reduced_log)
    write_text(Path(outputs["state_excerpt"]), state_excerpt)
    write_text(Path(outputs["qwen_prompt"]), filled)
    write_text(Path(outputs["qwen_prompt_annotated"]), annotated)
    return outputs


def build_codex(args: argparse.Namespace) -> dict[str, str]:
    codex_template = DEFAULT_CODEX_PROMPT
    state_excerpt = get_state_excerpt(args.state_path)
    reduced_log = get_reduced_log(args.log_path, args.max_log_lines, args.context_lines)
    qwen_json = read_text(args.qwen_json_path)
    files_text = ", ".join(args.relevant_files) if args.relevant_files else "<none provided>"

    filled = fill_template(
        codex_template,
        {
            "<phase>": args.phase,
            "<blocker>": args.blocker,
            "<command>": args.command_text,
            "<files>": files_text,
            "<state excerpt>": state_excerpt,
            "<json>": qwen_json,
            "<reduced failure output>": reduced_log,
        },
    )

    annotated = annotate_prompt(
        role="codex-main-agent",
        phase=args.phase,
        blocker=args.blocker,
        command_text=args.command_text,
        relevant_files=args.relevant_files,
        state_excerpt=state_excerpt,
        reduced_log=reduced_log,
        body=filled,
        qwen_json=qwen_json,
    )

    outputs = {
        "reduced_log": str(args.output_dir / "reduced-log.txt"),
        "state_excerpt": str(args.output_dir / "state-excerpt.md"),
        "codex_prompt": str(args.output_dir / "codex-handoff.md"),
        "codex_prompt_annotated": str(args.output_dir / "codex-handoff-annotated.md"),
    }
    write_text(Path(outputs["reduced_log"]), reduced_log)
    write_text(Path(outputs["state_excerpt"]), state_excerpt)
    write_text(Path(outputs["codex_prompt"]), filled)
    write_text(Path(outputs["codex_prompt_annotated"]), annotated)
    return outputs


def run_qwen(args: argparse.Namespace) -> dict[str, str | int | list[str]]:
    qwen_outputs = build_qwen(args)
    prompt_path = Path(qwen_outputs["qwen_prompt"])
    prompt_text = read_text(prompt_path)

    cmd = [args.executable]
    for arg in args.exec_args:
        cmd.append(arg.replace(args.prompt_argument_placeholder, str(prompt_path)))

    completed = subprocess.run(
        cmd,
        input=prompt_text if args.prompt_mode == "stdin" else None,
        capture_output=True,
        text=True,
        encoding="utf-8",
        timeout=args.timeout_seconds,
        check=False,
    )

    stdout_path = args.output_dir / "qwen-stdout.txt"
    stderr_path = args.output_dir / "qwen-stderr.txt"
    write_text(stdout_path, completed.stdout)
    write_text(stderr_path, completed.stderr)

    outputs: dict[str, str | int | list[str]] = {
        **qwen_outputs,
        "stdout": str(stdout_path),
        "stderr": str(stderr_path),
        "exit_code": completed.returncode,
        "executed_command": cmd,
    }

    success, json_text = try_parse_json(completed.stdout)
    if success and json_text is not None:
        json_path = args.output_dir / "qwen-triage.json"
        write_text(json_path, json_text)
        outputs["qwen_json"] = str(json_path)
    elif not args.allow_non_json:
        raise SystemExit(
            "Qwen output did not contain valid JSON. "
            f"See {stdout_path} and {stderr_path}."
        )

    return outputs


def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--phase", required=True)
    parser.add_argument("--blocker", required=True)
    parser.add_argument("--command-text", required=True)
    parser.add_argument("--log-path", type=Path, required=True)
    parser.add_argument("--state-path", type=Path, default=DEFAULT_STATE)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--relevant-files", nargs="*", default=[])
    parser.add_argument("--max-log-lines", type=int, default=220)
    parser.add_argument("--context-lines", type=int, default=2)


def add_run_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--executable", required=True)
    parser.add_argument("--exec-arg", action="append", default=[])
    parser.add_argument("--prompt-mode", choices=("stdin", "file"), default="stdin")
    parser.add_argument("--prompt-argument-placeholder", default="{{PROMPT_PATH}}")
    parser.add_argument("--timeout-seconds", type=int, default=180)
    parser.add_argument("--allow-non-json", action="store_true")


def prompt_text(label: str, default: str | None = None, required: bool = True) -> str:
    suffix = f" [{default}]" if default else ""
    while True:
        value = input(f"{label}{suffix}: ").strip()
        if value:
            return value
        if default is not None:
            return default
        if not required:
            return ""
        print("Value required.")


def prompt_list(label: str) -> list[str]:
    raw = input(f"{label} (comma-separated, blank for none): ").strip()
    if not raw:
        return []
    return normalize_paths([raw])


def prompt_yes_no(label: str, default: bool = False) -> bool:
    suffix = " [Y/n]" if default else " [y/N]"
    value = input(f"{label}{suffix}: ").strip().lower()
    if not value:
        return default
    return value in {"y", "yes"}


def build_namespace(command: str, values: dict[str, object]) -> argparse.Namespace:
    data = {"command": command, **values}
    return finalize_args(argparse.Namespace(**data))


def interactive_args() -> argparse.Namespace:
    print(HELPER_EXPLANATION)
    print(INTERACTIVE_MENU)
    choice = prompt_text("Action number", "1")
    mapping = {
        "1": "explain",
        "2": "qwen",
        "3": "run-qwen",
        "4": "codex",
    }
    command = mapping.get(choice, choice)
    if command not in {"explain", "qwen", "run-qwen", "codex"}:
        raise SystemExit(f"Unknown action: {choice}")

    if command == "explain":
        return argparse.Namespace(
            command="explain",
            show_prompts=prompt_yes_no("Show embedded prompts?", default=False),
        )

    common: dict[str, object] = {
        "phase": prompt_text("Phase"),
        "blocker": prompt_text("Blocker"),
        "command_text": prompt_text("Build/run command"),
        "log_path": Path(prompt_text("Log path", str(DEFAULT_STATE))),
        "state_path": Path(prompt_text("State path", str(DEFAULT_STATE))),
        "output_dir": Path(prompt_text("Output dir", str(DEFAULT_OUTPUT))),
        "relevant_files": prompt_list("Relevant files"),
        "max_log_lines": int(prompt_text("Max reduced log lines", "220")),
        "context_lines": int(prompt_text("Context lines around matched errors", "2")),
    }

    if command == "qwen":
        return build_namespace(command, common)

    if command == "codex":
        common["qwen_json_path"] = Path(prompt_text("Qwen JSON path"))
        return build_namespace(command, common)

    common["executable"] = prompt_text("Local Qwen executable")
    common["prompt_mode"] = prompt_text("Prompt mode", "stdin")
    exec_args = prompt_list("Executable args")
    common["exec_arg"] = exec_args
    common["prompt_argument_placeholder"] = prompt_text(
        "Prompt path placeholder", "{{PROMPT_PATH}}"
    )
    common["timeout_seconds"] = int(prompt_text("Timeout seconds", "180"))
    common["allow_non_json"] = prompt_yes_no("Allow non-JSON output?", default=False)
    return build_namespace(command, common)


def finalize_args(args: argparse.Namespace) -> argparse.Namespace:
    if hasattr(args, "log_path"):
        args.log_path = args.log_path.resolve()
    if hasattr(args, "state_path"):
        args.state_path = args.state_path.resolve()
    if hasattr(args, "output_dir"):
        args.output_dir = args.output_dir if args.output_dir.is_absolute() else (ROOT / args.output_dir)
        args.output_dir = args.output_dir.resolve()
    if hasattr(args, "relevant_files"):
        args.relevant_files = normalize_paths(args.relevant_files)
    if hasattr(args, "exec_arg"):
        args.exec_args = list(args.exec_arg)
    if hasattr(args, "qwen_json_path") and args.qwen_json_path is not None:
        args.qwen_json_path = args.qwen_json_path.resolve()
    return args


def main() -> int:
    if len(sys.argv) == 1:
        args = interactive_args()
        if args.command == "explain":
            print(HELPER_EXPLANATION)
            if getattr(args, "show_prompts", False):
                print("=== Embedded Qwen Prompt ===")
                print(DEFAULT_QWEN_PROMPT)
                print("=== Embedded Codex Prompt ===")
                print(DEFAULT_CODEX_PROMPT)
            return 0

        outputs = (
            build_qwen(args)
            if args.command == "qwen"
            else build_codex(args)
            if args.command == "codex"
            else run_qwen(args)
        )

        summary = {
            "mode": args.command,
            "phase": args.phase,
            "blocker": args.blocker,
            "command_text": args.command_text,
            "log_path": str(args.log_path),
            "state_path": str(args.state_path),
            "relevant_files": args.relevant_files,
            "outputs": outputs,
        }
        summary_path = args.output_dir / f"{args.command}-prompt-summary.json"
        write_text(summary_path, json.dumps(summary, indent=2))
        print(json.dumps({**summary, "summary": str(summary_path)}, indent=2))
        return 0

    parser = argparse.ArgumentParser(
        description="Single-file helper for the PS2 dual-model workflow.",
        epilog="Run `python tools/ai/prompt_builder.py explain` for embedded helper notes and default paths.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    explain_parser = subparsers.add_parser(
        "explain", help="Print embedded helper notes, prompts, and default paths"
    )
    explain_parser.add_argument(
        "--show-prompts",
        action="store_true",
        help="Also print the embedded Qwen and Codex prompt templates.",
    )

    qwen_parser = subparsers.add_parser("qwen", help="Build Qwen pre-pass artifacts")
    add_common_args(qwen_parser)

    codex_parser = subparsers.add_parser("codex", help="Build Codex handoff artifacts")
    add_common_args(codex_parser)
    codex_parser.add_argument("--qwen-json-path", type=Path, required=True)

    run_parser = subparsers.add_parser(
        "run-qwen", help="Build Qwen artifacts and run a local Qwen command"
    )
    add_common_args(run_parser)
    add_run_args(run_parser)

    args = finalize_args(parser.parse_args())

    if args.command == "explain":
        print(HELPER_EXPLANATION)
        if args.show_prompts:
            print("=== Embedded Qwen Prompt ===")
            print(DEFAULT_QWEN_PROMPT)
            print("=== Embedded Codex Prompt ===")
            print(DEFAULT_CODEX_PROMPT)
        return 0
    if args.command == "qwen":
        outputs = build_qwen(args)
    elif args.command == "codex":
        outputs = build_codex(args)
    else:
        outputs = run_qwen(args)

    summary = {
        "mode": args.command,
        "phase": args.phase,
        "blocker": args.blocker,
        "command_text": args.command_text,
        "log_path": str(args.log_path),
        "state_path": str(args.state_path),
        "relevant_files": args.relevant_files,
        "outputs": outputs,
    }

    summary_path = args.output_dir / f"{args.command}-prompt-summary.json"
    write_text(summary_path, json.dumps(summary, indent=2))
    print(json.dumps({**summary, "summary": str(summary_path)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
