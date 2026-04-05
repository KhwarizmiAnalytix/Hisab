"""
Ensure Linux/WSL CUDA toolkit from standard prefixes is visible to CMake and nvcc.

Non-login shells (e.g. wsl bash -lc) often omit /usr/local/cuda/bin from PATH even when
the NVIDIA wsl-ubuntu packages installed the toolkit there, which can make FindCUDAToolkit
fall through to a Windows CUDA path and fail to link libcudart.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path
from typing import Mapping


def _linux_cuda_roots() -> list[Path]:
    return [Path("/usr/local/cuda"), Path("/usr/lib/cuda")]


def augment_env_for_cuda_toolkit(
    base: Mapping[str, str] | None = None,
) -> dict[str, str]:
    """Return a copy of the environment with Linux CUDA toolkit paths applied when present."""
    out = dict(os.environ if base is None else base)
    if sys.platform != "linux":
        return out

    path = out.get("PATH", "")
    parts = [p for p in path.split(os.pathsep) if p]

    for root in _linux_cuda_roots():
        bin_dir = root / "bin"
        nvcc = bin_dir / "nvcc"
        if not nvcc.is_file():
            continue
        b = str(bin_dir)
        if b not in parts:
            parts.insert(0, b)
            out["PATH"] = os.pathsep.join(parts)
        if not out.get("CUDA_PATH"):
            out["CUDA_PATH"] = str(root)
        if not out.get("CUDAToolkit_ROOT"):
            out["CUDAToolkit_ROOT"] = str(root)
        break

    return out
