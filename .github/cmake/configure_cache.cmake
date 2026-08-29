# XSigma CI Compiler Cache Configuration This initial-cache file standardises compiler launcher
# settings across runners while still allowing manual overrides via the CMAKE_COMPILER_LAUNCHER
# environment variable.

if(NOT "$ENV{CMAKE_COMPILER_LAUNCHER}" STREQUAL "")
  set(CMAKE_C_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "$ENV{CMAKE_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
elseif("$ENV{RUNNER_OS}" STREQUAL "Windows")
  set(CMAKE_C_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "buildcache" CACHE STRING "" FORCE)
  set(xsigma_replace_uncacheable_flags ON CACHE BOOL "" FORCE)
else()
  set(CMAKE_C_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
  set(CMAKE_CXX_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "sccache" CACHE STRING "" FORCE)
endif()
