"""CUDA external repository for WORKSPACE — resolves install path per OS (mirrors CMake FindCUDA).

Priority:
  1. CUDA_PATH or CUDA_HOME (environment)
  2. Windows: newest CUDA under
     C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v*
  3. Linux: /usr/local/cuda, /opt/cuda
"""

def _is_windows(repository_ctx):
    name = repository_ctx.os.name.lower()
    return name == "windows" or name.startswith("win")

def _nonneg_int_segment(seg):
    """Parse a non-negative integer segment; return -1 if empty or non-digit."""
    if not seg:
        return 0
    for c in seg.elems():
        if c < "0" or c > "9":
            return -1
    return int(seg)

def _cuda_dir_version_key(basename):
    """Parse 'v12.6' / 'v11.8' into a tuple for ordering (not lexicographic on the string)."""
    if not basename.startswith("v"):
        return (-1, -1, -1)
    rest = basename[1:]
    parts = rest.split(".")
    maj = _nonneg_int_segment(parts[0]) if len(parts) > 0 else -1
    if maj < 0:
        return (-1, -1, -1)
    mnr = _nonneg_int_segment(parts[1]) if len(parts) > 1 else 0
    if len(parts) > 1 and mnr < 0:
        return (-1, -1, -1)
    pat = _nonneg_int_segment(parts[2]) if len(parts) > 2 else 0
    if len(parts) > 2 and pat < 0:
        return (-1, -1, -1)
    return (maj, mnr, pat)

def _resolve_cuda_path(repository_ctx):
    env = repository_ctx.os.environ.get("CUDA_PATH") or repository_ctx.os.environ.get("CUDA_HOME")
    if env:
        return env.replace("\\", "/").strip().rstrip("/")

    if _is_windows(repository_ctx):
        base = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA"
        p = repository_ctx.path(base)
        if not p.exists:
            return None
        best_name = None
        best_key = (-1, -1, -1)
        for d in p.readdir():
            b = d.basename
            if b.startswith("v"):
                k = _cuda_dir_version_key(b)
                if k[0] < 0:
                    continue
                if k > best_key:
                    best_key = k
                    best_name = b
        if best_name == None:
            return None
        return base + "/" + best_name

    for candidate in ("/usr/local/cuda", "/opt/cuda"):
        if repository_ctx.path(candidate).exists:
            return candidate
    return None

def _cuda_configure_impl(repository_ctx):
    cuda_path = _resolve_cuda_path(repository_ctx)
    if not cuda_path:
        fail(
            "CUDA toolkit not found. Install the NVIDIA CUDA Toolkit and either:\n" +
            "  - set CUDA_PATH (Windows: e.g. C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6), or\n" +
            "  - use the default Windows install path under Program Files/NVIDIA GPU Computing Toolkit/CUDA/v*\n" +
            "  - on Linux: install to /usr/local/cuda or set CUDA_PATH",
        )
    root = repository_ctx.path(cuda_path)
    if not root.exists:
        fail("CUDA_PATH resolves to a missing directory: " + cuda_path)

    repository_ctx.symlink(root, "cuda")

    if _is_windows(repository_ctx):
        build = _BUILD_CUDA_WINDOWS
    else:
        build = _BUILD_CUDA_LINUX
    repository_ctx.file("BUILD.bazel", build)

cuda_configure = repository_rule(
    implementation = _cuda_configure_impl,
    environ = ["CUDA_PATH", "CUDA_HOME"],
)

# -----------------------------------------------------------------------------
# BUILD files — all paths relative to repo root with embedded "cuda/" symlink tree
# -----------------------------------------------------------------------------

_BUILD_CUDA_LINUX = """
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "cudart",
    srcs = glob([
        "cuda/targets/x86_64-linux/lib/libcudart.so*",
        "cuda/lib64/libcudart.so*",
    ], allow_empty = True),
    hdrs = glob([
        "cuda/targets/x86_64-linux/include/**/*.h",
        "cuda/targets/x86_64-linux/include/**/*.hpp",
        "cuda/include/**/*.h",
        "cuda/include/**/*.hpp",
    ], allow_empty = True),
    includes = [
        "cuda/targets/x86_64-linux/include",
        "cuda/include",
    ],
    linkopts = ["-Wl,-rpath,$$ORIGIN"],
)

cc_library(
    name = "cupti",
    srcs = glob([
        "cuda/targets/x86_64-linux/lib/libcupti.so*",
        "cuda/targets/x86_64-linux/lib/libnvperf_host.so*",
        "cuda/targets/x86_64-linux/lib/libnvperf_target.so*",
        "cuda/extras/CUPTI/lib64/libcupti.so*",
        "cuda/lib64/libnvperf_host.so*",
        "cuda/lib64/libnvperf_target.so*",
    ], allow_empty = True),
    hdrs = glob([
        "cuda/targets/x86_64-linux/include/**/*.h",
        "cuda/targets/x86_64-linux/include/**/*.hpp",
        "cuda/extras/CUPTI/include/**/*.h",
        "cuda/extras/CUPTI/include/**/*.hpp",
        "cuda/include/**/*.h",
        "cuda/include/**/*.hpp",
    ], allow_empty = True),
    includes = [
        "cuda/targets/x86_64-linux/include",
        "cuda/extras/CUPTI/include",
        "cuda/include",
    ],
    linkopts = ["-Wl,-rpath,$$ORIGIN"],
    deps = [":cudart"],
)

cc_library(
    name = "cuda",
    deps = [
        ":cudart",
        ":cupti",
    ],
)
"""

_BUILD_CUDA_WINDOWS = """
package(default_visibility = ["//visibility:public"])

# MSVC: link import libraries from lib/x64 (CUDA 11+ on Windows)
cc_library(
    name = "cudart",
    hdrs = glob([
        "cuda/include/**/*.h",
        "cuda/include/**/*.hpp",
    ], allow_empty = True),
    includes = ["cuda/include"],
    srcs = glob(["cuda/lib/x64/cudart.lib"], allow_empty = True),
)

cc_library(
    name = "cupti",
    hdrs = glob([
        "cuda/extras/CUPTI/include/**/*.h",
        "cuda/extras/CUPTI/include/**/*.hpp",
    ], allow_empty = True),
    includes = [
        "cuda/extras/CUPTI/include",
        "cuda/include",
    ],
    srcs = glob(["cuda/extras/CUPTI/lib64/cupti.lib"], allow_empty = True),
    deps = [":cudart"],
)

cc_library(
    name = "cuda",
    deps = [
        ":cudart",
        ":cupti",
    ],
)
"""
