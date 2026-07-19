"""
Ensure the Intel SVML runtime shared libraries are resolvable when test binaries run.

The vendored libsvml.so (ThirdParty/svml) needs libintlc.so.5, but the vendored
binary carries no RPATH/RUNPATH of its own. The test executable's own RUNPATH
(pointing at the build's lib dir) does not help here because RUNPATH is not
propagated to the dependencies of the libraries it links against - only to its
own direct NEEDED entries. Without libintlc.so.5 on the loader search path,
running the test binary fails with:
    error while loading shared libraries: libintlc.so.5: cannot open shared object file
"""

from __future__ import annotations

import os
import sys
from typing import Mapping


def augment_env_for_svml_runtime(
    build_path: str, base: Mapping[str, str] | None = None
) -> dict[str, str]:
    """Return a copy of the environment with the build's lib dir prepended to
    LD_LIBRARY_PATH when the staged SVML runtime (libintlc.so.5) is present there."""
    out = dict(os.environ if base is None else base)
    if sys.platform != "linux":
        return out

    lib_dir = os.path.join(build_path, "lib")
    if not os.path.isfile(os.path.join(lib_dir, "libintlc.so.5")):
        return out

    ld_path = out.get("LD_LIBRARY_PATH", "")
    parts = [p for p in ld_path.split(os.pathsep) if p]
    if lib_dir not in parts:
        parts.insert(0, lib_dir)
        out["LD_LIBRARY_PATH"] = os.pathsep.join(parts)

    return out
