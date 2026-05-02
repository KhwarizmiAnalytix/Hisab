# =============================================================================
# Vectorization module — Bazel helpers
# Mirrors Library/Vectorization/CMakeLists.txt VECTORIZATION_* compile definitions.
# =============================================================================

load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

VECTORIZATION_CXX_STD = "c++20"

def _vectorization_target_copts():
    """Flags applied to Vectorization library/tests like CMake Vectorization target (MSVC /WX, Unix -include cstdlib)."""
    return select({
        "@platforms//os:windows": ["/WX"],
        "//conditions:default": [
            "-include",
            "cstdlib",
        ],
    })

def vectorization_copts():
    """Compiler options: C++ standard + ISA flags (tests/benchmarks; matches their CMake flags)."""
    return quarisma_copts(cxx_std = VECTORIZATION_CXX_STD) + vectorization_simd_copts()

def vectorization_library_copts():
    """vectorization_copts() plus MSVC /WX and Unix -include cstdlib (CMake Vectorization target only)."""
    return vectorization_copts() + _vectorization_target_copts()

def vectorization_simd_copts():
    """ISA flags — must match CMake Library/Vectorization/Cmake/utils.cmake.

    Flat select on //bazel:vec_*_{win,linux,macos} from define_vectorization_platform_settings().
    """
    msvc = {
        "no": [],
        "sse": [],
        "avx": ["/arch:AVX", "/D__F16C__"],
        "avx2": ["/arch:AVX2", "/D__F16C__", "/D__FMA__"],
        "avx512": [
            "/D__AVX512F__",
            "/D__AVX512DQ__",
            "/D__AVX512VL__",
            "/D__F16C__",
            "/D__FMA__",
        ],
    }
    unix = {
        "no": [],
        "sse": ["-msse4.2", "-msse4.1", "-msse2", "-msse"],
        "avx": ["-mavx", "-mf16c"],
        "avx2": ["-mavx2", "-mf16c", "-mfma"],
        "avx512": [
            "-mavx512f",
            "-mavx512dq",
            "-mavx512vl",
            "-mf16c",
            "-mfma",
        ],
    }
    d = {}
    for short in ("no", "sse", "avx", "avx2", "avx512"):
        d["//bazel:vec_%s_win" % short] = msvc[short]
        d["//bazel:vec_%s_linux" % short] = unix[short]
        d["//bazel:vec_%s_macos" % short] = unix[short]
    d["//conditions:default"] = unix["avx2"]
    return select(d)

def vectorization_defines():
    """Preprocessor defines for SIMD tier, packet size, optional SVML, Memory/Logging."""
    # Chain selects + lists (Starlark: cannot .append onto a select).
    return (
        quarisma_defines()
        + select({
            "//bazel:vectorization_type_no": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_VECTORIZED=0",
            ],
            "//bazel:vectorization_type_sse": [
                "VECTORIZATION_HAS_SSE=1",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx2": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=1",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx512": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=1",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//conditions:default": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=1",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
        })
        + select({
            "//bazel:enable_svml": ["VECTORIZATION_HAS_SVML=1"],
            "//conditions:default": ["VECTORIZATION_HAS_SVML=0"],
        })
        + select({
            "//bazel:disable_gtest": ["VECTORIZATION_HAS_GTEST=0"],
            "//conditions:default": ["VECTORIZATION_HAS_GTEST=1"],
        })
        + [
            "VECTORIZATION_HAS_MEMORY=1",
            "VECTORIZATION_HAS_LOGGING=1",
            "VECTORIZATION_PACKET_SIZE=4",
        ]
    )

def vectorization_linkopts():
    return quarisma_linkopts()
