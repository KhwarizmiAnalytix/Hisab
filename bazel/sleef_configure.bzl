"""SLEEF prebuilt discovery for --config=sleef.

SLEEF's own build (codegen + per-architecture dispatch, driven by its
upstream CMakeLists.txt) is not reimplemented natively in Bazel. CMake
builds it from the vendored ThirdParty/sleef submodule into
<cmake-build-dir>/ThirdParty/sleef_build (libsleef.a) and
<cmake-build-dir>/ThirdParty/install (libtlfloat.a) -- see the
VECTORIZATION_ENABLE_SLEEF block in Library/Vectorization/CMakeLists.txt.

This repository rule looks for those artifacts under a handful of
conventional CMake build-directory names:
  - found: generates a BUILD.bazel exposing them as `:sleef`, a drop-in
    static-library target for //bazel:vectorization.bzl's
    vectorization_sleef_deps().
  - not found: generates a BUILD.bazel whose `:sleef` target fails
    immediately with an actionable message, instead of surfacing a
    confusing "missing input file" error deep inside a Vectorization
    compile action.

Either way, `bazel build --config=sleef //Library/Vectorization/...` remains
dependent on a prior CMake build having produced these artifacts -- Bazel
does not (yet) build SLEEF hermetically from source. See
Docs/BAZEL_USER_GUIDE.md, "Known Gaps and CMake/Bazel Alignment Plan", for
the plan to close this gap (a rules_foreign_cc-based hermetic build).
"""

_CANDIDATE_BUILD_DIRS = ["build_ninja", "build", "build_ninja_project_vectorization"]

_SETUP_HINT = (
    "SLEEF (--config=sleef) requires a prior CMake build: SLEEF's own " +
    "CMake build (codegen + per-architecture dispatch) is not yet " +
    "reimplemented natively in Bazel. From Scripts/, run: " +
    "python3 setup.py config.build.native.sleef --project.vectorization " +
    "-- this populates <build-dir>/ThirdParty/sleef_build and " +
    "<build-dir>/ThirdParty/install. Re-run the Bazel build afterwards."
)

_MISSING_BUILD_FILE = """\
package(default_visibility = ["//visibility:public"])

genrule(
    name = "sleef_missing",
    outs = ["sleef_missing.cc"],
    cmd = "echo SLEEF_NOT_BUILT_SEE_MESSAGE_ABOVE >&2; exit 1",
    message = {message},
)

cc_library(
    name = "sleef",
    srcs = [":sleef_missing"],
)
""".format(message = repr(_SETUP_HINT))

_FOUND_BUILD_FILE = """\
package(default_visibility = ["//visibility:public"])

cc_import(
    name = "sleef_prebuilt",
    static_library = "{build_dir}/ThirdParty/sleef_build/lib/libsleef.a",
)

cc_import(
    name = "tlfloat_prebuilt",
    static_library = "{build_dir}/ThirdParty/install/lib/libtlfloat.a",
)

cc_library(
    name = "sleef",
    hdrs = ["{build_dir}/include/sleef.h"],
    includes = ["{build_dir}/include"],
    deps = [
        ":sleef_prebuilt",
        ":tlfloat_prebuilt",
    ],
)
"""

def _find_cmake_build_dir(repository_ctx):
    for build_dir in _CANDIDATE_BUILD_DIRS:
        root = repository_ctx.workspace_root.get_child(build_dir)
        lib = root.get_child("ThirdParty").get_child("sleef_build").get_child("lib").get_child("libsleef.a")
        tlfloat = root.get_child("ThirdParty").get_child("install").get_child("lib").get_child("libtlfloat.a")
        hdr = root.get_child("include").get_child("sleef.h")
        if lib.exists and tlfloat.exists and hdr.exists:
            return build_dir
    return None

def _sleef_configure_impl(repository_ctx):
    build_dir = _find_cmake_build_dir(repository_ctx)
    if build_dir == None:
        repository_ctx.file("BUILD.bazel", _MISSING_BUILD_FILE)
        return

    # Symlink the CMake build directory into this repository so the
    # generated BUILD file's relative paths resolve inside the sandbox.
    repository_ctx.symlink(repository_ctx.workspace_root.get_child(build_dir), build_dir)
    repository_ctx.file("BUILD.bazel", _FOUND_BUILD_FILE.format(build_dir = build_dir))

sleef_configure = repository_rule(
    implementation = _sleef_configure_impl,
    local = True,
)
