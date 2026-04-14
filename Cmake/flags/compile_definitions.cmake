include_guard(GLOBAL)

#=============================================================================
# Feature flag mapping

# Map CMake *ENABLE* variables to *HAS* compile definitions (1 or 0). definitions_list is the name
# of the list variable to append to (e.g. LOGGING_COMPILE_DEFINITIONS MEMORY_COMPILE_DEFINITIONS).

function(compile_definition definitions_list enable_flag)
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
  set(_current "${${definitions_list}}")
  list(APPEND _current "${definition_name}=${_value}")
  set(${definitions_list} "${_current}" PARENT_SCOPE)
endfunction()

# MKL support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_MKL)

# TBB support
# PARALLEL_HAS_TBB  → set in Library/Parallel/CMakeLists.txt
# MEMORY_HAS_TBB    → set in Library/Memory/CMakeLists.txt

# Loguru support
compile_definition(LOGGING_COMPILE_DEFINITIONS LOGGING_ENABLE_LOGURU)

# GLOG support
compile_definition(LOGGING_COMPILE_DEFINITIONS LOGGING_ENABLE_GLOG)

# Native logging support
compile_definition(LOGGING_COMPILE_DEFINITIONS LOGGING_ENABLE_NATIVE_LOGGING)

# Mimalloc support
compile_definition(MEMORY_COMPILE_DEFINITIONS MEMORY_ENABLE_MIMALLOC)

# NUMA (CMake option is MEMORY_ENABLE_NUMA; sources use MEMORY_HAS_NUMA)
if(MEMORY_ENABLE_NUMA)
  list(APPEND MEMORY_COMPILE_DEFINITIONS MEMORY_HAS_NUMA=1)
else()
  list(APPEND MEMORY_COMPILE_DEFINITIONS MEMORY_HAS_NUMA=0)
endif()

# SVML support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_SVML)

# CUDA / HIP support (Memory-specific: MEMORY_HAS_CUDA / MEMORY_HAS_HIP set in Library/Memory/CMakeLists.txt)

# ROCm support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_ROCM)

# Google Test support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_GTEST)

# Profiler backend flags — all three are derived from PROFILER_TYPE (set in
# CMakeLists.txt) and are mutually exclusive. compile_definition() maps each
# PROFILER_ENABLE_* variable to the corresponding PROFILER_HAS_* compile definition.
compile_definition(PROFILER_COMPILE_DEFINITIONS PROFILER_ENABLE_KINETO)
compile_definition(PROFILER_COMPILE_DEFINITIONS PROFILER_ENABLE_ITT)
compile_definition(PROFILER_COMPILE_DEFINITIONS PROFILER_ENABLE_NATIVE_PROFILER)

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

# OpenMP support (Parallel-specific: PARALLEL_HAS_OPENMP is set in Library/Parallel/CMakeLists.txt)

# Experimental features support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_EXPERIMENTAL)

# Magic Enum support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_MAGICENUM)

# Enzyme Automatic Differentiation support
compile_definition(CORE_COMPILE_DEFINITIONS CORE_ENABLE_ENZYME)

# Exception pointer support (detected by compiler checks in utils.cmake)
if(PROJECT_HAS_EXCEPTION_PTR)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_HAS_EXCEPTION_PTR=1)
else()
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_HAS_EXCEPTION_PTR=0)
endif()

# Vectorization support (SSE, AVX, AVX2, AVX512 - detected by compiler checks in utils.cmake)
if(PROJECT_SSE)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_SSE=1)
else()
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_SSE=0)
endif()

if(PROJECT_AVX)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX=1)
else()
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX=0)
endif()

if(PROJECT_AVX2)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX2=1)
else()
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX2=0)
endif()

if(PROJECT_AVX512)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX512=1)
else()
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_AVX512=0)
endif()

# Optional feature flags (not always set)
if(PROJECT_SOBOL_1111)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_SOBOL_1111=1)
endif()

if(PROJECT_LU_PIVOTING)
  list(APPEND PROJECT_COMPILE_DEFINITIONS PROJECT_LU_PIVOTING=1)
endif()

# Compression support
compile_definition(CORE_COMPILE_DEFINITIONS CORE_ENABLE_COMPRESSION)
if(CORE_ENABLE_COMPRESSION)
  if(CORE_COMPRESSION_TYPE_SNAPPY)
    list(APPEND CORE_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=1)
  else()
    list(APPEND CORE_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=0)
  endif()
else()
  list(APPEND CORE_COMPILE_DEFINITIONS CORE_COMPRESSION_TYPE_SNAPPY=0)
endif()

# Allocation statistics support (optional feature flag)
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_ALLOCATION_STATS)

# Threading support (Parallel-specific: PARALLEL_HAS_PTHREADS / PARALLEL_HAS_WIN32_THREADS
# are set in Library/Parallel/CMakeLists.txt after threads.cmake runs there)

set(PROJECT_DEPENDENCY_COMPILE_DEFINITIONS
  ${PROJECT_COMPILE_DEFINITIONS}
  ${LOGGING_COMPILE_DEFINITIONS}
  ${MEMORY_COMPILE_DEFINITIONS}
  ${PARALLEL_COMPILE_DEFINITIONS}
  ${PROFILER_COMPILE_DEFINITIONS}
  ${CORE_COMPILE_DEFINITIONS})
