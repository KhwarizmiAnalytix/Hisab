load("//bazel:xsigma.bzl", "xsigma_copts", "xsigma_defines", "xsigma_linkopts")

GRAPH_CXX_STD = "c++20"

def graph_copts():
    return xsigma_copts(cxx_std = GRAPH_CXX_STD)

def graph_defines():
    return xsigma_defines()

def graph_linkopts():
    return xsigma_linkopts()
