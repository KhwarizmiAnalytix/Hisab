from __future__ import annotations

import argparse
import concurrent.futures
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import time
from enum import Enum
from pathlib import Path
from sysconfig import get_paths as gp
from typing import NamedTuple


# Quarisma directory root
def scm_root() -> str:
    path = os.path.abspath(os.getcwd())
    # pyrefly: ignore  # bad-assignment
    while True:
        if os.path.exists(os.path.join(path, ".git")):
            return path
        if os.path.isdir(os.path.join(path, ".hg")):
            return path
        # pyrefly: ignore  # bad-argument-type
        n = len(path)
        path = os.path.dirname(path)
        if len(path) == n:
            raise RuntimeError("Unable to find SCM root")


QUARISMA_ROOT = scm_root()


def find_best_build_dir(preferred: str | None) -> Path | None:
    """
    Pick which build directory's compile_commands.json to check files
    against.

    Scripts/setup.py names build directories with a suffix per feature
    token (build_ninja_<tokens...>, e.g. build_ninja_clangtidy_project_
    memory) -- there is no single stable directory name to hardcode, and a
    directory built for a narrower scope (one --project.<lib>, or missing
    whatever tokens the current work needs) only has compile_commands.json
    entries for that scope. Files outside it fall back to a synthesized,
    broken compile command in clang-tidy (see check_file's compile-error
    handling), which silently makes them look "clean" instead of checked.

    If `preferred` is given and has a compile_commands.json, use it as-is
    (explicit and reproducible, e.g. for a future CI job pinned to one
    build). Otherwise auto-detect: the most recently modified build_ninja*
    directory under QUARISMA_ROOT that has a compile_commands.json --
    mirroring the freshness-based selection Scripts/setup.py's own
    BuildDirectoryDetector uses for its --analyze step, so lintrunner checks
    against whatever was built most recently instead of a stale guess.
    """
    root = Path(QUARISMA_ROOT)

    if preferred:
        preferred_path = Path(preferred)
        if not preferred_path.is_absolute():
            preferred_path = root / preferred_path
        if (preferred_path / "compile_commands.json").is_file():
            return preferred_path

    candidates = [
        entry
        for entry in root.glob("build_ninja*")
        if entry.is_dir() and (entry / "compile_commands.json").is_file()
    ]
    if not candidates:
        return None

    return max(candidates, key=lambda p: (p / "compile_commands.json").stat().st_mtime)


# Returns '/usr/local/include/python<version number>'
def get_python_include_dir() -> str:
    return gp()["include"]


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


# quarisma/core/DispatchKey.cpp:281:26: error: 'k' used after it was moved [bugprone-use-after-move]
RESULTS_RE: re.Pattern[str] = re.compile(
    r"""(?mx)
    ^
    (?P<file>.*?):
    (?P<line>\d+):
    (?:(?P<column>-?\d+):)?
    \s(?P<severity>\S+?):?
    \s(?P<message>.*)
    \s(?P<code>\[.*\])
    $
    """
)


def run_command(
    args: list[str],
) -> subprocess.CompletedProcess[bytes]:
    logging.debug("$ %s", " ".join(args))
    start_time = time.monotonic()
    try:
        return subprocess.run(
            args,
            capture_output=True,
            check=False,
        )
    finally:
        end_time = time.monotonic()
        logging.debug("took %dms", (end_time - start_time) * 1000)


# Severity is either "error" or "note":
# https://github.com/python/mypy/blob/8b47a032e1317fb8e3f9a818005a6b63e9bf0311/mypy/errors.py#L46-L47
severities = {
    "error": LintSeverity.ERROR,
    "warning": LintSeverity.WARNING,
}


def clang_search_dirs() -> list[str]:
    # Compilers are ordered based on fallback preference
    # We pick the first one that is available on the system
    compilers = ["clang", "gcc", "cpp", "cc"]
    compilers = [c for c in compilers if shutil.which(c) is not None]
    if len(compilers) == 0:
        raise RuntimeError(f"None of {compilers} were found")
    compiler = compilers[0]

    result = subprocess.run(
        [compiler, "-E", "-x", "c++", "-", "-v"],
        stdin=subprocess.DEVNULL,
        capture_output=True,
        check=True,
    )
    stderr = result.stderr.decode().strip().split("\n")
    search_start = r"#include.*search starts here:"
    search_end = r"End of search list."

    append_path = False
    search_paths = []
    for line in stderr:
        if re.match(search_start, line):
            if append_path:
                continue
            else:
                append_path = True
        elif re.match(search_end, line):
            break
        elif append_path:
            search_paths.append(line.strip())

    return search_paths


include_args = []
include_dir = [
    "/usr/lib/llvm-11/include/openmp",
    get_python_include_dir(),
    os.path.join(QUARISMA_ROOT, "ThirdParty/pybind11/include"),
] + clang_search_dirs()
for dir in include_dir:
    include_args += ["--extra-arg", f"-I{dir}"]


# Mirrors Cmake/tools/clang_tidy.cmake's quarisma_target_clang_tidy() exactly --
# without --header-filter, clang-tidy only reports diagnostics in the file
# passed on the command line, not in any header it includes. The
# build-integrated pass (CXX_CLANG_TIDY) analyzes each .cpp with this filter,
# so a finding physically located in a header only shows up there if the
# header's path matches this pattern; keep both in sync if either changes.
HEADER_FILTER = f"^{re.escape(QUARISMA_ROOT)}/(Library|Cmake|Tools|Examples)/.*"
EXCLUDE_HEADER_FILTER = r".*/(ThirdParty|third_party|3rdparty|third-party)/.*"
HEADER_EXTENSIONS = (".h", ".hxx", ".hpp")


def load_compiled_files(build_dir: Path) -> set[str]:
    """Absolute paths of every file with a real compile_commands.json entry."""
    try:
        with open(build_dir / "compile_commands.json", encoding="utf-8") as f:
            entries = json.load(f)
    except (OSError, json.JSONDecodeError):
        return set()
    return {
        str(Path(e["file"]).resolve()) for e in entries if isinstance(e.get("file"), str)
    }


def check_file(
    filename: str,
    binary: str,
    build_dir: Path,
    compiled_files: set[str],
) -> list[LintMessage]:
    # Headers are never their own translation unit -- compile_commands.json
    # only ever has entries for actually-compiled .cpp/.cc/.c files, so a
    # header checked directly here has no real compile command to use.
    # clang-tidy would fall back to a synthesized/guessed one missing the
    # project's real -D/-I flags, which can silently produce incomplete or
    # wrong results instead of a clear failure. This is exactly how the real
    # build validates headers too: CXX_CLANG_TIDY only ever analyzes .cpp
    # files, and surfaces header findings via --header-filter as a side
    # effect of that -- never by compiling a header standalone. Match that:
    # skip headers with no TU of their own rather than guess.
    if (
        filename.endswith(HEADER_EXTENSIONS)
        and str(Path(filename).resolve()) not in compiled_files
    ):
        logging.debug(
            "Skipping standalone check of %s -- has no compile_commands.json "
            "entry (headers aren't their own translation unit); it's covered "
            "via --header-filter when a .cpp that includes it is checked.",
            filename,
        )
        return []
    try:
        proc = run_command(
            [
                binary,
                f"-p={build_dir}",
                "-warnings-as-errors=*",
                f"--header-filter={HEADER_FILTER}",
                f"--exclude-header-filter={EXCLUDE_HEADER_FILTER}",
                *include_args,
                filename,
            ],
        )
    except OSError as err:
        return [
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code="CLANGTIDY",
                severity=LintSeverity.ERROR,
                name="command-failed",
                original=None,
                replacement=None,
                description=(f"Failed due to {err.__class__.__name__}:\n{err}"),
            )
        ]
    stdout_text = proc.stdout.decode()
    lint_messages = []
    try:
        # Change the current working directory to the build directory, since
        # clang-tidy will report files relative to the build directory.
        saved_cwd = os.getcwd()
        os.chdir(build_dir)

        for match in RESULTS_RE.finditer(stdout_text):
            # Convert the reported path to an absolute path.
            abs_path = str(Path(match["file"]).resolve())
            if not abs_path.startswith(QUARISMA_ROOT):
                continue
            message = LintMessage(
                path=abs_path,
                name=match["code"],
                description=match["message"],
                line=int(match["line"]),
                char=int(match["column"])
                if match["column"] is not None and not match["column"].startswith("-")
                else None,
                code="CLANGTIDY",
                severity=severities.get(match["severity"], LintSeverity.ERROR),
                original=None,
                replacement=None,
            )
            lint_messages.append(message)
    finally:
        os.chdir(saved_cwd)

    # clang-tidy exits non-zero when it couldn't actually compile the file --
    # typically because --build_dir's compile_commands.json has no entry (or a
    # stale one) for it, so it fell back to a bogus command missing include
    # paths (e.g. "<stddef.h> file not found", printed as "Found compiler
    # error(s)." on stderr). Those errors point at system headers, so the
    # QUARISMA_ROOT filter above silently drops them, which would otherwise
    # report this file as clean when clang-tidy never actually analyzed it --
    # i.e. a real check failure with zero project-code matches is exactly this
    # case, since genuine warnings-as-errors findings in project code would
    # have produced a QUARISMA_ROOT-matching entry in lint_messages already.
    # Surface it explicitly instead of returning an empty (falsely clean) list.
    if not lint_messages and proc.returncode != 0:
        lint_messages.append(
            LintMessage(
                path=filename,
                line=None,
                char=None,
                code="CLANGTIDY",
                severity=LintSeverity.ERROR,
                name="compile-error",
                original=None,
                replacement=None,
                description=(
                    f"clang-tidy exited {proc.returncode} analyzing this file "
                    f"using {build_dir}/compile_commands.json, with no "
                    "findings attributable to project code -- results are not "
                    "trustworthy. This usually means --build_dir points at a "
                    "stale or narrower build than the files being linted; "
                    "reconfigure/rebuild so compile_commands.json covers this "
                    "file. Raw output:\n\n"
                    + (stdout_text.strip() + "\n" + proc.stderr.decode().strip()).strip()
                ),
            )
        )

    return lint_messages


def main() -> None:
    parser = argparse.ArgumentParser(
        description="clang-tidy wrapper linter.",
        fromfile_prefix_chars="@",
    )
    parser.add_argument(
        "--binary",
        required=True,
        help="clang-tidy binary path",
    )
    parser.add_argument(
        "--build-dir",
        "--build_dir",
        default=None,
        help=(
            "Where the compile_commands.json file is located. Gets passed to "
            "clang-tidy -p. Optional: if omitted, or if it doesn't contain a "
            "compile_commands.json, auto-detects the most recently modified "
            "build_ninja* directory under the repo root that has one."
        ),
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

    # Resolved via PATH (shutil.which also validates a literal path such as
    # .lintbin/clang-tidy, checking that exact file rather than searching PATH
    # for it). Using PATH -- rather than a version pinned in
    # s3_init_config.json -- keeps this in lockstep with whatever compiler
    # toolchain actually produced compile_commands.json: clang-tidy's bundled
    # resource-dir/builtin headers are tightly version-coupled to libc++'s
    # internal header chaining, so even a few major versions of drift between
    # a separately pinned clang-tidy and the real build compiler reliably
    # breaks every file with bogus "system header not found" errors.
    resolved_binary = shutil.which(args.binary)
    if resolved_binary is None:
        err_msg = LintMessage(
            path="<none>",
            line=None,
            char=None,
            code="CLANGTIDY",
            severity=LintSeverity.ERROR,
            name="command-failed",
            original=None,
            replacement=None,
            description=(
                f"Could not find clang-tidy binary '{args.binary}' on PATH "
                "(or as a literal file path). Install clang-tidy from the "
                "same LLVM/Clang toolchain used to build this repo so it "
                "resolves on PATH, or pass --binary pointing at a specific "
                "executable."
            ),
        )
        print(json.dumps(err_msg._asdict()), flush=True)
        sys.exit(0)

    build_dir = find_best_build_dir(args.build_dir)
    if build_dir is None:
        err_msg = LintMessage(
            path="<none>",
            line=None,
            char=None,
            code="CLANGTIDY",
            severity=LintSeverity.ERROR,
            name="command-failed",
            original=None,
            replacement=None,
            description=(
                "No build_ninja* directory with a compile_commands.json was "
                f"found under {QUARISMA_ROOT}"
                + (f" (and '{args.build_dir}' has none either)" if args.build_dir else "")
                + ". Configure/build first (see the xsigma-build skill)."
            ),
        )
        print(json.dumps(err_msg._asdict()), flush=True)
        sys.exit(0)

    logging.info("Using build directory: %s", build_dir)
    abs_build_dir = build_dir.resolve()
    compiled_files = load_compiled_files(abs_build_dir)

    # Get the absolute path to clang-tidy and use this instead of a relative
    # one. The problem here is that os.chdir is per process, and the linter
    # uses it to move between the current directory and the build folder --
    # a relative binary path would break once cwd changes.
    binary_path = os.path.abspath(resolved_binary)

    with concurrent.futures.ThreadPoolExecutor(
        max_workers=os.cpu_count(),
        thread_name_prefix="Thread",
    ) as executor:
        futures = {
            executor.submit(
                check_file,
                filename,
                binary_path,
                abs_build_dir,
                compiled_files,
            ): filename
            for filename in args.filenames
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
