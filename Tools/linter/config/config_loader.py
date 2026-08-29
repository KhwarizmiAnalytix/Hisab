#!/usr/bin/env python3
"""
Configuration loader for XSigma linters.

This module provides utilities to load and access the XSigma linter configuration
from the YAML configuration file. It handles cross-platform path resolution and
provides convenient access to configuration values.

Organization: XSigmaAnalyitix
Project: XSigma
"""

from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError:
    yaml = None  # type: ignore


def get_config_path() -> Path:
    """
    Get the path to the XSigma linter configuration file.
    
    Returns:
        Path: Absolute path to xsigma_linter_config.yaml
    """
    config_dir = Path(__file__).parent.absolute()
    config_file = config_dir / "xsigma_linter_config.yaml"
    return config_file


def get_repo_root() -> Path:
    """
    Get the repository root directory.
    
    Returns:
        Path: Absolute path to the repository root
    """
    # Config file is at Tools/linter/config/config_loader.py
    # So repo root is 4 levels up
    return Path(__file__).resolve().parents[3]


def load_config() -> dict[str, Any]:
    """
    Load the XSigma linter configuration from YAML file.
    
    Returns:
        dict: Configuration dictionary
        
    Raises:
        ImportError: If PyYAML is not installed
        FileNotFoundError: If configuration file is not found
        yaml.YAMLError: If YAML parsing fails
    """
    if yaml is None:
        raise ImportError(
            "PyYAML is required to load linter configuration. "
            "Install it with: pip install pyyaml"
        )
    
    config_path = get_config_path()
    
    if not config_path.exists():
        raise FileNotFoundError(
            f"Configuration file not found: {config_path}"
        )
    
    try:
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
        return config if config is not None else {}
    except yaml.YAMLError as e:
        raise yaml.YAMLError(
            f"Failed to parse configuration file {config_path}: {e}"
        ) from e


def resolve_path(path_str: str, relative_to_repo: bool = True) -> Path:
    """
    Resolve a path string to an absolute Path object.
    
    Args:
        path_str: Path string (can be relative or absolute)
        relative_to_repo: If True, treat relative paths as relative to repo root
        
    Returns:
        Path: Absolute path
    """
    path = Path(path_str)
    
    if path.is_absolute():
        return path
    
    if relative_to_repo:
        repo_root = get_repo_root()
        return repo_root / path
    
    return path.resolve()


def get_header_only_config() -> dict[str, Any]:
    """
    Get header-only API configuration.
    
    Returns:
        dict: Header-only configuration with 'apis_file' and 'test_globs'
    """
    config = load_config()
    return config.get("header_only", {})


def get_header_only_apis_file() -> Path:
    """
    Get the path to the header-only APIs file.
    
    Returns:
        Path: Absolute path to header_only_apis.txt
    """
    config = get_header_only_config()
    apis_file = config.get("apis_file", "Library/Core/header_only_apis.txt")
    return resolve_path(apis_file)


def get_header_only_test_globs() -> list[str]:
    """
    Get glob patterns for header-only test files.
    
    Returns:
        list: List of glob patterns
    """
    config = get_header_only_config()
    return config.get("test_globs", [
        "Library/Core/test/cpp/aoti_abi_check/*.cpp",
        "Tests/**/header_only_test.cpp",
    ])


def get_dynamo_config() -> dict[str, Any]:
    """
    Get dynamo registry configuration.
    
    Returns:
        dict: Dynamo configuration
    """
    config = load_config()
    return config.get("dynamo", {})


def get_graph_break_registry_path() -> Path:
    """
    Get the path to the graph break registry JSON file.
    
    Returns:
        Path: Absolute path to graph_break_registry.json
    """
    config = get_dynamo_config()
    registry_path = config.get(
        "graph_break_registry",
        "Library/Core/_dynamo/graph_break_registry.json"
    )
    return resolve_path(registry_path)


def get_ordered_set_import() -> str:
    """
    Get the import statement for OrderedSet.
    
    Returns:
        str: Import statement
    """
    config = load_config()
    ordered_set_config = config.get("ordered_set", {})
    return ordered_set_config.get(
        "import_statement",
        "from xsigma.utils._ordered_set import OrderedSet\n\n"
    )


def get_import_allowlist() -> list[str]:
    """
    Get the list of allowed third-party imports.
    
    Returns:
        list: List of allowed module names
    """
    config = load_config()
    return config.get("import_allowlist", [])


if __name__ == "__main__":
    # Simple test to verify configuration loading
    try:
        config = load_config()
        print("✓ Configuration loaded successfully")
        print(f"✓ Repo root: {get_repo_root()}")
        print(f"✓ Header-only APIs file: {get_header_only_apis_file()}")
        print(f"✓ Graph break registry: {get_graph_break_registry_path()}")
        print(f"✓ OrderedSet import: {get_ordered_set_import()!r}")
    except Exception as e:
        print(f"✗ Error loading configuration: {e}", file=sys.stderr)
        sys.exit(1)

