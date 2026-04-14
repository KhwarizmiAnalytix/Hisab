# Code Coverage

Comprehensive guide to the Quarisma coverage subsystem, including instrumentation, report generation, and CI integration across every supported compiler toolchain.

## Table of Contents
- [Overview](#overview)
- [Location](#location)
- [Integration with setup.py](#integration-with-setuppy)
- [Coverage Tools](#coverage-tools)
- [Configuration](#configuration)
- [Output Formats](#output-formats)
- [Usage Examples](#usage-examples)
- [CI/CD Integration](#cicd-integration)
- [Troubleshooting](#troubleshooting)
- [Related Documentation](#related-documentation)

## Overview
- The coverage system instruments Quarisma builds, runs registered tests, and produces unified HTML/JSON/LCOV reports with consistent filtering across platforms.
- Instrumentation is controlled by `QUARISMA_ENABLE_COVERAGE`; when enabled it forces non-optimised compilation, disables LTO, and adds compiler-specific flags (`-fprofile-instr-generate`, `-fcoverage-mapping`, or `--coverage`).
- Report generation is orchestrated by `Tools/coverage/run_coverage.py`, which auto-detects the active compiler, discovers test executables, and delegates to compiler-specific drivers.
- Coverage results feed into `Scripts/setup.py` summary output and optional CI gates alongside other quality signals (sanitizers, static analysis, benchmarks).

## Location
All coverage tooling lives under `Tools/coverage/`:
- `run_coverage.py` – entry point used by `setup.py` and standalone runs; handles compiler detection and workflow orchestration.
- `clang_coverage.py`, `gcc_coverage.py`, `msvc_coverage.py` – compiler-specific runners wrapping LLVM tools, gcov/lcov, and OpenCppCoverage respectively.
- `common.py` – shared helpers: project root detection, exclusion filters (`CONFIG["exclude_patterns"]`), OpenCppCoverage discovery, module enumeration.
- `coverage_summary.py` – generates Cobertura-style JSON summaries consumed by CI and the `setup.py` summary reporter.
- `html_report/` – static assets used to assemble self-contained HTML dashboards.
- `test_html_report.py` – regression tests for HTML output templates.

## Integration with setup.py
Coverage is first-class in the build helper (`Scripts/setup.py`) and is enabled by adding the `coverage` token to any command sequence.

### Behaviour
- Setting `coverage` flips `QUARISMA_ENABLE_COVERAGE=ON`, disables LTO, enforces a Debug build, and schedules the coverage workflow after the build & test phases.
- Reports are emitted into `<build_dir>/coverage_report/` and summarised in the terminal (including overall percentage buckets).
- Re-running `python setup.py analyze` regenerates reports from existing coverage data without compiling or testing again.

### Common Commands
| Toolchain | Recommended command | Build Directory | Notes |
|-----------|---------------------|-----------------|-------|
| Clang/LLVM (Linux/macOS) | `python setup.py config.build.test.ninja.clang.debug.coverage` | `build_ninja_coverage` | Uses llvm-profdata/llvm-cov for coverage reports. |
| GCC (Linux) | `python setup.py config.build.test.ninja.gcc.debug.coverage` | `build_ninja_coverage` | Invokes gcov + lcov; requires `lcov`/`genhtml` installed. |
| MSVC (Windows) | `python setup.py config.build.test.vs22.debug.coverage` | `build_vs22_coverage` | Uses OpenCppCoverage, auto-detecting the installation path via `common.find_opencppcoverage()`. |
| Re-run reporting only | `python setup.py analyze` | (auto-detected) | Reads existing data from the latest coverage-enabled build directory. |

**Tip:** Combine coverage with other suffixes (e.g. `external`, `ccache`) exactly as you would for standard builds; the helper automatically appends `_coverage` to the build folder name so artefacts remain isolated.

## Coverage Tools
### LLVM / Clang (Unix-like platforms)
- Instrumentation: `-fprofile-instr-generate -fcoverage-mapping` via `Cmake/tools/coverage.cmake`.
- Data merge: `llvm-profdata` aggregates `.profraw` files into `.profdata`.
- Reporting: `llvm-cov show` renders annotated source plus summary metrics; `llvm-cov export` feeds JSON pipelines.

### GCC / lcov (Linux)
- Instrumentation: `--coverage -fprofile-arcs -ftest-coverage` applied during configuration.
- Data merge: `gcov` produces per-source `.gcov` files; `lcov` aggregates into `coverage.info`.
- Reporting: `genhtml` converts the LCOV trace into HTML dashboards; JSON summaries are still produced through `coverage_summary.py` for consistency.

### MSVC / OpenCppCoverage (Windows)
- Instrumentation: test executables built with `/DEBUG` and `/PROFILE` flags by CMake; OpenCppCoverage launches each executable and captures coverage data.
- Reporting: OpenCppCoverage exports Cobertura XML and HTML; wrappers post-process into the unified `coverage_report` layout.
- Requirement: `OpenCppCoverage.exe` must be installed and resolvable (PATH or common program directories).

### Unified Runner
- `run_coverage.py` exposes both a CLI and a `get_coverage()` function used by `setup.py`.
- Auto-detects compilers by parsing `CMakeCache.txt` and falls back to explicit `--compiler` when supplied programmatically.
- Discovers test binaries in canonical locations (`bin/`, `bin/Debug`, `lib/`) and avoids duplicates.
- Supports exclusions, custom output folders, and verbose tracing through keyword arguments.

## Configuration
- **CMake options**:
  - `-DQUARISMA_ENABLE_COVERAGE=ON` (primary toggle); enabling coverage automatically disables LTO (`QUARISMA_ENABLE_LTO=OFF`) to avoid linker conflicts.

- **Filtering**:
  - Use `--exclude-patterns` CLI flag or `QUARISMA_COVERAGE_EXCLUDE_PATTERNS` CMake option for user-provided exclusions.
  - Modify `CONFIG["exclude_patterns"]` or `CONFIG["llvm_ignore_regex"]` in `Tools/coverage/common.py` to customize default exclusions.
- **CLI arguments** (`python Tools/coverage/run_coverage.py --help`):
  - `--build` (required): Build directory path
  - `--filter` (defaults to `Library`): Controls module discovery
  - `--output` (defaults to `html-and-json`): Output format for all compilers:
    - `json`: Generate JSON coverage data only
    - `html`: Generate HTML report directly from coverage data
    - `html-and-json`: Generate both HTML and JSON reports
  - `--exclude-patterns` (optional): Comma-separated patterns to exclude from coverage analysis (e.g., `"Test,Benchmark,third_party"`). Merged with default patterns.
  - `--verbose`: Enable detailed logging
- **Programmatic usage**: call `get_coverage(compiler="clang", build_folder="build_ninja_clang_coverage", source_folder="Library", output_folder="custom_dir", exclude_patterns=["*Generated*", "*Benchmark*"], output_format="html-and-json")` from Python tooling.
- **Dependencies**: ensure LLVM tools (`llvm-profdata`, `llvm-cov`), GCC tooling (`gcov`, `lcov`, `genhtml`), or OpenCppCoverage are installed on the host machine used to execute the coverage pipeline.

## Output Formats
All reports are written beneath `<build_dir>/coverage_report/` unless `output_folder` is overridden.

### Output Format Options
The `--output` flag controls which reports are generated (applies to all compilers):
- `--output=json`: Generates JSON coverage data only
  - `coverage_summary.json` – Cobertura-compatible JSON summary consumed by CI and downstream tooling
- `--output=html`: Generates HTML report directly from coverage data
  - `html/index.html` – interactive dashboard with per-file drill downs
- `--output=html-and-json` (default): Generates both HTML and JSON reports
  - `html/index.html` – interactive dashboard with per-file drill downs
  - `coverage_summary.json` – Cobertura-compatible JSON summary

### Additional Artifacts
- `coverage.txt` / `coverage_summary.txt` – plain-text rollups (used by `setup.py` to extract the global percentage).
- `coverage.info` – LCOV trace (GCC only), suitable for upload to external dashboards (Codecov, Coveralls).
- `*.profraw` / `*.profdata` – raw and merged LLVM instrumentation artefacts retained for post-processing and incremental reruns.

## Usage Examples
### Clang workflow via setup.py
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.debug.coverage
# Optional: regenerate reports later without rebuilding
python setup.py analyze
```
Open `build_ninja_coverage/coverage_report/html/index.html` in a browser to review annotated source.

### GCC workflow with manual tooling
```bash
cmake -B build_gcc_cov -S . -DCMAKE_BUILD_TYPE=Debug -DQUARISMA_ENABLE_COVERAGE=ON -DBUILD_TESTING=ON
cmake --build build_gcc_cov --target all
ctest --test-dir build_gcc_cov --output-on-failure
python Tools/coverage/run_coverage.py --build=build_gcc_cov --verbose
```
The runner wraps `gcov`, captures `coverage.info`, and emits the same HTML/JSON artefacts.

### MSVC + OpenCppCoverage
```powershell
cd Scripts
python setup.py config.build.test.vs22.debug.coverage
```
If OpenCppCoverage is not on PATH, place it under `C:\Program Files\OpenCppCoverage\` (auto-detected). Reports appear in `build_vs22_debug_coverage/coverage_report/`.

### Using the runner directly
```bash
# Generate both HTML and JSON (default)
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --filter=Library

# Generate JSON only
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --filter=Library --output=json

# Generate HTML only
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --filter=Library --output=html

# With verbose output
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --filter=Library --output=html-and-json --verbose

# Exclude specific patterns from coverage
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --exclude-patterns="Test,Benchmark,Generated"

# Exclude multiple patterns with wildcards
python Tools/coverage/run_coverage.py --build=build_ninja_coverage --exclude-patterns="*Test*,*Benchmark*,*Generated*"
```
Useful when integrating with external automation or when tests were executed manually. The `--output` flag works consistently across all compilers (GCC, Clang, MSVC).

### Excluding files and folders from coverage
The `--exclude-patterns` flag allows you to exclude specific files or folders from coverage analysis. Patterns are merged with default exclusions (ThirdParty, Testing, /usr/*).

**Pattern matching behavior:**
- Each pattern is matched against file paths and folder names
- Patterns support wildcards (e.g., `*Generated*`, `*Serialization*`)
- If a pattern matches any part of a file's path, that file is excluded
- If a pattern matches a folder name, all files within that folder are excluded

**Examples:**
```bash
# Exclude test files and benchmarks
--exclude-patterns="Test,Benchmark"

# Exclude generated code and serialization modules
--exclude-patterns="*Generated*,*Serialization*"

# Exclude multiple patterns
--exclude-patterns="Test,Benchmark,third_party,*Generated*"
```

**CMake integration:**
```bash
cmake -B build -S . -DQUARISMA_ENABLE_COVERAGE=ON -DQUARISMA_COVERAGE_EXCLUDE_PATTERNS="Test,Benchmark,Generated"
```

**Default exclusions (always applied):**
- `*ThirdParty*` – Third-party libraries
- `*Testing*` – Test infrastructure
- `/usr/*` – System libraries

## CI/CD Integration
- Add a coverage stage that executes the same `setup.py` command as local runs; the helper prints the global percentage and exits non-zero if the underlying tests fail.
- Upload `coverage_report/html/` as a pipeline artefact to allow interactive browsing post-build.
- Parse `coverage_report/coverage.json` (Cobertura schema) for automated gating or comment bots.
- When running in matrix jobs, store each coverage artefact with a toolchain-specific suffix to avoid overwriting (e.g., `build_ninja_clang_coverage`, `build_ninja_gcc_coverage`).
- Combine with caching (e.g., `ccache`, `sccache`) to mitigate the overhead introduced by instrumentation and test execution.

## Troubleshooting
- **0% coverage** – ensure tests were executed after instrumentation (`ctest` / `setup.py … build.test.coverage`). Look for `.profraw`, `.gcda`, or OpenCppCoverage outputs inside the build tree.
- **Tools not found** – verify LLVM/GCC binaries or OpenCppCoverage are installed and on PATH; override locations via environment variables or update `common.py` to include custom search paths.
- **Missing executables** – confirm tests are built into `bin/` or `lib/`; the runner only picks up files matching `*Test*`, `*test*`, or `*CxxTests*`.
- **Windows permission errors** – run shells with Administrator privileges when using OpenCppCoverage, or relocate the build directory outside protected folders.
- **Out-of-date HTML** – delete `coverage_report/` before re-running if you change exclusion filters or compiler flags; stale artefacts are not automatically purged.

## Related Documentation
- [Setup Guide](setup.md) – command syntax, tokens, and builder naming conventions.
- [Build Configuration](build/build-configuration.md) – coverage-related CMake options and compiler flags.
- [Usage Examples](usage-examples.md) – end-to-end build scenarios incorporating coverage.
- [Static Analysis](static-analysis.md) – complementary code-quality tooling.
- [Sanitizer Guide](sanitizer.md) – runtime instrumentation to pair with coverage for deeper defect detection.
