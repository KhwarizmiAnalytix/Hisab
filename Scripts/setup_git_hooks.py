#!/usr/bin/env python
"""
XSigma Git Hooks Setup Script
==============================

Configures git to use the hooks stored in .githooks/ by setting
the core.hooksPath configuration. This makes hooks available to
every developer after cloning/pulling the repository.

Hooks provided:
  pre-commit  — formats C++ files with clang-format

Usage:
    python Scripts/setup_git_hooks.py [--install|--uninstall|--status]
"""

import os
import stat
import subprocess
import sys
from pathlib import Path
from typing import Optional


# ============================================================================
# Utility
# ============================================================================

def _color(code: str, msg: str) -> str:
    return f"\033[{code}m{msg}\033[0m"

def print_error(msg: str)   -> None: print(_color("91", f"✗ ERROR: {msg}"), file=sys.stderr)
def print_warning(msg: str) -> None: print(_color("93", f"⚠ WARNING: {msg}"))
def print_success(msg: str) -> None: print(_color("92", f"✓ {msg}"))
def print_info(msg: str)    -> None: print(_color("94", f"ℹ {msg}"))


def run(cmd: list[str], cwd: Optional[str] = None) -> tuple[int, str, str]:
    try:
        r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        return r.returncode, r.stdout, r.stderr
    except Exception as e:
        return 1, "", str(e)


def get_repo_root() -> Optional[Path]:
    code, out, err = run(["git", "rev-parse", "--show-toplevel"])
    if code != 0:
        print_error(f"Not in a git repository: {err.strip()}")
        return None
    return Path(out.strip())


# ============================================================================
# Install / Uninstall / Status
# ============================================================================

HOOKS_DIR = ".githooks"   # relative to repo root, tracked by git


def install(repo_root: Path) -> bool:
    hooks_path = repo_root / HOOKS_DIR

    if not hooks_path.is_dir():
        print_error(f"Hooks directory not found: {hooks_path}")
        return False

    # Make every hook executable (needed after a fresh clone on Linux/macOS)
    for hook in hooks_path.iterdir():
        if hook.is_file():
            mode = hook.stat().st_mode
            hook.chmod(mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    # Point git at the tracked hooks directory
    code, _, err = run(["git", "config", "core.hooksPath", HOOKS_DIR],
                       cwd=str(repo_root))
    if code != 0:
        print_error(f"git config failed: {err.strip()}")
        return False

    print_success(f"core.hooksPath set to '{HOOKS_DIR}'")
    for hook in sorted(hooks_path.iterdir()):
        if hook.is_file():
            print_info(f"  hook active: {hook.name}")
    return True


def uninstall(repo_root: Path) -> bool:
    code, _, err = run(["git", "config", "--unset", "core.hooksPath"],
                       cwd=str(repo_root))
    # exit code 5 means the key was not set — treat as success
    if code not in (0, 5):
        print_error(f"git config failed: {err.strip()}")
        return False
    print_success("core.hooksPath unset — git will use .git/hooks again")
    return True


def status(repo_root: Path) -> None:
    code, out, _ = run(["git", "config", "core.hooksPath"], cwd=str(repo_root))
    configured = code == 0 and out.strip() == HOOKS_DIR

    if configured:
        print_success(f"Hooks are active (core.hooksPath = '{HOOKS_DIR}')")
    else:
        print_warning("Hooks are NOT active — run with --install to enable")

    hooks_path = repo_root / HOOKS_DIR
    if hooks_path.is_dir():
        for hook in sorted(hooks_path.iterdir()):
            if hook.is_file():
                executable = os.access(hook, os.X_OK)
                mark = "✓" if executable else "⚠ (not executable)"
                print_info(f"  {hook.name}: {mark}")

    # clang-format availability
    cf_code, cf_out, _ = run(["clang-format", "--version"])
    if cf_code == 0:
        print_success(f"clang-format available: {cf_out.strip()}")
    else:
        print_warning("clang-format not found in PATH")
        print_info("  Linux:  sudo apt install clang-format")
        print_info("  macOS:  brew install clang-format")
        print_info("  Windows: choco install llvm")


# ============================================================================
# Main
# ============================================================================

def main() -> int:
    repo_root = get_repo_root()
    if not repo_root:
        return 1

    action = sys.argv[1].lower() if len(sys.argv) > 1 else "--install"

    if action in ("--install", "-i"):
        return 0 if install(repo_root) else 1
    elif action in ("--uninstall", "-u"):
        return 0 if uninstall(repo_root) else 1
    elif action in ("--status", "-s"):
        status(repo_root)
        return 0
    else:
        print_error(f"Unknown action: {action}")
        print_info("Usage: python Scripts/setup_git_hooks.py [--install|--uninstall|--status]")
        return 1


if __name__ == "__main__":
    sys.exit(main())
