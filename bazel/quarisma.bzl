# =============================================================================
# Quarisma Bazel Helper Functions — Project-level
# =============================================================================
# Shared compiler flags, linker options, and project-wide compile definitions.
#
# Module-specific defines live in the corresponding module bzl files:
#   bazel/core.bzl, bazel/memory.bzl, bazel/parallel.bzl,
#   bazel/logging.bzl, bazel/profiler.bzl
# =============================================================================

def quarisma_copts(cxx_std = "c++20"):
    """Returns common compiler options for Quarisma targets.

    Args:
        cxx_std: C++ standard to use (default: c++20, matches CMake default).
                 Each module passes its own standard so targets are self-contained
                 and do not rely on --cxxopt in .bazelrc.
    """
    return select({
        "@platforms//os:windows": [
            "/std:" + cxx_std,
            "/Zc:__cplusplus",  # expose correct __cplusplus value (mirrors CMake /Zc:__cplusplus)
            "/EHsc",            # structured exception handling
            "/bigobj",          # large object files (mirrors CMake /bigobj)
            "/utf-8",           # UTF-8 source/output encoding (mirrors CMake /utf-8)
            "/wd4244",          # narrowing conversion (mirrors CMake /wd4244)
            "/wd4267",          # size_t → int conversion (mirrors CMake /wd4267)
            "/wd4715",          # not all control paths return (mirrors CMake /wd4715)
            "/wd4018",          # signed/unsigned comparison (mirrors CMake /wd4018)
        ],
        "//conditions:default": [
            "-std=" + cxx_std,
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ],
    })

def quarisma_defines():
    """Returns project-wide preprocessor defines.

    All module-specific HAS_* flags are owned by their respective module bzl
    files (core.bzl, memory.bzl, etc.) and are NOT included here.
    """
    return select({
        # Mirrors CMake add_definitions(-D_CRT_SECURE_NO_DEPRECATE ...) in compiler_checks.cmake
        "@platforms//os:windows": [
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_NONSTDC_NO_DEPRECATE",
            "_CRT_SECURE_NO_WARNINGS",
            "_SCL_SECURE_NO_DEPRECATE",
            "_SCL_SECURE_NO_WARNINGS",
        ],
        "//conditions:default": [],
    })

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

    The -fpass-plugin=<path> flag is Clang-only; supply it via .bazelrc.user:
      build:enzyme --per_file_copt=Library/.*@-fpass-plugin=/path/to/LLDEnzyme-XX.so
    """
    return select({
        "//bazel:enable_enzyme": [],  # Plugin path supplied via .bazelrc.user --per_file_copt
        "//conditions:default": [],
    })

def quarisma_enzyme_linkopts():
    """Returns Enzyme AD link options.

    For LTO builds add to .bazelrc.user:
      build:enzyme --linkopt=-fpass-plugin=/path/to/LLDEnzyme-XX.so
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
