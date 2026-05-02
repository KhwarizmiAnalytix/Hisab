# ThirdParty/svml.BUILD — Bazel import targets for Intel SVML
#
# Mirrors CMake FindSVML.cmake:
#   Windows  : link svml_dispmd.lib, stage svml_dispmd.dll
#   Unix     : link libsvml.so + libirc.so + libintlc.so
#
# Repository root = ThirdParty/svml  (set via new_local_repository)

package(default_visibility = ["//visibility:public"])

# ---------------------------------------------------------------------------
# Windows — import lib + DLL
# ---------------------------------------------------------------------------
cc_import(
    name = "svml_win",
    interface_library = "windows/lib/svml_dispmd.lib",
    shared_library = "windows/svml/svml_dispmd.dll",
)

# ---------------------------------------------------------------------------
# Unix — shared objects (svml + IRC + INTLC companion runtimes)
# ---------------------------------------------------------------------------
cc_import(
    name = "svml_unix",
    shared_library = "unix/lib/libsvml.so",
)

cc_import(
    name = "irc",
    shared_library = "unix/lib/libirc.so",
)

cc_import(
    name = "intlc",
    shared_library = "unix/lib/libintlc.so",
)

# ---------------------------------------------------------------------------
# Platform-unified alias — consumed as @svml//:SVML
# ---------------------------------------------------------------------------
cc_library(
    name = "SVML",
    deps = select({
        "@platforms//os:windows": [":svml_win"],
        "//conditions:default": [":svml_unix", ":irc", ":intlc"],
    }),
)
