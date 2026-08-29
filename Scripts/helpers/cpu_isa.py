"""Runtime CPU ISA probes.

Compiler flags (e.g. -mavx512f) only mean the toolchain can emit an ISA.
Executing those binaries on a CPU without the feature traps (SIGILL).
Use these probes to skip test *runs* after a successful compile.
"""

from __future__ import annotations

import ctypes
import os
import platform
import re
import subprocess
import sys
from typing import Any, cast, Optional


def host_has_avx512f() -> bool:
    """True if this process can execute AVX-512 Foundation instructions."""
    system = platform.system()
    if system == "Linux":
        return _linux_cpuinfo_has_flag("avx512f")
    if system == "Darwin":
        return _sysctl_flag_enabled("hw.optional.avx512f")
    if system == "Windows":
        return _windows_xstate_has_avx512()
    return False


def runtime_test_skip_reason(cpu_backend: str) -> Optional[str]:
    """Return a skip message if tests for ``cpu_backend`` must not execute here."""
    backend = (cpu_backend or "").strip().lower()
    if backend == "avx512" and not host_has_avx512f():
        return (
            "AVX-512 tests skipped: this CPU does not support avx512f "
            "(compile-only; run tests on a processor with AVX-512)."
        )
    return None


def _linux_cpuinfo_has_flag(flag: str) -> bool:
    path = "/proc/cpuinfo"
    if not os.path.isfile(path):
        return False
    try:
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError:
        return False
    return re.search(rf"(?:^|[\s]){re.escape(flag)}(?:[\s]|$)", text) is not None


def _sysctl_flag_enabled(name: str) -> bool:
    try:
        result = subprocess.run(
            ["sysctl", "-n", name],
            check=False,
            capture_output=True,
            text=True,
            timeout=5,
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return result.returncode == 0 and result.stdout.strip() == "1"


def _windows_xstate_has_avx512() -> bool:
    # XSTATE_AVX512_KMASK=5, XSTATE_AVX512_ZMM_H=6, XSTATE_AVX512_ZMM=7.
    # All three must be enabled for AVX-512 to be usable (CPU + OS XSAVE).
    mask = (1 << 5) | (1 << 6) | (1 << 7)
    try:
        win_dll = cast(Any, getattr(ctypes, "WinDLL"))  # noqa: B009
        kernel32 = win_dll("kernel32", use_last_error=True)
        get_features = kernel32.GetEnabledXStateFeatures
        get_features.restype = ctypes.c_uint64
        get_features.argtypes = []
        return (int(get_features()) & mask) == mask
    except (AttributeError, OSError, ValueError):
        return False


def main(argv: list[str]) -> int:
    """Exit 0 if the named ISA is present, 1 otherwise. Usage: cpu_isa.py avx512"""
    if len(argv) != 2 or argv[1] in ("-h", "--help"):
        sys.stderr.write("usage: cpu_isa.py avx512\n")
        return 2
    isa = argv[1].lower()
    if isa in ("avx512", "avx512f"):
        return 0 if host_has_avx512f() else 1
    sys.stderr.write(f"unknown isa: {argv[1]}\n")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
