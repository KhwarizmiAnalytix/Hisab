#!/usr/bin/env python3
"""
IWYU Configure Header Detector for Quarisma Project

This script analyzes C++ source files to detect usage of QUARISMA_HAS_* macros
and automatically suggests including "common/configure.h" when needed.

It works in conjunction with IWYU to provide enhanced analysis for Quarisma-specific
patterns and generates detailed logging of the analysis process.

Note: Feature flags use QUARISMA_HAS_* in C++ code (e.g., QUARISMA_HAS_CUDA)
      while CMake uses QUARISMA_ENABLE_* (e.g., QUARISMA_ENABLE_CUDA)
"""

import argparse
import logging
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import Any, Optional


class QuarismaConfigureDetector:
    """Detects files that need common/configure.h based on QUARISMA_HAS_* usage."""

    # Pattern to match QUARISMA_HAS_* macros in various forms
    QUARISMA_PATTERNS = [
        r"QUARISMA_NUMA_ENABLED",
        r"QUARISMA_VECTORIZED",
        r"#QUARISMA_COMPRESSION_TYPE_SNAPPY",
    ]

    # Pattern to detect if common/configure.h is already included
    CONFIGURE_INCLUDE_PATTERN = r'#include\s+[<"]common/configure\.h[>"]'

    # C++ file extensions to analyze
    CPP_EXTENSIONS = {".h", ".hxx", ".hpp", ".cpp", ".cpp", ".cc", ".c"}

    def __init__(self, log_file: Optional[str] = None):
        """Initialize the detector with optional logging."""
        self.log_file = log_file
        self.setup_logging()
        self.files_analyzed = 0
        self.files_needing_configure = 0
        self.files_already_have_configure = 0

    def setup_logging(self) -> None:
        """Setup logging configuration."""
        log_level = logging.INFO
        log_format = "%(asctime)s - %(levelname)s - %(message)s"

        if self.log_file:
            logging.basicConfig(
                level=log_level,
                format=log_format,
                handlers=[
                    logging.FileHandler(self.log_file, mode="a"),
                    logging.StreamHandler(sys.stdout),
                ],
            )
        else:
            logging.basicConfig(level=log_level, format=log_format)

        self.logger = logging.getLogger(__name__)

    def extract_quarisma_macros(self, content: str) -> set[str]:
        """Extract all QUARISMA_HAS_* macros found in the content."""
        macros = set()

        for pattern in self.QUARISMA_PATTERNS:
            matches = re.findall(pattern, content, re.MULTILINE)
            for match in matches:
                # Extract just the macro name from the match
                macro_match = re.search(r"QUARISMA_HAS_\w+", match)
                if macro_match:
                    macros.add(macro_match.group())

        return macros

    def has_configure_include(self, content: str) -> bool:
        """Check if the file already includes common/configure.h."""
        return bool(re.search(self.CONFIGURE_INCLUDE_PATTERN, content, re.MULTILINE))

    def find_include_insertion_point(self, lines: list[str]) -> int:
        """Find the best place to insert common/configure.h include."""
        # Look for the last system include or first project include
        last_system_include = -1
        first_project_include = -1

        for i, line in enumerate(lines):
            stripped = line.strip()
            if stripped.startswith("#include"):
                if "<" in stripped and ">" in stripped:
                    # System include
                    last_system_include = i
                elif '"' in stripped:
                    # Project include
                    if first_project_include == -1:
                        first_project_include = i

        # Insert after system includes but before project includes
        if last_system_include >= 0:
            return last_system_include + 1
        elif first_project_include >= 0:
            return first_project_include
        else:
            # No includes found, insert after pragma once or at the beginning
            for i, line in enumerate(lines):
                if "#pragma once" in line:
                    return i + 2  # Skip pragma once and empty line
            return 0

    def analyze_file(self, file_path: Path) -> dict[str, Any]:
        """Analyze a single file for QUARISMA_ENABLE_* usage."""
        result = {
            "file": str(file_path),
            "needs_configure": False,
            "has_configure": False,
            "macros_found": set(),
            "suggestion": None,
            "error": None,
        }

        try:
            with open(file_path, encoding="utf-8", errors="ignore") as f:
                content = f.read()
                lines = content.splitlines()

            # Extract QUARISMA macros
            macros = self.extract_quarisma_macros(content)
            result["macros_found"] = macros

            # Check if configure.h is already included
            has_configure = self.has_configure_include(content)
            result["has_configure"] = has_configure

            # Determine if configure.h is needed
            if macros and not has_configure:
                result["needs_configure"] = True
                insertion_point = self.find_include_insertion_point(lines)
                result["suggestion"] = {
                    "line": insertion_point,
                    "include": '',
                }

        except Exception as e:
            result["error"] = str(e)
            self.logger.error("Error analyzing %s: %s", file_path, e)

        return result

    def analyze_directory(
        self, directory: Path, recursive: bool = True
    ) -> list[dict[str, Any]]:
        """Analyze all C++ files in a directory."""
        results = []

        if recursive:
            pattern = "**/*"
        else:
            pattern = "*"

        for file_path in directory.glob(pattern):
            if file_path.is_file() and file_path.suffix in self.CPP_EXTENSIONS:
                # Skip third-party directories
                if any(
                    part in str(file_path).lower()
                    for part in ["thirdparty", "third_party", "3rdparty"]
                ):
                    continue

                result = self.analyze_file(file_path)
                results.append(result)
                self.files_analyzed += 1

                if result["needs_configure"]:
                    self.files_needing_configure += 1
                elif result["has_configure"]:
                    self.files_already_have_configure += 1

        return results

    def generate_report(self, results: list[dict[str, Any]]) -> str:
        """Generate a comprehensive analysis report."""
        report_lines = [
            "=" * 80,
            "Quarisma Configure Header Analysis Report",
            "=" * 80,
            f"Analysis completed at: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
            f"Total files analyzed: {self.files_analyzed}",
            f"Files needing common/configure.h: {self.files_needing_configure}",
            f"Files already including common/configure.h: {self.files_already_have_configure}",
            "",
        ]

        # Files needing configure.h
        if self.files_needing_configure > 0:
            report_lines.extend(
                [
                    "FILES NEEDING common/configure.h:",
                    "-" * 40,
                ]
            )

            for result in results:
                if result["needs_configure"]:
                    report_lines.append(f"File: {result['file']}")
                    report_lines.append(
                        f"  Macros found: {', '.join(sorted(result['macros_found']))}"
                    )
                    if result["suggestion"]:
                        report_lines.append(
                            f"  Suggested insertion at line {result['suggestion']['line']}: {result['suggestion']['include']}"
                        )
                    report_lines.append("")

        # Summary of all macros found
        all_macros = set()
        for result in results:
            all_macros.update(result["macros_found"])

        if all_macros:
            report_lines.extend(
                [
                    "ALL QUARISMA_ENABLE_* MACROS FOUND:",
                    "-" * 40,
                ]
            )
            for macro in sorted(all_macros):
                count = sum(1 for r in results if macro in r["macros_found"])
                report_lines.append(f"  {macro}: used in {count} files")
            report_lines.append("")

        report_lines.append("=" * 80)
        return "\n".join(report_lines)


def main() -> None:
    """Main entry point for the configure detector."""
    parser = argparse.ArgumentParser(
        description="Detect Quarisma files that need common/configure.h"
    )
    parser.add_argument("directory", type=Path, help="Directory to analyze")
    parser.add_argument(
        "--log-file", type=str, help="Log file path for detailed output"
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        default=True,
        help="Analyze directories recursively",
    )
    parser.add_argument(
        "--report-file", type=str, help="Output file for the analysis report"
    )

    args = parser.parse_args()

    if not args.directory.exists():
        print(f"Error: Directory {args.directory} does not exist")
        sys.exit(1)

    # Initialize detector
    detector = QuarismaConfigureDetector(log_file=args.log_file)

    # Analyze directory
    detector.logger.info("Starting analysis of %s", args.directory)
    results = detector.analyze_directory(args.directory, recursive=args.recursive)

    # Generate report
    report = detector.generate_report(results)

    # Output report
    if args.report_file:
        with open(args.report_file, "w") as f:
            f.write(report)
        detector.logger.info("Report written to %s", args.report_file)
    else:
        print(report)

    # Exit with appropriate code
    if detector.files_needing_configure > 0:
        detector.logger.warning(
            "%d files need common/configure.h", detector.files_needing_configure
        )
        sys.exit(1)
    else:
        detector.logger.info("All files have proper configure.h includes")
        sys.exit(0)


if __name__ == "__main__":
    main()
