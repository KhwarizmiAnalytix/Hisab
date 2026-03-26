# Coverage Tools

Code coverage generation for the Quarisma project. Supports **GCC** (lcov),
**Clang** (LLVM), and **MSVC** (OpenCppCoverage) with automatic compiler
detection, and produces consistent HTML and JSON reports across all three.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Requirements](#requirements)
3. [Usage](#usage)
   - [CLI — `run_coverage.py`](#cli--run_coveragepy)
   - [Python API — `get_coverage()`](#python-api--get_coverage)
   - [HTML report standalone CLI](#html-report-standalone-cli)
4. [Visual Studio .sln without CMake](#visual-studio-sln-without-cmake)
5. [Output](#output)
6. [Configuration — `coverage.toml`](#configuration--coveragetoml)
   - [All options](#all-options)
   - [Customising exclude patterns](#customising-exclude-patterns)
   - [MSVC timeouts](#msvc-timeouts)
7. [Architecture](#architecture)
   - [File map](#file-map)
   - [Data flow](#data-flow)
   - [Compiler detection chain](#compiler-detection-chain)
8. [Compiler-specific notes](#compiler-specific-notes)
   - [GCC / lcov](#gcc--lcov)
   - [Clang / LLVM](#clang--llvm)
   - [MSVC / OpenCppCoverage](#msvc--opencppcoverage)
9. [HTML report module](#html-report-module)
10. [Running the tests](#running-the-tests)
11. [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
# Auto-detect compiler, generate both HTML and JSON (default)
python run_coverage.py --build=<path/to/build>

# JSON only
python run_coverage.py --build=build --output=json

# HTML only
python run_coverage.py --build=build --output=html

# Use a custom config file
python run_coverage.py --build=build --config=my_coverage.toml

# Override the source folder and add extra exclusions
python run_coverage.py --build=build --filter=Src --exclude-patterns="*Generated*,*Benchmark*"
```

---

## Requirements

| Compiler | Required tools |
|---|---|
| GCC | `lcov`, `genhtml`, `gcov` (install via `apt install lcov` / `brew install lcov`) |
| Clang | `llvm-profdata`, `llvm-cov` (part of your LLVM installation) |
| MSVC | [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage) on Windows |

Python 3.11+ is required for built-in TOML support (`tomllib`). On Python
3.10 and older, install `tomli`:

```bash
pip install tomli
```

---

## Usage

### CLI — `run_coverage.py`

```
python run_coverage.py --build=PATH [OPTIONS]
```

| Argument | Default | Description |
|---|---|---|
| `--build=PATH` | *(required)* | Build directory. Accepts absolute or relative paths (resolved against CWD, script dir, or project root). |
| `--compiler=NAME` | auto-detect | Force `gcc`, `clang`, or `msvc`. Use this for `.sln` builds or when auto-detection fails. |
| `--config=PATH` | auto-detected | Path to `coverage.toml`. Falls back to built-in defaults if not found. |
| `--filter=FOLDER` | `Library` (from config) | Source folder containing modules to analyse. All non-hidden subdirectories are treated as modules. |
| `--output=FORMAT` | `html-and-json` (from config) | `json`, `html`, or `html-and-json`. |
| `--exclude-patterns=LIST` | *(none)* | Comma-separated extra patterns to exclude (e.g. `"*Generated*,*Benchmark*"`). Merged with the defaults from config. |
| `--verbose` | off | Print extra diagnostic output. |
| `-h` / `--help` | — | Show detailed help. |

**Compiler resolution order**: `--compiler` flag → `compiler` in `coverage.toml` → auto-detection.

### Python API — `get_coverage()`

```python
from run_coverage import get_coverage

exit_code = get_coverage(
    compiler="auto",            # "clang" | "gcc" | "msvc" | "auto"
    build_folder="build",
    source_folder="Library",
    output_folder=None,         # defaults to build_folder/coverage_report
    exclude_patterns=["*Benchmark*"],
    verbose=False,
    output_format="html-and-json",
    quarisma_root="/path/to/project",
)
```

### HTML report standalone CLI


Convert an existing JSON coverage report to HTML:

```bash
python -m html_report.cli --json=coverage_report/coverage_summary.json --output=my_html
```

---

## Visual Studio .sln without CMake

When your project uses a Visual Studio Solution (`.sln`) instead of CMake,
there is no `CMakeCache.txt` for the tool to read. The compiler is still
detected automatically through a fallback chain, but you can also override it
explicitly.

### Automatic detection (no extra flags needed)

The tool tries the following in order until one succeeds:

| Step | What is checked | Detected as |
|---|---|---|
| 1 | `CMakeCache.txt` → `QUARISMA_COMPILER_ID` | gcc / clang / msvc |
| 2 | `.sln` or `.vcxproj` in `--build` dir or up to 2 parent dirs | msvc |
| 3 | VS environment variables (`VCINSTALLDIR`, `VSCMD_ARG_TGT_ARCH`, …) | msvc |
| 4 | `cl.exe` present in `PATH` | msvc |

Steps 2–4 are specifically designed for `.sln` projects. If you run the tool
from a **VS Developer Command Prompt** (or after calling `vcvarsall.bat`),
detection via environment variables (step 3) will succeed automatically.

### Explicit override (most reliable)

```bash
# Fastest option — skip detection entirely
python run_coverage.py --build=x64\Release --compiler=msvc --filter=Library

# Or set it permanently in coverage.toml so you never need the flag
```

```toml
# coverage.toml
[coverage]
compiler = "msvc"
```

### Where to point `--build`

For a typical Visual Studio output layout, `--build` should point to the
directory that contains your compiled binaries and test executables:

```
MyProject/
├── MyProject.sln
├── x64/
│   ├── Release/          ← pass this as --build
│   │   ├── MyTests.exe
│   │   └── *.dll
│   └── Debug/
└── Library/              ← pass this as --filter (or set filter in coverage.toml)
```

```bash
python run_coverage.py --build=x64\Release --compiler=msvc --filter=Library
```

If your binaries are in a non-standard location, configure the search
directories in `coverage.toml`:

```toml
[coverage.tests]
search_dirs = [".", "x64/Release", "x64/Debug", "bin/Release", "bin/Debug"]
```

### Finding OpenCppCoverage

OpenCppCoverage must be installed on Windows. Download from
[github.com/OpenCppCoverage/OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage/releases).

The tool searches for it in this order:

1. `opencppcoverage_path` in `coverage.toml`
2. `OPENCPPCOVERAGE_PATH` environment variable
3. System `PATH`
4. `C:\Program Files\OpenCppCoverage\`
5. `C:\Program Files (x86)\OpenCppCoverage\`

---

## Output

All output is written to `<build_dir>/coverage_report/` by default
(configurable via `coverage_report_dir`).

```
coverage_report/
├── coverage_summary.json   # Cobertura-compatible JSON (format v2.0)
├── html/
│   ├── index.html          # Summary page with per-file and per-directory tables
│   └── <module>/<file>.html  # Line-by-line annotated source pages
│
│   # MSVC only:
├── raw/
│   ├── <TestName>.xml      # Cobertura XML from OpenCppCoverage
│   └── <TestName>.cov      # Binary coverage data
│
│   # GCC intermediates (in build_dir, not coverage_report):
├── coverage.info
└── coverage_filtered.info
```

### `coverage_summary.json` schema

```json
{
  "metadata": {
    "format_version": "2.0",
    "generator": "quarisma_coverage_tool",
    "schema": "cobertura-compatible"
  },
  "summary": {
    "line_coverage":     { "total": 0, "covered": 0, "uncovered": 0, "percent": 0.0 },
    "function_coverage": { "total": 0, "covered": 0, "uncovered": 0, "percent": 0.0 },
    "region_coverage":   { "total": 0, "covered": 0, "uncovered": 0, "percent": 0.0 }
  },
  "files": [
    {
      "file": "/abs/path/to/source.cpp",
      "line_coverage":     { "total": 120, "covered": 95, "uncovered": 25, "percent": 79.17 },
      "function_coverage": { "total": 10,  "covered": 9,  "uncovered": 1,  "percent": 90.0  }
    }
  ]
}
```

---

## Configuration — `coverage.toml`

Place `coverage.toml` next to `run_coverage.py` (or at the project root).
The file is **optional** — all values have built-in defaults so the tool
works out of the box without it.

The config is loaded once and cached. Pass `--config=PATH` on the CLI to
use a non-standard location.

### All options

```toml
[coverage]
# Subfolder within the project containing modules to analyse
filter = "Library"

# Default source folder passed to compiler modules
source_folder = "Library"

# Default output format: "json" | "html" | "html-and-json"
output_format = "html-and-json"

# Name of the output directory (relative to build_dir)
coverage_report_dir = "coverage_report"


[coverage.exclude]
# lcov / OpenCppCoverage glob-style patterns to exclude from coverage
patterns = [
    "*ThirdParty*",
    "*Testing*",
    "/usr/*",
]

# LLVM -ignore-filename-regex patterns (Clang only)
llvm_ignore_regex = [
    ".*Testing[/\\\\].*",
    ".*Serialization[/\\\\].*",
    ".*ThirdParty[/\\\\].*",
    ".*quarismasys[/\\\\].*",
]


[coverage.project]
# Filenames/directories whose presence marks the project root
markers = [".git", ".gitignore", "pyproject.toml"]


[coverage.tests]
# Directories (relative to build_dir) searched for test executables
search_dirs = ["bin", "bin/Debug", "bin/Release", "lib", "tests"]

# Glob patterns identifying test executables
patterns = ["*Test*", "*test*", "*CxxTests*"]

# Test executable name template — {module} is replaced at runtime (Clang/MSVC)
exe_pattern = "{module}CxxTests"

# .profraw filename template (Clang only)
profraw_pattern = "{module}CxxTests.profraw"

# Working directory for test execution, relative to build_dir (Clang only)
# {filter} and {module} are replaced at runtime
test_dir_template = "{filter}/{module}/Testing/Cxx"


[coverage.gcc]
# Error types passed to lcov --ignore-errors (comma-joined at runtime)
lcov_ignore_errors = ["mismatch", "negative", "gcov"]


[coverage.msvc]
# Seconds before the test-verification run times out
verify_timeout = 30

# Seconds before the OpenCppCoverage instrumented run times out
coverage_timeout = 120

# Explicit path to OpenCppCoverage.exe — leave empty to auto-detect
# Auto-detection order: OPENCPPCOVERAGE_PATH env var → system PATH → common install dirs
opencppcoverage_path = ""


[coverage.html]
# Theme values — documented for future use; not yet wired into HtmlGenerator
primary_color    = "#007bff"
covered_color    = "#28a745"
uncovered_color  = "#dc3545"
neutral_color    = "#6c757d"
background_color = "#f5f5f5"
max_width        = "1200px"
font_family      = "Arial, sans-serif"
code_font_family = "'Courier New', monospace"
```

### Customising exclude patterns

Patterns passed via `--exclude-patterns` on the CLI are **merged** with the
defaults in `coverage.toml` (union, no duplicates). To replace the defaults
entirely, set `patterns = []` in `coverage.toml` and rely on the CLI flag.

```bash
# Add extra patterns on top of the configured defaults
python run_coverage.py --build=build --exclude-patterns="*Generated*,*Serialization*"
```

### MSVC timeouts

If OpenCppCoverage hangs on large test suites, increase the timeouts:

```toml
[coverage.msvc]
verify_timeout   = 60
coverage_timeout = 300
```

---

## Architecture

### File map

```
Tools/coverage/
├── coverage.toml           Configuration file (all hardcoded values)
├── run_coverage.py         Main CLI entry point & Python API (get_coverage)
├── common.py               Shared utilities, config loader (load_config / get_config)
├── clang_coverage.py       Clang/LLVM implementation (llvm-profdata, llvm-cov)
├── gcc_coverage.py         GCC implementation (lcov, genhtml, gcov)
├── msvc_coverage.py        MSVC implementation (OpenCppCoverage)
├── coverage_summary.py     JSON summary generator
├── test_html_report.py     Test suite for the HTML report module
└── html_report/
    ├── __init__.py         Exports HtmlGenerator, JsonHtmlGenerator
    ├── __main__.py         Entry point for `python -m html_report`
    ├── cli.py              Standalone CLI (--json → --output)
    ├── html_generator.py   Direct coverage data → HTML
    ├── json_html_generator.py  JSON report → HTML
    ├── directory_aggregator.py  Directory-level metric rollup
    ├── templates.py        Shared HTML/CSS templates
    └── custom.css          External stylesheet
```

### Data flow

```
build_dir/
  └── CMakeCache.txt
        │
        ▼ detect_compiler()
  ┌─────────────┐
  │ run_coverage│ ◄── coverage.toml (load_config)
  └──────┬──────┘
         │
   ┌─────┼──────────┐
   ▼     ▼          ▼
 gcc   clang      msvc
 coverage coverage coverage
   │     │          │
   └─────┼──────────┘
         │ LCOV / Cobertura XML / profraw
         ▼
  coverage_report/
    ├── coverage_summary.json
    └── html/
          ├── index.html
          └── <file>.html
                    ▲
             HtmlGenerator
             (html_report/)
```

**Config resolution order for each value:**

1. CLI flag (highest priority — overrides everything)
2. `coverage.toml` (auto-detected or `--config` path)
3. Built-in defaults in `common.CONFIG` (lowest priority)

---

## Compiler-specific notes

### GCC / lcov

**Prerequisites**: Build with `-fprofile-arcs -ftest-coverage` (or
`--coverage`). CMake: set `QUARISMA_COMPILER_ID=gcc`.

**What happens**:
1. `lcov --capture` collects `.gcda` files from `build_dir`.
2. `lcov --remove` strips excluded patterns.
3. Filtered data is parsed into JSON and/or rendered to HTML.

Intermediate files (`coverage.info`, `coverage_filtered.info`) are written
directly to `build_dir`. The final report goes to `coverage_report/`.

**Common issue — empty `coverage.info`**: no `.gcda` files were generated.
Verify tests actually ran and that the build used the coverage flags.

### Clang / LLVM

**Prerequisites**: Build with `-fprofile-instr-generate -fcoverage-mapping`.
CMake: set `QUARISMA_COMPILER_ID=clang`.

**What happens** (per module):
1. `prepare_llvm_coverage()` runs `<module>CxxTests` with
   `LLVM_PROFILE_FILE=<module>CxxTests.profraw`.
2. `llvm-profdata merge` merges all `.profraw` files into
   `all-merged.profdata`.
3. `llvm-cov export -format=lcov` produces `coverage.lcov`.
4. The LCOV file is parsed into JSON and/or HTML.

Test executable and profraw paths are driven by the config templates:

```toml
[coverage.tests]
exe_pattern      = "{module}CxxTests"
profraw_pattern  = "{module}CxxTests.profraw"
test_dir_template = "{filter}/{module}/Testing/Cxx"
```

### MSVC / OpenCppCoverage

**Prerequisites**: Windows only. MSVC build. OpenCppCoverage installed.

**What happens**:
1. Each test executable is run through `OpenCppCoverage.exe` with
   `--export_type=html`, `--export_type=cobertura`, and
   `--export_type=binary`.
2. Cobertura XML files (in `raw/`) are parsed into the standard JSON schema.
3. Line-by-line HTML is generated from the XML using `HtmlGenerator`.

**Finding OpenCppCoverage** (in order):
1. `opencppcoverage_path` in `coverage.toml`
2. `OPENCPPCOVERAGE_PATH` environment variable
3. System `PATH`
4. `C:\Program Files\OpenCppCoverage\`
5. `C:\Program Files (x86)\OpenCppCoverage\`

---

## HTML report module

The `html_report/` package can be used independently of the coverage runner.

### From Python

```python
from html_report import HtmlGenerator, JsonHtmlGenerator

# Generate from raw line coverage data
gen = HtmlGenerator(output_dir="my_report", source_root="/project/Library")
gen.generate_report(
    covered_lines={"src/foo.cpp": {1, 2, 5}},
    uncovered_lines={"src/foo.cpp": {3, 4}},
    execution_counts={"src/foo.cpp": {1: 10, 2: 3, 5: 1}},
)

# Generate from a coverage_summary.json file
jgen = JsonHtmlGenerator(output_dir="my_report")
jgen.generate_from_json("coverage_report/coverage_summary.json")
```

### From the CLI

```bash
python -m html_report --json=coverage_summary.json --output=html_out --verbose
# or
python -m html_report.cli --json=coverage_summary.json --output=html_out
```

### Classes

| Class | Module | Purpose |
|---|---|---|
| `HtmlGenerator` | `html_generator.py` | Builds HTML from raw `covered_lines` / `uncovered_lines` dicts |
| `JsonHtmlGenerator` | `json_html_generator.py` | Builds HTML from a `coverage_summary.json` file or dict |
| `DirectoryAggregator` | `directory_aggregator.py` | Rolls up line counts to directory level for the index page |

---

## Running the tests

```bash
cd Tools/coverage
python -m pytest test_html_report.py -v
```

Expected output: **17 passed**.

The test suite covers:
- `HtmlGenerator` — report creation, execution counts, HTML content
- `JsonHtmlGenerator` — JSON file and dict inputs, file pages, metrics
- CLI interface — valid JSON, missing JSON, verbose flag
- Edge cases — empty data, 100% coverage, 0% coverage

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `CMakeCache.txt not found` | Build directory is wrong or the project was never configured | Check `--build` points to the CMake build directory |
| `QUARISMA_COMPILER_ID not found` | CMake cache exists but the variable is missing | Reconfigure with the correct CMake toolchain |
| `coverage.info is empty` (GCC) | No `.gcda` files — tests didn't run or build is missing `--coverage` flag | Run the tests first; verify the build flags |
| `No profraw files generated` (Clang) | Test executable not found or failed to run | Check `exe_pattern` in `coverage.toml` matches the actual binary name |
| `OpenCppCoverage not found` (MSVC) | Tool not installed or not on PATH | Install from the GitHub releases page; set `OPENCPPCOVERAGE_PATH` or `opencppcoverage_path` in config |
| `No modules found in <source_dir>` | `--filter` folder doesn't exist under the project root | Verify `filter` in `coverage.toml` or pass `--filter` explicitly |
| TOML config silently ignored | Python < 3.11 and `tomli` not installed | `pip install tomli` |
