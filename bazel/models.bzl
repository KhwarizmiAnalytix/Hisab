load("//bazel:xsigma.bzl", "xsigma_copts", "xsigma_defines", "xsigma_linkopts")

MODELS_CXX_STD = "c++20"

def models_copts():
    return xsigma_copts(cxx_std = MODELS_CXX_STD)

def models_defines():
    return xsigma_defines()

def models_linkopts():
    return xsigma_linkopts()
