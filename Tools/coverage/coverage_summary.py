#!/usr/bin/env python3
"""JSON coverage summary generation for CI/CD integration.

Generates structured JSON files with per-file and global coverage metrics.
Supports multiple coverage metrics: line coverage, function coverage, and region coverage.
Compatible with standard CI/CD systems and coverage analysis tools.
"""

import json
from pathlib import Path
from typing import Dict, Set, Optional, List


class CoverageSummaryGenerator:
    """Generates JSON coverage summaries from coverage data."""

    def __init__(self):
        """Initialize the coverage summary generator."""
        self.summary = {
            "metadata": {
                "format_version": "2.0",
                "generator": "coverage_tool",
                "schema": "cobertura-compatible"
            },
            "global_metrics": {
                "total_files": 0,
                "files_with_coverage": 0,
                "total_lines": 0,
                "covered_lines": 0,
                "uncovered_lines": 0,
                "line_coverage_percent": 0.0,
                "total_functions": 0,
                "covered_functions": 0,
                "function_coverage_percent": 0.0,
                "total_regions": 0,
                "covered_regions": 0,
                "region_coverage_percent": 0.0,
            },
            "files": {}
        }

    def generate_summary(self, covered_lines: Dict[str, Set[int]],
                        uncovered_lines: Dict[str, Set[int]],
                        execution_counts: Optional[Dict] = None,
                        function_coverage: Optional[Dict] = None,
                        region_coverage: Optional[Dict] = None) -> dict:
        """Generate coverage summary from coverage data.

        Args:
            covered_lines: Dictionary mapping file paths to sets of covered line numbers.
            uncovered_lines: Dictionary mapping file paths to sets of uncovered line numbers.
            execution_counts: Optional dictionary with execution counts per line.
            function_coverage: Optional dict with per-file function coverage data.
            region_coverage: Optional dict with per-file region coverage data.

        Returns:
            Dictionary containing the coverage summary.
        """
        if execution_counts is None:
            execution_counts = {}
        if function_coverage is None:
            function_coverage = {}
        if region_coverage is None:
            region_coverage = {}

        total_lines = 0
        total_covered = 0
        total_uncovered = 0
        total_functions = 0
        total_covered_functions = 0
        total_regions = 0
        total_covered_regions = 0

        # Process each file
        for file_path in set(list(covered_lines.keys()) + list(uncovered_lines.keys())):
            covered = covered_lines.get(file_path, set())
            uncovered = uncovered_lines.get(file_path, set())

            file_total = len(covered) + len(uncovered)
            file_covered = len(covered)
            file_uncovered = len(uncovered)

            if file_total > 0:
                coverage_percent = (file_covered / file_total) * 100
            else:
                coverage_percent = 0.0

            file_metrics = {
                "total_lines": file_total,
                "covered_lines": file_covered,
                "uncovered_lines": file_uncovered,
                "line_coverage_percent": round(coverage_percent, 2)
            }

            # Add function coverage if available
            if file_path in function_coverage:
                func_data = function_coverage[file_path]
                file_metrics["total_functions"] = func_data.get("total", 0)
                file_metrics["covered_functions"] = func_data.get("covered", 0)
                if func_data.get("total", 0) > 0:
                    func_percent = (func_data.get("covered", 0) / func_data.get("total", 1)) * 100
                else:
                    func_percent = 0.0
                file_metrics["function_coverage_percent"] = round(func_percent, 2)
                total_functions += func_data.get("total", 0)
                total_covered_functions += func_data.get("covered", 0)

            # Add region coverage if available
            if file_path in region_coverage:
                region_data = region_coverage[file_path]
                file_metrics["total_regions"] = region_data.get("total", 0)
                file_metrics["covered_regions"] = region_data.get("covered", 0)
                if region_data.get("total", 0) > 0:
                    region_percent = (region_data.get("covered", 0) / region_data.get("total", 1)) * 100
                else:
                    region_percent = 0.0
                file_metrics["region_coverage_percent"] = round(region_percent, 2)
                total_regions += region_data.get("total", 0)
                total_covered_regions += region_data.get("covered", 0)

            self.summary["files"][file_path] = file_metrics

            total_lines += file_total
            total_covered += file_covered
            total_uncovered += file_uncovered

        # Calculate global metrics
        self.summary["global_metrics"]["total_files"] = len(self.summary["files"])
        self.summary["global_metrics"]["files_with_coverage"] = len(
            [f for f in self.summary["files"].values() if f["total_lines"] > 0]
        )
        self.summary["global_metrics"]["total_lines"] = total_lines
        self.summary["global_metrics"]["covered_lines"] = total_covered
        self.summary["global_metrics"]["uncovered_lines"] = total_uncovered

        if total_lines > 0:
            global_coverage = (total_covered / total_lines) * 100
        else:
            global_coverage = 0.0

        self.summary["global_metrics"]["line_coverage_percent"] = round(global_coverage, 2)

        # Set function coverage metrics
        self.summary["global_metrics"]["total_functions"] = total_functions
        self.summary["global_metrics"]["covered_functions"] = total_covered_functions
        if total_functions > 0:
            func_coverage_percent = (total_covered_functions / total_functions) * 100
        else:
            func_coverage_percent = 0.0
        self.summary["global_metrics"]["function_coverage_percent"] = round(func_coverage_percent, 2)

        # Set region coverage metrics
        self.summary["global_metrics"]["total_regions"] = total_regions
        self.summary["global_metrics"]["covered_regions"] = total_covered_regions
        if total_regions > 0:
            region_coverage_percent = (total_covered_regions / total_regions) * 100
        else:
            region_coverage_percent = 0.0
        self.summary["global_metrics"]["region_coverage_percent"] = round(region_coverage_percent, 2)

        return self.summary

    def save_to_file(self, output_path: Path) -> None:
        """Save the coverage summary to a JSON file.

        Args:
            output_path: Path where the JSON file should be saved.
        """
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(self.summary, f, indent=2)

        print(f"Coverage summary saved to: {output_path}")

