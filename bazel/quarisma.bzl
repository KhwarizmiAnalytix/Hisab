# =============================================================================
# Quarisma Bazel Helper Functions and Macros
# =============================================================================
# Common functions for compiler flags, defines, and link options.
# Defaults match root CMakeLists.txt + Cmake/tools (QUARISMA_* / PROFILER_*).
# Opt out with --define=quarisma_enable_*=false where disable_* config_settings exist.
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
    """Returns common preprocessor defines for Quarisma targets (CMake-equivalent defaults)."""
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

    # CUDA / HIP (opt-in)
    base_defines += select({
        "//bazel:enable_cuda": ["QUARISMA_ENABLE_CUDA", "QUARISMA_HAS_CUDA=1"],
        "//conditions:default": ["QUARISMA_HAS_CUDA=0"],
    })
    base_defines += select({
        "//bazel:enable_hip": ["QUARISMA_ENABLE_HIP", "QUARISMA_HAS_HIP=1"],
        "//conditions:default": ["QUARISMA_HAS_HIP=0"],
    })

    # TBB / MKL / OpenMP (opt-in)
    base_defines += select({
        "//bazel:enable_tbb": ["QUARISMA_HAS_TBB"],
        "//conditions:default": [],
    })
    base_defines += select({
        "//bazel:enable_mkl": ["QUARISMA_ENABLE_MKL"],
        "//conditions:default": [],
    })
    base_defines += select({
        "//bazel:enable_openmp": ["QUARISMA_ENABLE_OPENMP"],
        "//conditions:default": [],
    })

    # mimalloc — CMake default ON; code uses QUARISMA_HAS_MIMALLOC (compile_definitions.cmake).
    # Bazel //Library/Core defaults to static linking on Windows; mimalloc's MSVC override
    # requires a DLL build — match by disabling mimalloc there (use shared_libs or disable_mimalloc).
    base_defines += select({
        "//bazel:disable_mimalloc": ["QUARISMA_HAS_MIMALLOC=0"],
        "@platforms//os:windows": ["QUARISMA_HAS_MIMALLOC=0"],
        "//conditions:default": ["QUARISMA_HAS_MIMALLOC=1"],
    })

    # magic_enum — CMake default ON (QUARISMA_HAS_MAGICENUM in compile_definitions.cmake)
    base_defines += select({
        "//bazel:disable_magic_enum": ["QUARISMA_HAS_MAGICENUM=0"],
        "//conditions:default": ["QUARISMA_HAS_MAGICENUM=1"],
    })

    # Profiler backends — mutually exclusive; default KINETO (QUARISMA_PROFILER_TYPE default)
    base_defines += select({
        "//bazel:enable_native_profiler": [
            "PROFILER_HAS_NATIVE_PROFILER=1",
            "PROFILER_HAS_KINETO=0",
            "PROFILER_HAS_ITT=0",
        ],
        "//bazel:enable_itt": [
            "PROFILER_HAS_ITT=1",
            "PROFILER_HAS_KINETO=0",
            "PROFILER_HAS_NATIVE_PROFILER=0",
        ],
        "//conditions:default": [
            "PROFILER_HAS_KINETO=1",
            "PROFILER_HAS_ITT=0",
            "PROFILER_HAS_NATIVE_PROFILER=0",
        ],
    })

    # LU pivoting: default OFF — only when //bazel:lu_pivoting
    base_defines += select({
        "//bazel:lu_pivoting": ["QUARISMA_LU_PIVOTING"],
        "//conditions:default": [],
    })

    # Sobol 1111: default ON (QUARISMA_SOBOL_1111=ON in CMake)
    base_defines += select({
        "//bazel:disable_sobol_1111": [],
        "//conditions:default": ["QUARISMA_SOBOL_1111=1"],
    })

    # Enzyme (opt-in)
    base_defines += select({
        "//bazel:enable_enzyme": ["QUARISMA_HAS_ENZYME=1"],
        "//conditions:default": ["QUARISMA_HAS_ENZYME=0"],
    })

    # Logging — default LOGURU (QUARISMA_LOGGING_BACKEND default in Cmake/tools/logging.cmake)
    base_defines += select({
        "//bazel:logging_glog": ["QUARISMA_USE_GLOG"],
        "//bazel:logging_loguru": ["QUARISMA_USE_LOGURU"],
        "//bazel:logging_native": ["QUARISMA_USE_NATIVE_LOGGING"],
        "//conditions:default": ["QUARISMA_USE_LOGURU"],
    })

    # GPU allocation — default POOL_ASYNC
    base_defines += select({
        "//bazel:gpu_alloc_sync": ["QUARISMA_GPU_ALLOC_SYNC"],
        "//bazel:gpu_alloc_async": ["QUARISMA_GPU_ALLOC_ASYNC"],
        "//bazel:gpu_alloc_pool_async": ["QUARISMA_GPU_ALLOC_POOL_ASYNC"],
        "//conditions:default": ["QUARISMA_GPU_ALLOC_POOL_ASYNC"],
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
    Mirrors CMake: target_compile_options(Quarisma::enzyme INTERFACE -fpass-plugin=<path>)

    The -fpass-plugin=<path> flag is Clang-only and cannot be auto-discovered in Bazel.
    Provide it via --per_file_copt in .bazelrc.user, restricted to Library/* to avoid
    leaking the Clang-only flag to GCC-compiled third-party targets:
      build:enzyme --per_file_copt=Library/.*@-fpass-plugin=/path/to/LLDEnzyme-XX.so

    DO NOT use global --copt: third-party targets (benchmark, googletest, etc.) may be
    compiled with GCC which does not recognise -fpass-plugin.
    This mirrors CMake's: target_link_libraries(Core PRIVATE Quarisma::enzyme)
    """
    return select({
        "//bazel:enable_enzyme": [],  # Plugin path supplied via .bazelrc.user --per_file_copt
        "//conditions:default": [],
    })

def quarisma_enzyme_linkopts():
    """Returns Enzyme AD link options.
    Mirrors CMake: target_link_options(Quarisma::enzyme INTERFACE -fpass-plugin=<path>)

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
