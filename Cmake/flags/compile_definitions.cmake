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

# Logging backend flags (LOGGING_HAS_LOGURU / LOGGING_HAS_GLOG / LOGGING_HAS_NATIVE)
# → set in Library/Logging/CMakeLists.txt

# Mimalloc support (MEMORY_HAS_MIMALLOC) → set in Library/Memory/CMakeLists.txt
# NUMA support     (MEMORY_HAS_NUMA)     → set in Library/Memory/CMakeLists.txt

# SVML support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_SVML)

# CUDA / HIP support (Memory-specific: MEMORY_HAS_CUDA / MEMORY_HAS_HIP set in Library/Memory/CMakeLists.txt)

# ROCm support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_ROCM)

# Google Test support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_GTEST)

# Profiler backend flags (PROFILER_HAS_KINETO / PROFILER_HAS_ITT / PROFILER_HAS_NATIVE)
# → set in Library/Profiler/CMakeLists.txt

# OpenMP support (Parallel-specific: PARALLEL_HAS_OPENMP is set in Library/Parallel/CMakeLists.txt)

# Experimental features support
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_EXPERIMENTAL)

# Magic Enum support
compile_definition(PROJECT_COMPILE_DEFINITIONS CORE_ENABLE_MAGICENUM)

# Enzyme support (CORE_HAS_ENZYME) → set in Library/Core/CMakeLists.txt

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

# Compression support (CORE_HAS_COMPRESSION / CORE_COMPRESSION_TYPE_SNAPPY)
# → set in Library/Core/CMakeLists.txt

# Allocation statistics support (optional feature flag)
compile_definition(PROJECT_COMPILE_DEFINITIONS PROJECT_ENABLE_ALLOCATION_STATS)

# Threading support (Parallel-specific: PARALLEL_HAS_PTHREADS / PARALLEL_HAS_WIN32_THREADS
# are set in Library/Parallel/CMakeLists.txt after threads.cmake runs there)

# Project-wide definitions only. Module-specific HAS flags are appended from within
# each module's own CMakeLists.txt (Library/Logging, Memory, Core, Parallel, Profiler).
set(PROJECT_DEPENDENCY_COMPILE_DEFINITIONS
  ${PROJECT_COMPILE_DEFINITIONS})
