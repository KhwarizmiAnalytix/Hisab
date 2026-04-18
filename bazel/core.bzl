load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_enzyme_copts", "quarisma_enzyme_linkopts", "quarisma_linkopts")

def core_copts():
    return quarisma_copts()

def core_defines():
    """Returns compile definitions for Library/Core.

    Mirrors Library/Core/CMakeLists.txt CORE_HAS_* and CORE_* flags.
    """
    defines = quarisma_defines()

    # MKL — CORE_HAS_MKL
    defines += select({
        "//bazel:enable_mkl": ["CORE_HAS_MKL=1"],
        "//conditions:default": ["CORE_HAS_MKL=0"],
    })

    # SVML — CORE_HAS_SVML
    defines += select({
        "//bazel:enable_svml": ["CORE_HAS_SVML=1"],
        "//conditions:default": ["CORE_HAS_SVML=0"],
    })

    # ROCm — CORE_HAS_ROCM
    defines += select({
        "//bazel:enable_rocm": ["CORE_HAS_ROCM=1"],
        "//conditions:default": ["CORE_HAS_ROCM=0"],
    })

    # Experimental API gate — CORE_HAS_EXPERIMENTAL
    defines += select({
        "//bazel:enable_experimental": ["CORE_HAS_EXPERIMENTAL=1"],
        "//conditions:default": ["CORE_HAS_EXPERIMENTAL=0"],
    })

    # magic_enum static reflection — CORE_HAS_MAGICENUM (default ON)
    defines += select({
        "//bazel:disable_magic_enum": ["CORE_HAS_MAGICENUM=0"],
        "//conditions:default": ["CORE_HAS_MAGICENUM=1"],
    })

    # Google Test availability — CORE_HAS_GTEST
    defines += select({
        "//bazel:enable_gtest": ["CORE_HAS_GTEST=1"],
        "//conditions:default": ["CORE_HAS_GTEST=0"],
    })

    # LU pivoting — CORE_LU_PIVOTING (only defined when ON, matching CMake)
    defines += select({
        "//bazel:lu_pivoting": ["CORE_LU_PIVOTING=1"],
        "//conditions:default": [],
    })

    # Sobol 1111-dim — CORE_SOBOL_1111 (defined when ON, absent when OFF, matching CMake)
    defines += select({
        "//bazel:disable_sobol_1111": [],
        "//conditions:default": ["CORE_SOBOL_1111=1"],
    })

    # Enzyme AD — CORE_HAS_ENZYME
    defines += select({
        "//bazel:enable_enzyme": ["CORE_HAS_ENZYME=1"],
        "//conditions:default": ["CORE_HAS_ENZYME=0"],
    })

    # Compression — CORE_HAS_COMPRESSION / CORE_COMPRESSION_TYPE_SNAPPY
    defines += select({
        "//bazel:enable_compression_snappy": [
            "CORE_HAS_COMPRESSION=1",
            "CORE_COMPRESSION_TYPE_SNAPPY=1",
        ],
        "//bazel:enable_compression": [
            "CORE_HAS_COMPRESSION=1",
            "CORE_COMPRESSION_TYPE_SNAPPY=0",
        ],
        "//conditions:default": [
            "CORE_HAS_COMPRESSION=0",
            "CORE_COMPRESSION_TYPE_SNAPPY=0",
        ],
    })

    return defines

def core_linkopts():
    return quarisma_linkopts()

def core_enzyme_copts():
    return quarisma_enzyme_copts()

def core_enzyme_linkopts():
    return quarisma_enzyme_linkopts()
