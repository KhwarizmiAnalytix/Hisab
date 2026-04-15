# =============================================================================
# Quarisma Bazel Helper Functions and Macros — Project-level
# =============================================================================
# Provides shared compiler flags, linker options, and PROJECT_HAS_* compile
# definitions that apply across all modules.
#
# Module-specific defines (MEMORY_HAS_*, PARALLEL_HAS_*, LOGGING_HAS_*,
# PROFILER_HAS_*, CORE_HAS_*) live in the corresponding module bzl files:
#   bazel/memory.bzl, bazel/parallel.bzl, bazel/logging.bzl,
#   bazel/profiler.bzl, bazel/core.bzl
# =============================================================================

def quarisma_copts():
    """Returns common compiler options for Quarisma targets."""
    return select({
        "@platforms//os:windows": [
            "/std:c++17",
            "/W4",
            "/EHsc",
        ],
        "//conditions:default": [
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ],
    })

def quarisma_defines():
    """Returns project-wide preprocessor defines (PROJECT_HAS_* flags).

    Module-specific HAS flags are NOT included here — they are emitted by
    the corresponding module bzl file (memory.bzl, parallel.bzl, etc.) so
    that each module only sets the defines it owns.
    """
    base_defines = []

    # -------------------------------------------------------------------------
    # MKL — PROJECT_HAS_MKL (compile_definitions.cmake, opt-in)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:enable_mkl": ["PROJECT_HAS_MKL=1"],
        "//conditions:default": ["PROJECT_HAS_MKL=0"],
    })

    # -------------------------------------------------------------------------
    # magic_enum — PROJECT_HAS_MAGICENUM (compile_definitions.cmake, default ON)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:disable_magic_enum": ["PROJECT_HAS_MAGICENUM=0"],
        "//conditions:default": ["PROJECT_HAS_MAGICENUM=1"],
    })

    # -------------------------------------------------------------------------
    # SVML — PROJECT_HAS_SVML (compile_definitions.cmake, opt-in)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:enable_svml": ["PROJECT_HAS_SVML=1"],
        "//conditions:default": ["PROJECT_HAS_SVML=0"],
    })

    # -------------------------------------------------------------------------
    # ROCm — PROJECT_HAS_ROCM (compile_definitions.cmake, opt-in)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:enable_rocm": ["PROJECT_HAS_ROCM=1"],
        "//conditions:default": ["PROJECT_HAS_ROCM=0"],
    })

    # -------------------------------------------------------------------------
    # Experimental features — PROJECT_HAS_EXPERIMENTAL (compile_definitions.cmake)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:enable_experimental": ["PROJECT_HAS_EXPERIMENTAL=1"],
        "//conditions:default": ["PROJECT_HAS_EXPERIMENTAL=0"],
    })

    # -------------------------------------------------------------------------
    # Allocation statistics — PROJECT_HAS_ALLOCATION_STATS (compile_definitions.cmake)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:enable_allocation_stats": ["PROJECT_HAS_ALLOCATION_STATS=1"],
        "//conditions:default": ["PROJECT_HAS_ALLOCATION_STATS=0"],
    })

    # -------------------------------------------------------------------------
    # LU pivoting — PROJECT_LU_PIVOTING (CMakeLists.txt, default OFF)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:lu_pivoting": ["PROJECT_LU_PIVOTING=1"],
        "//conditions:default": [],
    })

    # -------------------------------------------------------------------------
    # Sobol 1111-dim — PROJECT_SOBOL_1111 (CMakeLists.txt, default ON)
    # -------------------------------------------------------------------------
    base_defines += select({
        "//bazel:disable_sobol_1111": [],
        "//conditions:default": ["PROJECT_SOBOL_1111=1"],
    })

    return base_defines

def quarisma_linkopts():
    """Returns common linker options for Quarisma targets."""
    return select({
        "@platforms//os:windows": [],
        "@platforms//os:macos": [
            "-undefined",
            "dynamic_lookup",
        ],
        "//conditions:default": [
            "-lpthread",
            "-ldl",
        ],
    })

def quarisma_enzyme_copts():
    """Returns Enzyme AD compile options.
    Mirrors CMake: target_compile_options(Enzyme::enzyme INTERFACE -fpass-plugin=<path>)

    The -fpass-plugin=<path> flag is Clang-only and cannot be auto-discovered in Bazel.
    Provide it via --per_file_copt in .bazelrc.user, restricted to Library/* to avoid
    leaking the Clang-only flag to GCC-compiled third-party targets:
      build:enzyme --per_file_copt=Library/.*@-fpass-plugin=/path/to/LLDEnzyme-XX.so

    DO NOT use global --copt: third-party targets (benchmark, googletest, etc.) may be
    compiled with GCC which does not recognise -fpass-plugin.
    This mirrors CMake's: target_link_libraries(Core PRIVATE Enzyme::enzyme)
    """
    return select({
        "//bazel:enable_enzyme": [],  # Plugin path supplied via .bazelrc.user --per_file_copt
        "//conditions:default": [],
    })

def quarisma_enzyme_linkopts():
    """Returns Enzyme AD link options.
    Mirrors CMake: target_link_options(Enzyme::enzyme INTERFACE -fpass-plugin=<path>)

    For non-LTO builds, Enzyme resolves __enzyme_* symbols at compile time (the pass
    transforms the IR in-place); no -fpass-plugin is needed at link time.
    For LTO builds, add to .bazelrc.user:
      build:enzyme --linkopt=-fpass-plugin=/path/to/LLDEnzyme-XX.so
    and ensure all link steps use Clang/lld (GCC does not support -fpass-plugin).
    """
    return select({
        "//bazel:enable_enzyme": [],  # Non-LTO: symbols resolved at compile time
        "//conditions:default": [],
    })

def quarisma_test_copts():
    """Returns compiler options for Quarisma test targets."""
    return quarisma_copts()

def quarisma_test_linkopts():
    """Returns linker options for Quarisma test targets."""
    return quarisma_linkopts()
