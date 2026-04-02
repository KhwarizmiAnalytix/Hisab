#!/usr/bin/env python3
"""Quarisma Bazel Build Configuration Script.

This script provides a simplified interface for building Quarisma with Bazel,
mirroring the functionality of setup.py but using Bazel instead of CMake.

Usage:
    python Scripts/setup_bazel.py config.build.test.release.avx2
    python setup_bazel.py config.build.release.test.vv.enzyme   # from repo root (wrapper)
    python setup_bazel.py build.test.debug
    python setup_bazel.py test
    python setup_bazel.py config.build.release.test.cxx20
"""

import argparse
import json
import os
import platform
import re
import subprocess
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import colorama
from colorama import Fore, Style

# Initialize colorama for cross-platform colored output
colorama.init()


def print_status(message: str, status: str = "INFO"):
    """Print colored status messages."""
    colors = {
        "INFO": Fore.CYAN,
        "SUCCESS": Fore.GREEN,
        "WARNING": Fore.YELLOW,
        "ERROR": Fore.RED,
    }
    color = colors.get(status, Fore.WHITE)
    print(f"{color}[{status}]{Style.RESET_ALL} {message}")


def check_bazel_installed() -> bool:
    """Check if Bazel or Bazelisk is installed."""
    for cmd in ["bazelisk", "bazel"]:
        try:
            result = subprocess.run(
                [cmd, "version"],
                capture_output=True,
                text=True,
                check=False,
                timeout=10,  # 10 second timeout to prevent hanging
            )
            if result.returncode == 0:
                print_status(f"Found {cmd}: {result.stdout.split()[0]}", "INFO")
                return True
        except FileNotFoundError:
            continue
        except subprocess.TimeoutExpired:
            print_status(f"Timeout checking {cmd} version (command took too long)", "WARNING")
            continue
    return False


def get_bazel_command() -> str:
    """Get the appropriate Bazel command (bazelisk or bazel)."""
    for cmd in ["bazelisk", "bazel"]:
        try:
            subprocess.run(
                [cmd, "version"],
                capture_output=True,
                check=True,
                timeout=10,  # 10 second timeout to prevent hanging
            )
            return cmd
        except (FileNotFoundError, subprocess.CalledProcessError):
            continue
        except subprocess.TimeoutExpired:
            print_status(f"Timeout checking {cmd} version (command took too long)", "WARNING")
            continue
    raise RuntimeError("Neither bazel nor bazelisk found in PATH")


class BazelConfiguration:
    """Manages Bazel build configuration and execution."""

    def __init__(self, args: list[str]) -> None:
        """Initialize configuration from command-line arguments."""
        self.args = args
        self.build_type = "debug"  # Default build type
        self.vectorization: Optional[str] = None
        self.cxx_standard: Optional[str] = None
        self.configs: List[str] = []
        self.targets: List[str] = ["//..."]  # Default: build everything
        self.run_tests = False
        self.run_build = False
        self.run_clean = False
        self.run_config = False
        self.timing_data: Dict[str, float] = {}
        # Verbose test output (--test_output=all) when token "vv" is present
        self.verbose_tests = False

        # Default backends (matching CMake defaults)
        self.logging_backend = "loguru"  # Default: LOGURU (matches CMake)
        self.profiler_backend = "kineto"  # Default: KINETO (matches CMake)

        # Compiler and build tool configuration
        self.compiler: Optional[str] = None  # Compiler type: "clang", "gcc", "msvc", etc.
        self.build_tool: Optional[str] = None  # Build tool: "ninja", "xcode", "msvc", etc.
        self.visual_studio_version: Optional[str] = None  # VS version: "vs17", "vs19", "vs22", "vs26"
        self.system = platform.system()

        self._parse_arguments()
        self._apply_defaults()

    def _is_visual_studio_arg(self, arg: str) -> bool:
        """Check if argument matches Visual Studio version pattern (.vsXX.)."""
        # Match patterns like .vs26., .vs22., .vs19., .vs17.
        return arg.lower() in ["vs17", "vs19", "vs22", "vs26"]

    def _is_xcode_arg(self, arg: str) -> bool:
        """Check if argument is xcode."""
        return arg.lower() == "xcode"

    def _is_ninja_arg(self, arg: str) -> bool:
        """Check if argument is ninja."""
        return arg.lower() == "ninja"

    def _is_clang_compiler(self, arg: str) -> bool:
        """Check if argument is a Clang compiler specification."""
        return "clang" in arg.lower() and arg.lower() not in ["clang-cl", "clangtidy"]

    def _is_gcc_compiler(self, arg: str) -> bool:
        """Check if argument is a GCC compiler specification."""
        return ("gcc" in arg.lower() or "g++" in arg.lower()) and arg.lower() not in ["cppcheck"]

    def _parse_arguments(self) -> None:
        """Parse command-line arguments to extract build configuration."""
        for arg in self.args:
            arg_lower = arg.lower()

            # Compiler and build tool detection
            if self._is_visual_studio_arg(arg):
                self.visual_studio_version = arg
                self.build_tool = "msvc"
                self.compiler = "msvc"
            elif self._is_xcode_arg(arg):
                self.build_tool = "xcode"
                self.compiler = "clang"
            elif self._is_ninja_arg(arg):
                self.build_tool = "ninja"
            elif self._is_clang_compiler(arg):
                self.compiler = "clang"
            elif self._is_gcc_compiler(arg):
                self.compiler = "gcc"

            # Build type
            elif arg_lower in ["debug", "release", "relwithdebinfo"]:
                self.build_type = arg_lower
                self.configs.append(arg_lower)

            # C++ Standard
            elif arg_lower in ["cxx17", "cxx20", "cxx23"]:
                self.cxx_standard = arg_lower

            # Vectorization
            elif arg_lower in ["sse", "avx", "avx2", "avx512"]:
                self.vectorization = arg_lower
                self.configs.append(arg_lower)

            # LTO
            elif arg_lower == "lto":
                self.configs.append("lto")

            # Optional features
            elif arg_lower in ["mimalloc", "magic_enum", "tbb", "mkl", "openmp", "cuda", "hip"]:
                self.configs.append(arg_lower)

            # Enzyme AD (matches .bazelrc build:enzyme)
            elif arg_lower == "enzyme":
                self.configs.append("enzyme")

            # Verbose test log output (maps to --test_output=all)
            elif arg_lower == "vv":
                self.verbose_tests = True

            # Logging backends (with logging_ prefix)
            elif arg_lower.startswith("logging_"):
                backend = arg_lower[8:]  # Remove "logging_" prefix
                if backend in ["glog", "loguru", "native"]:
                    self.logging_backend = backend
                    self.configs.append(arg_lower)

            # Profiler backends (with profiler_ prefix)
            elif arg_lower.startswith("profiler_"):
                backend = arg_lower[9:]  # Remove "profiler_" prefix
                if backend in ["kineto", "itt", "native"]:
                    self.profiler_backend = backend
                    # Map to Bazel config names
                    if backend == "kineto":
                        self.configs.append("kineto")
                    elif backend == "itt":
                        self.configs.append("itt")
                    elif backend == "native":
                        self.configs.append("native_profiler")

            # Sanitizers (with sanitizer_ prefix)
            elif arg_lower.startswith("sanitizer_"):
                sanitizer = arg_lower[10:]  # Remove "sanitizer_" prefix
                if sanitizer in ["asan", "tsan", "ubsan", "msan"]:
                    self.configs.append(sanitizer)

            # Actions
            elif arg_lower == "build":
                self.run_build = True
            elif arg_lower == "test":
                self.run_tests = True
            elif arg_lower == "clean":
                self.run_clean = True
            elif arg_lower == "config":
                self.run_config = True

    def _apply_defaults(self) -> None:
        """Apply default compiler and build tool settings.

        Default Behavior (All Platforms):
        - If no Visual Studio or Xcode is detected/specified
        - If the compiler or build tool is not explicitly defined
        - Then default to using Ninja + Clang on all platforms

        Windows-Specific Behavior:
        - If a Visual Studio version is specified (.vs26., .vs22., .vs19., .vs17.)
        - Then build using the corresponding Visual Studio version
        """
        # If Visual Studio is specified on Windows, use it
        if self.visual_studio_version and self.system == "Windows":
            print_status(
                f"Using Visual Studio {self.visual_studio_version.upper()}",
                "INFO"
            )
            return

        # If Xcode is specified on macOS, use it
        if self.build_tool == "xcode" and self.system == "Darwin":
            print_status("Using Xcode generator", "INFO")
            return

        # Default to Ninja + Clang on all platforms
        if not self.build_tool:
            self.build_tool = "ninja"
        if not self.compiler:
            self.compiler = "clang"

        print_status(
            f"Using default build configuration: {self.build_tool.upper()} + {self.compiler.upper()}",
            "INFO"
        )

    def build_bazel_command(self, action: str) -> list[str]:
        """Build the Bazel command with all configurations."""
        bazel_cmd = get_bazel_command()
        cmd = [bazel_cmd, action]

        # Add server management flags to prevent hanging
        # Use --noserver to disable Bazel server (prevents deadlocks on Windows)

        # Add compiler-specific configuration
        if self.compiler == "clang":
            cmd.append("--config=clang")
        elif self.compiler == "gcc":
            cmd.append("--config=gcc")
        elif self.compiler == "msvc":
            cmd.append("--config=msvc")

        # Add build tool specific configuration
        if self.build_tool == "xcode":
            cmd.append("--config=xcode")

        # Add default logging backend if not explicitly set
        if not any(c.startswith("logging_") for c in self.configs):
            self.configs.append(f"logging_{self.logging_backend}")

        # Add default profiler backend if not explicitly set
        profiler_configs = ["kineto", "itt", "native_profiler"]
        if not any(c in profiler_configs for c in self.configs):
            if self.profiler_backend == "kineto":
                self.configs.append("kineto")
            elif self.profiler_backend == "itt":
                self.configs.append("itt")
            elif self.profiler_backend == "native":
                self.configs.append("native_profiler")

        # Add all config flags
        for config in self.configs:
            cmd.append(f"--config={config}")

        # Add C++ standard if specified
        if self.cxx_standard:
            if self.cxx_standard == "cxx17":
                cmd.append("--cxxopt=-std=c++17")
            elif self.cxx_standard == "cxx20":
                cmd.append("--cxxopt=-std=c++20")
            elif self.cxx_standard == "cxx23":
                cmd.append("--cxxopt=-std=c++23")

        # Add targets
        cmd.extend(self.targets)

        return cmd

    def print_configuration_summary(self) -> None:
        """Print a summary of the build configuration."""
        print("\n" + "=" * 80)
        print("QUARISMA BAZEL BUILD CONFIGURATION SUMMARY")
        print("=" * 80)

        # Compiler and build tool
        print(f"\n{Fore.CYAN}Compiler & Build Tool:{Style.RESET_ALL}")
        print(f"  Platform:          {self.system}")
        print(f"  Build Tool:        {self.build_tool.upper() if self.build_tool else 'NINJA (default)'}")
        print(f"  Compiler:          {self.compiler.upper() if self.compiler else 'CLANG (default)'}")
        if self.visual_studio_version:
            print(f"  Visual Studio:     {self.visual_studio_version.upper()}")

        # Build type
        print(f"\n{Fore.CYAN}Build Configuration:{Style.RESET_ALL}")
        print(f"  Build Type:        {self.build_type.upper()}")

        # C++ Standard
        if self.cxx_standard:
            print(f"  C++ Standard:      {self.cxx_standard.upper()}")
        else:
            print(f"  C++ Standard:      C++17 (default)")

        # Vectorization
        if self.vectorization:
            print(f"  Vectorization:     {self.vectorization.upper()}")
        else:
            print(f"  Vectorization:     None")

        # Feature flags
        print(f"\n{Fore.CYAN}Feature Flags:{Style.RESET_ALL}")
        features = {
            "mimalloc": "QUARISMA_ENABLE_MIMALLOC",
            "magic_enum": "QUARISMA_ENABLE_MAGIC_ENUM",
            "tbb": "QUARISMA_ENABLE_TBB",
            "openmp": "QUARISMA_ENABLE_OPENMP",
            "cuda": "QUARISMA_ENABLE_CUDA",
            "hip": "QUARISMA_ENABLE_HIP",
            "lto": "QUARISMA_ENABLE_LTO",
            "enzyme": "QUARISMA_ENABLE_ENZYME",
        }

        for feature, flag in features.items():
            status = "ON" if feature in self.configs else "OFF"
            color = Fore.GREEN if status == "ON" else Fore.RED
            print(f"  {flag:30} {color}{status}{Style.RESET_ALL}")

        # Logging backend
        print(f"\n{Fore.CYAN}Logging Backend:{Style.RESET_ALL}")
        print(f"  Backend:           {self.logging_backend.upper()}")

        # Profiler backend
        print(f"\n{Fore.CYAN}Profiler Backend:{Style.RESET_ALL}")
        print(f"  Backend:           {self.profiler_backend.upper()}")

        # Sanitizers
        sanitizers = [c for c in self.configs if c in ["asan", "tsan", "ubsan", "msan"]]
        print(f"\n{Fore.CYAN}Sanitizers:{Style.RESET_ALL}")
        if sanitizers:
            for sanitizer in sanitizers:
                print(f"  {sanitizer.upper():30} ON")
        else:
            print(f"  {'None':30} (disabled)")

        print("\n" + "=" * 80 + "\n")

    def config(self) -> None:
        """Print configuration summary without building."""
        if not self.run_config:
            return

        self.print_configuration_summary()

    def clean(self) -> None:
        """Clean Bazel build artifacts."""
        if not self.run_clean:
            return

        print_status("Cleaning Bazel build artifacts...", "INFO")
        bazel_cmd = get_bazel_command()

        try:
            start_time = time.time()
            subprocess.run(
                [bazel_cmd, "clean", "--expunge"],
                check=True,
                timeout=300,  # 5 minute timeout for clean operation
            )
            elapsed = time.time() - start_time
            self.timing_data["clean"] = elapsed
            print_status(f"Clean completed successfully ({elapsed:.2f}s)", "SUCCESS")
        except subprocess.CalledProcessError as e:
            print_status(f"Clean failed with exit code {e.returncode}", "ERROR")
            sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status("Clean operation timed out (exceeded 5 minutes)", "ERROR")
            sys.exit(1)

    def build(self) -> None:
        """Execute Bazel build."""
        if not self.run_build:
            return

        print_status("Starting Bazel build...", "INFO")
        cmd = self.build_bazel_command("build")

        print_status(f"Running: {' '.join(cmd)}", "INFO")

        try:
            start_time = time.time()
            subprocess.run(cmd, check=True, timeout=3600)  # 1 hour timeout for build
            elapsed = time.time() - start_time
            self.timing_data["build"] = elapsed
            print_status(f"Build completed successfully ({elapsed:.2f}s)", "SUCCESS")
        except subprocess.CalledProcessError as e:
            print_status(f"Build failed with exit code {e.returncode}", "ERROR")
            sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status("Build operation timed out (exceeded 1 hour)", "ERROR")
            sys.exit(1)

    def test(self) -> None:
        """Execute Bazel tests."""
        if not self.run_tests:
            return

        print_status("Running Bazel tests...", "INFO")
        cmd = self.build_bazel_command("test")

        # Add test output flags (vv = full log for failures)
        if self.verbose_tests:
            cmd.append("--test_output=all")
            cmd.append("--verbose_failures")
        else:
            cmd.append("--test_output=errors")

        print_status(f"Running: {' '.join(cmd)}", "INFO")

        try:
            start_time = time.time()
            result = subprocess.run(cmd, check=False, timeout=3600)  # 1 hour timeout for tests
            elapsed = time.time() - start_time
            self.timing_data["test"] = elapsed

            if result.returncode == 0:
                print_status(f"Tests completed successfully ({elapsed:.2f}s)", "SUCCESS")
            elif result.returncode == 4:
                # No test targets found - this is not an error
                print_status(f"No test targets found ({elapsed:.2f}s)", "WARNING")
            else:
                print_status(f"Tests failed with exit code {result.returncode}", "ERROR")
                sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status("Test operation timed out (exceeded 1 hour)", "ERROR")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            print_status(f"Tests failed with exit code {e.returncode}", "ERROR")
            sys.exit(1)

    def print_timing_summary(self) -> None:
        """Print timing summary for build phases."""
        if not self.timing_data:
            return

        print("\n" + "=" * 80)
        print("BUILD TIMING SUMMARY")
        print("=" * 80)

        total_time = sum(self.timing_data.values())

        for phase, elapsed in self.timing_data.items():
            percentage = (elapsed / total_time * 100) if total_time > 0 else 0
            print(f"  {phase.capitalize():20} {elapsed:8.2f}s ({percentage:5.1f}%)")

        print(f"  {'-' * 40}")
        print(f"  {'Total':20} {total_time:8.2f}s (100.0%)")
        print("=" * 80 + "\n")

    def execute(self) -> None:
        """Execute the build pipeline."""
        # Only print summary if not running config action
        # (config action will print it separately)
        if not self.run_config:
            self.print_configuration_summary()
        self.config()
        self.clean()
        self.build()
        self.test()
        self.print_timing_summary()


def parse_args(args: list[str]) -> list[str]:
    """Parse command line arguments."""
    processed_args = []

    for arg in args:
        # Handle dot-separated arguments (e.g., config.build.test.release)
        if "." in arg:
            parts = arg.split(".")
            processed_args.extend(parts)
        else:
            processed_args.append(arg)

    return processed_args


def print_help() -> None:
    """Print help message."""
    print_status("Quarisma Bazel Build Configuration Helper", "INFO")
    print("\n" + "=" * 80)
    print("BAZEL BUILD SYSTEM")
    print("=" * 80)
    print("\nUsage examples:")
    print("  1. Show configuration (no build):")
    print("     python setup_bazel.py config.release")
    print("  2. Default debug build (Ninja + Clang):")
    print("     python setup_bazel.py build.test")
    print("  3. Release build with AVX2:")
    print("     python setup_bazel.py build.test.release.avx2")
    print("  4. Release build with C++20:")
    print("     python setup_bazel.py config.build.release.test.cxx20")
    print("  5. Release build with optimizations:")
    print("     python setup_bazel.py build.test.release.lto.avx2")
    print("  6. Build with optional features:")
    print("     python setup_bazel.py build.release.avx2.mimalloc.magic_enum")
    print("  7. Run tests only:")
    print("     python setup_bazel.py test")
    print("  8. Clean build:")
    print("     python setup_bazel.py clean.build.test.release")
    print("  9. Build with Visual Studio 2026 (Windows only):")
    print("     python setup_bazel.py build.test.release.vs26")
    print("  10. Build with Xcode (macOS only):")
    print("      python setup_bazel.py build.test.release.xcode")
    print("\nBuild types:")
    print("  debug         - Debug build (default)")
    print("  release       - Release build with optimizations")
    print("  relwithdebinfo- Release with debug info")
    print("\nCompiler & Build Tool (Default: Ninja + Clang on all platforms):")
    print("  ninja         - Ninja build system (default)")
    print("  xcode         - Xcode generator (macOS only)")
    print("  vs17          - Visual Studio 2017 (Windows only)")
    print("  vs19          - Visual Studio 2019 (Windows only)")
    print("  vs22          - Visual Studio 2022 (Windows only)")
    print("  vs26          - Visual Studio 2026 (Windows only)")
    print("  clang         - Clang compiler (default)")
    print("  gcc           - GCC compiler")
    print("\nC++ Standard:")
    print("  cxx17         - C++17 (default)")
    print("  cxx20         - C++20")
    print("  cxx23         - C++23")
    print("\nVectorization options:")
    print("  sse           - SSE vectorization")
    print("  avx           - AVX vectorization")
    print("  avx2          - AVX2 vectorization (recommended)")
    print("  avx512        - AVX512 vectorization")
    print("\nOptional features:")
    print("  lto           - Link-time optimization")
    print("  mimalloc      - Microsoft mimalloc allocator")
    print("  magic_enum    - Magic enum library")
    print("  tbb           - Intel TBB")
    print("  openmp        - OpenMP support")
    print("  enzyme        - Enzyme AD defines (see .bazelrc build:enzyme)")
    print("  vv            - Verbose Bazel test output (--test_output=all)")
    print("\nLogging backends:")
    print("  glog          - Google glog")
    print("  loguru        - Loguru logging")
    print("  native        - Native logging")
    print("\nSanitizers:")
    print("  asan          - AddressSanitizer")
    print("  tsan          - ThreadSanitizer")
    print("  ubsan         - UndefinedBehaviorSanitizer")
    print("  msan          - MemorySanitizer")
    print("\nActions:")
    print("  config        - Show configuration summary (no build)")
    print("  build         - Build the project")
    print("  test          - Run tests")
    print("  clean         - Clean build artifacts")
    print("\nDefault Behavior:")
    print("  If no compiler or build tool is specified, defaults to Ninja + Clang")
    print("  on all platforms (Windows, macOS, Linux).")
    print("\nEquivalent to CMake setup.py:")
    print("  CMake:  python setup.py config.build.test.ninja.clang.release")
    print("  Bazel:  python setup_bazel.py config.build.test.release")
    print()


def main() -> None:
    """Main entry point."""
    if len(sys.argv) == 2 and sys.argv[1] == "--help":
        print_help()
        return

    # Check if Bazel is installed
    '''if not check_bazel_installed():
        print_status("Bazel or Bazelisk is not installed!", "ERROR")
        print_status("Install Bazelisk:", "INFO")
        print_status("  macOS:  brew install bazelisk", "INFO")
        print_status("  Linux:  npm install -g @bazel/bazelisk", "INFO")
        print_status("  Or download from: https://github.com/bazelbuild/bazelisk/releases", "INFO")
        sys.exit(1)'''

    if len(sys.argv) < 2:
        print_status("No build configuration specified. Use --help for usage information.", "ERROR")
        sys.exit(1)

    try:
        # Parse arguments
        arg_list = parse_args(sys.argv[1:])

        print_status(f"Starting Bazel build for {platform.system()}", "INFO")

        # Create configuration
        config = BazelConfiguration(arg_list)

        # If no actions specified, default to build
        if not (config.run_build or config.run_tests or config.run_clean or config.run_config):
            config.run_build = True

        # Execute build pipeline
        config.execute()

        print_status("Build process completed successfully!", "SUCCESS")

    except KeyboardInterrupt:
        print_status("\nBuild process interrupted by user", "WARNING")
        sys.exit(1)
    except Exception as e:
        print_status(f"An unexpected error occurred: {e}", "ERROR")
        sys.exit(1)


if __name__ == "__main__":
    main()


