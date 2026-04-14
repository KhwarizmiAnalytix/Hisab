include_guard(GLOBAL)

# =============================================================================
# Feature Flag Mapping
# =============================================================================
# Map CMake PROJECT_ENABLE_* variables to PROJECT_HAS_* compile definitions This ensures consistent
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
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS "${definition_name}=${_value}")
  set(PROJECT_DEPENDENCY_COMPILE_DEFINITIONS "${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS}" PARENT_SCOPE)
endfunction()

# MKL support
compile_definition(PROJECT_ENABLE_MKL)

# TBB support
compile_definition(PARALLEL_ENABLE_TBB)

# Loguru support
compile_definition(LOGGING_ENABLE_LOGURU)

# GLOG support
compile_definition(LOGGING_ENABLE_GLOG)

# Native logging support
compile_definition(LOGGING_ENABLE_NATIVE_LOGGING)

# Mimalloc support
compile_definition(MEMORY_ENABLE_MIMALLOC)

# NUMA support
compile_definition(PROJECT_ENABLE_NUMA)

# SVML support
compile_definition(PROJECT_ENABLE_SVML)

# CUDA support
compile_definition(PROJECT_ENABLE_CUDA)

# HIP support
compile_definition(PROJECT_ENABLE_HIP)

# ROCm support
compile_definition(PROJECT_ENABLE_ROCM)

# Google Test support
compile_definition(PROJECT_ENABLE_GTEST)

# Profiler backend flags — all three are derived from PROFILER_TYPE (set in
# CMakeLists.txt) and are mutually exclusive. compile_definition() maps each
# PROFILER_ENABLE_* variable to the corresponding PROFILER_HAS_* compile definition.
compile_definition(PROFILER_ENABLE_KINETO)          # → PROFILER_HAS_KINETO
compile_definition(PROFILER_ENABLE_ITT)             # → PROFILER_HAS_ITT
compile_definition(PROFILER_ENABLE_NATIVE_PROFILER) # → PROFILER_HAS_NATIVE_PROFILER

# Defensive mutual-exclusivity check (PROFILER_TYPE already enforces this,
# but guard against any direct variable manipulation downstream).
set(_profiler_backends_enabled 0)
if(PROFILER_ENABLE_KINETO)
  math(EXPR _profiler_backends_enabled "${_profiler_backends_enabled} + 1")
endif()
if(PROFILER_ENABLE_ITT)
  math(EXPR _profiler_backends_enabled "${_profiler_backends_enabled} + 1")
endif()
if(PROFILER_ENABLE_NATIVE_PROFILER)
  math(EXPR _profiler_backends_enabled "${_profiler_backends_enabled} + 1")
endif()
if(_profiler_backends_enabled GREATER 1)
  message(FATAL_ERROR
    "PROFILER_ENABLE_KINETO, PROFILER_ENABLE_ITT, and PROFILER_ENABLE_NATIVE_PROFILER "
    "are mutually exclusive. Only one may be set to ON at a time.")
endif()
unset(_profiler_backends_enabled)

# OpenMP support
compile_definition(PARALLEL_ENABLE_OPENMP)

# Experimental features support
compile_definition(PROJECT_ENABLE_EXPERIMENTAL)

# Magic Enum support
compile_definition(PROJECT_ENABLE_MAGICENUM)

# Enzyme Automatic Differentiation support
compile_definition(CORE_ENABLE_ENZYME)

# Exception pointer support (detected by compiler checks in utils.cmake)
if(PROJECT_HAS_EXCEPTION_PTR)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_HAS_EXCEPTION_PTR=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_HAS_EXCEPTION_PTR=0)
endif()

# Vectorization support (SSE, AVX, AVX2, AVX512 - detected by compiler checks in utils.cmake)
if(PROJECT_SSE)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_SSE=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_SSE=0)
endif()

if(PROJECT_AVX)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX=0)
endif()

if(PROJECT_AVX2)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX2=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX2=0)
endif()

if(PROJECT_AVX512)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX512=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_AVX512=0)
endif()

# Optional feature flags (not always set)
if(PROJECT_SOBOL_1111)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_SOBOL_1111=1)
endif()

if(PROJECT_LU_PIVOTING)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROJECT_LU_PIVOTING=1)
endif()

# Compression support
compile_definition(CORE_ENABLE_COMPRESSION)
if(CORE_ENABLE_COMPRESSION)
  if(CORE_COMPRESSION_TYPE_SNAPPY)
    list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=1)
  else()
    list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=0)
  endif()
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=0)
endif()

# Allocation statistics support (optional feature flag)
compile_definition(PROJECT_ENABLE_ALLOCATION_STATS)

if(PROFILER_ENABLE_NATIVE_PROFILER)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROFILER_HAS_NATIVE_PROFILER=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PROFILER_HAS_NATIVE_PROFILER=0)
endif()

# Threading support (detected by threads.cmake)
# These flags indicate which threading library is available on the platform
if(PARALLEL_USE_PTHREADS)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PARALLEL_HAS_PTHREADS=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PARALLEL_HAS_PTHREADS=0)
endif()

if(PARALLEL_USE_WIN32_THREADS)
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PARALLEL_HAS_WIN32_THREADS=1)
else()
  list(APPEND PROJECT_DEPENDENCY_COMPILE_DEFINITIONS PARALLEL_HAS_WIN32_THREADS=0)
endif()

message("PROJECT_DEPENDENCY_COMPILE_DEFINITIONS: ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS}")
