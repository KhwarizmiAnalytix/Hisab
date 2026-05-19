"""
Configuration Generation Helper Module

This module handles CMake configuration generation.
Extracted from setup.py for better modularity and maintainability.
"""

import os
import subprocess
import sys
from pathlib import Path

from helpers.cuda_env import augment_env_for_cuda_toolkit


def _find_libtorch() -> str | None:
    """Return the first libtorch install directory that contains TorchConfig.cmake, or None.

    Only active on macOS (darwin). Returns None immediately on other platforms.
    """
    if sys.platform != "darwin":
        return None

    candidates = []

    # 1. Explicit env var wins
    env_val = os.environ.get("LIBTORCH_DIR") or os.environ.get("Torch_DIR")
    if env_val:
        candidates.append(Path(env_val))

    # 2. Well-known home-directory installs (macOS paths)
    home = Path.home()
    candidates += [
        home / "libtorch",
        home / "local" / "libtorch",
        Path("/usr/local/libtorch"),
        Path("/opt/libtorch"),
        Path("/opt/homebrew/opt/libtorch"),   # Homebrew ARM
        Path("/usr/local/opt/libtorch"),       # Homebrew Intel
    ]

    for base in candidates:
        if (base / "share" / "cmake" / "Torch" / "TorchConfig.cmake").is_file():
            return str(base)
    return None


def configure_build(
    source_path: str,
    build_path: str,
    cmake_generator: str,
    cmake_cxx_compiler: str,
    cmake_c_compiler: str,
    cmake_flags: list[str],
    arg_cmake_verbose: str,
    shell_flag: bool,
    generator_toolset: str | None = None,
) -> int:
    """
    Configure the build system using CMake.

    Args:
        source_path: Path to source directory
        build_path: Path to build directory
        cmake_generator: CMake generator to use
        cmake_cxx_compiler: C++ compiler specification
        cmake_c_compiler: C compiler specification
        cmake_flags: Additional CMake flags
        arg_cmake_verbose: Verbosity level
        shell_flag: Whether to use shell execution
        generator_toolset: Optional VS toolset passed via -T (e.g. "v143")

    Returns:
        Exit code (0 for success, non-zero for failure)
    """
    try:
        build_folder = f"-B {build_path}"
        source_folder = f"-S {source_path}"

        cmake_cmd = [
            "cmake",
            source_folder,
            build_folder,
            "-G",
            cmake_generator,
            arg_cmake_verbose,
        ]

        if generator_toolset:
            cmake_cmd.extend(["-T", generator_toolset])

        if cmake_cxx_compiler:
            cmake_cmd.append(cmake_cxx_compiler)
        if cmake_c_compiler:
            cmake_cmd.append(cmake_c_compiler)

        # Add additional CMake flags
        cmake_cmd.extend(cmake_flags)

        # Auto-inject LibTorch path into CMAKE_PREFIX_PATH when found
        libtorch_dir = _find_libtorch()
        if libtorch_dir:
            existing = next(
                (f for f in cmake_cmd if f.startswith("-DCMAKE_PREFIX_PATH=")), None
            )
            if existing:
                cmake_cmd[cmake_cmd.index(existing)] = (
                    existing.rstrip(";") + ";" + libtorch_dir
                )
            else:
                cmake_cmd.append(f"-DCMAKE_PREFIX_PATH={libtorch_dir}")

        env = augment_env_for_cuda_toolkit()
        subprocess.check_call(
            cmake_cmd, stderr=subprocess.STDOUT, shell=shell_flag, env=env
        )
        return 0

    except subprocess.CalledProcessError:
        return 1
    except Exception:
        return 1


def handle_xcode_project_opening() -> None:
    """Open the generated Xcode project (non-interactive; always opens)."""
    try:
        xcodeproj_files = [f for f in os.listdir(".") if f.endswith(".xcodeproj")]
        if xcodeproj_files:
            project_file = xcodeproj_files[0]
            subprocess.run(["open", project_file], check=True)
    except Exception:
        pass
