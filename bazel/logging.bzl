load("//bazel:quarisma.bzl", _copts = "quarisma_copts", _defines = "quarisma_defines", _linkopts = "quarisma_linkopts")

def logging_copts():
    return _copts()

def logging_defines():
    return _defines()

def logging_linkopts():
    return _linkopts()
