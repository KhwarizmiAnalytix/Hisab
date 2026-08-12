"""HIP/ROCm external repository for WORKSPACE — resolves install path on Linux
(mirrors CMake's find_package(hip), Library/Parallel/Cmake/hip.cmake... actually
Library/Memory/Cmake/hip.cmake).

HIP/ROCm is Unix-only in this project (hip.cmake fails fast on WIN32) -- this
rule does the same: it only probes on Linux, matching Memory's actual HIP
usage, which is host-side runtime linking only (gpu/gpu_runtime.h reuses the
CUDA allocator implementation with HIP API spellings; there's no separate
device-language .hip source needing hipcc in this codebase), so exposing the
ROCm runtime as a plain cc_library -- the same shape as cuda_configure.bzl's
:cudart -- is sufficient; no device-code compilation support is needed here.

Priority:
  1. ROCM_PATH or HIP_PATH (environment)
  2. Linux: /opt/rocm

Not verified against a real ROCm install (none available in this repo's
development environment) -- verified only for the not-found fail-fast path,
the same bar bazel/sleef_configure.bzl's fail-fast path was held to.
"""

def _is_windows(repository_ctx):
    name = repository_ctx.os.name.lower()
    return name == "windows" or name.startswith("win")

def _resolve_rocm_path(repository_ctx):
    env = repository_ctx.os.environ.get("ROCM_PATH") or repository_ctx.os.environ.get("HIP_PATH")
    if env:
        return env.replace("\\", "/").strip().rstrip("/")

    if repository_ctx.path("/opt/rocm").exists:
        return "/opt/rocm"
    return None

_SETUP_HINT = (
    "HIP/ROCm (--define memory_enable_hip=true or --define vectorization_enable_hip=true) " +
    "requires the ROCm toolkit. Install ROCm (https://rocm.docs.amd.com) to the default " +
    "/opt/rocm location, or set ROCM_PATH/HIP_PATH to your install root. HIP/ROCm is " +
    "Unix-only in this project (matches Library/Memory/Cmake/hip.cmake's and " +
    "Library/Vectorization/CMakeLists.txt's own WIN32 fail-fast) -- use " +
    "MEMORY_GPU_BACKEND=cuda / VECTORIZATION_GPU_BACKEND=cuda on Windows."
)

_MISSING_BUILD_FILE = """\
package(default_visibility = ["//visibility:public"])

genrule(
    name = "hip_missing",
    outs = ["hip_missing.cc"],
    cmd = "echo HIP_NOT_FOUND_SEE_MESSAGE_ABOVE >&2; exit 1",
    message = {message},
)

cc_library(
    name = "hip",
    srcs = [":hip_missing"],
)
""".format(message = repr(_SETUP_HINT))

_FOUND_BUILD_FILE = """\
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hip",
    srcs = glob([
        "rocm/lib/libamdhip64.so*",
    ], allow_empty = True),
    hdrs = glob([
        "rocm/include/**/*.h",
        "rocm/include/**/*.hpp",
    ], allow_empty = True),
    includes = [
        "rocm/include",
    ],
    linkopts = ["-Wl,-rpath,$$ORIGIN"],
)
"""

def _hip_configure_impl(repository_ctx):
    if _is_windows(repository_ctx):
        repository_ctx.file("BUILD.bazel", _MISSING_BUILD_FILE)
        return

    rocm_path = _resolve_rocm_path(repository_ctx)
    if not rocm_path:
        repository_ctx.file("BUILD.bazel", _MISSING_BUILD_FILE)
        return

    root = repository_ctx.path(rocm_path)
    if not root.exists:
        repository_ctx.file("BUILD.bazel", _MISSING_BUILD_FILE)
        return

    repository_ctx.symlink(root, "rocm")
    repository_ctx.file("BUILD.bazel", _FOUND_BUILD_FILE)

hip_configure = repository_rule(
    implementation = _hip_configure_impl,
    environ = ["ROCM_PATH", "HIP_PATH"],
)
