#!/usr/bin/env bash
# Build Enzyme from scratch on MSYS2 (MinGW64, UCRT64, or CLANG64).
# Intended for a new machine: install MSYS2, open the matching shell, run this script.
# See repo-root BUILD-MSYS2-ENZYME.md for MSYS2-specific CMake/fork notes and C++ linking.
#
# What it does:
#   1) Optionally install toolchain deps via pacman (SKIP_PACMAN=1 to skip)
#   2) Optionally create a repo-local Python venv with lit (SKIP_VENV=1 to skip)
#   3) Git-clone Enzyme if ENZYME_SRC is not already a checkout (see defaults below)
#   4) Verify LLVM_ENABLE_PLUGINS=ON
#   5) CMake configure (Ninja), build, run tests, install, remove BUILD_DIR (see below)
#
# This script does not assume you already have the Enzyme repo. By default it clones
# https://github.com/EnzymeAD/Enzyme.git into $HOME/src/Enzyme at tag v0.0.256.
# Override: ENZYME_GIT_BRANCH=main (or another branch/tag). Remote default branch: ENZYME_GIT_BRANCH=
# Refresh an old checkout: git -C "$ENZYME_SRC" fetch --tags && git -C "$ENZYME_SRC" checkout v0.0.256
# Copy this .sh anywhere and run it from MinGW64 (or pass bash the absolute path).
#
# Usage:
#   bash /path/to/build-enzyme-msys2-from-scratch.sh
#
# Use your fork or another directory:
#   export ENZYME_REPO_URL=https://github.com/you/Enzyme.git
#   export ENZYME_SRC=/c/src/Enzyme
#   bash /path/to/build-enzyme-msys2-from-scratch.sh
#
# Rebuild an existing checkout (skip clone):
#   export ENZYME_SRC=/c/path/to/existing/enzyme
#   bash /path/to/build-enzyme-msys2-from-scratch.sh
#
# Environment (all optional unless noted):
#   ENZYME_SRC        — Enzyme source root (default: $HOME/src/Enzyme)
#   ENZYME_REPO_URL   — git remote for clone (default: https://github.com/EnzymeAD/Enzyme.git)
#   ENZYME_GIT_BRANCH — branch or tag to clone (default: v0.0.256). Set empty (ENZYME_GIT_BRANCH=) for remote default branch.
#   ENZYME_SHALLOW_CLONE=1 — use git --depth 1 (faster; disable if checkout looks empty)
#   BUILD_DIR         — CMake build dir (default: after clone: $ENZYME_SRC/build-$MSYSTEM-enzyme)
#   INSTALL_PREFIX    — install prefix (default: /c/Program Files (x86)/Enzyme)
#   LLVM_DIR          — default: $MSYS_PREFIX/lib/cmake/llvm
#   CMAKE_BUILD_TYPE  — default RelWithDebInfo
#   SKIP_PACMAN=1     — do not run pacman -S
#   SKIP_VENV=1       — do not create .venv-lit / install lit
#   SKIP_INSTALL=1    — build only (does not remove BUILD_DIR)
#   SKIP_TESTS=1      — skip ninja check-* targets
#   KEEP_BUILD_DIR=1  — after install, do not delete BUILD_DIR (default: delete)
#   ENZYME_CLANG=OFF  — force Clang plugin off
#   LLVM_EXTERNAL_LIT — path to lit.exe (default: auto from .venv-lit)
#   CLEAN_ENZYME=1    — remove existing ENZYME_SRC (and BUILD_DIR) before cloning fresh
#   SKIP_CLANG_CPP_PATCH=1 — do not apply scripts/patches/clangenzyme-mingw-libclang-cpp.patch
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

die() { echo "ERROR: $*" >&2; exit 1; }

case "${MSYSTEM:-}" in
  MINGW64)
    MSYS_PREFIX=/mingw64
    PACMAN_PKG_PREFIX=mingw-w64-x86_64
    ;;
  UCRT64)
    MSYS_PREFIX=/ucrt64
    PACMAN_PKG_PREFIX=mingw-w64-ucrt-x86_64
    ;;
  CLANG64)
    MSYS_PREFIX=/clang64
    PACMAN_PKG_PREFIX=mingw-w64-clang-x86_64
    ;;
  *)
    die "Use MSYS2 MinGW64, UCRT64, or CLANG64 shell (MSYSTEM=${MSYSTEM:-unset}). Not plain MSYS."
    ;;
esac

ENZYME_REPO_URL="${ENZYME_REPO_URL:-https://github.com/EnzymeAD/Enzyme.git}"
DEFAULT_ENZYME_SRC="${HOME}/src/Enzyme"
ENZYME_SRC="${ENZYME_SRC:-$DEFAULT_ENZYME_SRC}"
# BUILD_DIR is set after ENZYME_SRC is final (clone + optional enzyme/ subfolder).
# Default: system-wide install (MSYS path). Requires an elevated shell if Windows denies writes.
DEFAULT_INSTALL_PREFIX="/c/Program Files (x86)/Enzyme"
INSTALL_PREFIX="${INSTALL_PREFIX:-$DEFAULT_INSTALL_PREFIX}"
LLVM_DIR="${LLVM_DIR:-$MSYS_PREFIX/lib/cmake/llvm}"
# Default tag v0.0.256; use ${VAR-word} so explicit ENZYME_GIT_BRANCH= still means "remote default branch".
ENZYME_GIT_BRANCH="${ENZYME_GIT_BRANCH-v0.0.256}"

if [[ "${SKIP_PACMAN:-0}" != "1" ]]; then
  echo "== Installing / updating MSYS2 packages ($PACMAN_PKG_PREFIX) =="
  pacman -S --needed --noconfirm \
    "${PACMAN_PKG_PREFIX}-llvm" \
    "${PACMAN_PKG_PREFIX}-clang" \
    "${PACMAN_PKG_PREFIX}-lld" \
    "${PACMAN_PKG_PREFIX}-cmake" \
    "${PACMAN_PKG_PREFIX}-ninja" \
    "${PACMAN_PKG_PREFIX}-libxml2" \
    "${PACMAN_PKG_PREFIX}-mpfr" \
    "${PACMAN_PKG_PREFIX}-python" \
    "${PACMAN_PKG_PREFIX}-git" \
    "${PACMAN_PKG_PREFIX}-clang-tools-extra" \
    || die "pacman failed (run 'pacman -Syu' and retry, or SKIP_PACMAN=1 if deps are already installed)"
fi

command -v cmake >/dev/null 2>&1 || die "cmake not on PATH"
command -v ninja >/dev/null 2>&1 || die "ninja not on PATH"
command -v git >/dev/null 2>&1 || die "git not on PATH"

# Default to clang — GCC 15 hits ICEs on Enzyme's large generated TUs (BlasDerivatives.inc).
export CC="${CC:-$MSYS_PREFIX/bin/clang}"
export CXX="${CXX:-$MSYS_PREFIX/bin/clang++}"

# Prefer MSYS /usr/bin/git so the worktree path matches $ENZYME_SRC (Git for Windows can confuse MSYS paths).
if [[ -x /usr/bin/git ]]; then
  git() { command /usr/bin/git "$@"; }
fi

normalize_enzyme_root() {
  # Standalone EnzymeAD/Enzyme has CMakeLists.txt at repo root; some monorepos use enzyme/CMakeLists.txt.
  if [[ -f "$1/CMakeLists.txt" ]]; then
    printf '%s\n' "$1"
  elif [[ -f "$1/enzyme/CMakeLists.txt" ]]; then
    printf '%s/enzyme\n' "$1"
  else
    return 1
  fi
}

_USER_BUILD_DIR="${BUILD_DIR-}"

if [[ "${CLEAN_ENZYME:-0}" == "1" ]] && [[ -d "$ENZYME_SRC" ]]; then
  echo "== CLEAN_ENZYME=1: removing existing $ENZYME_SRC =="
  _old_build="${_USER_BUILD_DIR:-$ENZYME_SRC/build-${MSYSTEM}-enzyme}"
  if [[ -d "$_old_build" ]]; then
    echo "   also removing build dir: $_old_build"
    rm -rf "$_old_build"
  fi
  rm -rf "$ENZYME_SRC"
  unset _old_build
fi

if _root="$(normalize_enzyme_root "$ENZYME_SRC")"; then
  ENZYME_SRC="$_root"
  echo "== Using existing Enzyme sources at $ENZYME_SRC =="
else
  if [[ -d "$ENZYME_SRC" ]] && [[ -n "$(ls -A "$ENZYME_SRC" 2>/dev/null || true)" ]]; then
    die "ENZYME_SRC=$ENZYME_SRC exists, is not empty, and has no Enzyme CMakeLists.txt at root or enzyme/. Remove it or set ENZYME_SRC to a different path."
  fi
  if [[ -n "${ENZYME_GIT_BRANCH:-}" ]]; then
    echo "== Cloning Enzyme from $ENZYME_REPO_URL (branch/tag: $ENZYME_GIT_BRANCH) into $ENZYME_SRC =="
    _clone_args=(--branch "$ENZYME_GIT_BRANCH" --single-branch)
  else
    echo "== Cloning Enzyme from $ENZYME_REPO_URL (default branch = latest tip) into $ENZYME_SRC =="
    _clone_args=(--single-branch)
  fi
  mkdir -p "$(dirname "$ENZYME_SRC")"
  if [[ "${ENZYME_SHALLOW_CLONE:-0}" == "1" ]]; then
    _clone_args+=(--depth 1)
  fi
  git clone "${_clone_args[@]}" "$ENZYME_REPO_URL" "$ENZYME_SRC"
  if ! _root="$(normalize_enzyme_root "$ENZYME_SRC")"; then
    echo "ERROR: Clone finished but no Enzyme CMakeLists.txt under $ENZYME_SRC or $ENZYME_SRC/enzyme" >&2
    echo "  git: $(command -v git)" >&2
    echo "  Contents of $ENZYME_SRC:" >&2
    ls -la "$ENZYME_SRC" 2>&1 >&2 || true
    die "Fix: rm -rf \"$ENZYME_SRC\" and retry; use ENZYME_SHALLOW_CLONE=0 (default); check ENZYME_REPO_URL; set ENZYME_GIT_BRANCH=main (or a tag) if the default branch is not what you expect"
  fi
  ENZYME_SRC="$_root"
  if [[ "$(basename "$ENZYME_SRC")" == enzyme ]]; then
    echo "NOTE: using nested Enzyme CMake root: $ENZYME_SRC"
  fi
fi

BUILD_DIR="${_USER_BUILD_DIR:-$ENZYME_SRC/build-${MSYSTEM}-enzyme}"
unset _USER_BUILD_DIR _root _clone_args

LLVM_CONFIG="$LLVM_DIR/LLVMConfig.cmake"
[[ -f "$LLVM_CONFIG" ]] || die "LLVM not found: $LLVM_CONFIG (wrong LLVM_DIR for $MSYSTEM?)"

if ! grep -q "set(LLVM_ENABLE_PLUGINS ON)" "$LLVM_CONFIG" 2>/dev/null; then
  die "LLVM at $LLVM_DIR has LLVM_ENABLE_PLUGINS=OFF. Enzyme needs plugins. Use an MSYS2 LLVM built with plugins ON, or build LLVM from source with -DLLVM_ENABLE_PLUGINS=ON."
fi


# Lit: venv under ENZYME_SRC so the path is stable and Ninja/cmd.exe on Windows can invoke lit.exe
LIT_CMAKE=()
if [[ -n "${LLVM_EXTERNAL_LIT:-}" ]]; then
  LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="$LLVM_EXTERNAL_LIT")
elif [[ "${SKIP_VENV:-0}" != "1" ]]; then
  VENV_DIR="$ENZYME_SRC/.venv-lit"
  if [[ ! -x "$VENV_DIR/bin/python" ]] && [[ ! -x "$VENV_DIR/Scripts/python.exe" ]]; then
    echo "== Creating Python venv for lit at $VENV_DIR =="
    "$MSYS_PREFIX/bin/python" -m venv "$VENV_DIR"
  fi
  _pip=( "$VENV_DIR/bin/pip" )
  [[ -x "${_pip[0]}" ]] || _pip=( "$VENV_DIR/Scripts/pip.exe" )
  [[ -x "${_pip[0]}" ]] || die "pip not found in venv"
  "${_pip[0]}" install -q lit
  if [[ -x "$VENV_DIR/bin/lit.exe" ]]; then
    _winlit="$(cygpath -w "$VENV_DIR/bin/lit.exe" 2>/dev/null || true)"
    if [[ -n "$_winlit" ]]; then
      LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="$_winlit")
    else
      LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="$VENV_DIR/bin/lit.exe")
    fi
  elif [[ -x "$VENV_DIR/Scripts/lit.exe" ]]; then
    _winlit="$(cygpath -w "$VENV_DIR/Scripts/lit.exe" 2>/dev/null || true)"
    LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="${_winlit:-$VENV_DIR/Scripts/lit.exe}")
  else
    die "lit not found in venv after pip install"
  fi
elif [[ -x "$MSYS_PREFIX/bin/llvm-lit" ]]; then
  LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="$MSYS_PREFIX/bin/llvm-lit")
elif command -v lit >/dev/null 2>&1; then
  LIT_CMAKE=(-DLLVM_EXTERNAL_LIT="$(command -v lit)")
fi

CLANG_DIR="$MSYS_PREFIX/lib/cmake/clang"
CLANG_CMAKE=()
if [[ "${ENZYME_CLANG:-}" == "OFF" ]]; then
  CLANG_CMAKE=(-DENZYME_CLANG=OFF)
elif [[ -f "$CLANG_DIR/ClangConfig.cmake" ]]; then
  if [[ -f "$MSYS_PREFIX/lib/libclangApplyReplacements.a" ]] || [[ -f "$MSYS_PREFIX/lib/libclangApplyReplacements.dll.a" ]]; then
    CLANG_CMAKE=(-DClang_DIR="$CLANG_DIR")
  else
    echo "NOTE: clang-tools-extra libraries missing; building without Clang plugin (ENZYME_CLANG=OFF)."
    CLANG_CMAKE=(-DENZYME_CLANG=OFF)
  fi
else
  CLANG_CMAKE=(-DENZYME_CLANG=OFF)
fi

# MinGW: ClangEnzyme needs libclang-cpp but upstream doesn't link it.
# Inject target_link_libraries after the ClangEnzyme ENZYME_RUNPASS definition
# if the fix isn't already present.
_enzyme_cmake="$ENZYME_SRC/Enzyme/CMakeLists.txt"
if [[ -f "$_enzyme_cmake" ]] && ! grep -q 'target_link_libraries(ClangEnzyme.*clang-cpp' "$_enzyme_cmake" 2>/dev/null; then
  if grep -q 'target_compile_definitions(ClangEnzyme' "$_enzyme_cmake" 2>/dev/null; then
    echo "== Patching Enzyme CMakeLists.txt: linking ClangEnzyme against libclang-cpp =="
    sed -i '/target_compile_definitions(ClangEnzyme.*ENZYME_RUNPASS)/a\
    target_link_libraries(ClangEnzyme-${LLVM_VERSION_MAJOR} PRIVATE clang-cpp)' "$_enzyme_cmake"
  fi
fi
unset _enzyme_cmake

echo "== CMake configure =="
echo "    MSYSTEM=$MSYSTEM  MSYS_PREFIX=$MSYS_PREFIX"
echo "    ENZYME_SRC=$ENZYME_SRC"
echo "    BUILD_DIR=$BUILD_DIR"
echo "    INSTALL_PREFIX=$INSTALL_PREFIX"
echo "    LLVM_DIR=$LLVM_DIR"

cmake -G Ninja -S "$ENZYME_SRC" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-RelWithDebInfo}" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR="$LLVM_DIR" \
  "${LIT_CMAKE[@]}" \
  "${CLANG_CMAKE[@]}" \
  -DENZYME_ENABLE_PLUGINS=ON

echo "== Build =="
cmake --build "$BUILD_DIR" --parallel

# Tests need the build tree; run before install + removal of BUILD_DIR.
if [[ "${SKIP_TESTS:-0}" != "1" ]]; then
  echo "== Tests (subset) =="
  if cmake --build "$BUILD_DIR" --target check-typeanalysis --parallel; then :; else
    echo "WARNING: check-typeanalysis failed (may differ by LLVM version)."
  fi
  if cmake --build "$BUILD_DIR" --target check-activityanalysis --parallel; then :; else
    echo "WARNING: check-activityanalysis failed (may differ by LLVM version)."
  fi
  echo "== check-enzyme-bench =="
  cmake --build "$BUILD_DIR" --target check-enzyme-bench --parallel
else
  echo "== Tests (skipped) =="
fi

if [[ "${SKIP_INSTALL:-0}" != "1" ]]; then
  echo "== Install =="
  cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
  if [[ "${KEEP_BUILD_DIR:-0}" != "1" ]]; then
    echo "== Remove build directory =="
    rm -rf "$BUILD_DIR"
  else
    echo "== KEEP_BUILD_DIR=1: leaving $BUILD_DIR =="
  fi
else
  echo "== Install (skipped); build tree kept at $BUILD_DIR =="
fi

echo ""
echo "OK: Enzyme built and installed."
if [[ -d "$BUILD_DIR" ]]; then
  echo "    build:   $BUILD_DIR"
else
  echo "    build:   (removed)"
fi
echo "    install: $INSTALL_PREFIX"
echo "    CMake package: $INSTALL_PREFIX/CMake/EnzymeConfig.cmake"
echo ""
echo "Next (MinGW C++ sample in a sibling repo): see BUILD-MSYS2-ENZYME.md and enzyme-cpp-link-test/"
