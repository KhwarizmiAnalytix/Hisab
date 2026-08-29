"""
XSigma Linter Configuration Package

This package provides configuration management for XSigma linters.
It centralizes hardcoded paths and settings used by various linters,
allowing for easier maintenance and cross-platform compatibility.

Organization: XSigmaAnalyitix
Project: XSigma
"""

from .config_loader import (
    get_config_path,
    get_repo_root,
    load_config,
    resolve_path,
    get_header_only_config,
    get_header_only_apis_file,
    get_header_only_test_globs,
    get_dynamo_config,
    get_graph_break_registry_path,
    get_ordered_set_import,
    get_import_allowlist,
)

__all__ = [
    "get_config_path",
    "get_repo_root",
    "load_config",
    "resolve_path",
    "get_header_only_config",
    "get_header_only_apis_file",
    "get_header_only_test_globs",
    "get_dynamo_config",
    "get_graph_break_registry_path",
    "get_ordered_set_import",
    "get_import_allowlist",
]

