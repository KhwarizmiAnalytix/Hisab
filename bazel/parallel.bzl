load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

def parallel_copts():
    return quarisma_copts()

def parallel_defines():
    """Returns compile definitions for Library/Parallel.

    Mirrors Library/Parallel/CMakeLists.txt: PARALLEL_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via quarisma_defines().
    """
    defines = quarisma_defines()

    # Threading — PARALLEL_HAS_PTHREADS / PARALLEL_HAS_WIN32_THREADS
    defines += select({
        "@platforms//os:windows": [
            "PARALLEL_HAS_WIN32_THREADS=1",
            "PARALLEL_HAS_PTHREADS=0",
        ],
        "//conditions:default": [
            "PARALLEL_HAS_PTHREADS=1",
            "PARALLEL_HAS_WIN32_THREADS=0",
        ],
    })

    # TBB multithreading — PARALLEL_HAS_TBB
    defines += select({
        "//bazel:enable_tbb": ["PARALLEL_HAS_TBB=1"],
        "//conditions:default": ["PARALLEL_HAS_TBB=0"],
    })

    # OpenMP — PARALLEL_HAS_OPENMP
    defines += select({
        "//bazel:enable_openmp": ["PARALLEL_HAS_OPENMP=1"],
        "//conditions:default": ["PARALLEL_HAS_OPENMP=0"],
    })

    return defines

def parallel_linkopts():
    return quarisma_linkopts()
