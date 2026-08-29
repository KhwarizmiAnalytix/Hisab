load("//bazel:xsigma.bzl", "xsigma_copts", "xsigma_defines", "xsigma_linkopts")

# C++ standard for Logging — mirrors CMake LOGGING_CXX_STANDARD (default: 20)
LOGGING_CXX_STD = "c++20"

def logging_copts():
    return xsigma_copts(cxx_std = LOGGING_CXX_STD)

def logging_defines():
    """Returns compile definitions for Library/Logging.

    Mirrors Library/Logging/CMakeLists.txt: LOGGING_HAS_* flags.
    Project-wide PROJECT_HAS_* flags are included via xsigma_defines().
    """
    defines = xsigma_defines()

    # Logging backend — mutually exclusive; default SPDLOG (matches CMake LOGGING_BACKEND default)
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
        "//bazel:logging_loguru": [
            "LOGGING_HAS_LOGURU=1",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=0",
        ],
        "//bazel:logging_spdlog": [
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=1",
            "SPDLOG_FMT_EXTERNAL=1",
        ],
        "//conditions:default": [  # SPDLOG when no --define=logging_backend (matches CMake default)
            "LOGGING_HAS_LOGURU=0",
            "LOGGING_HAS_GLOG=0",
            "LOGGING_HAS_NATIVE=0",
            "LOGGING_HAS_SPDLOG=1",
            "SPDLOG_FMT_EXTERNAL=1",
        ],
    })

    defines += select({
        "//bazel:disable_magic_enum": ["LOGGING_HAS_MAGICENUM=0"],
        "//conditions:default": ["LOGGING_HAS_MAGICENUM=1"],
    })

    return defines

def logging_linkopts():
    return xsigma_linkopts()
