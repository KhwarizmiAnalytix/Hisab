"""
cmake-format linter adapter for Quarisma lintrunner.

This adapter runs cmake-format to check and format CMake files.
It follows the same pattern as clangformat_linter.py for consistency.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import logging
import os
import subprocess
import sys
import time
from enum import Enum
from pathlib import Path
from typing import NamedTuple


IS_WINDOWS: bool = os.name == "nt"

def _is_under_third_party(path: str) -> bool:
    """True if path is inside ThirdParty/ (never format vendored CMake)."""
    norm = path.replace("\\", "/")
    return "/ThirdParty/" in norm


class LintSeverity(str, Enum):
    """Severity levels for lint messages."""

    ERROR = "error"
    WARNING = "warning"
    ADVICE = "advice"
    DISABLED = "disabled"


class LintMessage(NamedTuple):
    """Structured lint message for lintrunner."""

    path: str | None
    line: int | None
    char: int | None
    code: str
    severity: LintSeverity
    name: str
    original: str | None
    replacement: str | None
    description: str | None


def as_posix(name: str) -> str:
    """Convert Windows paths to POSIX format for display."""
    return name.replace("\\", "/") if IS_WINDOWS else name


def _run_command(
    args: list[str],
    *,
    timeout: int,
) -> subprocess.CompletedProcess[bytes]:
    """Run a command with timeout."""
    logging.debug("$ %s", " ".join(args))
    start_time = time.monotonic()
    try:
        return subprocess.run(
            args,
            capture_output=True,
            shell=IS_WINDOWS,  # So batch scripts are found
            timeout=timeout,
            check=False,  # Don't raise on non-zero exit
        )
    finally:
        end_time = time.monotonic()
        logging.debug("took %dms", (end_time - start_time) * 1000)


def run_command(
    args: list[str],
    *,
    retries: int,
    timeout: int,
) -> subprocess.CompletedProcess[bytes]:
    """Run a command with retry logic for timeouts."""
    remaining_retries = retries
    while True:
        try:
            return _run_command(args, timeout=timeout)
        except subprocess.TimeoutExpired as err:
            if remaining_retries == 0:
                raise err
            remaining_retries -= 1
            logging.warning(
                "(%s/%s) Retrying because command failed with: %r",
                retries - remaining_retries,
                retries,
                err,
            )
            time.sleep(1)


def check_file(
    filename: str,
    config: str,
    retries: int,
    timeout: int,
) -> list[LintMessage]:
    """Check a CMake file with cmake-format."""
    if _is_under_third_party(filename):
        return []
    try:
        with open(filename, "rb") as f:
            original = f.read()
        # Note: cmake-format requires '-o -' to output to stdout when given a filename
        # Without this flag, it modifies the file in-place or produces no output
        proc = run_command(
            ["cmake-format", "--config-file", config, "-o", "-", filename],
            retries=retries,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return [
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code="CMAKEFORMAT",
                severity=LintSeverity.ERROR,
                name="timeout",
                original=None,
                replacement=None,
                description=(
                    "cmake-format timed out while trying to process a file. "
                    "Please report an issue in quarisma/quarisma with the "
                    "label 'module: lint'"
                ),
            )
        ]
    except OSError as err:
        return [
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code="CMAKEFORMAT",
                severity=LintSeverity.ADVICE,
                name="command-failed",
                original=None,
                replacement=None,
                description=(
                    f"Failed due to {err.__class__.__name__}:\n{err}"
                ),
            )
        ]

    if proc.returncode != 0:
        stderr = proc.stderr.decode("utf-8").strip()
        return [
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code="CMAKEFORMAT",
                severity=LintSeverity.ADVICE,
                name="command-failed",
                original=None,
                replacement=None,
                description=(
                    f"cmake-format failed (exit code {proc.returncode}):\n{stderr}"
                ),
            )
        ]

    replacement = proc.stdout
    if original == replacement:
        return []

    return [
        LintMessage(
            path=filename,
            line=None,
            char=None,
            code="CMAKEFORMAT",
            severity=LintSeverity.WARNING,
            name="format",
            original=original.decode("utf-8"),
            replacement=replacement.decode("utf-8"),
            description=(
                "See https://cmake-format.readthedocs.io/\n"
                "Run `lintrunner -a` to apply this patch."
            ),
        )
    ]


def main() -> None:
    """Main entry point for the linter."""
    parser = argparse.ArgumentParser(
        description="Format files with cmake-format.",
        fromfile_prefix_chars="@",
    )
    parser.add_argument(
        "--config",
        required=True,
        help="cmake-format config file path",
    )
    parser.add_argument(
        "--retries",
        default=3,
        type=int,
        help="times to retry timed out cmake-format",
    )
    parser.add_argument(
        "--timeout",
        default=90,
        type=int,
        help="seconds to wait for cmake-format",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="verbose logging",
    )
    parser.add_argument(
        "filenames",
        nargs="+",
        help="paths to lint",
    )
    args = parser.parse_args()

    logging.basicConfig(
        format="<%(threadName)s:%(levelname)s> %(message)s",
        level=logging.NOTSET
        if args.verbose
        else logging.DEBUG
        if len(args.filenames) < 1000
        else logging.INFO,
        stream=sys.stderr,
    )

    config = os.path.normpath(args.config) if IS_WINDOWS else args.config
    if not Path(config).exists():
        lint_message = LintMessage(
            path=None,
            line=None,
            char=None,
            code="CMAKEFORMAT",
            severity=LintSeverity.ERROR,
            name="init-error",
            original=None,
            replacement=None,
            description=(
                f"Could not find cmake-format config at {config}, "
                "did you forget to run `lintrunner init`?"
            ),
        )
        print(json.dumps(lint_message._asdict()), flush=True)
        sys.exit(0)

    with concurrent.futures.ThreadPoolExecutor(
        max_workers=os.cpu_count(),
        thread_name_prefix="Thread",
    ) as executor:
        futures = {
            executor.submit(
                check_file,
                x,
                config,
                args.retries,
                args.timeout,
            ): x
            for x in args.filenames
        }
        for future in concurrent.futures.as_completed(futures):
            try:
                for lint_message in future.result():
                    print(json.dumps(lint_message._asdict()), flush=True)
            except Exception:
                logging.critical('Failed at "%s".', futures[future])
                raise


if __name__ == "__main__":
    main()

