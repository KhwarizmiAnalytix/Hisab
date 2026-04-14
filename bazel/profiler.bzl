load("//bazel:quarisma.bzl", _copts = "quarisma_copts", _defines = "quarisma_defines", _linkopts = "quarisma_linkopts")

def profiler_copts():
    return _copts()

def profiler_defines():
    return _defines()

def profiler_linkopts():
    return _linkopts()
