#!/usr/bin/env python3
"""Clang/LLVM-specific code coverage generation.

Handles coverage generation for Clang compiler using LLVM coverage tools
(llvm-profdata, llvm-cov). Generates both HTML and JSON coverage reports.
"""

import os
import subprocess
import json
from pathlib import Path
from typing import List, Dict
import logging

from common import get_config, get_platform_config, find_library

logger = logging.getLogger(__name__)

def _validate_llvm_tools() -> None:
    """Validate that required LLVM tools are available.

    Raises:
        RuntimeError: If llvm-profdata or llvm-cov are not found.
    """
    for tool in ["llvm-profdata", "llvm-cov"]:
        try:
            subprocess.run(
                [tool, "--version"],
                capture_output=True,
                text=True,
                check=True
            )
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            raise RuntimeError(
                f"Required LLVM tool '{tool}' not found or not working. "
                f"Please ensure LLVM tools are installed and in PATH. Error: {e}"
            )


def _generate_json_export(build_dir: Path, coverage_dir: Path, binaries: List[str],
                         profraw_files: List[str], output_format: str = "html-and-json",
                         source_root: str = "") -> None:
    """Generate JSON coverage export using llvm-cov export with lcov format.

    Note: LLVM 21.1.0 doesn't support JSON format directly, so we use lcov format
    and parse it into JSON.

    Args:
        build_dir: Path to build directory.
        coverage_dir: Path to coverage report directory.
        binaries: List of binary objects to analyze.
        profraw_files: List of profraw files to use.
        output_format: Output format - 'json', 'html', or 'html-and-json'
        source_root: Root directory for source files (for relative paths).

    Raises:
        RuntimeError: If LLVM tools are not available or export fails.
    """
    # Validate LLVM tools are available
    _validate_llvm_tools()

    try:
        # Use llvm-cov export with lcov format (supported in LLVM 21.1.0)
        lcov_file = coverage_dir / "coverage.lcov"
        export_cmd = [
            "llvm-cov", "export"
        ] + binaries + [
            f"-instr-profile={coverage_dir / 'all-merged.profdata'}",
            "-format=lcov"
        ]

        # Add ignore patterns from config
        for pattern in get_config()["llvm_ignore_regex"]:
            export_cmd.insert(-2, f"-ignore-filename-regex={pattern}")

        result = subprocess.run(export_cmd, capture_output=True, text=True, check=True)

        if result.stdout:
            # Save LCOV output to file
            with open(lcov_file, 'w', encoding='utf-8') as f:
                f.write(result.stdout)
            print(f"LCOV file generated: {lcov_file}")
            # Generate coverage report based on output format
            _generate_summary_json_from_lcov(lcov_file, coverage_dir, output_format,
                                            source_root)
        else:
            raise RuntimeError("llvm-cov export produced no output")
    except subprocess.CalledProcessError as e:
        error_msg = f"LLVM coverage export failed: {e}"
        if e.stderr:
            error_msg += f"\nError details: {e.stderr}"
        raise RuntimeError(error_msg)


def _parse_lcov_for_line_coverage(lcov_file: Path) -> Dict[str, Dict]:
    """Parse LCOV file to extract line-by-line coverage data.

    Args:
        lcov_file: Path to LCOV file.

    Returns:
        Dictionary mapping file paths to coverage data with covered/uncovered lines.
    """
    file_coverage = {}
    current_file = None
    covered_lines = set()
    uncovered_lines = set()
    execution_counts = {}

    try:
        with open(lcov_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()

                if line.startswith("SF:"):
                    # Save previous file data
                    if current_file:
                        file_coverage[current_file] = {
                            "covered": covered_lines,
                            "uncovered": uncovered_lines,
                            "execution_counts": execution_counts
                        }
                    # Start new file
                    current_file = line[3:]
                    covered_lines = set()
                    uncovered_lines = set()
                    execution_counts = {}

                elif line.startswith("DA:") and current_file:
                    # Line data: DA:line_number,hit_count
                    parts = line[3:].split(',')
                    if len(parts) == 2:
                        line_num = int(parts[0])
                        hit_count = int(parts[1])
                        execution_counts[line_num] = hit_count
                        if hit_count > 0:
                            covered_lines.add(line_num)
                        else:
                            uncovered_lines.add(line_num)

                elif line == "end_of_record" and current_file:
                    # End of file record
                    file_coverage[current_file] = {
                        "covered": covered_lines,
                        "uncovered": uncovered_lines,
                        "execution_counts": execution_counts
                    }
                    current_file = None
                    covered_lines = set()
                    uncovered_lines = set()
                    execution_counts = {}

    except Exception as e:
        print(f"Warning: Failed to parse LCOV file for line coverage: {e}")

    return file_coverage


def _generate_html_from_lcov(lcov_file: Path, coverage_dir: Path,
                             source_root: str = "") -> bool:
    """Generate HTML coverage report directly from LCOV data.

    Extracts line-by-line coverage information from LCOV file and generates
    detailed HTML reports using HtmlGenerator.

    Args:
        lcov_file: Path to LCOV file.
        coverage_dir: Path to coverage report directory.
        source_root: Root directory for source files (for relative paths).

    Returns:
        True if HTML was generated successfully, False otherwise.
    """
    try:
        from html_report import HtmlGenerator

        line_coverage_data = _parse_lcov_for_line_coverage(lcov_file)

        # Prepare data for HtmlGenerator
        covered_lines = {}
        uncovered_lines = {}
        execution_counts = {}

        for file_path, cov_data in line_coverage_data.items():
            covered_lines[file_path] = cov_data["covered"]
            uncovered_lines[file_path] = cov_data["uncovered"]
            execution_counts[file_path] = cov_data["execution_counts"]

        # Generate custom HTML reports
        html_dir = coverage_dir / "html"
        html_dir.mkdir(exist_ok=True)
        generator = HtmlGenerator(html_dir, source_root, preserve_hierarchy=True)
        generator.generate_report(covered_lines, uncovered_lines, execution_counts)
        print(f"[OK] HTML coverage report generated at: {html_dir}/index.html")
        return True
    except Exception as e:
        print(f"Warning: Failed to generate HTML from LCOV: {e}")
        return False


def _generate_summary_json_from_lcov(lcov_file: Path, coverage_dir: Path,
                                     output_format: str = "html-and-json",
                                     source_root: str = "") -> None:
    """Generate summary JSON from LCOV coverage data.

    Args:
        lcov_file: Path to LCOV file generated by llvm-cov show.
        coverage_dir: Path to coverage report directory.
        output_format: Output format - 'json', 'html', or 'html-and-json'
        source_root: Root directory for source files (for relative paths).
    """
    try:
        summary = {
            "metadata": {
                "format_version": "2.0",
                "generator": "quarisma_coverage_tool",
                "schema": "cobertura-compatible"
            },
            "summary": {
                "line_coverage": {
                    "total": 0,
                    "covered": 0,
                    "uncovered": 0,
                    "percent": 0.0
                },
                "function_coverage": {
                    "total": 0,
                    "covered": 0,
                    "uncovered": 0,
                    "percent": 0.0
                },
                "region_coverage": {
                    "total": 0,
                    "covered": 0,
                    "uncovered": 0,
                    "percent": 0.0
                }
            },
            "files": []
        }

        # Parse LCOV file
        current_file = None
        file_data = {}

        with open(lcov_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()

                if line.startswith("SF:"):
                    # Source file
                    current_file = line[3:]
                    file_data[current_file] = {
                        "line_coverage": {"total": 0, "covered": 0, "uncovered": 0},
                        "function_coverage": {"total": 0, "covered": 0, "uncovered": 0}
                    }

                elif line.startswith("FNF:") and current_file:
                    # Function count
                    file_data[current_file]["function_coverage"]["total"] = int(line[4:])

                elif line.startswith("FNH:") and current_file:
                    # Function hit count
                    file_data[current_file]["function_coverage"]["covered"] = int(line[4:])

                elif line.startswith("LF:") and current_file:
                    # Line count
                    file_data[current_file]["line_coverage"]["total"] = int(line[3:])

                elif line.startswith("LH:") and current_file:
                    # Line hit count
                    file_data[current_file]["line_coverage"]["covered"] = int(line[3:])

        # Build summary from file data
        for filename, data in file_data.items():
            file_summary = {"file": filename}

            # Line coverage - copy dict before mutating to prevent data leakage
            line_cov = data["line_coverage"].copy()
            line_cov["uncovered"] = line_cov["total"] - line_cov["covered"]
            line_cov["percent"] = round(
                (line_cov["covered"] / line_cov["total"] * 100) if line_cov["total"] > 0 else 0.0,
                2
            )
            file_summary["line_coverage"] = line_cov

            # Function coverage - copy dict before mutating to prevent data leakage
            func_cov = data["function_coverage"].copy()
            func_cov["uncovered"] = func_cov["total"] - func_cov["covered"]
            func_cov["percent"] = round(
                (func_cov["covered"] / func_cov["total"] * 100) if func_cov["total"] > 0 else 0.0,
                2
            )
            file_summary["function_coverage"] = func_cov

            summary["files"].append(file_summary)

            # Accumulate summary
            summary["summary"]["line_coverage"]["total"] += line_cov["total"]
            summary["summary"]["line_coverage"]["covered"] += line_cov["covered"]
            summary["summary"]["line_coverage"]["uncovered"] += line_cov["uncovered"]

            summary["summary"]["function_coverage"]["total"] += func_cov["total"]
            summary["summary"]["function_coverage"]["covered"] += func_cov["covered"]
            summary["summary"]["function_coverage"]["uncovered"] += func_cov["uncovered"]

        # Calculate summary percentages
        if summary["summary"]["line_coverage"]["total"] > 0:
            line_percent = (summary["summary"]["line_coverage"]["covered"] /
                           summary["summary"]["line_coverage"]["total"]) * 100
            summary["summary"]["line_coverage"]["percent"] = round(line_percent, 2)

        if summary["summary"]["function_coverage"]["total"] > 0:
            func_percent = (summary["summary"]["function_coverage"]["covered"] /
                           summary["summary"]["function_coverage"]["total"]) * 100
            summary["summary"]["function_coverage"]["percent"] = round(func_percent, 2)

        # Generate output based on format
        if output_format == "json":
            # Save summary JSON only
            summary_file = coverage_dir / "coverage_summary.json"
            with open(summary_file, 'w', encoding='utf-8') as f:
                json.dump(summary, f, indent=2)
            print(f"[OK] JSON coverage report saved to: {summary_file}")

        elif output_format == "html":
            # Generate HTML directly from LCOV data
            _generate_html_from_lcov(lcov_file, coverage_dir, source_root)

        elif output_format == "html-and-json":
            # Generate both JSON and HTML
            summary_file = coverage_dir / "coverage_summary.json"
            with open(summary_file, 'w', encoding='utf-8') as f:
                json.dump(summary, f, indent=2)
            print(f"[OK] JSON coverage report saved to: {summary_file}")
            _generate_html_from_lcov(lcov_file, coverage_dir, source_root)

        else:
            print(f"Warning: Unknown output format '{output_format}', defaulting to html-and-json")
            # Fall back to html-and-json
            summary_file = coverage_dir / "coverage_summary.json"
            with open(summary_file, 'w', encoding='utf-8') as f:
                json.dump(summary, f, indent=2)
            print(f"[OK] JSON coverage report saved to: {summary_file}")
            _generate_html_from_lcov(lcov_file, coverage_dir, source_root)

    except Exception as e:
        print(f"Warning: Failed to generate summary JSON from LCOV: {e}")


def prepare_llvm_coverage(build_dir: Path, module_name: str, binaries_list: str, profraw_list: str) -> bool:
    """Discover and run all test executables for a specific module.

    Searches for test executables containing the module name in their filename
    using flexible pattern matching.

    Args:
        build_dir: Path to build directory.
        module_name: Name of the module to find tests for.
        binaries_list: Path to file for storing binary objects.
        profraw_list: Path to file for storing profraw files.

    Returns:
        True if coverage data was successfully generated, False otherwise.
    """
    platform_cfg = get_platform_config()
    exe_extension = platform_cfg["exe_extension"]
    bin_folder = platform_cfg["lib_folder"]
    dll_extension = platform_cfg["dll_extension"]

    cfg = get_config()
    coverage_report_dir = cfg["coverage_report_dir"]
    coverage_dir = build_dir / coverage_report_dir
    coverage_dir.mkdir(exist_ok=True)

    dll_path = find_library(build_dir, bin_folder, module_name, dll_extension)
    test_exe_name = cfg["test_exe_pattern"].format(module=module_name)
    test_dir_rel = cfg["test_dir_template"].format(
        filter=cfg["filter"], module=module_name
    )
    test_dir = build_dir / test_dir_rel
    test_executable = build_dir / "bin" / f"{test_exe_name}{exe_extension}"
    profraw_name = cfg["profraw_pattern"].format(module=module_name)
    profraw_file = coverage_dir / profraw_name

    # Validate that library was found
    if dll_path is None:
        print(f"Warning: Library not found for module {module_name}, skipping")
        return False

    if not test_executable.exists():
        print(f"Warning: Test executable not found for {module_name}, skipping")
        return False

    # Run test executable to generate profraw file
    env = os.environ.copy()
    env['LLVM_PROFILE_FILE'] = str(profraw_file)
    try:
        subprocess.run([str(test_executable)], env=env, check=False, cwd=str(test_dir), capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        print(f"Warning: Test execution failed for {module_name}: {e}")
        return False
    except FileNotFoundError as e:
        print(f"Warning: Could not execute {module_name}CxxTests: {e}")
        return False

    # Only write to files after all validations and successful test execution
    with open(binaries_list, 'a') as f:
        print(f"Adding {dll_path} to binaries list")
        f.write(f"-object={dll_path}\n")

    with open(binaries_list, 'a') as f:
        print(f"Adding {test_executable} to binaries list")
        f.write(f"-object={test_executable}\n")

    with open(profraw_list, 'a') as f:
        print(f"Adding {profraw_file} to profraw list")
        f.write(f"{profraw_file}\n")

    return True

def generate_llvm_coverage(
    build_dir: Path,
    modules: List[str],
    source_folder: Path,
    llvm_ignore_regex: List[str] = None,
    exclude_patterns: List[str] = None,
    verbose: bool = False,
    output_format: str = "html-and-json"
) -> None:
    """Generate code coverage using LLVM (for Clang).

    Args:
        build_dir: Path to build directory.
        modules: List of module names to analyze.
        source_folder: Path to source folder containing modules.
        llvm_ignore_regex: List of regex patterns to ignore. If None, read from config.
        exclude_patterns: List of file/folder patterns to exclude. If None, read from config.
        verbose: Enable verbose output for debugging. Default: False.
        output_format: Output format - 'json', 'html', or 'html-and-json'
    """
    cfg = get_config()
    if llvm_ignore_regex is None:
        llvm_ignore_regex = cfg["llvm_ignore_regex"]

    if exclude_patterns is None:
        exclude_patterns = cfg.get("exclude_patterns", [])

    if verbose:
        print(f"[VERBOSE] Build directory: {build_dir}")
        print(f"[VERBOSE] Modules: {modules}")
        print(f"[VERBOSE] Exclusion patterns: {exclude_patterns}")
        print(f"[VERBOSE] Output format: {output_format}")

    build_dir = Path(build_dir)
    coverage_dir = build_dir / "coverage_report"
    coverage_dir.mkdir(exist_ok=True)
    
    binaries_list = coverage_dir / "binaries.list"
    profraw_list = coverage_dir / "profraw.list"
    
    binaries_list.write_text("")
    profraw_list.write_text("")

    # Discover and run test executables
    print("Discovering test executables...")
    all_profraw_files = []
    successful_modules = 0
    for module in modules:
        if verbose:
            print(f"[VERBOSE] Processing module: {module}")
        if prepare_llvm_coverage(build_dir, module, str(binaries_list), str(profraw_list)):
            successful_modules += 1
            if verbose:
                print(f"[VERBOSE] Successfully processed module: {module}")

    if successful_modules == 0:
        print("Error: No modules processed successfully")
        return

    # Find all .profraw files
    print(f"Successfully processed {successful_modules}/{len(modules)} modules")

    print("Merging profile data...")
    profraw_files = profraw_list.read_text().strip().split('\n')
    profraw_files = [f for f in profraw_files if f]

    if verbose:
        print(f"[VERBOSE] Found {len(profraw_files)} profraw files:")
        for pf in profraw_files:
            print(f"[VERBOSE]   - {pf}")

    if not profraw_files:
        print("Error: No profraw files generated")
        return

    merge_cmd = [
        "llvm-profdata", "merge", "-o",
        str(coverage_dir / "all-merged.profdata"), "-sparse"
    ] + profraw_files

    if verbose:
        print(f"[VERBOSE] Running: {' '.join(merge_cmd)}")

    subprocess.run(merge_cmd, check=True)

    print("Generating coverage report...")
    binaries = binaries_list.read_text().strip().split('\n')
    binaries = [b for b in binaries if b]

    if verbose:
        print(f"[VERBOSE] Found {len(binaries)} binaries:")
        for binary in binaries:
            print(f"[VERBOSE]   - {binary}")

    # Generate coverage report based on output format
    print(f"Generating {output_format} coverage report...")
    if verbose:
        print(f"[VERBOSE] Output format: {output_format}")
    _generate_json_export(build_dir, coverage_dir, binaries, profraw_files, output_format,
                         str(source_folder))

