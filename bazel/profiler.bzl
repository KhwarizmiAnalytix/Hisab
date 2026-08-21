load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

# C++ standard for Profiler — mirrors CMake PROFILER_CXX_STANDARD (default: 20)
PROFILER_CXX_STD = "c++20"

def profiler_copts():
    return quarisma_copts(cxx_std = PROFILER_CXX_STD)

def profiler_defines():
    """Returns compile definitions for Library/Profiler.

    Mirrors Library/Profiler/CMakeLists.txt: PROFILER_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via quarisma_defines().
    """
    defines = quarisma_defines()

    # Instrumentation backend — mutually exclusive; default KINETO (matches CMake
    # PROFILER_BACKEND default). PROFILER_HAS_KINETO / PROFILER_HAS_ITT
    defines += select({
        "//bazel:enable_itt": [
            "PROFILER_HAS_ITT=1",
            "PROFILER_HAS_KINETO=0",
        ],
        "//conditions:default": [
            "PROFILER_HAS_KINETO=1",
            "PROFILER_HAS_ITT=0",
        ],
    })

    # Native pipeline (traceme/xplane/host_tracer/profiler_session) is always compiled
    # alongside whichever instrumentation backend is selected above — no HAS_* gate.

    # PROFILER_HAS_CUDA — independent of backend selection above. Gates
    # bespoke/base/cuda.cpp's CUDA event-fallback stub and profiler_kineto.h's
    # hasCUDA(). Mirrors CMakeLists.txt's find_package(CUDAToolkit) gate.
    defines += select({
        "//bazel:enable_cuda": ["PROFILER_HAS_CUDA=1"],
        "//conditions:default": ["PROFILER_HAS_CUDA=0"],
    })

    return defines

def profiler_linkopts():
    return quarisma_linkopts()
