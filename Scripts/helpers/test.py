"""
Test Execution Helper Module

This module handles test execution using ctest and valgrind.
Extracted from setup.py for better modularity and maintainability.
"""

import os
import platform
import subprocess

from helpers.cuda_env import augment_env_for_cuda_toolkit


def _get_compiler_bin_dir_from_cmake_cache() -> str | None:
    """
    Read CMAKE_CXX_COMPILER from CMakeCache.txt in the current directory and
    return its parent bin directory (e.g. C:/msys64/mingw64/bin).
    Returns None if the cache is absent or the entry is not found.
    """
    cache_path = os.path.join(os.getcwd(), "CMakeCache.txt")
    if not os.path.isfile(cache_path):
        return None
    try:
        with open(cache_path, encoding="utf-8", errors="ignore") as f:
            for line in f:
                if line.startswith("CMAKE_CXX_COMPILER:"):
                    compiler = line.split("=", 1)[-1].strip()
                    if compiler:
                        return os.path.dirname(compiler)
    except OSError:
        pass
    return None


def run_ctest(
    builder: str,
    build_enum: str,
    system: str,
    verbosity: str,
    shell_flag: bool,
    sanitizer_type: str = None,
    source_path: str = None,
) -> int:
    """
    Run tests using ctest.

    Args:
        builder: Build system (ninja, make, xcodebuild, cmake)
        build_enum: Build type (Release, Debug, RelWithDebInfo)
        system: Operating system (Linux, Darwin, Windows)
        verbosity: Verbosity flag
        shell_flag: Whether to use shell execution
        sanitizer_type: Type of sanitizer being used (address, thread, etc.)
        source_path: Path to source directory

    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    try:
        ctest_cmd = ["ctest"]

        if system == "Windows":
            if builder != "ninja":
                ctest_cmd.extend(["-C", build_enum])

        # Handle Xcode configuration
        if builder == "xcodebuild":
            ctest_cmd.extend(["-C", build_enum])

        if verbosity:
            ctest_cmd.append(verbosity)

        # Run tests in parallel when CTest has multiple tests (faster wall time).
        # try:
        #     n_jobs = os.cpu_count() or 4
        # except (TypeError, NotImplementedError):
        #     n_jobs = 4
        # n_jobs = max(1, min(n_jobs, 16))
        # ctest_cmd.extend(["-j", str(n_jobs)])

        # Set up sanitizer suppressions if using a sanitizer
        env = augment_env_for_cuda_toolkit()

        # On Windows with a MinGW/non-MSVC compiler, prepend the compiler's bin
        # directory to PATH so runtime DLLs (libwinpthread-1.dll, etc.) are found.
        if system == "Windows":
            compiler_bin = _get_compiler_bin_dir_from_cmake_cache()
            if compiler_bin and os.path.isdir(compiler_bin):
                current_path = env.get("PATH", "")
                if compiler_bin not in current_path.split(os.pathsep):
                    env["PATH"] = compiler_bin + os.pathsep + current_path

        if sanitizer_type and source_path:
            suppressions_file = os.path.join(
                source_path, "Scripts", "suppressions", f"{sanitizer_type}san_suppressions.txt"
            )
            if os.path.exists(suppressions_file):
                sanitizer_option = f"{sanitizer_type.upper()}SAN_OPTIONS"
                env[sanitizer_option] = (
                    f"suppressions={suppressions_file}:print_suppressions=1"
                )

        return subprocess.check_call(
            ctest_cmd, stderr=subprocess.STDOUT, shell=shell_flag, env=env
        )

    except subprocess.CalledProcessError:
        return 1
    except Exception:
        return 1


def run_valgrind_test(source_path: str, build_path: str, shell_flag: bool) -> int:
    """
    Run tests with Valgrind memory checking.

    Args:
        source_path: Path to source directory
        build_path: Path to build directory
        shell_flag: Whether to use shell execution

    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    script_full_path = os.path.join(source_path, "Scripts", "valgrind_ctest.sh")

    # Check if script exists
    if not os.path.exists(script_full_path):
        return 1

    # Make script executable
    try:
        os.chmod(script_full_path, 0o755)
    except Exception:
        pass

    # Check platform compatibility
    if platform.system() == "Darwin" and platform.machine() == "arm64":
        pass  # Valgrind doesn't support Apple Silicon

    try:
        return subprocess.call(
            [script_full_path, build_path],
            stdin=subprocess.PIPE,
            shell=shell_flag,
        )
    except Exception:
        return 1
