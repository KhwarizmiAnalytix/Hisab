load("//bazel:quarisma.bzl", _copts = "quarisma_copts", _defines = "quarisma_defines", _linkopts = "quarisma_linkopts")

def parallel_copts():
    return _copts()

def parallel_defines():
    return _defines()

def parallel_linkopts():
    return _linkopts()
