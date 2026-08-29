#!/usr/bin/env python3
"""Drop ThirdParty CMake trees whose cache was generated in another workspace.

GitHub Actions caches ``build/ThirdParty``. SLEEF's tlfloat ExternalProject
embeds absolute paths in ``CMakeCache.txt``. After a repo rename (or a
``restore-keys`` hit from another checkout) ninja fails with::

    CMakeCache.txt directory is different than the directory where it was created
"""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path


def workspace_root() -> Path:
    env = os.environ.get("GITHUB_WORKSPACE")
    if env:
        return Path(env).resolve()
    return Path(__file__).resolve().parents[3]


def candidate_roots(workspace: Path, extra: list[Path]) -> list[Path]:
    if extra:
        return [path for path in extra if path.is_dir()]
    found: list[Path] = []
    for pattern in ("build/ThirdParty", "build_*/ThirdParty", "build/*/ThirdParty"):
        found.extend(p for p in workspace.glob(pattern) if p.is_dir())
    return found


def cmake_cache_stale(cache: Path, workspace: Path) -> bool:
    text = cache.read_text(encoding="utf-8", errors="replace")
    actual = cache.parent.resolve()
    for line in text.splitlines():
        if line.startswith("CMAKE_CACHEFILE_DIR:INTERNAL="):
            cached = Path(line.split("=", 1)[1].strip())
            if cached.resolve() != actual:
                return True
        if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
            home = Path(line.split("=", 1)[1].strip()).resolve()
            if not home.is_relative_to(workspace):
                return True
    return False


def thirdparty_root_for(cache: Path, roots: list[Path]) -> Path | None:
    resolved = cache.resolve()
    for root in roots:
        try:
            resolved.relative_to(root.resolve())
            return root
        except ValueError:
            continue
    return None


def main(argv: list[str]) -> int:
    workspace = workspace_root()
    extra = [Path(arg).expanduser() for arg in argv[1:]]
    roots = candidate_roots(workspace, extra)
    stale_roots: set[Path] = set()
    for third_party in roots:
        for cache in third_party.rglob("CMakeCache.txt"):
            if not cmake_cache_stale(cache, workspace):
                continue
            victim = thirdparty_root_for(cache, roots) or third_party
            print(
                f"[sanitize-thirdparty-cache] stale CMakeCache at {cache} "
                f"(workspace is {workspace}); removing {victim}",
                file=sys.stderr,
            )
            stale_roots.add(victim)
            break
    for victim in stale_roots:
        shutil.rmtree(victim)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
