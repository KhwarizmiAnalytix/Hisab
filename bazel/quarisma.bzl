# =============================================================================
# Quarisma Bazel Helper Functions and Macros
# =============================================================================
# Common functions for compiler flags, defines, and link options
# Equivalent to various CMake modules in Cmake/tools and Cmake/flags
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
    """Returns common preprocessor defines for Quarisma targets."""
    base_defines = [
        # Threading configuration (matches Cmake/tools/threads.cmake)
        "QUARISMA_MAX_THREADS=64",
    ]

    # Platform-specific threading defines
    base_defines += select({
        "@platforms//os:windows": [
            "QUARISMA_USE_WIN32_THREADS=1",
            "QUARISMA_USE_PTHREADS=0",
        ],
        "//conditions:default": [
            "QUARISMA_USE_PTHREADS=1",
            "QUARISMA_USE_WIN32_THREADS=0",
        ],
    })

    # Add feature-specific defines based on build configuration
    return base_defines + select({
        "//bazel:enable_cuda": ["QUARISMA_ENABLE_CUDA", "QUARISMA_HAS_CUDA=1"],
        "//conditions:default": ["QUARISMA_HAS_CUDA=0"],
    }) + select({
        "//bazel:enable_hip": ["QUARISMA_ENABLE_HIP", "QUARISMA_HAS_HIP=1"],
        "//conditions:default": ["QUARISMA_HAS_HIP=0"],
    }) + select({
        "//bazel:enable_tbb": ["QUARISMA_HAS_TBB"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_mkl": ["QUARISMA_ENABLE_MKL"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_mimalloc": ["QUARISMA_ENABLE_MIMALLOC"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_magic_enum": ["QUARISMA_ENABLE_MAGICENUM"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_kineto": ["PROFILER_ENABLE_KINETO", "PROFILER_HAS_KINETO=1"],
        "//conditions:default": ["PROFILER_HAS_KINETO=0"],
    }) + select({
        "//bazel:enable_native_profiler": ["PROFILER_ENABLE_NATIVE_PROFILER"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_itt": ["PROFILER_ENABLE_ITT"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_openmp": ["QUARISMA_ENABLE_OPENMP"],
        "//conditions:default": [],
    }) + select({
        "//bazel:lu_pivoting": ["QUARISMA_LU_PIVOTING"],
        "//conditions:default": [],
    }) + select({
        "//bazel:sobol_1111": ["QUARISMA_SOBOL_1111"],
        "//conditions:default": [],
    }) + select({
        "//bazel:enable_enzyme": ["QUARISMA_HAS_ENZYME=1"],
        "//conditions:default": ["QUARISMA_HAS_ENZYME=0"],
    }) + select({
        "//bazel:logging_glog": ["QUARISMA_USE_GLOG"],
        "//bazel:logging_loguru": ["QUARISMA_USE_LOGURU"],
        "//bazel:logging_native": ["QUARISMA_USE_NATIVE_LOGGING"],
        "//conditions:default": [],
    }) + select({
        "//bazel:gpu_alloc_sync": ["QUARISMA_GPU_ALLOC_SYNC"],
        "//bazel:gpu_alloc_async": ["QUARISMA_GPU_ALLOC_ASYNC"],
        "//bazel:gpu_alloc_pool_async": ["QUARISMA_GPU_ALLOC_POOL_ASYNC"],
        "//conditions:default": ["QUARISMA_GPU_ALLOC_POOL_ASYNC"],  # Default
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

def quarisma_test_copts():
    """Returns compiler options for Quarisma test targets."""
    return quarisma_copts()

def quarisma_test_linkopts():
    """Returns linker options for Quarisma test targets."""
    return quarisma_linkopts()
