load("//bazel:quarisma.bzl", "quarisma_copts", "quarisma_defines", "quarisma_linkopts")

MODELS_CXX_STD = "c++20"

def models_copts():
    return quarisma_copts(cxx_std = MODELS_CXX_STD)

def models_defines():
    return quarisma_defines()

def models_linkopts():
    return quarisma_linkopts()
