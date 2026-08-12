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
        # CMake: /arch:SSE2
        "sse": ["/arch:SSE2"],
        "avx": ["/arch:AVX", "/D__F16C__"],
        "avx2": ["/arch:AVX2", "/D__F16C__", "/D__FMA__"],
        "avx512": [
            # CMake: /arch:AVX512 implies the feature macros; keep explicit defines for parity.
            "/arch:AVX512",
            "/D__AVX512F__",
            "/D__AVX512CD__",
            "/D__AVX512BW__",
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
        # _aarch64 counterparts (same value): strict supersets of "cpu_aarch64" below, so an
        # explicit --define=vectorization_type=X on an aarch64 host resolves unambiguously
        # instead of erroring against "cpu_aarch64" (see vectorization_settings.bzl).
        d["//bazel:vec_%s_win_aarch64" % short] = msvc[short]
        d["//bazel:vec_%s_linux_aarch64" % short] = unix[short]
        d["//bazel:vec_%s_macos_aarch64" % short] = unix[short]
    arm_neon = ["-march=armv8-a"]
    arm_sve = ["-march=armv8-a+sve", "-msve-vector-bits=128"]
    for os_suffix in ("linux_aarch64", "macos_aarch64", "windows_aarch64"):
        d["//bazel:vec_neon_%s" % os_suffix] = arm_neon
        d["//bazel:vec_sve_%s" % os_suffix] = arm_sve
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
        "//conditions:default": _svml_macro(SVML_NEEDED_AVX2),
    })

def vectorization_svml_defines():
    """SVML define: force on/off via --define, else autodetect (utils.cmake parity)."""
    # Use the (vectorization_type × OS) config_setting_groups so matches are unambiguous
    # (Bazel requires exactly one matching key in a select()).
    return selects.with_or({
        ("//bazel:enable_svml",): ["VECTORIZATION_HAS_SVML=1"],
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve"): ["VECTORIZATION_HAS_SVML=0"],
        # Windows toolchains provide vector-math intrinsics in headers; do not require SVML.
        "//bazel:vec_no_win": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_sse_win": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_avx_win": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_avx2_win": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_avx512_win": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_no_linux": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_sse_linux": _svml_macro(SVML_NEEDED_SSE),
        "//bazel:vec_avx_linux": _svml_macro(SVML_NEEDED_AVX),
        "//bazel:vec_avx2_linux": _svml_macro(SVML_NEEDED_AVX2),
        "//bazel:vec_avx512_linux": _svml_macro(SVML_NEEDED_AVX512),
        "//bazel:vec_no_macos": ["VECTORIZATION_HAS_SVML=0"],
        "//bazel:vec_sse_macos": _svml_macro(SVML_NEEDED_SSE),
        "//bazel:vec_avx_macos": _svml_macro(SVML_NEEDED_AVX),
        "//bazel:vec_avx2_macos": _svml_macro(SVML_NEEDED_AVX2),
        "//bazel:vec_avx512_macos": _svml_macro(SVML_NEEDED_AVX512),
        "//conditions:default": _svml_macro(SVML_NEEDED_AVX2),
    })

def vectorization_svml_deps():
    """Link @svml when HAS_SVML=1 for the active configuration."""
    return selects.with_or({
        ("//bazel:enable_svml",): ["@svml//:SVML"],
        ("//bazel:disable_svml", "//bazel:vectorization_type_no", "//bazel:vectorization_type_neon", "//bazel:vectorization_type_sve"): [],
        ("//bazel:vec_no_win", "//bazel:vec_no_win_aarch64"): [],
        ("//bazel:vec_sse_win", "//bazel:vec_sse_win_aarch64"): [],
        ("//bazel:vec_avx_win", "//bazel:vec_avx_win_aarch64"): [],
        ("//bazel:vec_avx2_win", "//bazel:vec_avx2_win_aarch64"): [],
        ("//bazel:vec_avx512_win", "//bazel:vec_avx512_win_aarch64"): [],
        ("//bazel:vec_no_linux", "//bazel:vec_no_linux_aarch64"): [],
        ("//bazel:vec_sse_linux", "//bazel:vec_sse_linux_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_SSE else [],
        ("//bazel:vec_avx_linux", "//bazel:vec_avx_linux_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX else [],
        ("//bazel:vec_avx2_linux", "//bazel:vec_avx2_linux_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX2 else [],
        ("//bazel:vec_avx512_linux", "//bazel:vec_avx512_linux_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX512 else [],
        ("//bazel:vec_no_macos", "//bazel:vec_no_macos_aarch64"): [],
        ("//bazel:vec_sse_macos", "//bazel:vec_sse_macos_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_SSE else [],
        ("//bazel:vec_avx_macos", "//bazel:vec_avx_macos_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX else [],
        ("//bazel:vec_avx2_macos", "//bazel:vec_avx2_macos_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX2 else [],
        ("//bazel:vec_avx512_macos", "//bazel:vec_avx512_macos_aarch64"): ["@svml//:SVML"] if SVML_NEEDED_AVX512 else [],
        "//bazel:cpu_aarch64": [],
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
        ("//bazel:vectorization_type_avx", "//bazel:vectorization_type_avx_aarch64"): [svml_hdr] if SVML_NEEDED_AVX else [],
        ("//bazel:vectorization_type_avx2", "//bazel:vectorization_type_avx2_aarch64"): [svml_hdr] if SVML_NEEDED_AVX2 else [],
        "//bazel:cpu_aarch64": [],
        "//conditions:default": [svml_hdr] if SVML_NEEDED_AVX2 else [],
    })

def vectorization_defines():
    """Preprocessor defines for SIMD tier, optional SVML, Memory/Logging."""
    # Chain selects + lists (Starlark: cannot .append onto a select).
    return (
        quarisma_defines()
        # Each tier is paired with its "_aarch64" counterpart (same defines, plus the aarch64
        # constraint) via selects.with_or(): on an aarch64 host, the paired setting is a strict
        # superset of "cpu_aarch64" below, so Bazel's select() picks it over the arch-based
        # default instead of erroring out as an ambiguous match (see the vectorization_type_X_aarch64
        # config_setting comment in bazel/BUILD.bazel for the full explanation).
        + selects.with_or({
            (
                "//bazel:vectorization_type_no",
                "//bazel:vectorization_type_no_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=0",
            ],
            (
                "//bazel:vectorization_type_sse",
                "//bazel:vectorization_type_sse_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=1",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            (
                "//bazel:vectorization_type_avx",
                "//bazel:vectorization_type_avx_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            (
                "//bazel:vectorization_type_avx2",
                "//bazel:vectorization_type_avx2_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=1",
                "VECTORIZATION_HAS_AVX2=1",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            (
                "//bazel:vectorization_type_avx512",
                "//bazel:vectorization_type_avx512_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=1",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            (
                "//bazel:vectorization_type_neon",
                "//bazel:vectorization_type_neon_aarch64",
                "//bazel:cpu_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=1",
                "VECTORIZATION_HAS_SVE=0",
                "VECTORIZATION_VECTORIZED=1",
            ],
            (
                "//bazel:vectorization_type_sve",
                "//bazel:vectorization_type_sve_aarch64",
            ): [
                "VECTORIZATION_HAS_SSE=0",
                "VECTORIZATION_HAS_AVX=0",
                "VECTORIZATION_HAS_AVX2=0",
                "VECTORIZATION_HAS_AVX512=0",
                "VECTORIZATION_HAS_NEON=0",
                "VECTORIZATION_HAS_SVE=1",
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
        # Apple Accelerate vForce (NEON). CMake's compile_definition() helper (Cmake/flags/
        # compile_definitions.cmake) maps the VECTORIZATION_ENABLE_ACCELERATE *option* to a
        # VECTORIZATION_HAS_ACCELERATE *compile definition* (ENABLE -> HAS substring rename) --
        # that HAS_ name is what backend/cpu/neon/svml.h actually guards on.
        + select({
            "//bazel:enable_accelerate": ["VECTORIZATION_HAS_ACCELERATE=1"],
            "//conditions:default": ["VECTORIZATION_HAS_ACCELERATE=0"],
        })
        # MKL VML backend (x86_64 only, matching CMakeLists.txt:434-438's processor guard).
        # No @mkl WORKSPACE dependency is wired here (see bazel/vectorization.bzl's deps -- not
        # added -- and WORKSPACE.bazel:196-202's own commented-out @mkl repo): there's no real
        # MKL install anywhere to verify a hermetic Bazel dep against, on this machine or
        # otherwise, so only the *definition* side mirrors CMake exactly; enabling this flag on
        # Bazel today only affects the macro, matching CMake's fallback-to-off behavior when
        # MKL isn't found, but without CMake's own warning message.
        + select({
            "//bazel:vectorization_mkl_x86_64": ["VECTORIZATION_HAS_MKL=1"],
            "//conditions:default": ["VECTORIZATION_HAS_MKL=0"],
        })
        + vectorization_packet_size_define()
        # GPU backend (mutually exclusive, matches CMakeLists.txt:337-352's
        # if/elif/elif/else on VECTORIZATION_GPU_BACKEND).
        + select({
            "//bazel:vectorization_enable_cuda": [
                "VECTORIZATION_HAS_CUDA=1",
                "VECTORIZATION_HAS_HIP=0",
                "VECTORIZATION_HAS_METAL=0",
            ],
            "//bazel:vectorization_enable_hip": [
                "VECTORIZATION_HAS_CUDA=0",
                "VECTORIZATION_HAS_HIP=1",
                "VECTORIZATION_HAS_METAL=0",
            ],
            "//bazel:vectorization_enable_metal": [
                "VECTORIZATION_HAS_CUDA=0",
                "VECTORIZATION_HAS_HIP=0",
                "VECTORIZATION_HAS_METAL=1",
            ],
            "//conditions:default": [
                "VECTORIZATION_HAS_CUDA=0",
                "VECTORIZATION_HAS_HIP=0",
                "VECTORIZATION_HAS_METAL=0",
            ],
        })
        + [
            "VECTORIZATION_HAS_MEMORY=1",
            "VECTORIZATION_HAS_LOGGING=1",
        ]
    )

def vectorization_gpu_deps():
    """CUDA/HIP runtime deps for Vectorization's own GPU backend (VECTORIZATION_GPU_BACKEND).

    No device-language (.cu/.hip) compilation is needed for the *library* target itself --
    Vectorization's only CMake-side use of enable_language(CUDA)/enable_language(HIP) is for
    device-tagging specific *test*/*benchmark* files (Testing/Cxx/CMakeLists.txt), which have
    no Bazel equivalent (same class of gap as Core's CudaEnzymeADTest.cu -- see
    Docs/BAZEL_USER_GUIDE.md). The library itself only needs the CUDA/HIP runtime headers/libs,
    reusing the same @local_config_cuda / @local_config_hip repos Memory's GPU backend uses.
    """
    return select({
        "//bazel:vectorization_enable_cuda": ["@local_config_cuda//:cuda"],
        "//bazel:vectorization_enable_hip": ["@local_config_hip//:hip"],
        "//conditions:default": [],
    })

def vectorization_packet_size_define():
    """VECTORIZATION_PACKET_SIZE=N from --//bazel:vectorization_packet_size (default 1).

    Mirrors CMakeLists.txt:201-217's VECTORIZATION_PACKET_SIZE cache var (bounded [1,16]).
    """
    return select({
        "//bazel:vectorization_packet_size_%d" % n: ["VECTORIZATION_PACKET_SIZE=%d" % n]
        for n in range(1, 17)
    })

def vectorization_linkopts():
    return quarisma_linkopts() + select({
        # Mirrors CMakeLists.txt:784-786 (if(VECTORIZATION_ENABLE_ACCELERATE AND APPLE)).
        "//bazel:vec_accelerate_macos": ["-framework", "Accelerate"],
        "//conditions:default": [],
    }) + select({
        # Mirrors CMakeLists.txt:719 (target_link_libraries(Vectorization PUBLIC
        # ${METAL_FRAMEWORK} ${FOUNDATION_FRAMEWORK})) for Vectorization's own Metal GPU
        # backend (separate from Memory's, see Library/Memory/BUILD.bazel).
        "//bazel:vectorization_enable_metal": ["-framework", "Metal", "-framework", "Foundation"],
        "//conditions:default": [],
    })

def vectorization_sleef_defines():
    """Expose VECTORIZATION_HAS_SLEEF consistently with CMake flags."""
    return select({
        "//bazel:enable_sleef": ["VECTORIZATION_HAS_SLEEF=1"],
        "//conditions:default": ["VECTORIZATION_HAS_SLEEF=0"],
    })

def vectorization_sleef_deps():
    """Prebuilt SLEEF dependency used when --config=sleef is enabled.

    Resolved by the @sleef_cmake repository rule (see
    bazel/sleef_configure.bzl), which locates CMake-built SLEEF artifacts or
    fails with an actionable message if none are found.
    """
    return select({
        "//bazel:enable_sleef": ["@sleef_cmake//:sleef"],
        "//conditions:default": [],
    })
