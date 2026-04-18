# =============================================================================
# Quarisma Bazel Helper Functions — Project-level
# =============================================================================
# Shared compiler flags, linker options, and project-wide compile definitions.
#
# Module-specific defines live in the corresponding module bzl files:
#   bazel/core.bzl, bazel/memory.bzl, bazel/parallel.bzl,
#   bazel/logging.bzl, bazel/profiler.bzl
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
    """Returns project-wide preprocessor defines.

    All module-specific HAS_* flags are owned by their respective module bzl
    files (core.bzl, memory.bzl, etc.) and are NOT included here.
    """
    return []

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
