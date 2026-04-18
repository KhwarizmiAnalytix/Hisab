load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

def memory_copts():
    return quarisma_copts()

def memory_defines():
    """Returns compile definitions for Library/Memory.

    Mirrors Library/Memory/CMakeLists.txt MEMORY_HAS_* flags.
    """
    defines = quarisma_defines()

    # CUDA — MEMORY_HAS_CUDA
    defines += select({
        "//bazel:enable_cuda": ["MEMORY_HAS_CUDA=1"],
        "//conditions:default": ["MEMORY_HAS_CUDA=0"],
    })

    # HIP — MEMORY_HAS_HIP
    defines += select({
        "//bazel:enable_hip": ["MEMORY_HAS_HIP=1"],
        "//conditions:default": ["MEMORY_HAS_HIP=0"],
    })

    # TBB scalable allocator — MEMORY_HAS_TBB
    defines += select({
        "//bazel:enable_tbb": ["MEMORY_HAS_TBB=1"],
        "//conditions:default": ["MEMORY_HAS_TBB=0"],
    })

    # mimalloc — MEMORY_HAS_MIMALLOC (default ON; disabled on Windows)
    defines += select({
        "//bazel:disable_mimalloc": ["MEMORY_HAS_MIMALLOC=0"],
        "@platforms//os:windows": ["MEMORY_HAS_MIMALLOC=0"],
        "//conditions:default": ["MEMORY_HAS_MIMALLOC=1"],
    })

    # NUMA — MEMORY_HAS_NUMA (Unix only)
    defines += select({
        "//bazel:enable_numa": ["MEMORY_HAS_NUMA=1"],
        "//conditions:default": ["MEMORY_HAS_NUMA=0"],
    })

    # memkind — MEMORY_HAS_MEMKIND (Linux only)
    defines += select({
        "//bazel:enable_memkind": ["MEMORY_HAS_MEMKIND=1"],
        "//conditions:default": ["MEMORY_HAS_MEMKIND=0"],
    })

    # Per-allocator call-site tracking — MEMORY_HAS_ALLOCATION_STATS
    defines += select({
        "//bazel:enable_allocation_stats": ["MEMORY_HAS_ALLOCATION_STATS=1"],
        "//conditions:default": ["MEMORY_HAS_ALLOCATION_STATS=0"],
    })

    return defines

def memory_linkopts():
    return quarisma_linkopts()
