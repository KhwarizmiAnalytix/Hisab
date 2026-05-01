# Config setting groups: (VECTORIZATION SIMD tier) × (host OS) for flat SIMD copts selects.
load("@bazel_skylib//lib:selects.bzl", "selects")

def define_vectorization_platform_settings():
    """Registers //bazel:vec_{no,sse,avx,avx2,avx512}_{win,linux,macos}."""
    specs = [
        ("no", "vectorization_type_no"),
        ("sse", "vectorization_type_sse"),
        ("avx", "vectorization_type_avx"),
        ("avx2", "vectorization_type_avx2"),
        ("avx512", "vectorization_type_avx512"),
    ]
    for short, full in specs:
        selects.config_setting_group(
            name = "vec_%s_win" % short,
            match_all = [
                "//bazel:%s" % full,
                "@platforms//os:windows",
            ],
        )
        selects.config_setting_group(
            name = "vec_%s_linux" % short,
            match_all = [
                "//bazel:%s" % full,
                "@platforms//os:linux",
            ],
        )
        selects.config_setting_group(
            name = "vec_%s_macos" % short,
            match_all = [
                "//bazel:%s" % full,
                "@platforms//os:macos",
            ],
        )
