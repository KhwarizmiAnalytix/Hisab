#!/usr/bin/env bash
# Cloud Agent environment bootstrap for XSigma.
#
# XSigma is configured/built/tested through Scripts/setup.py, which drives
# CMake + Ninja + Clang. This script makes a fresh checkout buildable on the
# default base image and is safe to re-run (idempotent).
#
# Note: system toolchain packages are installed here rather than in a
# Dockerfile because the base image is the default Cloud Agent image; apt-get
# install is idempotent, so re-runs are fast no-ops.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

# --- System toolchain -------------------------------------------------------
# cmake + clang/gcc + ninja are required to configure/build.
# libstdc++-14-dev is required because Clang 18 selects the newest GCC install
# (gcc-14); without it, configuration fails with "cannot find -lstdc++".
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
    build-essential \
    clang \
    cmake \
    curl \
    git \
    libstdc++-14-dev \
    ninja-build \
    python3 \
    python3-pip

# --- Vendored third-party dependencies --------------------------------------
# fmt, googletest, mimalloc, kineto, sleef, ... live as git submodules.
git submodule update --init --recursive

# --- Python dependencies used by Scripts/setup.py ---------------------------
python3 -m pip install -r requirements.txt
