load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

def logging_copts():
    return quarisma_copts()

def logging_defines():
    """Returns compile definitions for Library/Logging.

    Mirrors Library/Logging/CMakeLists.txt: LOGGING_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via quarisma_defines().
    """
    defines = quarisma_defines()

    # Logging backend — mutually exclusive; default LOGURU (matches CMake LOGGING_BACKEND default)
    # LOGGING_HAS_LOGURU / LOGGING_HAS_GLOG / LOGGING_HAS_NATIVE
    defines += select({
        "//bazel:logging_glog": [
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=1",
            "LOGGING_HAS_NATIVE=0",
        ],
        "//bazel:logging_native": [
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=1",
        ],
        "//conditions:default": [  # LOGURU is the default
            "LOGGING_HAS_LOGURU=1",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
        ],
    })

    return defines

def logging_linkopts():
    return quarisma_linkopts()
