"""
This linter ensures that users don't set a SHA hash checksum in Bazel for the http_archive.
Although the security practice of setting the checksum is good, it doesn't work when the
archive is downloaded from some sites like GitHub because it can change. Specifically,
GitHub gives no guarantee to keep the same value forever. Check for more details at
https://github.com/community/community/discussions/46034.
"""

from __future__ import annotations

import argparse
import ast
import json
import re
import sys
from enum import Enum
from typing import NamedTuple
from urllib.parse import urlparse


LINTER_CODE = "BAZEL_LINTER"
SHA256_REGEX = re.compile(r"\s*sha256\s*=\s*['\"](?P<sha256>[a-zA-Z0-9]{64})['\"]\s*,")
DOMAINS_WITH_UNSTABLE_CHECKSUM = {"github.com"}


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


def is_required_checksum(urls: list[str | None]) -> bool:
    if not urls:
        return False

    for url in urls:
        if not url:
            continue

        parsed_url = urlparse(url)
        if parsed_url.hostname in DOMAINS_WITH_UNSTABLE_CHECKSUM:
            return False

    return True


def _string_list(node: ast.expr) -> list[str | None]:
    if not isinstance(node, (ast.List, ast.Tuple)):
        return []
    values: list[str | None] = []
    for elt in node.elts:
        if isinstance(elt, ast.Constant) and isinstance(elt.value, str):
            values.append(elt.value)
        else:
            values.append(None)
    return values


def get_disallowed_checksums(
    filenames: list[str],
) -> set[str]:
    """
    Return the set of disallowed checksums from all http_archive rules found in
    the given WORKSPACE/MODULE.bazel/*.bzl files.

    This parses the files directly rather than shelling out to `bazel query
    'kind(http_archive, //external:*)'`: that query targets the legacy
    WORKSPACE-only `//external` pseudo-package, which Bazel no longer exposes
    once a repo has MODULE.bazel (Bzlmod) -- it fails with "no such package
    'external'" regardless of --enable_workspace. http_archive() calls are
    plain Starlark function calls with literal keyword arguments, so an AST
    parse is both simpler and unaffected by that migration.
    """
    disallowed_checksums: set[str] = set()

    for filename in filenames:
        try:
            with open(filename, encoding="utf-8") as f:
                source = f.read()
            tree = ast.parse(source, filename=filename)
        except (OSError, SyntaxError, UnicodeDecodeError):
            continue

        for node in ast.walk(tree):
            if not isinstance(node, ast.Call):
                continue
            if not isinstance(node.func, ast.Name) or node.func.id != "http_archive":
                continue

            urls: list[str | None] = []
            checksum: str | None = None
            for kw in node.keywords:
                if kw.arg == "urls":
                    urls = _string_list(kw.value)
                elif kw.arg == "sha256" and isinstance(kw.value, ast.Constant):
                    if isinstance(kw.value.value, str):
                        checksum = kw.value.value

            if not checksum:
                continue

            if not is_required_checksum(urls):
                disallowed_checksums.add(checksum)

    return disallowed_checksums


def check_bazel(
    filename: str,
    disallowed_checksums: set[str],
) -> list[LintMessage]:
    original = ""
    replacement = ""

    with open(filename) as f:
        for line in f:
            original += f"{line}"

            m = SHA256_REGEX.match(line)
            if m:
                sha256 = m.group("sha256")

                if sha256 in disallowed_checksums:
                    continue

            replacement += f"{line}"

        if original == replacement:
            return []

        return [
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code=LINTER_CODE,
                severity=LintSeverity.ADVICE,
                name="format",
                original=original,
                replacement=replacement,
                description="Found redundant SHA checksums. Run `lintrunner -a` to apply this patch.",
            )
        ]


def main() -> None:
    parser = argparse.ArgumentParser(
        description="A custom linter to detect redundant SHA checksums in Bazel",
        fromfile_prefix_chars="@",
    )
    parser.add_argument(
        "filenames",
        nargs="+",
        help="paths to lint",
    )
    args = parser.parse_args()

    try:
        disallowed_checksums = get_disallowed_checksums(args.filenames)
    except Exception as e:
        err_msg = LintMessage(
            path=None,
            line=None,
            char=None,
            code=LINTER_CODE,
            severity=LintSeverity.ERROR,
            name="command-failed",
            original=None,
            replacement=None,
            description=(f"Failed due to {e.__class__.__name__}:\n{e}"),
        )
        print(json.dumps(err_msg._asdict()), flush=True)
        sys.exit(0)

    for filename in args.filenames:
        for lint_message in check_bazel(filename, disallowed_checksums):
            print(json.dumps(lint_message._asdict()), flush=True)


if __name__ == "__main__":
    main()
