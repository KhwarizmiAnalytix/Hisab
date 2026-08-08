"""
EDITORCONFIG: verify files against the repo-root .editorconfig.

Wraps the `ec` binary from the `editorconfig-checker` PyPI package
(https://pypi.org/project/editorconfig-checker/). Policy for which checks
run lives in `.editorconfig-checker.json` (Disable / Exclude); this adapter
only supplies output format and the path list from lintrunner.
"""

from __future__ import annotations

import argparse
import json
import logging
import subprocess
import sys
import time
from enum import Enum
from pathlib import Path
from typing import Any, NamedTuple


LINTER_CODE = "EDITORCONFIG"
DEFAULT_CONFIG = ".editorconfig-checker.json"


class LintSeverity(str, Enum):
    ERROR = "error"
    WARNING = "warning"
    ADVICE = "advice"
    DISABLED = "disabled"


class LintMessage(NamedTuple):
    path: str | None
    line: int | None
    char: int | None
    code: str
    severity: LintSeverity
    name: str
    original: str | None
    replacement: str | None
    description: str | None


def run_command(args: list[str]) -> subprocess.CompletedProcess[bytes]:
    logging.debug("$ %s", " ".join(args))
    start_time = time.monotonic()
    try:
        return subprocess.run(args, capture_output=True)
    finally:
        logging.debug("took %dms", (time.monotonic() - start_time) * 1000)


def check_files(files: list[str], *, config: str) -> list[LintMessage]:
    if not files:
        return []

    config_path = Path(config)
    if not config_path.is_file():
        return [
            LintMessage(
                path=None,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name="command-failed",
                original=None,
                replacement=None,
                description=(
                    f"editorconfig-checker config not found: {config_path}\n"
                    f"Expected repo-root {DEFAULT_CONFIG}."
                ),
            )
        ]

    cmd = [
        "ec",
        f"-config={config_path}",
        "-format=codeclimate",
        *files,
    ]
    try:
        proc = run_command(cmd)
    except OSError as err:
        return [
            LintMessage(
                path=None,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name="command-failed",
                original=None,
                replacement=None,
                description=(
                    f"Failed to run editorconfig-checker (`ec`): "
                    f"{err.__class__.__name__}: {err}\n"
                    "Run `lintrunner init` to install it."
                ),
            )
        ]

    stdout = proc.stdout.decode("utf-8", errors="replace").strip()
    stderr = proc.stderr.decode("utf-8", errors="replace").strip()

    # ec exits 1 when it finds issues; anything else is a tool failure.
    if proc.returncode not in (0, 1):
        return [
            LintMessage(
                path=None,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name="command-failed",
                original=None,
                replacement=None,
                description=stderr or stdout or f"ec exited {proc.returncode}",
            )
        ]

    if not stdout and not stderr:
        return []

    # Warnings (e.g. deprecated config keys) may be mixed into stdout ahead of
    # the JSON payload — isolate the codeclimate array.
    payload = stdout
    start = payload.find("[")
    end = payload.rfind("]")
    if start == -1 or end == -1 or end < start:
        # No findings, or only a warning on stdout.
        if proc.returncode == 0:
            return []
        return [
            LintMessage(
                path=None,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name="parse-failed",
                original=None,
                replacement=None,
                description=f"Could not find codeclimate JSON in ec output:\n{stdout}\n{stderr}",
            )
        ]
    payload = payload[start : end + 1]

    try:
        results: list[dict[str, Any]] = json.loads(payload)
    except json.JSONDecodeError as err:
        return [
            LintMessage(
                path=None,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name="parse-failed",
                original=None,
                replacement=None,
                description=f"Could not parse ec codeclimate JSON: {err}\n{payload}",
            )
        ]

    messages: list[LintMessage] = []
    for result in results:
        location = result.get("location") or {}
        path = location.get("path")
        lines = location.get("lines") or {}
        begin = lines.get("begin")
        messages.append(
            LintMessage(
                path=path,
                line=int(begin) if begin else None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ERROR,
                name=result.get("check_name") or "editorconfig",
                original=None,
                replacement=None,
                description=result.get("description") or "editorconfig violation",
            )
        )
    return messages


def main() -> None:
    parser = argparse.ArgumentParser(
        description="editorconfig-checker runner",
        fromfile_prefix_chars="@",
    )
    parser.add_argument(
        "--config",
        default=DEFAULT_CONFIG,
        help=f"path to editorconfig-checker JSON config (default: {DEFAULT_CONFIG})",
    )
    parser.add_argument(
        "filenames",
        nargs="+",
        help="paths to lint",
    )
    args = parser.parse_args()

    logging.basicConfig(
        format="<%(threadName)s:%(levelname)s> %(message)s",
        level=logging.DEBUG if len(args.filenames) < 1000 else logging.INFO,
        stream=sys.stderr,
    )

    for message in check_files(args.filenames, config=args.config):
        print(json.dumps(message._asdict()), flush=True)


if __name__ == "__main__":
    main()
