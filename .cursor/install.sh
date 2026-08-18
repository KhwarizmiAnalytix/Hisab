#!/usr/bin/env bash
# Idempotent repository bootstrap for XSigma Cloud Agent environments.
#
# System toolchain (cmake, clang/gcc, ninja-build, libstdc++-14-dev, python3)
# is provided by the base environment/snapshot. This script only refreshes the
# repository-derived state that must track the checked-out revision.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

# Vendored third-party dependencies (fmt, googletest, mimalloc, kineto, sleef, ...).
git submodule update --init --recursive

# Python dependencies used by the Scripts/setup.py build/test driver.
python3 -m pip install -r requirements.txt
