#!/usr/bin/env python3
"""MSVC-specific code coverage generation.

Handles coverage generation for MSVC compiler using OpenCppCoverage tool.
Generates both HTML and JSON coverage reports.
"""

import subprocess
import json
import argparse
import sys
from pathlib import Path
import logging

from common import (
    get_config,
    get_platform_config,
    find_opencppcoverage,
    discover_test_executables,
)

logger = logging.getLogger(__name__)


def _parse_cobertura_xml(xml_file: Path) -> dict:
    """Parse Cobertura XML coverage file to extract coverage data.

    Handles multiple Cobertura XML structures:
    - Standard: coverage > package > classes > class > lines > line
    - Alternative: coverage > package > classes > class (with line-rate attribute)
    - Direct: coverage > sources > source > package > class > lines > line

    Args:
        xml_file: Path to Cobertura XML file.

    Returns:
        Dictionary with coverage data including file-level statistics.
    """
    try:
        import xml.etree.ElementTree as ET

        coverage_data = {
            "line_coverage": {
                "total": 0,
                "covered": 0,
                "uncovered": 0,
                "percent": 0.0
            },
            "files": []
        }

        if not xml_file.exists():
            return coverage_data

        tree = ET.parse(xml_file)
        root = tree.getroot()

        # Extract overall coverage
        # In Cobertura format, the root element is <coverage> with attributes
        line_rate = root.get("line-rate", "0")
        try:
            coverage_data["line_coverage"]["percent"] = round(float(line_rate) * 100, 2)
        except (ValueError, TypeError):
            pass

        # Extract per-file coverage - try multiple possible structures
        files_found = set()  # Track files to avoid duplicates

        # Structure 1: package > classes > class (standard OpenCppCoverage format)
        for package in root.findall(".//package"):
            for source_file in package.findall("classes/class"):
                filename = source_file.get("filename", "unknown")
                if filename in files_found:
                    continue
                files_found.add(filename)

                line_rate = source_file.get("line-rate", "0")

                try:
                    file_coverage_percent = round(float(line_rate) * 100, 2)
                except (ValueError, TypeError):
                    file_coverage_percent = 0.0

                file_data = {
                    "file": filename,
                    "line_coverage": {
                        "total": 0,
                        "covered": 0,
                        "uncovered": 0,
                        "percent": file_coverage_percent
                    }
                }

                # Count lines
                for line in source_file.findall("lines/line"):
                    hits = line.get("hits", "0")
                    try:
                        hit_count = int(hits)
                        file_data["line_coverage"]["total"] += 1
                        if hit_count > 0:
                            file_data["line_coverage"]["covered"] += 1
                        else:
                            file_data["line_coverage"]["uncovered"] += 1
                    except (ValueError, TypeError):
                        pass

                coverage_data["files"].append(file_data)

        return coverage_data

    except Exception as e:
        logger.warning(f"Failed to parse Cobertura XML {xml_file}: {e}")
        return {
            "line_coverage": {
                "total": 0,
                "covered": 0,
                "uncovered": 0,
                "percent": 0.0
            },
            "files": []
        }



def _generate_detailed_html_reports(html_dir: Path, raw_dir: Path, source_root: str) -> bool:
    """Generate detailed line-by-line HTML reports from Cobertura XML.

    Extracts source code and coverage data to create detailed per-file HTML reports
    with line-by-line coverage highlighting, matching the Clang coverage report format.

    Args:
        html_dir: Directory where HTML reports will be saved.
        raw_dir: Directory containing Cobertura XML files.
        source_root: Root directory for source files.

    Returns:
        True if reports were generated successfully, False otherwise.
    """
    try:
        from html_report import HtmlGenerator
        import xml.etree.ElementTree as ET

        if not raw_dir or not raw_dir.exists():
            print("No Cobertura XML directory found, skipping detailed HTML generation")
            return False

        xml_files = list(raw_dir.glob("*.xml"))
        if not xml_files:
            print("No Cobertura XML files found, skipping detailed HTML generation")
            return False

        # Parse all XML files to extract file-level coverage data
        covered_lines: dict[str, set] = {}
        uncovered_lines: dict[str, set] = {}
        execution_counts: dict[str, dict] = {}

        print(f"Parsing {len(xml_files)} Cobertura XML file(s) for detailed coverage...")

        for xml_file in xml_files:
            print(f"  Processing {xml_file.name}...")
            try:
                tree = ET.parse(xml_file)
                root = tree.getroot()

                # Extract file coverage data from Cobertura XML
                for package in root.findall('.//package'):
                    for cls in package.findall('classes/class'):
                        filename = cls.get('filename', '')
                        if not filename:
                            continue

                        # Normalize path - convert to absolute path
                        norm_path = filename.replace('/', '\\')

                        # If filename is relative, make it absolute
                        drive_letters = (
                            'C:', 'D:', 'E:', 'F:', 'G:', 'H:', 'I:', 'J:',
                            'K:', 'L:', 'M:', 'N:', 'O:', 'P:', 'Q:', 'R:',
                            'S:', 'T:', 'U:', 'V:', 'W:', 'X:', 'Y:', 'Z:'
                        )
                        if not norm_path.startswith(drive_letters):
                            # Relative path - prepend C: drive
                            norm_path = 'C:\\' + norm_path

                        # Filter to only include source files from the source_root
                        if source_root:
                            norm_src = source_root.replace('/', '\\').lower()
                            if norm_src not in norm_path.lower():
                                continue

                        # Extract line coverage data
                        covered = set()
                        uncovered = set()
                        counts = {}

                        for line in cls.findall('lines/line'):
                            try:
                                line_num = int(line.get('number', 0))
                                if line_num == 0:
                                    continue

                                hits = int(line.get('hits', 0))
                                counts[line_num] = hits

                                if hits > 0:
                                    covered.add(line_num)
                                else:
                                    uncovered.add(line_num)
                            except (ValueError, TypeError):
                                continue

                        if covered or uncovered:
                            # Verify the file exists before adding to coverage data
                            if Path(norm_path).exists():
                                covered_lines[norm_path] = covered
                                uncovered_lines[norm_path] = uncovered
                                execution_counts[norm_path] = counts
                            else:
                                # Try to find the file in the source_root
                                if source_root:
                                    # Extract just the filename and relative path
                                    parts = norm_path.split('\\')
                                    # Find the part that starts with 'Library'
                                    try:
                                        lib_idx = next(i for i, p in enumerate(parts) if p.lower() == 'library')
                                        rel_path = '\\'.join(parts[lib_idx:])
                                        full_path = Path(source_root).parent / rel_path
                                        if full_path.exists():
                                            covered_lines[str(full_path)] = covered
                                            uncovered_lines[str(full_path)] = uncovered
                                            execution_counts[str(full_path)] = counts
                                    except (StopIteration, IndexError):
                                        pass

                print(f"    Found {len(covered_lines)} file(s) with coverage data")

            except Exception as e:
                print(f"  Error parsing {xml_file.name}: {e}")
                import traceback
                traceback.print_exc()
                continue

        if not covered_lines and not uncovered_lines:
            print("Warning: No coverage data extracted from Cobertura XML")
            return False

        # Generate HTML reports using HtmlGenerator
        try:
            print(f"Generating detailed HTML reports for {len(covered_lines)} file(s)...")
            generator = HtmlGenerator(html_dir, source_root, preserve_hierarchy=True)
            generator.generate_report(covered_lines, uncovered_lines, execution_counts)
            print(f"Generated detailed HTML reports in: {html_dir}")
        except Exception as e:
            print(f"Error generating HTML reports: {e}")
            raise RuntimeError(f"Failed to generate detailed HTML reports: {e}") from e
        return True

    except Exception as e:
        print(f"Error generating detailed HTML reports: {e}")
        import traceback
        traceback.print_exc()
        return False


def _generate_json_summary(html_dir: Path, output_dir: Path, raw_dir: Path = None) -> dict:
    """Generate JSON coverage summary from OpenCppCoverage output.

    Parses OpenCppCoverage Cobertura XML and HTML reports to extract coverage data.

    Args:
        html_dir: Directory containing OpenCppCoverage HTML reports.
        output_dir: Directory where JSON report will be saved.
        raw_dir: Optional directory containing Cobertura XML files.

    Returns:
        Dictionary containing the coverage summary.
    """
    import re

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
            }
        },
        "files": []
    }

    # Try to parse Cobertura XML files first (more detailed)
    if raw_dir and raw_dir.exists():
        xml_files = list(raw_dir.glob("*.xml"))
        if xml_files:
            print(f"Found {len(xml_files)} Cobertura XML file(s), parsing for detailed coverage...")
            for xml_file in xml_files:
                xml_data = _parse_cobertura_xml(xml_file)
                if xml_data["files"]:
                    # Merge files, avoiding duplicates
                    existing_files = {f["file"] for f in summary["files"]}
                    for file_info in xml_data["files"]:
                        if file_info["file"] not in existing_files:
                            summary["files"].append(file_info)
                            existing_files.add(file_info["file"])
                    # Update overall coverage from XML
                    if xml_data["line_coverage"]["percent"] > 0:
                        summary["summary"]["line_coverage"]["percent"] = xml_data["line_coverage"]["percent"]

                print(f"  Parsed {xml_file.name}: {len(xml_data['files'])} files found")

    # If no XML data, fall back to parsing HTML
    if not summary["files"]:
        print("No Cobertura XML found, parsing HTML for coverage data...")

        # Parse HTML files to extract coverage data
        # OpenCppCoverage generates index.html with coverage summary
        index_file = html_dir / "index.html"
        if not index_file.exists():
            print(f"Warning: OpenCppCoverage index.html not found at {index_file}")
            return summary

        # Extract coverage data from HTML
        with open(index_file, encoding='utf-8', errors='ignore') as f:
            content = f.read()

            # Look for coverage percentage in HTML
            # OpenCppCoverage format: look for coverage metrics
            # Pattern to find coverage percentages
            coverage_pattern = r'(\d+(?:\.\d+)?)\s*%'
            matches = re.findall(coverage_pattern, content)

            if matches:
                # Try to extract overall coverage
                try:
                    overall_coverage = float(matches[0])
                    summary["summary"]["line_coverage"]["percent"] = round(overall_coverage, 2)
                except (ValueError, IndexError):
                    pass

        # Look for individual file reports
        html_files = list(html_dir.glob("*.html"))
        print(f"Found {len(html_files)} HTML file(s) in coverage report")
        for html_file in html_files:
            if html_file.name == "index.html":
                continue

            try:
                with open(html_file, encoding='utf-8', errors='ignore') as f:
                    file_content = f.read()

                    # Extract file path and coverage info
                    file_data = {
                        "file": html_file.stem,
                        "line_coverage": {
                            "total": 0,
                            "covered": 0,
                            "uncovered": 0,
                            "percent": 0.0
                        }
                    }

                    # Try to extract coverage percentage from file
                    coverage_match = re.search(r'(\d+(?:\.\d+)?)\s*%', file_content)
                    if coverage_match:
                        try:
                            file_coverage = float(coverage_match.group(1))
                            file_data["line_coverage"]["percent"] = round(file_coverage, 2)
                        except ValueError:
                            pass

                    summary["files"].append(file_data)

            except Exception as e:
                logger.warning(f"Failed to parse {html_file}: {e}")

    return summary


def _save_json_report(summary: dict, output_dir: Path) -> bool:
    """Save JSON coverage summary to file.

    Args:
        summary: Coverage summary dictionary.
        output_dir: Directory where JSON report will be saved.

    Returns:
        True if saved successfully, False otherwise.
    """
    try:
        json_file = output_dir / "coverage_summary.json"
        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(summary, f, indent=2)
        print(f"[OK] JSON coverage report saved to: {json_file}")
        return True
    except Exception as e:
        print(f"Warning: Failed to save JSON report: {e}")
        return False


def _verify_html_report(html_dir: Path) -> bool:
    """Verify that HTML coverage report exists.

    Args:
        html_dir: Directory containing HTML reports.

    Returns:
        True if HTML report exists, False otherwise.
    """
    if (html_dir / "index.html").exists():
        print(f"[OK] HTML coverage report available at: {html_dir}/index.html")
        return True
    else:
        print(f"Warning: HTML report not found at {html_dir}/index.html")
        return False


def _generate_json_from_html(html_dir: Path, output_dir: Path, raw_dir: Path = None,
                             output_format: str = "html-and-json") -> None:
    """Generate JSON coverage report from OpenCppCoverage output.

    Parses OpenCppCoverage Cobertura XML and HTML reports to generate both
    JSON summary and styled HTML reports using the standard html_report templates.

    Args:
        html_dir: Directory containing OpenCppCoverage HTML reports.
        output_dir: Directory where JSON report will be saved.
        raw_dir: Optional directory containing Cobertura XML files.
        output_format: Output format - 'json', 'html', or 'html-and-json'
    """
    try:
        # Generate JSON summary from coverage data
        summary = _generate_json_summary(html_dir, output_dir, raw_dir)

        # Generate output based on format
        if output_format == "json":
            # Save JSON only
            _save_json_report(summary, output_dir)

        elif output_format == "html":
            # HTML is already generated by _generate_detailed_html_reports
            # Just verify it exists
            _verify_html_report(html_dir)

        elif output_format == "html-and-json":
            # Save JSON and verify HTML
            _save_json_report(summary, output_dir)
            _verify_html_report(html_dir)

        else:
            print(f"Warning: Unknown output format '{output_format}', defaulting to html-and-json")
            # Fall back to html-and-json
            _save_json_report(summary, output_dir)
            _verify_html_report(html_dir)

    except Exception as e:
        print(f"Warning: Failed to generate JSON from HTML: {e}")


def generate_msvc_coverage(
    build_dir: Path,
    modules: list[str],
    source_folder: Path,
    exclude_patterns: list[str] = None,
    verbose: bool = False,
    output_format: str = "html-and-json"
) -> None:
    """Generate code coverage using opencppcoverage (for MSVC on Windows).

    Generates HTML reports in html/ folder and raw coverage data in raw/ folder.
    Uses coverage.toml / get_config() for all configurable parameters including exclude patterns.

    Args:
        build_dir: Path to build directory.
        modules: List of module names to analyze.
        source_folder: Path to source folder containing modules.
        exclude_patterns: List of patterns to exclude from coverage. If None, read from config.
        verbose: Enable verbose output for debugging. Default: False.
        output_format: Output format - 'json', 'html', or 'html-and-json'

    Raises:
        RuntimeError: If not on Windows or opencppcoverage not found.
    """
    build_dir = Path(build_dir)
    coverage_dir = build_dir / "coverage_report"
    html_dir = coverage_dir / "html"
    raw_dir = coverage_dir / "raw"

    # Create output directories
    html_dir.mkdir(parents=True, exist_ok=True)
    raw_dir.mkdir(parents=True, exist_ok=True)

    platform_cfg = get_platform_config()
    cfg = get_config()
    if exclude_patterns is None:
        excludes = cfg.get("exclude_patterns", [])
    else:
        excludes = exclude_patterns
    verify_timeout = cfg["msvc_verify_timeout"]
    coverage_timeout = cfg["msvc_coverage_timeout"]

    if verbose:
        print(f"[VERBOSE] Build directory: {build_dir}")
        print(f"[VERBOSE] Modules: {modules}")
        print(f"[VERBOSE] Exclusion patterns: {excludes}")
        print(f"[VERBOSE] Output format: {output_format}")

    if platform_cfg["os_name"] != "Windows":
        raise RuntimeError("MSVC coverage only supported on Windows")

    # Find OpenCppCoverage
    opencpp_path = find_opencppcoverage()
    if not opencpp_path:
        raise RuntimeError("OpenCppCoverage not found. Please install it.")

    print(f"OpenCppCoverage found at: {opencpp_path}")
    if verbose:
        print(f"[VERBOSE] OpenCppCoverage path: {opencpp_path}")

    # Discover test executables
    print("\nDiscovering test executables...")
    test_executables = discover_test_executables(build_dir)

    if not test_executables:
        print("Warning: No test executables found")
        print(f"Searched in: {build_dir}")
        print("Expected locations:")
        print(f"  - {build_dir / 'bin'}")
        print(f"  - {build_dir / 'bin/Debug'}")
        print(f"  - {build_dir / 'bin/Release'}")
        print(f"  - {build_dir / 'lib'}")
        print(f"  - {build_dir / 'tests'}")
        return

    print(f"Found {len(test_executables)} test executable(s):")
    for exe in test_executables:
        print(f"  - {exe.name} ({exe.stat().st_size} bytes)")
        if verbose:
            print(f"[VERBOSE]   Full path: {exe}")

    # Run coverage for each test executable
    print("\nRunning coverage analysis...")
    print(f"Analyzing coverage for: {source_folder}")
    print(f"OpenCppCoverage path: {opencpp_path}")

    failed_tests = []
    successful_tests = 0

    for test_exe in test_executables:
        test_name = test_exe.stem
        separator = "=" * 60
        print(f"\n{separator}")
        print(f"Running coverage for: {test_name}")
        print(f"Executable: {test_exe}")
        print(separator)

        # Build OpenCppCoverage command
        cov_cmd = [
            str(opencpp_path),
            "--optimized_build",  # suppress false-zero lines caused by inlining in Release builds
        ]

        # Add HTML export
        cov_cmd.append(f"--export_type=html:{html_dir}")

        # Add Cobertura XML export for structured coverage data
        xml_file = raw_dir / f"{test_name}.xml"
        cov_cmd.append(f"--export_type=cobertura:{xml_file}")

        # Add binary export for raw coverage data
        raw_file = raw_dir / f"{test_name}.cov"
        cov_cmd.append(f"--export_type=binary:{raw_file}")

        # Add source filter for specific folder (Windows path separators)
        # Use Path to normalize and avoid duplicates if source_folder already has wildcard
        source_path = Path(source_folder)
        windows_source_path = str(source_path).replace("/", "\\")

        # Add source directory - OpenCppCoverage will recursively include subdirectories
        cov_cmd.append(f"--sources={windows_source_path}")

        # Add exclusion patterns with Windows path separators
        if verbose:
            print(f"[VERBOSE] Applying {len(excludes)} exclusion patterns:")
            for exclude_pattern in excludes:
                print(f"[VERBOSE]   - {exclude_pattern}")

        for exclude_pattern in excludes:
            windows_pattern = exclude_pattern.replace("/", "\\")
            cov_cmd.append(f"--excluded_sources={windows_pattern}")

        # Add the test executable to run
        cov_cmd.append("--")
        cov_cmd.append(str(test_exe))

        print(f"Command: {' '.join(cov_cmd)}\n")

        if verbose:
            print(f"[VERBOSE] OpenCppCoverage command:")
            print(f"[VERBOSE]   {' '.join(cov_cmd)}")
            print(f"[VERBOSE] Output files:")
            print(f"[VERBOSE]   HTML: {html_dir}")
            print(f"[VERBOSE]   XML: {xml_file}")
            print(f"[VERBOSE]   Binary: {raw_file}")

        try:
            # First, verify the test executable runs without coverage
            print("Verifying test executable runs...")
            if verbose:
                print(f"[VERBOSE] Running test executable: {test_exe}")
            verify_result = subprocess.run(
                [str(test_exe)],
                cwd=str(test_exe.parent),
                capture_output=True,
                text=True,
                timeout=verify_timeout,
                check=False
            )

            if verify_result.returncode != 0:
                print(f"Warning: Test executable returned non-zero exit code: {verify_result.returncode}")
                if verify_result.stderr:
                    print(f"  Test stderr: {verify_result.stderr[:200]}")
                if verbose:
                    print(f"[VERBOSE] Test stdout: {verify_result.stdout[:200]}")
            else:
                print("Test executable runs successfully")

            # Now run with coverage
            if verbose:
                print(f"[VERBOSE] Running OpenCppCoverage for {test_name}...")
            result = subprocess.run(
                cov_cmd,
                # Use the test executable's directory as CWD so resources/DLLs resolve
                cwd=str(test_exe.parent),
                capture_output=True,
                text=True,
                check=False,
                timeout=coverage_timeout
            )

            # Print output for debugging
            if result.stdout:
                print(f"Coverage tool output:\n{result.stdout}")

            if result.returncode == 0:
                print(f"Coverage generated for: {test_name}")
                successful_tests += 1
                if verbose:
                    print(f"[VERBOSE] Successfully generated coverage for {test_name}")
            else:
                print(f"Coverage failed for: {test_name} (exit code: {result.returncode})")
                if result.stderr:
                    print(f"  Error: {result.stderr}")
                if verbose:
                    print(f"[VERBOSE] Coverage generation failed for {test_name}")
                    print(f"[VERBOSE] stderr: {result.stderr}")
                failed_tests.append(test_name)
        except subprocess.TimeoutExpired:
            print(f"Coverage timed out for: {test_name}")
            failed_tests.append(test_name)
        except Exception as e:
            print(f"Exception running coverage for {test_name}: {e}")
            failed_tests.append(test_name)

    # Verify output
    html_files = list(html_dir.glob("**/*.html"))
    raw_files = list(raw_dir.glob("*.cov"))

    if verbose:
        print(f"[VERBOSE] Coverage output verification:")
        print(f"[VERBOSE] HTML files found: {len(html_files)}")
        for html_file in html_files[:5]:  # Show first 5
            print(f"[VERBOSE]   - {html_file}")
        if len(html_files) > 5:
            print(f"[VERBOSE]   ... and {len(html_files) - 5} more")
        print(f"[VERBOSE] Raw coverage files found: {len(raw_files)}")
        for raw_file in raw_files:
            print(f"[VERBOSE]   - {raw_file}")

    separator = "=" * 60
    print(f"\n{separator}")
    print("Coverage Report Summary")
    print(separator)
    print(f"Tests processed: {successful_tests}/{len(test_executables)}")
    print(f"HTML files generated: {len(html_files)}")
    print(f"Raw coverage files: {len(raw_files)}")
    print(f"HTML report location: {html_dir}")
    print(f"Raw data location: {raw_dir}")

    if not html_files and not raw_files:
        print("\nWarning: No coverage output generated!")
        raise RuntimeError("Coverage generation produced no output")

    # Generate detailed line-by-line HTML pages from Cobertura XML (matches Clang styling)
    # This creates per-file reports with source code and line-by-line coverage highlighting
    if output_format in ["html", "html-and-json"]:
        print("\nGenerating detailed line-by-line HTML reports from Cobertura XML...")
        detailed_generated = _generate_detailed_html_reports(html_dir, raw_dir, str(source_folder))
        if not detailed_generated:
            print("Warning: Detailed HTML generation skipped or failed.")
    else:
        print("\nSkipping HTML generation (output format is json only)")

    # Generate JSON coverage report (for summary and styled index)
    # This will enhance or replace the index.html with styled version
    if output_format in ["json", "html-and-json"]:
        print(f"\nGenerating {output_format} coverage report...")
        _generate_json_from_html(html_dir, coverage_dir, raw_dir, output_format)
    else:
        print("\nSkipping JSON generation (output format is html only)")

    if failed_tests:
        print(f"\n{len(failed_tests)} test(s) had issues:")
        for test_name in failed_tests:
            print(f"  - {test_name}")
    else:
        print(f"\nAll {len(test_executables)} test(s) processed successfully!")
