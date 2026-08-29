import re
import sys
from pathlib import Path

from mypy.plugin import Plugin


def get_correct_mypy_version() -> str:
    """Extract the mypy version from .lintrunner.toml configuration file.

    Returns:
        The mypy version string (e.g., "1.16.0").

    Raises:
        FileNotFoundError: If .lintrunner.toml is not found.
        ValueError: If mypy version cannot be extracted from the file.
    """
    lintrunner_toml_path = Path(__file__).parent.parent / ".lintrunner.toml"

    # Check if file exists
    if not lintrunner_toml_path.exists():
        print(
            f"Error: .lintrunner.toml not found at {lintrunner_toml_path}",
            file=sys.stderr,
        )
        return ""

    # Read the file content
    try:
        content = lintrunner_toml_path.read_text(encoding="utf-8")
    except OSError as e:
        print(
            f"Error reading .lintrunner.toml: {e}",
            file=sys.stderr,
        )
        return ""

    # Extract mypy version using regex
    matches = list(re.finditer(r"mypy==(\d+(?:\.\d+)*)", content))

    if not matches:
        print(
            "Error: Could not find mypy version in .lintrunner.toml",
            file=sys.stderr,
        )
        return ""

    if len(matches) > 1:
        print(
            f"Warning: Found {len(matches)} mypy version specifications in .lintrunner.toml, using the first one",
            file=sys.stderr,
        )

    version = matches[0].group(1)
    return version


def plugin(version: str) -> type[Plugin]:
    correct_version = get_correct_mypy_version()
    if version != correct_version:
        print(
            f"""\
You are using mypy version {version}, which is not supported
in the XSigma repo. Please switch to mypy version {correct_version}.

For example, if you installed mypy via pip, run this:

    pip install mypy=={correct_version}

Or if you installed mypy via conda, run this:

    conda install -c conda-forge mypy={correct_version}
""",
            file=sys.stderr,
        )
    return Plugin
