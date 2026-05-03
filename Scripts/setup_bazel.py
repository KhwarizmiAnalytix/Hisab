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

import glob
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Optional

import colorama
from colorama import Fore, Style

# Initialize colorama for cross-platform colored output
colorama.init()


def print_status(message: str, status: str = "INFO") -> None:
    """Print colored status messages."""
    colors = {
        "INFO": Fore.CYAN,
        "SUCCESS": Fore.GREEN,
        "WARNING": Fore.YELLOW,
        "ERROR": Fore.RED,
    }
    color = colors.get(status, Fore.WHITE)
    print(f"{color}[{status}]{Style.RESET_ALL} {message}")


def _find_bazel_executable() -> Optional[str]:
    """Find bazelisk or bazel on PATH without spawning a process."""
    # On Windows, shutil.which also resolves .exe/.cmd/.ps1 extensions
    for cmd in ["bazelisk", "bazel"]:
        path = shutil.which(cmd)
        if path:
            return cmd
    return None


def check_bazel_installed() -> bool:
    """Check if Bazel or Bazelisk is installed."""
    cmd = _find_bazel_executable()
    if cmd:
        print_status(f"Found {cmd}", "INFO")
        return True
    return False


def get_bazel_command() -> str:
    """Get the appropriate Bazel command (bazelisk or bazel)."""
    cmd = _find_bazel_executable()
    if cmd:
        return cmd
    raise RuntimeError("Neither bazel nor bazelisk found in PATH")


def find_enzyme_pass_plugin() -> Optional[str]:
    """Resolve the Enzyme LLVM plugin path (mirrors Cmake/tools/enzyme.cmake).

    Prefer LLDEnzyme over ClangEnzyme when multiple matches exist. Honour
    ENZYME_PLUGIN_PATH when set to an existing file.
    """
    env_path = os.environ.get("ENZYME_PLUGIN_PATH", "").strip()
    if env_path and os.path.isfile(env_path):
        return env_path

    system = platform.system()
    search_dirs: List[str] = []
    if system == "Darwin":
        search_dirs = ["/opt/homebrew/lib", "/usr/local/lib", "/usr/lib"]
    elif system == "Linux":
        search_dirs = ["/usr/lib", "/usr/local/lib"]
        llvm_dir = os.environ.get("LLVM_DIR", "").strip()
        if llvm_dir:
            search_dirs.insert(0, os.path.join(llvm_dir, "lib"))
    elif system == "Windows":
        # Match FindEnzyme.cmake: optional ignore of LLVM_DIR for unwanted prefixes.
        restrict = os.environ.get("ENZYME_RESTRICT_TO_SYSTEM_LLVM_INSTALL", "").strip().lower() in (
            "1",
            "true",
            "yes",
            "on",
        )
        if restrict:
            search_dirs = [
                r"C:\Program Files\LLVM\bin",
                r"C:\Program Files\LLVM\lib",
                r"C:\Program Files (x86)\LLVM\bin",
                r"C:\Program Files (x86)\LLVM\lib",
            ]
        else:
            search_dirs = [
                os.path.join(os.environ.get("LLVM_DIR", ""), "bin"),
                os.path.join(os.environ.get("LLVM_DIR", ""), "lib"),
                r"C:\Program Files\LLVM\bin",
                r"C:\Program Files\LLVM\lib",
            ]

    patterns: List[str] = []
    if system == "Darwin":
        patterns = ["LLDEnzyme-*.dylib", "ClangEnzyme-*.dylib", "LLVMEnzyme-*.dylib"]
    elif system == "Linux":
        patterns = ["LLDEnzyme-*.so", "ClangEnzyme-*.so", "LLVMEnzyme-*.so"]
    elif system == "Windows":
        patterns = ["LLDEnzyme-*.dll", "ClangEnzyme-*.dll", "LLVMEnzyme-*.dll"]

    candidates: List[str] = []
    for directory in search_dirs:
        if not directory or not os.path.isdir(directory):
            continue
        for pattern in patterns:
            candidates.extend(glob.glob(os.path.join(directory, pattern)))

    # Prefer LLDEnzyme (same as CMake enzyme.cmake)
    for marker in ("LLDEnzyme", "ClangEnzyme", "LLVMEnzyme"):
        for path in candidates:
            if marker in os.path.basename(path):
                return path
    return candidates[0] if candidates else None


# Maps CMake sanitizer names (setup.py / -DSANITIZER_TYPE) to Bazel --config names in .bazelrc
_CMAKE_SAN_TO_BAZEL = {
    "address": "asan",
    "undefined": "ubsan",
    "thread": "tsan",
    "memory": "msan",
    "leak": "lsan",
}


def _merge_dotted_segments(parts: List[str]) -> List[str]:
    """Merge split segments like parallel.openmp, sanitizer.address into single tokens."""
    out: List[str] = []
    pl = [p.lower() for p in parts]
    i = 0
    while i < len(pl):
        if pl[i] == "parallel" and i + 1 < len(pl) and pl[i + 1] in ("std", "openmp", "tbb"):
            out.append(f"parallel.{pl[i + 1]}")
            i += 2
        elif pl[i] == "sanitizer" and i + 1 < len(pl) and pl[i + 1] in _CMAKE_SAN_TO_BAZEL:
            out.append(_CMAKE_SAN_TO_BAZEL[pl[i + 1]])
            i += 2
        elif pl[i] == "profiler" and i + 1 < len(pl) and pl[i + 1] in ("kineto", "itt", "native"):
            out.append(f"profiler_{pl[i + 1]}")
            i += 2
        elif pl[i] == "logging" and i + 1 < len(pl) and pl[i + 1] in ("native", "loguru", "glog"):
            out.append(f"logging_{pl[i + 1]}")
            i += 2
        else:
            out.append(pl[i])
            i += 1
    return out


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
        self.run_coverage = False
        self.timing_data: Dict[str, float] = {}
        # Warn when a phase exceeds this many seconds (incremental builds should stay fast)
        self.slow_phase_warn_seconds: float = 60.0
        # Pass --batch to bazel (avoids waiting on a stuck server; slower per invocation)
        self.use_batch: bool = False
        # Verbose test output (--test_output=all) when token "vv" is present
        self.verbose_tests = False
        # Timeout in seconds for build/test/coverage subprocesses (default: 10 min)
        self.subprocess_timeout: int = 600

        # Default backends (matching CMake defaults)
        self.logging_backend = "loguru"  # Default: LOGURU (matches CMake)
        self.profiler_backend = "kineto"  # Default: KINETO (matches CMake)

        # Compiler and build tool configuration
        self.compiler: Optional[str] = None  # Compiler type: "clang", "gcc", "msvc", etc.
        self.build_tool: Optional[str] = None  # Build tool: "ninja", "xcode", "msvc", etc.
        self.visual_studio_version: Optional[str] = None  # VS version: "vs17", "vs19", "vs22", "vs26"
        self.system = platform.system()

        # Mirrors CMake PARALLEL_BACKEND (std | openmp | tbb). None = infer only from tbb/openmp tokens.
        self.parallel_backend: Optional[str] = None
        # CMake: passing `gtest` disables per-module ENABLE_GTEST (default ON).
        self.disable_gtest: bool = False
        # CMake: token `static` sets BUILD_SHARED_LIBS=ON (shared DLLs).
        self.shared_libs: bool = False

        # CMake-only flags — not executed in Bazel but tracked so the summary is accurate.
        self.spell:      bool = False
        self.clangtidy:  bool = False
        self.fix:        bool = False
        self.iwyu:       bool = False
        self.valgrind:   bool = False
        self.icecc:      bool = False
        self.examples:   bool = False
        self.cppcheck:   bool = False

        self._parse_arguments()
        self._finalize_parallel_configs()
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
        return "clang" in arg.lower() and arg.lower() not in ["clang-cl", "clangtidy", "clangtid", "clang-tidy", "clang_tidy"]

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
            elif arg_lower in ["sse", "avx", "avx2", "avx512", "neon", "sve"]:
                self.vectorization = arg_lower
                self.configs.append(arg_lower)

            # LTO
            elif arg_lower == "lto":
                self.configs.append("lto")

            # Optional features
            elif arg_lower in ["mimalloc", "magic_enum", "tbb", "mkl", "openmp", "cuda", "hip"]:
                self.configs.append(arg_lower)

            # NUMA / memkind (see .bazelrc build:numa / build:memkind)
            elif arg_lower in ("numa", "memkind"):
                self.configs.append(arg_lower)

            # Google Benchmark (opt-in, matches CMake)
            elif arg_lower == "benchmark":
                self.configs.append("benchmark")

            # CMake inverse: `gtest` token disables CORE_HAS_GTEST / test framework defines
            elif arg_lower == "gtest":
                self.disable_gtest = True

            # CMake: token `static` enables shared libraries (BUILD_SHARED_LIBS=ON)
            elif arg_lower == "static":
                self.shared_libs = True

            # SMP backend (same semantics as setup.py --parallel.* / parallel.openmp in dotted args)
            elif arg_lower.startswith("parallel."):
                backend = arg_lower.split(".", 1)[1]
                if backend in ("std", "openmp", "tbb"):
                    self.parallel_backend = backend
                else:
                    print_status(
                        f"Invalid parallel backend '{backend}'. Use std, openmp, or tbb.",
                        "ERROR",
                    )
                    sys.exit(1)

            # Sanitizer shorthand (matches help text and CMake names via parse_args)
            elif arg_lower in ("asan", "tsan", "ubsan", "msan", "lsan"):
                self.configs.append(arg_lower)

            # Enzyme AD (matches .bazelrc build:enzyme)
            elif arg_lower == "enzyme":
                self.configs.append("enzyme")

            # CMake-only developer flags — tracked for summary accuracy, not executed in Bazel
            elif arg_lower in ["spell"]:
                self.spell = True
                print_status("Token 'spell': spell-checking is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["clangtidy", "clangtid", "clang-tidy", "clang_tidy"]:
                self.clangtidy = True
                print_status("Token 'clangtidy': clang-tidy is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["fix"]:
                self.fix = True
                print_status("Token 'fix': clang-tidy --fix is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["iwyu"]:
                self.iwyu = True
                print_status("Token 'iwyu': include-what-you-use is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["valgrind"]:
                self.valgrind = True
                print_status("Token 'valgrind': Valgrind is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["icecc"]:
                self.icecc = True
                print_status("Token 'icecc': Icecream compiler is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["examples"]:
                self.examples = True
                print_status("Token 'examples': examples are CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ["cppcheck"]:
                self.cppcheck = True
                print_status("Token 'cppcheck': cppcheck is CMake-only (shown in summary, not run).", "WARNING")

            elif arg_lower in ("external", "cache", "linker"):
                print_status(
                    f"Token '{arg_lower}': CMake-only, no Bazel equivalent (see Docs/readme/bazel.md).",
                    "WARNING",
                )

            # Verbose test log output (maps to --test_output=all)
            elif arg_lower == "vv":
                self.verbose_tests = True

            # Avoid blocking on another Bazel server PID (use if you see "Another command is running")
            elif arg_lower == "batch":
                self.use_batch = True

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

            # Sanitizers: sanitizer_asan or legacy sanitizer_address -> asan
            elif arg_lower.startswith("sanitizer_"):
                sanitizer = arg_lower[10:]
                if sanitizer in _CMAKE_SAN_TO_BAZEL:
                    self.configs.append(_CMAKE_SAN_TO_BAZEL[sanitizer])
                elif sanitizer in ("asan", "tsan", "ubsan", "msan", "lsan"):
                    self.configs.append(sanitizer)
                else:
                    print_status(
                        f"Unknown sanitizer token '{arg_lower}'. "
                        f"Use asan, tsan, ubsan, msan, lsan or CMake-style sanitizer.*",
                        "ERROR",
                    )
                    sys.exit(1)

            # Actions
            elif arg_lower == "build":
                self.run_build = True
            elif arg_lower == "test":
                self.run_tests = True
            elif arg_lower == "coverage":
                self.run_coverage = True
            elif arg_lower == "clean":
                self.run_clean = True
            elif arg_lower == "config":
                self.run_config = True

    def _finalize_parallel_configs(self) -> None:
        """Apply PARALLEL_BACKEND-style exclusivity (std vs OpenMP vs TBB)."""
        if self.parallel_backend is None:
            return
        self.configs = [c for c in self.configs if c not in ("openmp", "tbb")]
        if self.parallel_backend == "openmp":
            self.configs.append("openmp")
        elif self.parallel_backend == "tbb":
            self.configs.append("tbb")

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

        # If Xcode is specified on macOS, use it (Kineto unsupported — matches setup.py)
        if self.build_tool == "xcode" and self.system == "Darwin":
            print_status("Using Xcode generator", "INFO")
            self.profiler_backend = "native"
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
        cmd = [bazel_cmd]
        if self.use_batch:
            cmd.append("--batch")
        cmd.append(action)

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

        if self.shared_libs:
            cmd.append("--define=build_shared_libs=true")

        # Stable, de-duplicated config list (do not mutate self.configs — execute() may call twice)
        cfg_list: List[str] = []
        seen_cfg: set[str] = set()
        for c in self.configs:
            if c not in seen_cfg:
                seen_cfg.add(c)
                cfg_list.append(c)

        # GoogleTest: CMake defaults ENABLE_GTEST ON; token `gtest` turns it OFF (see disable_gtest).
        if self.disable_gtest:
            cmd.append("--define=enable_gtest=false")
            cfg_list = [c for c in cfg_list if c != "gtest"]
        elif "gtest" not in cfg_list:
            cfg_list.append("gtest")

        # Google Benchmark: CMake defaults *ENABLE_BENCHMARK ON for all library modules.
        if "benchmark" not in cfg_list:
            cfg_list.append("benchmark")

        # Add default logging backend if not explicitly set
        if not any(c.startswith("logging_") for c in cfg_list):
            cfg_list.append(f"logging_{self.logging_backend}")

        # Add default profiler backend if not explicitly set
        profiler_configs = ["kineto", "itt", "native_profiler"]
        if not any(c in profiler_configs for c in cfg_list):
            if self.profiler_backend == "kineto":
                cfg_list.append("kineto")
            elif self.profiler_backend == "itt":
                cfg_list.append("itt")
            elif self.profiler_backend == "native":
                cfg_list.append("native_profiler")

        # Add all config flags
        for config in cfg_list:
            cmd.append(f"--config={config}")

        # Enzyme: CMake applies -fpass-plugin at compile and link for Enzyme::enzyme.
        # Bazel only toggles QUARISMA_HAS_ENZYME; without the plugin, __enzyme_* calls are
        # unresolved (crash at null). Restrict --per_file_copt to //Library so GCC-built
        # third-party targets never see -fpass-plugin.
        if "enzyme" in cfg_list:
            if self.compiler != "clang":
                print_status(
                    "Enzyme requires Clang; skipping -fpass-plugin (Enzyme tests may fail).",
                    "WARNING",
                )
            else:
                plugin = find_enzyme_pass_plugin()
                if plugin:
                    cmd.append(f"--per_file_copt=//Library/Core/.*\\.cpp$@-fpass-plugin={plugin}")
                    cmd.append(f"--linkopt=-fpass-plugin={plugin}")
                    print_status(f"Enzyme LLVM pass plugin: {plugin}", "INFO")
                else:
                    print_status(
                        "Enzyme enabled but no plugin found. Set ENZYME_PLUGIN_PATH or install "
                        "enzyme (e.g. brew install enzyme). Enzyme AD tests may crash.",
                        "WARNING",
                    )

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

    def _kill_stale_bazel_processes(self) -> None:
        """Force-kill any stale Bazel server processes holding the output base lock.

        Uses taskkill directly — never calls 'bazel shutdown' which itself requires
        the lock and will hang if another instance is holding it.
        """
        print_status("Killing any stale Bazel server processes...", "INFO")
        for proc_name in ["bazel-real.exe", "bazel.exe"]:
            try:
                result = subprocess.run(
                    ["taskkill", "/F", "/IM", proc_name],
                    capture_output=True,
                    timeout=10,
                    check=False,
                )
                if result.returncode == 0:
                    print_status(f"Killed stale process: {proc_name}", "WARNING")
            except (subprocess.TimeoutExpired, FileNotFoundError):
                pass

    def _shutdown_bazel_for_batch(self) -> None:
        """Stop the Bazel server so --batch does not warn about differing startup options."""
        if not self.use_batch:
            return
        bazel_cmd = get_bazel_command()
        print_status(
            "Running `bazel shutdown` before `--batch` (avoids startup-option mismatch on the server).",
            "INFO",
        )
        try:
            subprocess.run(
                [bazel_cmd, "shutdown"],
                capture_output=True,
                text=True,
                timeout=120,
                check=False,
            )
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass

    # -------------------------------------------------------------------------
    # Per-module summary helpers
    # -------------------------------------------------------------------------

    def _on_off(self, condition: bool) -> str:
        return f"{Fore.GREEN}ON{Style.RESET_ALL}" if condition else f"{Fore.RED}OFF{Style.RESET_ALL}"

    def _na(self) -> str:
        return self._on_off(False)

    def _cxx_std_display(self) -> str:
        if self.cxx_standard:
            return self.cxx_standard.replace("cxx", "C++")
        return "C++20"

    def _sanitizer_info(self) -> tuple:
        for token, san_type in [
            ("asan", "address"),
            ("tsan", "thread"),
            ("ubsan", "undefined"),
            ("msan", "memory"),
            ("lsan", "leak"),
        ]:
            if token in self.configs:
                return True, san_type
        return False, "address"

    def _pf(self, label: str, value: str, width: int = 20) -> None:
        print(f"  {label:{width}}: {value}")

    def print_module_summaries(self) -> None:
        """Print per-module configuration summaries mirroring CMake's per-module output."""
        W         = 20   # uniform column width across all modules
        na        = self._na()
        cxx       = self._cxx_std_display()
        has_san, san_type = self._sanitizer_info()
        lto       = self._on_off("lto" in self.configs)
        coverage  = self._on_off(self.run_coverage)
        testing   = self._on_off(self.run_tests)
        gtest     = self._on_off(not self.disable_gtest)
        benchmark = self._on_off("benchmark" in self.configs)
        vec       = self.vectorization.upper() if self.vectorization else "None"
        sanitizer = self._on_off(has_san)
        mimalloc_on = True  # default: .bazelrc + memory.bzl; opt out: --define=memory_enable_mimalloc=false

        # Common trailing fields shared by all modules — every flag reflects actual passed state
        def common() -> None:
            self._pf("Cache",          na,                                   W)
            self._pf("Cache backend",  na,                                   W)
            self._pf("Icecc",          self._on_off(self.icecc),             W)
            self._pf("Linker",         na,                                   W)
            self._pf("Lto",            lto,                                  W)
            self._pf("Coverage",       coverage,                             W)
            self._pf("Testing",        testing,                              W)
            self._pf("Examples",       self._on_off(self.examples),          W)
            self._pf("Gtest",          gtest,                                W)
            self._pf("Benchmark",      benchmark,                            W)
            self._pf("Clang-tidy",     self._on_off(self.clangtidy),         W)
            self._pf("Fix",            self._on_off(self.fix),               W)
            self._pf("Iwyu",           self._on_off(self.iwyu),              W)
            self._pf("Sanitizer",      sanitizer,                            W)
            self._pf("Sanitizer type", san_type,                             W)
            self._pf("Spell",          self._on_off(self.spell),             W)
            self._pf("Valgrind",       self._on_off(self.valgrind),          W)

        # ── Core ──────────────────────────────────────────────────────────────
        print(f"\n{Fore.CYAN}******** Core module (Bazel flags) ********{Style.RESET_ALL}")
        self._pf("Vectorization type", vec,                                   W)
        self._pf("Mkl",               self._on_off("mkl" in self.configs),    W)
        self._pf("Svml",              na,                                      W)
        self._pf("Rocm",              na,                                      W)
        self._pf("Experimental",      na,                                      W)
        self._pf("Magic enum",        self._on_off(True),                      W)
        self._pf("Enzyme",            self._on_off("enzyme" in self.configs),  W)
        self._pf("Compression",       na,                                      W)
        self._pf("Cxx standard",      cxx,                                     W)
        common()

        # ── Logging ───────────────────────────────────────────────────────────
        print(f"\n{Fore.CYAN}******** Logging module (Bazel flags) ********{Style.RESET_ALL}")
        self._pf("Backend",      self.logging_backend.upper(), W)
        self._pf("Cxx standard", cxx,                          W)
        common()

        # ── Memory ────────────────────────────────────────────────────────────
        print(f"\n{Fore.CYAN}******** Memory module (Bazel flags) ********{Style.RESET_ALL}")
        gpu_configs = [c for c in self.configs if c.startswith("gpu_alloc_")]
        gpu_alloc   = gpu_configs[0].replace("gpu_alloc_", "").upper() if gpu_configs else "POOL_ASYNC"
        self._pf("Mimalloc",     self._on_off(mimalloc_on),           W)
        self._pf("Memkind",      na,                                   W)
        self._pf("Numa",         na,                                   W)
        self._pf("Tbb",          self._on_off("tbb"  in self.configs), W)
        self._pf("Cuda",         self._on_off("cuda" in self.configs), W)
        self._pf("Hip",          self._on_off("hip"  in self.configs), W)
        self._pf("Gpu alloc",    gpu_alloc,                            W)
        self._pf("Cxx standard", cxx,                                  W)
        common()

        # ── Parallel ──────────────────────────────────────────────────────────
        print(f"\n{Fore.CYAN}******** Parallel module (Bazel flags) ********{Style.RESET_ALL}")
        self._pf("Tbb",          self._on_off("tbb"    in self.configs), W)
        self._pf("Openmp",       self._on_off("openmp" in self.configs), W)
        self._pf("Cxx standard", cxx,                                    W)
        common()

        # ── Profiler ──────────────────────────────────────────────────────────
        print(f"\n{Fore.CYAN}******** Profiler module (Bazel flags) ********{Style.RESET_ALL}")
        self._pf("Backend",      self.profiler_backend.upper(), W)
        self._pf("Cxx standard", cxx,                           W)
        common()

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
            print("  C++ Standard:      C++20 (default)")

        # Vectorization
        if self.vectorization:
            print(f"  Vectorization:     {self.vectorization.upper()}")
        else:
            print("  Vectorization:     None")

        # Feature flags — computed from the same state as per-module summaries
        mimalloc_on = True  # Bazel default ON (see .bazelrc memory_enable_mimalloc)
        gtest_on    = not self.disable_gtest     # ON by default (mirrors CMake option(... ON))

        print(f"\n{Fore.CYAN}Feature Flags:{Style.RESET_ALL}")
        flags = [
            ("MEMORY_ENABLE_MIMALLOC",    mimalloc_on),
            ("CORE_HAS_MAGICENUM",        True),
            ("PARALLEL/MEMORY_HAS_TBB",   "tbb"    in self.configs),
            ("PARALLEL_HAS_OPENMP",       "openmp" in self.configs),
            ("MEMORY_HAS_CUDA",           "cuda"   in self.configs),
            ("MEMORY_HAS_HIP",            "hip"    in self.configs),
            ("QUARISMA_ENABLE_LTO",       "lto"    in self.configs),
            ("CORE_HAS_ENZYME",           "enzyme" in self.configs),
            ("QUARISMA_ENABLE_GTEST",     gtest_on),
            ("BUILD_SHARED_LIBS",         self.shared_libs),
            ("ENABLE_SPELL",              self.spell),
            ("ENABLE_CLANGTIDY",          self.clangtidy),
            ("ENABLE_FIX",                self.fix),
            ("ENABLE_IWYU",               self.iwyu),
            ("ENABLE_CPPCHECK",           self.cppcheck),
            ("ENABLE_VALGRIND",           self.valgrind),
            ("ENABLE_ICECC",              self.icecc),
            ("ENABLE_EXAMPLES",           self.examples),
        ]
        for flag, state in flags:
            print(f"  {flag:30} {self._on_off(state)}")

        # Logging / Profiler backends
        print(f"\n{Fore.CYAN}Logging Backend:{Style.RESET_ALL}")
        print(f"  Backend:           {self.logging_backend.upper()}")
        print(f"\n{Fore.CYAN}Profiler Backend:{Style.RESET_ALL}")
        print(f"  Backend:           {self.profiler_backend.upper()}")

        # Sanitizers
        sanitizers = [c for c in self.configs if c in ["asan", "tsan", "ubsan", "msan"]]
        print(f"\n{Fore.CYAN}Sanitizers:{Style.RESET_ALL}")
        if sanitizers:
            for san in sanitizers:
                print(f"  {san.upper():30} {self._on_off(True)}")
        else:
            print(f"  {'None':30} {self._on_off(False)}")

        # Bazel command preview
        if self.run_build or self.run_tests or self.run_coverage:
            action = "test" if self.run_tests else ("coverage" if self.run_coverage else "build")
            cmd = self.build_bazel_command(action)
            print(f"\n{Fore.CYAN}Bazel Command:{Style.RESET_ALL}")
            print(f"  {' '.join(cmd)}")

        print("\n" + "=" * 80 + "\n")

    def config(self) -> None:
        """Handle config action (summary already printed by execute())."""
        pass

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
        self._shutdown_bazel_for_batch()
        cmd = self.build_bazel_command("build")

        print_status(f"Running: {' '.join(cmd)}", "INFO")

        try:
            start_time = time.time()
            subprocess.run(cmd, check=True, timeout=self.subprocess_timeout)
            elapsed = time.time() - start_time
            self.timing_data["build"] = elapsed
            print_status(f"Build completed successfully ({elapsed:.2f}s)", "SUCCESS")
            if elapsed > self.slow_phase_warn_seconds:
                print_status(
                    f"Build took longer than {self.slow_phase_warn_seconds:.0f}s — check cold cache, "
                    "machine load, or run a narrower target (e.g. //Library/Core:core_lib).",
                    "WARNING",
                )
        except subprocess.CalledProcessError as e:
            print_status(f"Build failed with exit code {e.returncode}", "ERROR")
            sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status(f"Build timed out (exceeded {self.subprocess_timeout}s)", "ERROR")
            sys.exit(1)

    def test(self) -> None:
        """Execute Bazel tests."""
        if not self.run_tests:
            return

        print_status("Running Bazel tests...", "INFO")
        self._shutdown_bazel_for_batch()
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
            result = subprocess.run(cmd, check=False, timeout=self.subprocess_timeout)
            elapsed = time.time() - start_time
            self.timing_data["test"] = elapsed

            if result.returncode == 0:
                print_status(f"Tests completed successfully ({elapsed:.2f}s)", "SUCCESS")
                if elapsed > self.slow_phase_warn_seconds:
                    print_status(
                        f"Tests took longer than {self.slow_phase_warn_seconds:.0f}s — "
                        "narrow targets or check for hangs.",
                        "WARNING",
                    )
            elif result.returncode == 4:
                # No test targets found - this is not an error
                print_status(f"No test targets found ({elapsed:.2f}s)", "WARNING")
            else:
                print_status(f"Tests failed with exit code {result.returncode}", "ERROR")
                sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status(f"Tests timed out (exceeded {self.subprocess_timeout}s)", "ERROR")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            print_status(f"Tests failed with exit code {e.returncode}", "ERROR")
            sys.exit(1)

    def coverage(self) -> None:
        """Execute Bazel coverage."""
        if not self.run_coverage:
            return

        print_status("Running Bazel coverage...", "INFO")
        self._shutdown_bazel_for_batch()
        cmd = self.build_bazel_command("coverage")

        if self.verbose_tests:
            cmd.append("--test_output=all")
            cmd.append("--verbose_failures")
        else:
            cmd.append("--test_output=errors")

        print_status(f"Running: {' '.join(cmd)}", "INFO")

        try:
            start_time = time.time()
            result = subprocess.run(cmd, check=False, timeout=self.subprocess_timeout)
            elapsed = time.time() - start_time
            self.timing_data["coverage"] = elapsed

            if result.returncode == 0:
                print_status(f"Coverage completed successfully ({elapsed:.2f}s)", "SUCCESS")
                print_status("Coverage report: bazel-out/_coverage/_coverage_report.dat", "INFO")
            elif result.returncode == 4:
                print_status(f"No coverage targets found ({elapsed:.2f}s)", "WARNING")
            else:
                print_status(f"Coverage failed with exit code {result.returncode}", "ERROR")
                sys.exit(1)
        except subprocess.TimeoutExpired:
            print_status(f"Coverage timed out (exceeded {self.subprocess_timeout}s)", "ERROR")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            print_status(f"Coverage failed with exit code {e.returncode}", "ERROR")
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
        self.print_module_summaries()
        self.print_configuration_summary()
        self.config()
        # config token implies a clean slate — force expunge before any build/test.
        if self.run_config and (self.run_build or self.run_tests or self.run_coverage):
            print_status("Config requested: forcing clean build (bazel clean --expunge).", "INFO")
            self.run_clean = True
        self.clean()
        if self.run_build or self.run_tests or self.run_coverage:
            self._kill_stale_bazel_processes()
        self.build()
        self.test()
        self.coverage()
        self.print_timing_summary()


def parse_args(args: list[str]) -> list[str]:
    """Parse argv like Scripts/setup.py: long flags, dotted shortcuts, compiler paths.

    Supports the same long-option spellings as setup.py where applicable:
      --sanitizer.address, --logging.LOGURU, --profiler.kineto, --parallel.tbb
    """
    processed: List[str] = []

    for arg in args:
        if arg.startswith("--sanitizer."):
            st = arg.split(".", 1)[1].lower()
            if st in _CMAKE_SAN_TO_BAZEL:
                processed.append(_CMAKE_SAN_TO_BAZEL[st])
            else:
                print_status(
                    f"Invalid sanitizer type: {st}. Valid: {', '.join(_CMAKE_SAN_TO_BAZEL)}",
                    "ERROR",
                )
                sys.exit(1)
            continue

        if arg.startswith("--logging."):
            bt = arg.split(".", 1)[1].lower()
            if bt in ("native", "loguru", "glog"):
                processed.append(f"logging_{bt}")
                print_status(f"Logging backend set to {bt.upper()}", "INFO")
            else:
                print_status(
                    f"Invalid logging backend: {bt}. Valid: native, loguru, glog",
                    "ERROR",
                )
                sys.exit(1)
            continue

        if arg.startswith("--profiler."):
            bt = arg.split(".", 1)[1].lower()
            prof_map = {"kineto": "kineto", "itt": "itt", "native": "native"}
            if bt in prof_map:
                processed.append(f"profiler_{bt}")
                print_status(f"Profiler backend set to {prof_map[bt]}", "INFO")
            else:
                print_status(
                    f"Invalid profiler backend: {bt}. Valid: {', '.join(prof_map)}",
                    "ERROR",
                )
                sys.exit(1)
            continue

        if arg.startswith("--parallel."):
            bt = arg.split(".", 1)[1].lower()
            if bt in ("std", "openmp", "tbb"):
                processed.append(f"parallel.{bt}")
                print_status(f"SMP backend set to {bt}", "INFO")
            else:
                print_status(
                    f"Invalid SMP backend: {bt}. Valid: std, openmp, tbb",
                    "ERROR",
                )
                sys.exit(1)
            continue

        # Compiler path: pass through verbatim (matches setup.py)
        if re.search(r"[/\\]", arg) and re.search(
            r"[Cc]lang|[Gg][Cc][Cc]|[Gg]\+\+", arg
        ):
            processed.append(arg)
            continue

        # Dot-separated convenience tokens: config.build.parallel.openmp
        if "." in arg and not arg.startswith("--"):
            parts = arg.split(".")
            processed.extend(_merge_dotted_segments(parts))
            continue

        processed.append(arg.lower())

    return processed


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
    print("  cxx17         - C++17")
    print("  cxx20         - C++20 (default)")
    print("  cxx23         - C++23")
    print("\nVectorization options:")
    print("  sse           - SSE vectorization")
    print("  avx           - AVX vectorization")
    print("  avx2          - AVX2 vectorization (recommended)")
    print("  avx512        - AVX512 vectorization")
    print("  neon          - AArch64 NEON (128-bit SIMD)")
    print("  sve           - AArch64 SVE fixed 128-bit (-msve-vector-bits=128)")
    print("\nOptional features:")
    print("  lto           - Link-time optimization")
    print("  mimalloc      - Microsoft mimalloc allocator")
    print("  magic_enum    - Magic enum library")
    print("  tbb           - Intel TBB")
    print("  openmp        - OpenMP support")
    print("  enzyme        - Enzyme AD defines (see .bazelrc build:enzyme)")
    print("  numa          - NUMA (build:numa)")
    print("  memkind       - memkind (build:memkind)")
    print("  benchmark     - Google Benchmark (default ON; token optional)")
    print("  gtest         - Disables GTest defines (CMake inverse; default is ON in both systems)")
    print("  static        - Shared libraries (CMake: BUILD_SHARED_LIBS=ON)")
    print("  parallel.std | parallel.openmp | parallel.tbb  — exclusive SMP backend")
    print("  --parallel.* / --logging.* / --profiler.*  — same long flags as setup.py")
    print("  vv            - Verbose Bazel test output (--test_output=all)")
    print("  batch         - Pass --batch to Bazel; script runs `bazel shutdown` first to avoid")
    print("                  startup-option warnings (or run: bazel shutdown; bazel --batch ...)")
    print("  spell         - (CMake only) Spell-check — ignored in Bazel, warning emitted")
    print("  clangtidy     - (CMake only) Clang-tidy — ignored in Bazel, warning emitted")
    print("\nLogging backends:")
    print("  glog          - Google glog")
    print("  loguru        - Loguru logging")
    print("  native        - Native logging")
    print("\nSanitizers (Bazel --config; CMake names accepted via dotted args or --sanitizer.*):")
    print("  asan          - AddressSanitizer (CMake: address)")
    print("  tsan          - ThreadSanitizer (CMake: thread)")
    print("  ubsan         - UndefinedBehaviorSanitizer (CMake: undefined)")
    print("  msan          - MemorySanitizer (CMake: memory)")
    print("  lsan          - LeakSanitizer (CMake: leak)")
    print("  --sanitizer.address | .undefined | .thread | .memory | .leak  (same as setup.py)")
    print("\nActions:")
    print("  config        - Show configuration summary (no build)")
    print("  build         - Build the project")
    print("  test          - Run tests")
    print("  coverage      - Run tests with coverage instrumentation (lcov report)")
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
        if not (config.run_build or config.run_tests or config.run_clean or config.run_config or config.run_coverage):
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
