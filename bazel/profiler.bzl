load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

def profiler_copts():
    return quarisma_copts()

def profiler_defines():
    """Returns compile definitions for Library/Profiler.

    Mirrors Library/Profiler/CMakeLists.txt: PROFILER_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via quarisma_defines().
    """
    defines = quarisma_defines()

    # Profiler backend — mutually exclusive; default KINETO (matches CMake PROFILER_BACKEND default)
    # PROFILER_HAS_KINETO / PROFILER_HAS_ITT / PROFILER_HAS_NATIVE
    defines += select({
        "//bazel:enable_native_profiler": [
            "PROFILER_HAS_NATIVE=1",
            "PROFILER_HAS_KINETO=0",
            "PROFILER_HAS_ITT=0",
        ],
        "//bazel:enable_itt": [
            "PROFILER_HAS_ITT=1",
            "PROFILER_HAS_KINETO=0",
            "PROFILER_HAS_NATIVE=0",
        ],
        "//conditions:default": [
            "PROFILER_HAS_KINETO=1",
            "PROFILER_HAS_ITT=0",
            "PROFILER_HAS_NATIVE=0",
        ],
    })

    return defines

def profiler_linkopts():
    return quarisma_linkopts()
