load("//bazel:quarisma.bzl", _copts = "quarisma_copts", _defines = "quarisma_defines", _linkopts = "quarisma_linkopts")

def memory_copts():
    return _copts()

def memory_defines():
    return _defines()

def memory_linkopts():
    return _linkopts()
