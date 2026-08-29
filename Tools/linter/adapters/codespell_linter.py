from __future__ import annotations

import argparse
import concurrent.futures
import json
import logging
import os
import re
import subprocess
import sys
from enum import Enum
from pathlib import Path
from typing import NamedTuple


REPO_ROOT = Path(__file__).absolute().parents[3]
PYPROJECT = REPO_ROOT / "pyproject.toml"
DICTIONARY = REPO_ROOT / "Scripts" / "suppressions" / "spell_suppressions.txt"

# codespell uses sysexits EX_DATAERR (65) when it reports misspellings.
_CODESPELL_FOUND_ISSUES = 65

# "path:line: word ==> suggestion[, suggestion]"
_HIT_RE = re.compile(
    r"^(?P<path>.*):(?P<line>\d+):\s+(?P<word>\S+)\s+==>\s+(?P<suggestions>.+)$"
)

_THIRD_PARTY_DIR_NAMES = frozenset({"thirdparty", "third_party", "3rdparty"})

FORBIDDEN_WORDS = {
    "multipy",  # project xsigma/multipy is dead  # codespell:ignore multipy
}


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


def format_error_message(
    filename: str,
    error: Exception | None = None,
    *,
    message: str | None = None,
) -> LintMessage:
    if message is None and error is not None:
        message = (
            f"Failed due to {error.__class__.__name__}:\n{error}\n"
            "Please either fix the error or add the word(s) to "
            "Scripts/suppressions/spell_suppressions.txt.\n"
            "HINT: all-lowercase words in the suppression file can cover all case variations."
        )
    return LintMessage(
        path=filename,
        line=None,
        char=None,
        code="CODESPELL",
        severity=LintSeverity.ERROR,
        name="spelling error",
        original=None,
        replacement=None,
        description=message,
    )


def is_third_party_path(path: Path) -> bool:
    """True for any path under ThirdParty / third_party / 3rdparty."""
    return any(part.lower() in _THIRD_PARTY_DIR_NAMES for part in path.parts)


def run_codespell(path: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            "-m",
            "codespell_lib",
            "--toml",
            str(PYPROJECT),
            str(path),
        ],
        capture_output=True,
        text=True,
        encoding="utf-8",
        check=False,
    )


def parse_codespell_hits(filename: str, output: str) -> list[LintMessage]:
    messages: list[LintMessage] = []
    for raw_line in output.splitlines():
        match = _HIT_RE.match(raw_line)
        if match is None:
            continue
        word = match.group("word")
        suggestions = match.group("suggestions").strip()
        messages.append(
            LintMessage(
                path=filename,
                line=int(match.group("line")),
                char=None,
                code="CODESPELL",
                severity=LintSeverity.ERROR,
                name="spelling error",
                original=None,
                replacement=None,
                description=f"{word} ==> {suggestions}",
            )
        )
    return messages


def check_file(filename: str) -> list[LintMessage]:
    path = Path(filename).absolute()
    if path.resolve() == DICTIONARY.resolve() or is_third_party_path(path):
        return []
    try:
        proc = run_codespell(path)
    except Exception as err:
        return [format_error_message(filename, err)]

    output = f"{proc.stdout or ''}{proc.stderr or ''}"
    messages = parse_codespell_hits(filename, output)
    if messages:
        return messages
    if proc.returncode not in (0, _CODESPELL_FOUND_ISSUES):
        return [
            format_error_message(
                filename,
                message=output.strip() or f"codespell exited with {proc.returncode}",
            )
        ]
    return []


def check_dictionary(filename: str) -> list[LintMessage]:
    """Check the suppression file for duplicates and sort order."""
    path = Path(filename).absolute()
    try:
        words = [
            line
            for line in path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        words_set = set(words)
        if len(words) != len(words_set):
            raise ValueError("The suppression file contains duplicate entries.")
        # pyrefly: ignore  # no-matching-overload
        uncased_words = list(map(str.lower, words))
        if uncased_words != sorted(uncased_words):
            raise ValueError(
                "The suppression file is not sorted alphabetically (case-insensitive)."
            )
        for forbidden_word in sorted(
            FORBIDDEN_WORDS & (words_set | set(uncased_words))
        ):
            raise ValueError(
                f"The suppression file contains a forbidden word: {forbidden_word!r}. "
                "Please remove it from Scripts/suppressions/spell_suppressions.txt and use "
                "'codespell:ignore' inline comment instead."
            )
    except Exception as err:
        return [format_error_message(str(filename), err)]
    return []


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Check files for spelling mistakes using codespell.",
        fromfile_prefix_chars="@",
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
        format="<%(processName)s:%(levelname)s> %(message)s",
        level=logging.NOTSET
        if args.verbose
        else logging.DEBUG
        if len(args.filenames) < 1000
        else logging.INFO,
        stream=sys.stderr,
    )

    with concurrent.futures.ProcessPoolExecutor(
        max_workers=os.cpu_count(),
    ) as executor:
        futures = {executor.submit(check_file, x): x for x in args.filenames}
        futures[executor.submit(check_dictionary, str(DICTIONARY))] = str(DICTIONARY)
        for future in concurrent.futures.as_completed(futures):
            try:
                for lint_message in future.result():
                    print(json.dumps(lint_message._asdict()), flush=True)
            except Exception:
                logging.critical('Failed at "%s".', futures[future])
                raise


if __name__ == "__main__":
    main()
