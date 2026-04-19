#!/usr/bin/env python3
"""Common utilities for code coverage generation.

Provides shared functions used across multiple coverage modules to eliminate
code duplication and improve maintainability.
"""

import os
import subprocess
import platform
from pathlib import Path
from typing import List, Optional, Union
import logging

try:
    import tomllib
except ImportError:
    try:
        import tomli as tomllib  # type: ignore[no-redef]
    except ImportError:
        tomllib = None  # type: ignore[assignment]

logger = logging.getLogger(__name__)

# ============================================================================
# CONFIGURATION - Centralized hardcoded values
# ============================================================================

CONFIG = {
    # Default filter folder (subfolder within source containing modules)
    "filter": "Library",

    # Default source folder
    "source_folder": "Library",

    # Default output format
    "output_format": "html-and-json",

    # Name of the coverage output directory (relative to build_dir)
    "coverage_report_dir": "coverage_report",

    # File patterns to exclude from coverage reports
    "exclude_patterns": [
        "*ThirdParty*",
        "*Testing*",
        "/usr/*",
    ],

    # Regex patterns for LLVM to ignore
    "llvm_ignore_regex": [
        ".*Testing[/\\\\].*",
        ".*Serialization[/\\\\].*",
        ".*ThirdParty[/\\\\].*",
        ".*quarismasys[/\\\\].*",
    ],

    # Markers to detect project root
    "project_markers": [".git", ".gitignore", "pyproject.toml"],

    # Search directories for test executables (relative to build_dir)
    # Order matters: searched in this order to find tests
    "test_search_dirs": [
        "bin",
        "bin/Debug",
        "bin/Release",
        "lib",
        "tests",
    ],

    # Test executable name patterns
    "test_patterns": ["*Test*", "*test*", "*CxxTests*"],

    # Name template for a module's test executable ({module} replaced at runtime)
    "test_exe_pattern": "{module}CxxTests",

    # Name template for a module's .profraw file (Clang only)
    "profraw_pattern": "{module}CxxTests.profraw",

    # Relative-path template for the test working directory (Clang only)
    # {filter} and {module} are replaced at runtime
    "test_dir_template": "{filter}/{module}/Testing/Cxx",

    # lcov --ignore-errors values (GCC only)
    "lcov_ignore_errors": ["mismatch", "negative", "gcov"],

    # Seconds to wait for the test-executable verification run (MSVC only)
    "msvc_verify_timeout": 30,

    # Seconds to wait for each OpenCppCoverage run (MSVC only)
    "msvc_coverage_timeout": 120,

    # Explicit path to OpenCppCoverage.exe; empty = auto-detect (MSVC only)
    "opencppcoverage_path": "",

    # Force a specific compiler instead of auto-detecting.
    # Accepted values: "gcc" | "clang" | "msvc" | "" (empty = auto-detect)
    "compiler": "",
}

# ============================================================================
# END CONFIGURATION


# ============================================================================
# CONFIG FILE LOADER
# ============================================================================

_config_cache: Optional[dict] = None


def _flatten_toml_config(toml_data: dict) -> dict:
    """Flatten a nested coverage.toml dict to the flat CONFIG key format."""
    cov = toml_data.get("coverage", {})
    result: dict = {}

    # [coverage]
    for key in ("filter", "source_folder", "output_format", "coverage_report_dir", "compiler"):
        if key in cov:
            result[key] = cov[key]

    # [coverage.exclude]
    excl = cov.get("exclude", {})
    if "patterns" in excl:
        result["exclude_patterns"] = excl["patterns"]
    if "llvm_ignore_regex" in excl:
        result["llvm_ignore_regex"] = excl["llvm_ignore_regex"]

    # [coverage.project]
    proj = cov.get("project", {})
    if "markers" in proj:
        result["project_markers"] = proj["markers"]

    # [coverage.tests]
    tests = cov.get("tests", {})
    _tests_map = {
        "search_dirs": "test_search_dirs",
        "patterns": "test_patterns",
        "exe_pattern": "test_exe_pattern",
        "profraw_pattern": "profraw_pattern",
        "test_dir_template": "test_dir_template",
    }
    for toml_key, cfg_key in _tests_map.items():
        if toml_key in tests:
            result[cfg_key] = tests[toml_key]

    # [coverage.gcc]
    gcc = cov.get("gcc", {})
    if "lcov_ignore_errors" in gcc:
        result["lcov_ignore_errors"] = gcc["lcov_ignore_errors"]

    # [coverage.msvc]
    msvc = cov.get("msvc", {})
    _msvc_map = {
        "verify_timeout": "msvc_verify_timeout",
        "coverage_timeout": "msvc_coverage_timeout",
        "opencppcoverage_path": "opencppcoverage_path",
    }
    for toml_key, cfg_key in _msvc_map.items():
        if toml_key in msvc:
            result[cfg_key] = msvc[toml_key]

    return result


def load_config(config_path=None) -> dict:
    """Load configuration from coverage.toml merged with built-in defaults.

    Searches for coverage.toml in the script directory then the project root.
    Falls back to built-in CONFIG if the file is not found or cannot be parsed.

    Args:
        config_path: Explicit path to coverage.toml. If None, auto-detected.

    Returns:
        Configuration dictionary.
    """
    global _config_cache

    # Return cached result for repeated auto-detect calls
    if config_path is None and _config_cache is not None:
        return _config_cache

    if tomllib is None:
        logger.debug("No TOML parser available (needs Python 3.11+ or tomli); "
                     "using built-in defaults")
        return CONFIG

    # Locate the config file
    resolved_path: Optional[Path] = None
    if config_path is not None:
        resolved_path = Path(config_path)
    else:
        candidates = [Path(__file__).resolve().parent / "coverage.toml"]
        try:
            project_root = get_project_root()
            candidates.append(project_root / "coverage.toml")
            candidates.append(project_root / "Tools" / "coverage" / "coverage.toml")
        except Exception:
            pass
        resolved_path = next((p for p in candidates if p.exists()), None)

    if resolved_path is None or not resolved_path.exists():
        logger.debug("coverage.toml not found; using built-in defaults")
        return CONFIG

    try:
        with open(resolved_path, "rb") as f:
            toml_data = tomllib.load(f)
        overrides = _flatten_toml_config(toml_data)
        merged = {**CONFIG, **overrides}
        logger.debug("Loaded config from %s", resolved_path)
        if config_path is None:
            _config_cache = merged
        return merged
    except Exception as e:
        logger.warning("Failed to load %s: %s; using built-in defaults", resolved_path, e)
        return CONFIG


def get_config() -> dict:
    """Return the active configuration (auto-detected and cached).

    Equivalent to load_config() with no arguments. Use load_config(path) to
    load from an explicit path (e.g. from a --config CLI flag).
    """
    return load_config()


# ============================================================================
# PATTERN MERGING UTILITIES
# ============================================================================


def merge_exclude_patterns(
    user_patterns: Optional[List[str]] = None,
    include_defaults: bool = True
) -> List[str]:
    """Merge user-provided exclusion patterns with default patterns.

    Combines user-provided patterns with default exclusion patterns from CONFIG.
    Removes duplicates while preserving order (defaults first, then user patterns).

    Args:
        user_patterns: List of user-provided patterns to exclude. Can be None.
        include_defaults: Whether to include default patterns from CONFIG.
                         Default: True.

    Returns:
        List of merged exclusion patterns with duplicates removed.

    Example:
        >>> patterns = merge_exclude_patterns(["*Generated*", "*Benchmark*"])
        >>> # Returns: ["*ThirdParty*", "*Testing*", "/usr/*", "*Generated*", "*Benchmark*"]
    """
    merged = []
    seen = set()

    # Add default patterns first
    if include_defaults:
        for pattern in get_config().get("exclude_patterns", []):
            if pattern not in seen:
                merged.append(pattern)
                seen.add(pattern)

    # Add user patterns
    if user_patterns:
        for pattern in user_patterns:
            if pattern not in seen:
                merged.append(pattern)
                seen.add(pattern)

    return merged


def parse_exclude_patterns_string(patterns_str: str) -> List[str]:
    """Parse comma-separated exclusion patterns from a string.

    Splits a comma-separated string into individual patterns, stripping whitespace.
    Empty strings are filtered out.

    Args:
        patterns_str: Comma-separated pattern string (e.g., "Test,Benchmark,third_party").

    Returns:
        List of individual patterns.

    Example:
        >>> patterns = parse_exclude_patterns_string("Test, Benchmark, third_party")
        >>> # Returns: ["Test", "Benchmark", "third_party"]
    """
    if not patterns_str or not patterns_str.strip():
        return []

    patterns = [p.strip() for p in patterns_str.split(",")]
    return [p for p in patterns if p]  # Filter out empty strings


# ============================================================================


def get_platform_config() -> dict:
    """Infer platform-specific extensions and paths.

    Returns:
        Dictionary with platform-specific configuration including
        dll_extension, exe_extension, lib_folder, and os_name.

    Raises:
        RuntimeError: If platform is not supported.
    """
    system = platform.system()

    if system == "Windows":
        return {
            "dll_extension": ".dll",
            "exe_extension": ".exe",
            "lib_folder": "bin",
            "os_name": "Windows"
        }
    elif system == "Darwin":
        return {
            "dll_extension": ".dylib",
            "exe_extension": "",
            "lib_folder": "lib",
            "os_name": "macOS"
        }
    elif system == "Linux":
        return {
            "dll_extension": ".so",
            "exe_extension": "",
            "lib_folder": "lib",
            "os_name": "Linux"
        }
    else:
        raise RuntimeError(f"Unsupported platform: {system}")


def find_opencppcoverage() -> Optional[str]:
    """Find OpenCppCoverage executable in system PATH or environment variable.

    Searches for OpenCppCoverage in the following order:
    1. OPENCPPCOVERAGE_PATH environment variable
    2. System PATH
    3. Common installation directories

    Returns:
        Path to OpenCppCoverage.exe or None if not found.
    """
    # Check environment variable first
    env_path = os.environ.get("OPENCPPCOVERAGE_PATH")
    if env_path:
        env_path_obj = Path(env_path)
        if env_path_obj.exists():
            logger.info(f"Found OpenCppCoverage via OPENCPPCOVERAGE_PATH: {env_path}")
            return str(env_path_obj)
        else:
            logger.warning(f"OPENCPPCOVERAGE_PATH set but not found: {env_path}")

    # Try system PATH
    try:
        subprocess.run(
            ["OpenCppCoverage.exe", "--help"],
            capture_output=True,
            check=True,
            text=True
        )
        logger.info("Found OpenCppCoverage in system PATH")
        return "OpenCppCoverage.exe"
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass

    # Try common installation paths
    common_paths = [
        Path("C:\\Program Files\\OpenCppCoverage\\OpenCppCoverage.exe"),
        Path("C:\\Program Files (x86)\\OpenCppCoverage\\OpenCppCoverage.exe"),
    ]

    for path in common_paths:
        if path.exists():
            logger.info(f"Found OpenCppCoverage at: {path}")
            return str(path)

    logger.warning("OpenCppCoverage not found in PATH or common installation directories")
    return None


def discover_test_executables(build_dir: Path) -> List[Path]:
    """Discover all test executables in the build directory.

    Searches for executables matching common test patterns in configured search
    directories. Uses CONFIG["test_search_dirs"] and CONFIG["test_patterns"].

    Args:
        build_dir: Path to the build directory.

    Returns:
        List of Path objects pointing to test executables.
    """
    build_dir = Path(build_dir)
    test_executables = []
    config = get_platform_config()
    exe_ext = config["exe_extension"]

    # Use configured search directories and patterns
    cfg = get_config()
    search_dirs = [build_dir / d for d in cfg["test_search_dirs"]]
    test_patterns = cfg["test_patterns"]

    for search_dir in search_dirs:
        if not search_dir.exists():
            continue
        for pattern in test_patterns:
            for exe_file in search_dir.glob(f"{pattern}{exe_ext}"):
                if exe_file.is_file():
                    test_executables.append(exe_file)

    # Remove duplicates while preserving order
    seen = set()
    unique_executables = []
    for exe in test_executables:
        exe_str = str(exe.resolve())
        if exe_str not in seen:
            seen.add(exe_str)
            unique_executables.append(exe)

    return unique_executables


def find_library(build_dir: Path, lib_folder: str, module_name: str,
                 dll_extension: str) -> Optional[str]:
    """Find library file for a module.

    Searches for library files matching the module name pattern.

    Args:
        build_dir: Path to build directory.
        lib_folder: Folder name containing libraries (e.g., "lib", "bin").
        module_name: Name of the module to find.
        dll_extension: File extension for libraries (e.g., ".so", ".dll").

    Returns:
        Path to the library file or None if not found.
    """
    lib_path = build_dir / lib_folder
    if not lib_path.exists():
        return None

    patterns = [
        f"{module_name}{dll_extension}",
        f"lib{module_name}{dll_extension}",
        f"{module_name}*{dll_extension}",
    ]

    for pattern in patterns:
        matches = list(lib_path.glob(pattern))
        if matches:
            selected = matches[0]
            print(f"Found library for module '{module_name}': {selected.name} "
                  f"using {selected.name}")
            return str(selected)

    return None


def get_project_root() -> Path:
    """Find the project root directory by searching for common markers.

    Searches upward from the script directory for markers like .git,
    .gitignore, or pyproject.toml.

    Returns:
        Path to project root directory.
    """
    current = Path(__file__).resolve().parent
    for _ in range(10):  # Search up to 10 levels
        for marker in CONFIG["project_markers"]:
            if (current / marker).exists():
                return current
        current = current.parent

    return Path(__file__).resolve().parent


def resolve_build_dir(build_dir_arg: str,
                      project_root: Optional[Path] = None) -> Path:
    """Resolve build directory from argument (absolute or relative).

    Tries multiple resolution strategies:
    1. Absolute path
    2. Relative to current working directory
    3. Relative to script directory
    4. Relative to project root

    Args:
        build_dir_arg: Build directory argument (can be absolute or relative).
        project_root: Optional project root for relative resolution.

    Returns:
        Resolved Path to build directory.

    Raises:
        ValueError: If build directory cannot be resolved.
    """
    build_dir_arg = str(build_dir_arg)

    # Try as absolute path
    abs_path = Path(build_dir_arg).resolve()
    if abs_path.exists():
        return abs_path

    # Try relative to CWD
    cwd_relative = Path.cwd() / build_dir_arg
    if cwd_relative.exists():
        return cwd_relative

    # Try relative to script directory
    script_dir = Path(__file__).resolve().parent
    script_relative = script_dir / build_dir_arg
    if script_relative.exists():
        return script_relative

    # Try relative to project root
    if project_root is None:
        project_root = get_project_root()
    project_relative = project_root / build_dir_arg
    if project_relative.exists():
        return project_relative

    raise ValueError(
        f"Build directory '{build_dir_arg}' not found. Tried:\n"
        f"    - Absolute: {abs_path}\n"
        f"    - CWD relative: {cwd_relative}\n"
        f"    - Script relative: {script_relative}\n"
        f"    - Project relative: {project_relative}"
    )


def validate_build_structure(build_dir: Path, config: dict,
                             library_folder: str) -> dict:
    """Validate that the build directory has expected structure.

    Args:
        build_dir: Path to build directory.
        config: Configuration dictionary.
        library_folder: Name of library folder to check.

    Returns:
        Dictionary with 'valid' boolean and 'issues' list.
    """
    issues = []

    if not (build_dir / "CMakeCache.txt").exists():
        issues.append(f"CMakeCache.txt not found in {build_dir}")

    if not (build_dir / library_folder).exists():
        issues.append(f"Expected source folder not found: "
                      f"{build_dir / library_folder}")

    return {"valid": len(issues) == 0, "issues": issues}


def _detect_from_cmake_cache(cmake_cache: Path) -> Optional[str]:
    """Extract compiler from CMakeCache.txt.

    Tries QUARISMA_COMPILER_ID first, then falls back to CMAKE_CXX_COMPILER.

    Args:
        cmake_cache: Path to CMakeCache.txt.

    Returns:
        Compiler name, or None if not found / unrecognised.
    """
    try:
        quarisma_id: Optional[str] = None
        cxx_compiler: Optional[str] = None
        with open(cmake_cache, encoding='utf-8', errors='ignore') as f:
            for line in f:
                if '=' not in line:
                    continue
                if 'QUARISMA_COMPILER_ID' in line:
                    compiler_id = line.split('=', 1)[-1].strip().lower()
                    logger.info("QUARISMA_COMPILER_ID found: %s", compiler_id)
                    if compiler_id in ("gcc", "clang", "msvc", "intel"):
                        quarisma_id = compiler_id
                    else:
                        logger.warning("Unknown QUARISMA_COMPILER_ID: %s", compiler_id)
                elif 'CMAKE_CXX_COMPILER:' in line and cxx_compiler is None:
                    cxx_compiler = line.split('=', 1)[-1].strip().lower()

        if quarisma_id:
            return quarisma_id

        # Fall back to inferring from CMAKE_CXX_COMPILER path
        if cxx_compiler:
            logger.info("Inferring compiler from CMAKE_CXX_COMPILER: %s", cxx_compiler)
            if 'clang' in cxx_compiler:
                return 'clang'
            if 'g++' in cxx_compiler or 'gcc' in cxx_compiler:
                return 'gcc'
            if 'cl.exe' in cxx_compiler or 'cl' == os.path.basename(cxx_compiler):
                return 'msvc'
            if 'icpx' in cxx_compiler or 'icpc' in cxx_compiler:
                return 'intel'
    except Exception as e:
        logger.warning("Could not read CMakeCache.txt: %s", e)
    return None


def _detect_from_vs_project(search_dir: Path) -> Optional[str]:
    """Detect MSVC by looking for .sln or .vcxproj files.

    Searches the given directory and up to two levels of parent directories
    so that both in-source and out-of-source layouts are covered.

    Args:
        search_dir: Directory to start searching (usually the build dir).

    Returns:
        "msvc" if a Visual Studio project file is found, otherwise None.
    """
    dirs_to_search = [search_dir]
    # Also check immediate parent and grandparent — typical for out-of-source builds
    parent = search_dir.parent
    if parent != search_dir:
        dirs_to_search.append(parent)
        grandparent = parent.parent
        if grandparent != parent:
            dirs_to_search.append(grandparent)

    for d in dirs_to_search:
        if not d.is_dir():
            continue
        # Shallow glob — avoid walking entire trees
        if any(d.glob("*.sln")) or any(d.glob("*.vcxproj")):
            logger.info("Visual Studio project file found in %s → MSVC", d)
            return "msvc"
    return None


def _detect_from_vs_env() -> Optional[str]:
    """Detect MSVC from Visual Studio environment variables.

    These variables are set by the VS Developer Command Prompt and
    vcvarsall.bat / vsdevcmd.bat.

    Returns:
        "msvc" if a VS environment is active, otherwise None.
    """
    vs_env_vars = (
        "VCINSTALLDIR",       # set by vcvarsall.bat
        "VSCMD_ARG_TGT_ARCH", # set by vsdevcmd.bat
        "VSAPPIDNAME",        # Visual Studio app identity
        "VisualStudioVersion",# set by VS build environment
    )
    for var in vs_env_vars:
        if os.environ.get(var):
            logger.info("VS environment variable %s found → MSVC", var)
            return "msvc"
    return None


def _detect_cl_in_path() -> Optional[str]:
    """Detect MSVC by checking whether cl.exe is available in PATH.

    Returns:
        "msvc" if cl.exe is found, otherwise None.
    """
    import shutil
    if shutil.which("cl.exe") or shutil.which("cl"):
        logger.info("cl.exe found in PATH → MSVC")
        return "msvc"
    return None


def detect_compiler(build_dir: Union[Path, str]) -> str:
    """Detect the compiler used in the build directory.

    Detection order:
    1. ``CMakeCache.txt`` — reads ``QUARISMA_COMPILER_ID`` (CMake builds).
    2. ``.sln`` / ``.vcxproj`` files — indicates a Visual Studio / MSVC build.
    3. Visual Studio environment variables (``VCINSTALLDIR`` etc.).
    4. ``cl.exe`` present in ``PATH``.

    For non-CMake Visual Studio solutions, steps 2–4 provide automatic
    detection. You can also bypass detection entirely with the ``--compiler``
    CLI flag or the ``compiler`` key in ``coverage.toml``.

    Args:
        build_dir: Path to the build (or solution output) directory.

    Returns:
        Compiler name: ``"gcc"``, ``"clang"``, ``"msvc"``, or ``"intel"``.

    Raises:
        RuntimeError: If the compiler cannot be determined.
    """
    build_dir = Path(build_dir)

    # 1. CMakeCache.txt (CMake builds)
    cmake_cache = build_dir / "CMakeCache.txt"
    if cmake_cache.exists():
        result = _detect_from_cmake_cache(cmake_cache)
        if result:
            return result
        # CMakeCache exists but QUARISMA_COMPILER_ID is absent — fall through
        logger.warning(
            "CMakeCache.txt found but QUARISMA_COMPILER_ID is missing; "
            "trying Visual Studio detection"
        )
    else:
        logger.info("No CMakeCache.txt in %s; trying Visual Studio detection", build_dir)

    # 2. .sln / .vcxproj files
    result = _detect_from_vs_project(build_dir)
    if result:
        return result

    # 3. Visual Studio environment variables
    result = _detect_from_vs_env()
    if result:
        return result

    # 4. cl.exe in PATH
    result = _detect_cl_in_path()
    if result:
        return result

    raise RuntimeError(
        f"Could not detect compiler for build directory: {build_dir}\n"
        "\n"
        "Tried:\n"
        "  1. CMakeCache.txt  (QUARISMA_COMPILER_ID)\n"
        "  2. .sln / .vcxproj files in or near the build directory\n"
        "  3. Visual Studio environment variables (VCINSTALLDIR, etc.)\n"
        "  4. cl.exe in PATH\n"
        "\n"
        "To fix this, pass the compiler explicitly:\n"
        "  python run_coverage.py --build=<path> --compiler=msvc\n"
        "Or set it in coverage.toml:\n"
        "  [coverage]\n"
        "  compiler = \"msvc\""
    )
