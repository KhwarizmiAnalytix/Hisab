load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_enzyme_copts", "quarisma_enzyme_linkopts", "quarisma_linkopts")

def core_copts():
    return quarisma_copts()

def core_defines():
    """Returns compile definitions for Library/Core.

    Mirrors Library/Core/CMakeLists.txt: CORE_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via quarisma_defines().
    """
    defines = quarisma_defines()

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
