load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

# C++ standard for Logging — mirrors CMake LOGGING_CXX_STANDARD (default: 20)
LOGGING_CXX_STD = "c++20"

def logging_copts():
    return quarisma_copts(cxx_std = LOGGING_CXX_STD)

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
            "LOGGING_HAS_SPDLOG=0",
        ],
        "//bazel:logging_native": [
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=1",
            "LOGGING_HAS_SPDLOG=0",
        ],
        "//bazel:logging_spdlog": [
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=1",
            "SPDLOG_FMT_EXTERNAL=1",
        ],
        "//bazel:logging_loguru": [
            "LOGGING_HAS_LOGURU=1",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=0",
        ],
        "//conditions:default": [  # LOGURU when no --define=logging_backend (matches CMake default)
            "LOGGING_HAS_LOGURU=1",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=0",
        ],
    })

    return defines

def logging_linkopts():
    return quarisma_linkopts()
