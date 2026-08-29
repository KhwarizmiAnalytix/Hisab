#!/bin/bash
# Shared helpers for docker/ci/jobs.sh. Sourced, not executed directly.

REPO_ROOT="/workspace"
JOBS="${JOBS:-$(nproc)}"

log()  { printf '\033[1;34m[ci]\033[0m %s\n' "$*"; }
ok()   { printf '\033[1;32m[ci]\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[ci]\033[0m %s\n' "$*" >&2; exit 1; }

# cmake_configure <build_dir> <use_launcher_cache: yes|no> [extra cmake args...]
#
# use_launcher_cache=yes mirrors every ci.yml job that passes
# "-C .github/cmake/configure_cache.cmake" (wires CMAKE_*_COMPILER_LAUNCHER=sccache).
# build-matrix and clang-coverage-tests are the two jobs that do NOT pass it.
cmake_configure() {
    local build_dir="$1"; shift
    local use_cache="$1"; shift
    cd "$REPO_ROOT" || die "repo not mounted at $REPO_ROOT"

    local cache_args=()
    if [ "$use_cache" = "yes" ]; then
        cache_args=(-C .github/cmake/configure_cache.cmake)
    fi

    log "Configuring in $build_dir"
    python3 "$REPO_ROOT/.github/workflows/install/sanitize_thirdparty_cache.py" \
        "$REPO_ROOT/$build_dir/ThirdParty" || true
    cmake -B "$build_dir" -S . -G Ninja "${cache_args[@]}" "$@"
}

cmake_build() {
    local build_dir="$1"
    log "Building $build_dir (-j $JOBS)"
    cmake --build "$build_dir" -j "$JOBS"
}

ctest_run() {
    local build_dir="$1"; shift
    log "Testing $build_dir"
    ctest --test-dir "$build_dir" --output-on-failure -j "$JOBS" "$@"
}

bazel_run() {
    cd "$REPO_ROOT/Scripts" || die "Scripts/ not found"
    log "bazel: python3 setup_bazel.py $*"
    python3 setup_bazel.py "$@"
}

# Sets ASAN_OPTIONS / UBSAN_OPTIONS / TSAN_OPTIONS / LSAN_OPTIONS to match the
# "Setup sanitizer environment" step in ci.yml's sanitizer-tests job.
export_sanitizer_env() {
    local sanitizer="$1"
    case "$sanitizer" in
        address)
            export ASAN_OPTIONS="print_stacktrace=1:check_initialization_order=1:strict_init_order=1:suppressions=$REPO_ROOT/Scripts/suppressions/asan_suppressions.txt"
            ;;
        leak)
            export LSAN_OPTIONS="print_suppressions=0:suppressions=$REPO_ROOT/Scripts/suppressions/lsan_suppressions.txt"
            ;;
        thread)
            export TSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:suppressions=$REPO_ROOT/Scripts/suppressions/tsan_suppressions.txt"
            ;;
        undefined)
            export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:suppressions=$REPO_ROOT/Scripts/suppressions/ubsan_suppressions.txt"
            ;;
        memory)
            export MSAN_OPTIONS="print_stats=1:halt_on_error=1:suppressions=$REPO_ROOT/Scripts/suppressions/msan_suppressions.txt"
            ;;
    esac
}
