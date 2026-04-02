include_guard(GLOBAL)

# =============================================================================
# Feature Flag Mapping
# =============================================================================
# Map CMake QUARISMA_ENABLE_* variables to QUARISMA_HAS_* compile definitions This ensures consistent
# naming: CMake uses ENABLE, C++ code uses HAS All feature flags are defined as compile definitions
# (1 or 0) rather than using configure_file()

function(compile_definition enable_flag)
  string(REPLACE "ENABLE" "HAS" definition_name "${enable_flag}")
  if(NOT DEFINED ${enable_flag})
    set(_enabled OFF)
  else()
    set(_enabled ${${enable_flag}})
  endif()
  if(_enabled)
    set(_value 1)
  else()
    set(_value 0)
  endif()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS "${definition_name}=${_value}")
  set(QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS "${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS}" PARENT_SCOPE)
endfunction()

# MKL support
compile_definition(QUARISMA_ENABLE_MKL)

# TBB support
compile_definition(QUARISMA_ENABLE_TBB)

# Loguru support
compile_definition(QUARISMA_ENABLE_LOGURU)

# GLOG support
compile_definition(QUARISMA_ENABLE_GLOG)

# Native logging support
compile_definition(QUARISMA_ENABLE_NATIVE_LOGGING)

# Mimalloc support
compile_definition(QUARISMA_ENABLE_MIMALLOC)

# NUMA support
compile_definition(QUARISMA_ENABLE_NUMA)

# SVML support
compile_definition(QUARISMA_ENABLE_SVML)

# CUDA support
compile_definition(QUARISMA_ENABLE_CUDA)

# HIP support
compile_definition(QUARISMA_ENABLE_HIP)

# ROCm support
compile_definition(QUARISMA_ENABLE_ROCM)

# Google Test support
compile_definition(QUARISMA_ENABLE_GTEST)

# Kineto support
compile_definition(QUARISMA_ENABLE_KINETO)
# Profiler bespoke code uses PROFILER_HAS_KINETO (mirrors Bazel); map from CMake enable flag.
if(QUARISMA_ENABLE_KINETO)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS PROFILER_HAS_KINETO=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS PROFILER_HAS_KINETO=0)
endif()

# Intel ITT API support
compile_definition(QUARISMA_ENABLE_ITT)

# Native profiler support (derived from QUARISMA_PROFILER_TYPE)
compile_definition(QUARISMA_ENABLE_NATIVE_PROFILER)

# OpenMP support
compile_definition(QUARISMA_ENABLE_OPENMP)

# Experimental features support
compile_definition(QUARISMA_ENABLE_EXPERIMENTAL)

# Magic Enum support
compile_definition(QUARISMA_ENABLE_MAGICENUM)

# Enzyme Automatic Differentiation support
compile_definition(QUARISMA_ENABLE_ENZYME)

# Exception pointer support (detected by compiler checks in utils.cmake)
if(QUARISMA_HAS_EXCEPTION_PTR)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_EXCEPTION_PTR=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_EXCEPTION_PTR=0)
endif()

# Vectorization support (SSE, AVX, AVX2, AVX512 - detected by compiler checks in utils.cmake)
if(QUARISMA_SSE)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SSE=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SSE=0)
endif()

if(QUARISMA_AVX)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX=0)
endif()

if(QUARISMA_AVX2)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX2=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX2=0)
endif()

if(QUARISMA_AVX512)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX512=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_AVX512=0)
endif()

# Optional feature flags (not always set)
if(QUARISMA_SOBOL_1111)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_SOBOL_1111=1)
endif()

if(QUARISMA_LU_PIVOTING)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_LU_PIVOTING=1)
endif()

# Compression support
compile_definition(QUARISMA_ENABLE_COMPRESSION)
if(QUARISMA_ENABLE_COMPRESSION)
  if(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=1)
  else()
    list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=0)
  endif()
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_COMPRESSION_TYPE_SNAPPY=0)
endif()

# Allocation statistics support (optional feature flag)
compile_definition(QUARISMA_ENABLE_ALLOCATION_STATS)

if(QUARISMA_ENABLE_NATIVE_PROFILER)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_NATIVE_PROFILER=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_NATIVE_PROFILER=0)
endif()

# Threading support (detected by threads.cmake)
# These flags indicate which threading library is available on the platform
if(QUARISMA_USE_PTHREADS)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_PTHREADS=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_PTHREADS=0)
endif()

if(QUARISMA_USE_WIN32_THREADS)
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_WIN32_THREADS=1)
else()
  list(APPEND QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS QUARISMA_HAS_WIN32_THREADS=0)
endif()

message("QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS: ${QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS}")
