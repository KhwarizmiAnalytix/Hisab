#!/usr/bin/env python3
"""GCC/gcov-specific code coverage generation.

Handles coverage generation for GCC compiler using lcov/genhtml tools.
Generates both HTML and JSON coverage reports with consistent styling.
"""

import os
import subprocess
import re
import json
import shutil
import argparse
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Set
import logging
from common import get_config
logger = logging.getLogger(__name__)


def _check_required_tools() -> Tuple[bool, List[str]]:
    """Check if required tools for GCC coverage are installed.

    Returns:
        Tuple of (all_found: bool, missing_tools: List[str])
    """
    required_tools = ["lcov", "genhtml", "gcov"]
    missing = []

    for tool in required_tools:
        if shutil.which(tool) is None:
            missing.append(tool)

    return len(missing) == 0, missing



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
        with open(lcov_file, 'r', encoding='utf-8', errors='ignore') as f:
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


def _generate_json_from_lcov(lcov_file: Path, output_dir: Path) -> Dict:
    """Generate JSON coverage report from LCOV data.

    Args:
        lcov_file: Path to the LCOV coverage file.
        output_dir: Directory where JSON report will be saved.

    Returns:
        Dictionary containing the coverage summary data.
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

        with open(lcov_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()

                if line.startswith("SF:"):
                    current_file = line[3:]
                    file_data = {
                        "file": current_file,
                        "line_coverage": {
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
                    }

                elif line.startswith("FN:") and current_file:
                    # Function definition
                    if "functions" not in file_data:
                        file_data["functions"] = []

                elif line.startswith("FNDA:") and current_file:
                    # Function data (execution count)
                    pass

                elif line.startswith("FNF:") and current_file:
                    # Total functions
                    total_funcs = int(line[4:])
                    file_data["function_coverage"] = {
                        "total": total_funcs,
                        "covered": 0,
                        "uncovered": total_funcs,
                        "percent": 0.0
                    }

                elif line.startswith("FNH:") and current_file:
                    # Functions hit
                    covered_funcs = int(line[4:])
                    if "function_coverage" in file_data:
                        file_data["function_coverage"]["covered"] = covered_funcs
                        file_data["function_coverage"]["uncovered"] = (
                            file_data["function_coverage"]["total"] - covered_funcs
                        )
                        if file_data["function_coverage"]["total"] > 0:
                            func_percent = (covered_funcs / file_data["function_coverage"]["total"]) * 100
                            file_data["function_coverage"]["percent"] = round(func_percent, 2)

                elif line.startswith("LF:") and current_file:
                    # Total lines
                    total_lines = int(line[3:])
                    file_data["line_coverage"]["total"] = total_lines

                elif line.startswith("LH:") and current_file:
                    # Lines hit
                    covered_lines = int(line[3:])
                    file_data["line_coverage"]["covered"] = covered_lines
                    file_data["line_coverage"]["uncovered"] = (
                        file_data["line_coverage"]["total"] - covered_lines
                    )
                    if file_data["line_coverage"]["total"] > 0:
                        line_percent = (covered_lines / file_data["line_coverage"]["total"]) * 100
                        file_data["line_coverage"]["percent"] = round(line_percent, 2)

                elif line == "end_of_record":
                    if current_file and file_data:
                        summary["files"].append(file_data)
                        summary["summary"]["line_coverage"]["total"] += file_data["line_coverage"]["total"]
                        summary["summary"]["line_coverage"]["covered"] += file_data["line_coverage"]["covered"]
                        summary["summary"]["line_coverage"]["uncovered"] += file_data["line_coverage"]["uncovered"]

                        if "function_coverage" in file_data:
                            summary["summary"]["function_coverage"]["total"] += file_data["function_coverage"]["total"]
                            summary["summary"]["function_coverage"]["covered"] += file_data["function_coverage"]["covered"]
                            summary["summary"]["function_coverage"]["uncovered"] += file_data["function_coverage"]["uncovered"]

                    current_file = None
                    file_data = {}

        # Calculate summary percentages
        if summary["summary"]["line_coverage"]["total"] > 0:
            line_percent = (summary["summary"]["line_coverage"]["covered"] /
                           summary["summary"]["line_coverage"]["total"]) * 100
            summary["summary"]["line_coverage"]["percent"] = round(line_percent, 2)

        if summary["summary"]["function_coverage"]["total"] > 0:
            func_percent = (summary["summary"]["function_coverage"]["covered"] /
                           summary["summary"]["function_coverage"]["total"]) * 100
            summary["summary"]["function_coverage"]["percent"] = round(func_percent, 2)

        # Save JSON
        json_file = output_dir / "coverage_summary.json"
        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2)
        print(f"JSON coverage report saved to: {json_file}")

        return summary

    except Exception as e:
        print(f"Warning: Failed to generate JSON from LCOV: {e}")
        return {}


def _generate_html_from_lcov(lcov_file: Path, output_dir: Path,
                             source_root: str = "") -> None:
    """Generate HTML coverage report directly from LCOV data using custom templates.

    Args:
        lcov_file: Path to the LCOV coverage file.
        output_dir: Directory where HTML report will be saved.
        source_root: Root directory for source files (for relative paths).
    """
    try:
        from html_report import HtmlGenerator

        # Parse LCOV for line-by-line coverage
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
        html_dir = output_dir / "html"
        html_dir.mkdir(exist_ok=True)
        generator = HtmlGenerator(html_dir, source_root, preserve_hierarchy=True)
        generator.generate_report(covered_lines, uncovered_lines, execution_counts)
        print(f"HTML coverage report generated at: {html_dir}/index.html")

    except Exception as e:
        print(f"Warning: Failed to generate HTML from LCOV: {e}")


def _generate_html_from_json(json_file: Path, output_dir: Path) -> None:
    """Generate HTML coverage report from JSON data.

    Args:
        json_file: Path to the JSON coverage file.
        output_dir: Directory where HTML report will be saved.
    """
    try:
        from html_report import JsonHtmlGenerator

        json_html_dir = output_dir / "html_from_json"
        generator = JsonHtmlGenerator(json_html_dir)
        index_file = generator.generate_from_json(json_file)
        print(f"HTML report generated from JSON at: {index_file}")

    except Exception as e:
        print(f"Warning: Failed to generate HTML from JSON: {e}")


def generate_lcov_coverage(build_dir: Path, modules: List[str],
                          exclude_patterns: List[str] = None,
                          verbose: bool = False,
                          output_format: str = "json",
                          source_root: str = "") -> None:
    """
    Generate LCOV coverage report from build directory.

    Args:
        build_dir: Path to the build directory containing .gcda files
        modules: List of modules to include in coverage
        exclude_patterns: List of patterns to exclude (e.g., ['/usr/*', '*/ThirdParty/*'])
        verbose: Enable verbose output for debugging
        output_format: Output format - 'json', 'html', or 'html-and-json'
        source_root: Root directory for source files (for relative paths).
    """
    import subprocess

    # Check for required tools
    tools_ok, missing_tools = _check_required_tools()
    if not tools_ok:
        print(f"ERROR: Required tools not found: {', '.join(missing_tools)}")
        print("\nTo fix this, install the required tools:")
        print("  Ubuntu/Debian: sudo apt-get install lcov")
        print("  Fedora/RHEL:   sudo dnf install lcov")
        print("  macOS:         brew install lcov")
        print("\nFor WSL, run the Ubuntu/Debian command in your WSL terminal.")
        return

    cfg = get_config()

    if exclude_patterns is None:
        exclude_patterns = []

    # Default exclusions
    default_excludes = cfg["exclude_patterns"]
    exclude_patterns = list(set(default_excludes + exclude_patterns))

    coverage_info = build_dir / "coverage.info"
    coverage_filtered = build_dir / "coverage_filtered.info"
    coverage_report_dir = build_dir / cfg["coverage_report_dir"]

    if verbose:
        print(f"[VERBOSE] Build directory: {build_dir}")
        print(f"[VERBOSE] Modules: {modules}")
        print(f"[VERBOSE] Exclusion patterns: {exclude_patterns}")
        print(f"[VERBOSE] Output format: {output_format}")

    print(f"Capturing coverage data from {build_dir}...")

    # Capture coverage data
    capture_cmd = [
        "lcov",
        "--directory", str(build_dir),
        "--capture",
        "--ignore-errors", ",".join(cfg["lcov_ignore_errors"]),
        "--output-file", str(coverage_info)
    ]

    if verbose:
        print(f"[VERBOSE] Running: {' '.join(capture_cmd)}")

    result = subprocess.run(capture_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error capturing coverage: {result.stderr}")
        if verbose:
            print(f"[VERBOSE] stdout: {result.stdout}")
        return

    # Check if coverage.info has content
    coverage_info_size = coverage_info.stat().st_size if coverage_info.exists() else 0
    if coverage_info_size == 0:
        print("ERROR: coverage.info is empty!")
        print("\nPossible causes:")
        print("  1. No tests were executed (check test results above)")
        print("  2. No .gcda files were generated (check compilation flags)")
        print("  3. Tests didn't exercise any instrumented code")
        print("\nDebugging steps:")
        print(f"  1. Check for .gcda files: find {build_dir} -name '*.gcda' | head -10")
        print(f"  2. Check for .gcno files: find {build_dir} -name '*.gcno' | head -10")
        print("  3. Verify tests ran: check test output above")
        return

    if verbose:
        print(f"[VERBOSE] coverage.info size: {coverage_info_size} bytes")
        print(f"[VERBOSE] First 20 lines of coverage.info:")
        with open(coverage_info, 'r', encoding='utf-8', errors='ignore') as f:
            for i, line in enumerate(f):
                if i >= 20:
                    break
                print(f"[VERBOSE]   {line.rstrip()}")

    print("Coverage data captured successfully.")
    print(f"Filtering exclusions: {exclude_patterns}")

    # Remove excluded patterns - each pattern as separate argument to preserve order
    # and allow proper quoting of paths with spaces
    remove_cmd = ["lcov", "--remove", str(coverage_info)]
    for pattern in exclude_patterns:
        remove_cmd.append(pattern)
    remove_cmd.extend([
        "--output-file", str(coverage_filtered)
    ])

    if verbose:
        print(f"[VERBOSE] Running: {' '.join(remove_cmd)}")

    result = subprocess.run(remove_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error filtering coverage: {result.stderr}")
        if verbose:
            print(f"[VERBOSE] stdout: {result.stdout}")
        return

    # Check if filtered coverage has content
    coverage_filtered_size = coverage_filtered.stat().st_size if coverage_filtered.exists() else 0
    if coverage_filtered_size == 0:
        print("ERROR: coverage_filtered.info is empty after filtering!")
        print("\nThis usually means the exclusion patterns removed all coverage data.")
        print("Exclusion patterns used:")
        for pattern in exclude_patterns:
            print(f"  - {pattern}")
        print("\nDebugging: Run with verbose flag to see what was captured before filtering.")
        return

    if verbose:
        print(f"[VERBOSE] coverage_filtered.info size: {coverage_filtered_size} bytes")

    print("Coverage data filtered.")

    # Generate output based on format
    if output_format == "json":
        print("Generating JSON coverage report...")
        _generate_json_from_lcov(coverage_filtered, coverage_report_dir)
        print(f"[OK] JSON coverage report generated at {coverage_report_dir}/coverage_summary.json")

    elif output_format == "html":
        print(f"Generating HTML report to {coverage_report_dir}...")
        _generate_html_from_lcov(coverage_filtered, coverage_report_dir, source_root)
        print(f"[OK] HTML coverage report generated at {coverage_report_dir}/html/index.html")

    elif output_format == "html-and-json":
        print("Generating JSON coverage report...")
        _generate_json_from_lcov(coverage_filtered, coverage_report_dir)
        print(f"[OK] JSON coverage report generated at {coverage_report_dir}/coverage_summary.json")
        print(f"Generating HTML report to {coverage_report_dir}...")
        _generate_html_from_lcov(coverage_filtered, coverage_report_dir, source_root)
        print(f"[OK] HTML coverage report generated at {coverage_report_dir}/html/index.html")

    else:
        print(f"Warning: Unknown output format '{output_format}', defaulting to JSON")
        _generate_json_from_lcov(coverage_filtered, coverage_report_dir)


def main():
    """Main entry point for command-line usage."""
    parser = argparse.ArgumentParser(
        description="GCC Coverage Report Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Output Formats:
  json              Generate JSON coverage data only (default)
  html              Generate HTML report directly from coverage data
  html-and-json    Generate HTML report from existing JSON coverage data

Examples:
  # Generate JSON only (default)
  python gcc_coverage.py --build=build_ninja_python --filter=Library

  # Generate HTML report directly
  python gcc_coverage.py --build=build_ninja_python --filter=Library --output=html

  # Generate HTML from JSON
  python gcc_coverage.py --build=build_ninja_python --filter=Library --output=html-and-json

  # Verbose output
  python gcc_coverage.py --build=build_ninja_python --filter=Library --output=html --verbose
        """
    )

    parser.add_argument(
        "--build",
        required=True,
        help="Build directory path containing .gcda files"
    )
    parser.add_argument(
        "--filter",
        default="Library",
        help="Filter folder name (default: Library)"
    )
    parser.add_argument(
        "--output", "-o",
        choices=["json", "html", "html-and-json"],
        default="json",
        help="Output format: json (default), html, or html-and-json"
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose output for debugging"
    )
    parser.add_argument(
        "--exclude",
        action="append",
        help="Additional exclusion patterns (can be specified multiple times)"
    )

    args = parser.parse_args()

    try:
        # Resolve build directory
        build_dir = Path(args.build)
        if not build_dir.is_absolute():
            build_dir = Path.cwd() / build_dir
        build_dir = build_dir.resolve()

        if not build_dir.exists():
            print(f"Error: Build directory does not exist: {build_dir}", file=sys.stderr)
            sys.exit(1)

        # Resolve source directory
        source_dir = Path.cwd() / args.filter
        if not source_dir.exists():
            print(f"Error: Source directory does not exist: {source_dir}", file=sys.stderr)
            sys.exit(1)

        # Discover modules
        modules = [d.name for d in source_dir.iterdir()
                   if d.is_dir() and not d.name.startswith("_")]

        if not modules:
            print(f"Error: No modules found in {source_dir}", file=sys.stderr)
            sys.exit(1)

        if args.verbose:
            print(f"Build directory: {build_dir}")
            print(f"Source directory: {source_dir}")
            print(f"Modules: {', '.join(modules)}")
            print(f"Output format: {args.output}")

        # Generate coverage
        generate_lcov_coverage(
            build_dir=build_dir,
            modules=modules,
            exclude_patterns=args.exclude,
            verbose=args.verbose,
            output_format=args.output
        )

        print("\n[SUCCESS] Coverage generation completed.")
        sys.exit(0)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()