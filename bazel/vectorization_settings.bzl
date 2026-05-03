# Config setting groups: (VECTORIZATION SIMD tier) × (host OS) for flat SIMD copts selects.
# AArch64 NEON/SVE add a CPU dimension so x86 hosts do not receive -march=armv8 flags.
load("@bazel_skylib//lib:selects.bzl", "selects")

def define_vectorization_platform_settings():
    """Registers //bazel:vec_{no,sse,avx,avx2,avx512}_{win,linux,macos} and
    //bazel:vec_{neon,sve}_{linux,macos,windows}_aarch64.
    """
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
                "@platforms//os:osx",
            ],
        )

    arm_specs = [
        ("neon", "vectorization_type_neon"),
        ("sve", "vectorization_type_sve"),
    ]
    for short, full in arm_specs:
        for os_suffix, os_label in (
            ("linux_aarch64", "@platforms//os:linux"),
            ("macos_aarch64", "@platforms//os:osx"),
            ("windows_aarch64", "@platforms//os:windows"),
        ):
            selects.config_setting_group(
                name = "vec_%s_%s" % (short, os_suffix),
                match_all = [
                    "//bazel:%s" % full,
                    os_label,
                    "@platforms//cpu:aarch64",
                ],
            )
