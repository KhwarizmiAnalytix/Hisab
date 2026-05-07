# =============================================================================
# Vectorization module — Bazel helpers
# Mirrors Library/Vectorization/CMakeLists.txt VECTORIZATION_* compile definitions.
# =============================================================================

load("@bazel_skylib//lib:selects.bzl", "selects")
load("@vectorization_svml_autodetect//:config.bzl", "SVML_NEEDED_AVX", "SVML_NEEDED_AVX2", "SVML_NEEDED_AVX512", "SVML_NEEDED_SSE")
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
    arm_neon = ["-march=armv8-a"]
    arm_sve = ["-march=armv8-a+sve", "-msve-vector-bits=128"]
    for os_suffix in ("linux_aarch64", "macos_aarch64", "windows_aarch64"):
        d["//bazel:vec_neon_%s" % os_suffix] = arm_neon
        d["//bazel:vec_sve_%s" % os_suffix] = arm_sve
    d["//bazel:cpu_x86_64"] = unix["avx2"]
    d["//bazel:cpu_aarch64"] = arm_neon
    d["//conditions:default"] = unix["avx2"]
    return select(d)

def _svml_macro(needed):
    """VECTORIZATION_HAS_SVML from autodetect bool (mirrors CMake VECTORIZATION_ENABLE_SVML)."""
    return ["VECTORIZATION_HAS_SVML=1"] if needed else ["VECTORIZATION_HAS_SVML=0"]

def _svml_autodetect_defines():
    return selects.with_or({
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve"): ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vectorization_type_sse": _svml_macro(SVML_NEEDED_SSE),
        "//bazel:vectorization_type_avx": _svml_macro(SVML_NEEDED_AVX),
        "//bazel:vectorization_type_avx2": _svml_macro(SVML_NEEDED_AVX2),
        "//bazel:vectorization_type_avx512": _svml_macro(SVML_NEEDED_AVX512),
        "//bazel:cpu_aarch64": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:cpu_x86_64": _svml_macro(SVML_NEEDED_AVX2),
        "//conditions:default": _svml_macro(SVML_NEEDED_AVX2),
    })

def vectorization_svml_defines():
    """SVML define: force on/off via --define, else autodetect (utils.cmake parity)."""
    return selects.with_or({
        ("//bazel:enable_svml",): ["VECTORIZATION_HAS_SVML=1"],
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve"): ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vectorization_type_sse": _svml_macro(SVML_NEEDED_SSE),
        "//bazel:vectorization_type_avx": _svml_macro(SVML_NEEDED_AVX),
        "//bazel:vectorization_type_avx2": _svml_macro(SVML_NEEDED_AVX2),
        "//bazel:vectorization_type_avx512": _svml_macro(SVML_NEEDED_AVX512),
        "//conditions:default": _svml_macro(SVML_NEEDED_AVX2),
    })

def vectorization_svml_deps():
    """Link @svml when HAS_SVML=1 for the active configuration."""
    return selects.with_or({
        ("//bazel:enable_svml",): ["@svml//:SVML"],
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve"): [],
        "//bazel:vectorization_type_sse": ["@svml//:SVML"] if SVML_NEEDED_SSE else [],
        "//bazel:vectorization_type_avx": ["@svml//:SVML"] if SVML_NEEDED_AVX else [],
        "//bazel:vectorization_type_avx2": ["@svml//:SVML"] if SVML_NEEDED_AVX2 else [],
        "//bazel:vectorization_type_avx512": ["@svml//:SVML"] if SVML_NEEDED_AVX512 else [],
        "//bazel:cpu_aarch64": [],
        "//bazel:cpu_x86_64": ["@svml//:SVML"] if SVML_NEEDED_AVX2 else [],
        "//conditions:default": ["@svml//:SVML"] if SVML_NEEDED_AVX2 else [],
    })

def vectorization_svml_hdrs_extra(svml_hdr):
    """Extra hdr for backend/avx/svml.h when it is excluded from the hdrs glob but SVML is used.

    SSE and AVX-512 ship their own backend/*/svml.h via the backend glob; only AVX / AVX2 need a
    re-added header when it was excluded from the glob.
    """
    return selects.with_or({
        ("//bazel:enable_svml",): [svml_hdr],
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve", "//bazel:vectorization_type_sse", "//bazel:vectorization_type_avx512"): [],
        "//bazel:vectorization_type_avx": [svml_hdr] if SVML_NEEDED_AVX else [],
        "//bazel:vectorization_type_avx2": [svml_hdr] if SVML_NEEDED_AVX2 else [],
        "//bazel:cpu_aarch64": [],
        "//bazel:cpu_x86_64": [svml_hdr] if SVML_NEEDED_AVX2 else [],
        "//conditions:default": [svml_hdr] if SVML_NEEDED_AVX2 else [],
    })

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
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=0",
            ],
            "//bazel:vectorization_type_sse": [
                "VECTORIZATION_HAS_SSE=1",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx2": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=1",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_avx512": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=1",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_neon": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=1",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:vectorization_type_sve": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=1",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//bazel:cpu_aarch64": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=1",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            "//conditions:default": [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=1",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
        })
        + vectorization_svml_defines()
        + vectorization_sleef_defines()
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

def vectorization_sleef_defines():
    """Expose VECTORIZATION_HAS_SLEEF consistently with CMake flags."""
    return select({
        "//bazel:enable_sleef": ["VECTORIZATION_HAS_SLEEF=1"],
        "//conditions:default": ["VECTORIZATION_HAS_SLEEF=0"],
    })

def vectorization_sleef_deps():
    """Prebuilt SLEEF dependency used when --config=sleef is enabled."""
    return select({
        "//bazel:enable_sleef": ["//:sleef"],
        "//conditions:default": [],
    })
