# ============================================================================= Quarisma CUDA
# Configuration Module
# =============================================================================
# This module handles CUDA compilation support for GPU acceleration. It manages CUDA toolkit
# detection, architecture configuration, and allocation strategy selection for GPU memory
# management.
# =============================================================================

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# CUDA Support Flag Controls whether CUDA GPU acceleration is enabled for the build. When enabled,
# requires CUDA 12.0+ and configures GPU compilation.
option(QUARISMA_ENABLE_CUDA "Enable CUDA compilation" OFF)
mark_as_advanced(QUARISMA_ENABLE_CUDA)

if(NOT QUARISMA_ENABLE_CUDA)
  return()
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  # Use Clang directly as the CUDA compiler instead of nvcc.
  # CMAKE_CUDA_COMPILER must be set as a CACHE variable BEFORE enable_language(CUDA)
  # so the cmake try_compile subprocess that performs CUDA compiler identification
  # inherits the value.
  set(CMAKE_CUDA_COMPILER "${CMAKE_CXX_COMPILER}"
      CACHE FILEPATH "CUDA compiler (Clang)" FORCE)
  message(STATUS "CUDA: using Clang ${CMAKE_CXX_COMPILER_VERSION} as CUDA compiler")
endif()

find_package(CUDAToolkit REQUIRED)

# Enable CUDA language support
enable_language(CUDA)

# Version checks using consistent CUDAToolkit variables
if(CUDAToolkit_VERSION VERSION_LESS "12.0")
  message(FATAL_ERROR "Quarisma requires CUDA 12.0 or above. Found: ${CUDAToolkit_VERSION}")
endif()

if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CUDA_COMPILER_VERSION VERSION_LESS "9.2")
  message(FATAL_ERROR "QUARISMA CUDA support requires compiler version 9.2+")
endif()

# Check for version conflicts (nvcc only — Clang reports its own version, not the toolkit version)
if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA"
   AND NOT CMAKE_CUDA_COMPILER_VERSION VERSION_EQUAL CUDAToolkit_VERSION)
  message(
    FATAL_ERROR "Found two conflicting CUDA versions:\n"
                "Compiler V${CMAKE_CUDA_COMPILER_VERSION} and\n" "Toolkit V${CUDAToolkit_VERSION}"
  )
endif()

message(STATUS "Quarisma: CUDA detected: ${CUDAToolkit_VERSION}")
message(STATUS "Quarisma: CUDA compiler is: ${CMAKE_CUDA_COMPILER}")
message(STATUS "Quarisma: CUDA toolkit directory: ${CUDAToolkit_ROOT}")

# Set C++ standard based on CUDA compiler:
#   Clang:    inherits the host C++ standard (C++17 minimum)
#   nvcc 12.0+: supports C++20
#   nvcc 11.0+: supports C++17
#   nvcc older: falls back to C++14
if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  if(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
    message(STATUS "CUDA supports C++20 standard")
    set(CMAKE_CUDA_STANDARD 20)
  elseif(CMAKE_CUDA_COMPILER_VERSION VERSION_GREATER_EQUAL "11.0")
    message(STATUS "CUDA supports C++17 standard")
    set(CMAKE_CUDA_STANDARD 17)
  else()
    set(CMAKE_CUDA_STANDARD 14)
  endif()
else()
  # Clang as CUDA compiler: match the host C++ standard
  set(CMAKE_CUDA_STANDARD ${CMAKE_CXX_STANDARD})
  if(NOT CMAKE_CUDA_STANDARD)
    set(CMAKE_CUDA_STANDARD 17)
  endif()
  message(STATUS "CUDA: Clang will use C++${CMAKE_CUDA_STANDARD} standard")
endif()

set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Use modern CMake CUDA architecture handling
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES "native")
endif()

# GPU Architecture options
set(QUARISMA_CUDA_ARCH_OPTIONS "native" CACHE STRING "Which GPU Architecture(s) to compile for")
set_property(
  CACHE QUARISMA_CUDA_ARCH_OPTIONS
  PROPERTY STRINGS
           native
           fermi
           kepler
           maxwell
           pascal
           volta
           turing
           ampere
           ada
           hopper
           all
           none
)

# Set architectures based on user selection
if(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "native")
  # Let CMake handle native detection
  set(CMAKE_CUDA_ARCHITECTURES "native")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "fermi")
  set(CMAKE_CUDA_ARCHITECTURES "20")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "kepler")
  set(CMAKE_CUDA_ARCHITECTURES "30;35")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "maxwell")
  set(CMAKE_CUDA_ARCHITECTURES "50")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "pascal")
  set(CMAKE_CUDA_ARCHITECTURES "60;61")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "volta")
  set(CMAKE_CUDA_ARCHITECTURES "70")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "turing")
  set(CMAKE_CUDA_ARCHITECTURES "75")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "ampere")
  set(CMAKE_CUDA_ARCHITECTURES "80;86")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "ada")
  set(CMAKE_CUDA_ARCHITECTURES "89;90")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "hopper")
  set(CMAKE_CUDA_ARCHITECTURES "90")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "all")
  set(CMAKE_CUDA_ARCHITECTURES "50;60;70;75;80;86;89;90")
elseif(QUARISMA_CUDA_ARCH_OPTIONS STREQUAL "none")
  # Don't set any architectures, let parent project handle it
endif()

# CUDA Allocation Strategy Configuration (defined in main CMakeLists.txt)
message(STATUS "CUDA allocation strategy: ${QUARISMA_GPU_ALLOC}")

# Set preprocessor definitions based on allocation strategy
if(QUARISMA_GPU_ALLOC STREQUAL "SYNC")
  add_compile_definitions(QUARISMA_CUDA_ALLOC_SYNC)
  message(STATUS "Using synchronous CUDA allocation (cuMemAlloc/cuMemFree)")
elseif(QUARISMA_GPU_ALLOC STREQUAL "ASYNC")
  add_compile_definitions(QUARISMA_CUDA_ALLOC_ASYNC)
  message(STATUS "Using asynchronous CUDA allocation (cuMemAllocAsync/cuMemFreeAsync)")
elseif(QUARISMA_GPU_ALLOC STREQUAL "POOL_ASYNC")
  add_compile_definitions(QUARISMA_CUDA_ALLOC_POOL_ASYNC)
  message(STATUS "Using pool-based asynchronous CUDA allocation (cuMemAllocFromPoolAsync)")
endif()

# Set up CUDA libraries using modern imported targets
set(QUARISMA_CUDA_LIBRARIES CUDA::cudart CUDA::cuda_driver CUDA::cusparse CUDA::curand CUDA::cublas)

# Add nvperf_host if available (required by Kineto for CUPTI range profiler)
if(TARGET CUDA::nvperf_host)
  list(APPEND QUARISMA_CUDA_LIBRARIES CUDA::nvperf_host)
endif()

# Add CUDA libraries to the dependency list
list(APPEND QUARISMA_DEPENDENCY_LIBS ${QUARISMA_CUDA_LIBRARIES})

# Add include directories (using modern CUDAToolkit variables)
include_directories(SYSTEM "${CUDAToolkit_INCLUDE_DIRS}")
include_directories(SYSTEM "${CUDAToolkit_INCLUDE_DIRS}/thrust/system/cuda/detail")

# Add CUDA compiler flags
if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  # nvcc-specific flags
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink=--suppress-stack-size-warning")
  string(APPEND CMAKE_CUDA_FLAGS " -Wno-deprecated-gpu-targets --expt-extended-lambda")
  if(NOT MSVC)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      string(APPEND CMAKE_CUDA_FLAGS " -g -G")
    else()
      string(APPEND CMAKE_CUDA_FLAGS " -O3")
    endif()
  endif()
else()
  # Clang CUDA flags
  string(APPEND CMAKE_CUDA_FLAGS " -Wno-unknown-cuda-version")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND CMAKE_CUDA_FLAGS " -g")
  else()
    string(APPEND CMAKE_CUDA_FLAGS " -O3")
  endif()
endif()

# For backward compatibility, set legacy variables (if needed elsewhere)
set(QUARISMA_CUDA_FOUND TRUE)
set(QUARISMA_ENABLE_CUDA ON)
