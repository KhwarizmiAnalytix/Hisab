# =============================================================================
# Quarisma Main CMake Configuration
# =============================================================================
# This is the root CMake configuration file that orchestrates the Quarisma build system.
# It manages third-party dependencies, feature flags, compiler settings, and library targets.
# Third-party dependencies are handled by ThirdParty/CMakeLists.txt, and feature flags are
# mapped to compile definitions by Cmake/flags/compile_definitions.cmake.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)

# Prohibit in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source build are not supported")
endif()

# Set project name and version
project(
  Quarisma
  VERSION 1.0.0
  DESCRIPTION "Quarisma High-Performance Computing Library"
  LANGUAGES CXX C
)

# Set version macros
set(QUARISMA_MAJOR_VERSION 1)
set(QUARISMA_MINOR_VERSION 0)
set(QUARISMA_BUILD_VERSION 0)
set(QUARISMA_VERSION_FULL "${QUARISMA_MAJOR_VERSION}.${QUARISMA_MINOR_VERSION}.${QUARISMA_BUILD_VERSION}")

# Add CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/tools")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/flags")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/packages")

# Distributed compilation with Icecream
option(QUARISMA_ENABLE_ICECC "Use Icecream distributed compilation" OFF)

if(QUARISMA_ENABLE_ICECC)
  find_program(ICECC_EXECUTABLE icecc)
  if(ICECC_EXECUTABLE)
    set(CMAKE_C_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
    message(STATUS "Using Icecream: ${ICECC_EXECUTABLE}")
  endif()
endif()

# Enable colored output for diagnostics
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Build optimization settings
option(QUARISMA_ENABLE_LTO "Enable Link Time Optimization" OFF)

# Set CMAKE_INTERPROCEDURAL_OPTIMIZATION based on QUARISMA_ENABLE_LTO
if(QUARISMA_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  message(STATUS "Link Time Optimization (LTO) disabled")
endif()

# Include necessary CMake modules early
include(CMakeDependentOption)

# Set C++ standard configuration
set(QUARISMA_CXX_STANDARD "17" CACHE STRING "C++ standard to use. Options are 11, 14, 17, 20, or 23")
mark_as_advanced(QUARISMA_CXX_STANDARD)

# Validate C++ standard
if(NOT QUARISMA_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "QUARISMA_CXX_STANDARD must be 11,14, 17, 20, or 23")
endif()

set(CMAKE_CXX_STANDARD ${QUARISMA_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(QUARISMA_DEPENDENCY_LIBS "")
mark_as_advanced(QUARISMA_DEPENDENCY_LIBS)
set_property(GLOBAL PROPERTY QUARISMA_DEPENDENCY_LIBS "")
set_property(GLOBAL PROPERTY QUARISMA_DEPENDENCY_INCLUDE_DIRS "")
set_property(GLOBAL PROPERTY QUARISMA_DEPENDENCY_COMPILE_DEFINITIONS "")

# =============================================================================
# General Project-Wide
# Configuration Flags
# =============================================================================
# These flags control general project settings that are not specific to individual feature modules.
# Feature-specific flags are declared in their respective module files (Cmake/tools/*.cmake).
# =============================================================================

# Enzyme Automatic Differentiation Support Flag Controls whether Enzyme automatic differentiation is
# enabled. Enzyme provides high-performance automatic differentiation capabilities.
option(QUARISMA_ENABLE_ENZYME "Enable Enzyme" OFF)
mark_as_advanced(QUARISMA_ENABLE_ENZYME)

# GPU Allocation Strategy Configuration This flag controls which GPU allocation functions are used
# by allocator_gpu (CUDA/HIP)
set(QUARISMA_GPU_ALLOC "POOL_ASYNC"
    CACHE STRING "GPU allocation strategy to use. Options are SYNC, ASYNC, POOL_ASYNC"
)
set_property(CACHE QUARISMA_GPU_ALLOC PROPERTY STRINGS SYNC ASYNC POOL_ASYNC)

# Validate CUDA allocation strategy
if(NOT QUARISMA_GPU_ALLOC MATCHES "^(SYNC|ASYNC|POOL_ASYNC)$")
  message(
    FATAL_ERROR
      "Invalid QUARISMA_GPU_ALLOC: ${QUARISMA_GPU_ALLOC}. Valid options are: SYNC, ASYNC, POOL_ASYNC"
  )
endif()

# =============================================================================
# Algorithm and
# Feature Configuration Flags
# =============================================================================
# These flags control specific algorithm implementations and optional features.
# =============================================================================

# Sobol Sequence 1111-Dimension Support Flag Controls whether Sobol quasi-random sequence generation
# supports 1111 dimensions. When enabled, provides extended Sobol sequence capabilities.
option(QUARISMA_SOBOL_1111 "Build support sobol with 1111 dimension" ON)

# LU Decomposition Pivoting Flag Controls whether LU decomposition uses pivoting for numerical
# stability. When enabled, improves numerical stability at the cost of additional computation.
option(QUARISMA_LU_PIVOTING "Enable LU PIVOTING" OFF)

# =============================================================================
# Testing and Quality
# Assurance Configuration Flags
# =============================================================================
# These flags control testing infrastructure, benchmarking, and code quality tools.
# =============================================================================

# Build Testing Configuration Controls whether test directories are built: ON (always), OFF (never),
# or WANT (optional).
set(QUARISMA_BUILD_TESTING "ON" CACHE STRING "Build module testing directories")
set_property(CACHE QUARISMA_BUILD_TESTING PROPERTY STRINGS "ON;OFF;WANT")

# Build Examples Configuration Controls whether example programs are built. When enabled, builds
# comprehensive examples demonstrating parallel APIs and other features.
option(QUARISMA_ENABLE_EXAMPLES "Build example programs" OFF)

# Google Benchmark Support Flag Controls whether Google Benchmark is enabled for performance
# benchmarking. When enabled, allows creation of benchmark targets for performance analysis.
option(QUARISMA_ENABLE_BENCHMARK "Enable google benchmark" OFF)

# Google Test Support Flag Controls whether Google Test framework is enabled for unit testing. When
# enabled, provides comprehensive testing infrastructure.
option(QUARISMA_ENABLE_GTEST "Enable google test" ON)

# =============================================================================
# Performance and
# Optimization Configuration Flags
# =============================================================================
# These flags control performance-related settings and optimization strategies.
# =============================================================================

# Vectorization Strategy Configuration Specifies which SIMD vectorization to use: no, sse, avx,
# avx2, or avx512. Enables platform-specific optimizations for numerical computations.
set(QUARISMA_VECTORIZATION_TYPE "avx2"
    CACHE STRING "vectorization implementation to use. Options are no, sse, avx, avx2, or avx512"
)
set_property(CACHE QUARISMA_VECTORIZATION_TYPE PROPERTY STRINGS no sse avx avx2 avx512)

# =============================================================================
# Third-Party Library
# and Dependency Configuration Flags
# =============================================================================
# These flags control the use of external libraries and optional dependencies. Mandatory core
# dependencies: fmt (formatting), cpuinfo (CPU detection)
# =============================================================================
# Magic Enum Static Reflection Flag Controls whether magic_enum is enabled for compile-time enum
# reflection. When enabled, provides powerful enum introspection capabilities.
option(QUARISMA_ENABLE_MAGICENUM "Enable magic_enum static reflection for enums." ON)
mark_as_advanced(QUARISMA_ENABLE_MAGICENUM)

# Microsoft Mimalloc Memory Allocator Flag Controls whether Microsoft's mimalloc high-performance
# allocator is enabled. When enabled, provides faster memory allocation with better performance
# characteristics.
option(QUARISMA_ENABLE_MIMALLOC
       "Enable Microsoft mimalloc high-performance memory allocator support." ON
)
mark_as_advanced(QUARISMA_ENABLE_MIMALLOC)

# ROCm Support Flag Controls whether ROCm (Radeon Open Compute) support is enabled for AMD GPUs.
# When enabled, allows integration with ROCm ecosystem and AMD GPU acceleration.
option(QUARISMA_ENABLE_ROCM "Enable ROCm support for AMD GPUs" OFF)
mark_as_advanced(QUARISMA_ENABLE_ROCM)

# Profiler backend selection — the single control point for the profiler backend.
# PROFILER_ENABLE_KINETO, PROFILER_ENABLE_ITT, and PROFILER_ENABLE_NATIVE_PROFILER
# are derived from this value and are mutually exclusive. Do not set them directly.
# Defaults to KINETO for downstream compatibility.
set(QUARISMA_PROFILER_TYPE "KINETO"
    CACHE STRING "Profiler backend type. Options are KINETO, NATIVE, or ITT")
set_property(CACHE QUARISMA_PROFILER_TYPE PROPERTY STRINGS KINETO NATIVE ITT)

if(NOT QUARISMA_PROFILER_TYPE MATCHES "^(KINETO|NATIVE|ITT)$")
  message(
    FATAL_ERROR
      "Invalid QUARISMA_PROFILER_TYPE: ${QUARISMA_PROFILER_TYPE}. Valid options are KINETO, NATIVE, or ITT")
endif()

if(QUARISMA_PROFILER_TYPE STREQUAL "KINETO")
  set(PROFILER_ENABLE_KINETO          ON)
  set(PROFILER_ENABLE_ITT             OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(QUARISMA_PROFILER_TYPE STREQUAL "ITT")
  set(PROFILER_ENABLE_KINETO          OFF)
  set(PROFILER_ENABLE_ITT             ON)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(QUARISMA_PROFILER_TYPE STREQUAL "NATIVE")
  set(PROFILER_ENABLE_KINETO          OFF)
  set(PROFILER_ENABLE_ITT             OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER ON)
endif()

# =============================================================================
# Platform-Specific
# Configuration and Feature Validation
# =============================================================================
# Adjusts feature availability based on platform constraints. Some features are only available on
# specific operating systems.
# =============================================================================

# NUMA is only available on Unix systems
if(NOT UNIX)
  set(QUARISMA_ENABLE_NUMA OFF)
endif()

# Memkind is only available on Unix systems (not macOS or Windows)
if(NOT UNIX OR APPLE)
  set(QUARISMA_ENABLE_MEMKIND OFF)
endif()

# =============================================================================
# Global Project
# Variables
# =============================================================================
# Set global variables used throughout the build system.
# =============================================================================

set(QUARISMA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(QUARISMA_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(QUARISMA_VERSION "${PROJECT_VERSION}")

# =============================================================================
# Build Library Type
# Configuration
# =============================================================================
# Controls whether libraries are built as shared or static. Quarisma requires shared libraries (DLLs
# on Windows) for proper functionality.
# =============================================================================

# Build Shared Libraries Flag Controls whether libraries are built as shared (ON) or static (OFF).
# Quarisma requires shared libraries for proper DLL export/import handling.
option(QUARISMA_BUILD_SHARED_LIBS "Build shared libraries" ON)
set(BUILD_SHARED_LIBS ${QUARISMA_BUILD_SHARED_LIBS})

# Include necessary CMake modules
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

# Include Quarisma-specific CMake modules
include(compiler_checks)
include(xcode_toolchain)
include(cache)
include(build_type)
include(checks)
include(utils)
include(platform)
include(logging)
include(compression)
include(threads)

# Configure SMP backend selection (must be before openmp and tbb includes)
include(parallel_backend)

# Create build interface target for common settings FIRST
# This target is needed by sanitize.cmake and other modules
add_library(quarismabuild INTERFACE)
add_library(Quarisma::build ALIAS quarismabuild)

# Include sanitizer configuration EARLY to disable incompatible features
# Must be before compile_definitions so QUARISMA_HAS_MIMALLOC is set correctly
include(sanitize)

# Configure optional features
include(openmp)
include(cuda)
include(hip)
include(mkl)
include(numa)
include(tbb)
include(enzyme)
include(clang_tidy)
include(spell)
include(iwyu)
include(coverage)
include(valgrind)
include(helper_macros)
include(compile_definitions)

# Include optimization modules AFTER quarismabuild target is created These modules apply flags to the
# quarismabuild target to affect only Quarisma targets, not third-party dependencies
include(linker)

# Set compile features
target_compile_features(quarismabuild INTERFACE cxx_std_${QUARISMA_CXX_STANDARD})

# Apply common linker flags
if(DEFINED QUARISMA_LINKER_FLAGS_COMMON)
  target_link_options(quarismabuild INTERFACE ${QUARISMA_LINKER_FLAGS_COMMON})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${QUARISMA_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${QUARISMA_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${QUARISMA_BINARY_DIR}/lib")

# Configure testing
if(QUARISMA_BUILD_TESTING STREQUAL "ON" OR QUARISMA_BUILD_TESTING STREQUAL "WANT")
  enable_testing()
  include(CTest)
endif()

# Add third-party dependencies
add_subdirectory(ThirdParty)

# Add subdirectories
add_subdirectory(Library/Core)
add_subdirectory(Library/Profiler)

# Add examples if enabled
if(QUARISMA_ENABLE_EXAMPLES)
  add_subdirectory(Examples/Profiling)
  message(STATUS "✓ Profiling Examples enabled")
endif()

# Apply Valgrind timeout multiplier to all tests (if Valgrind is enabled) This must be called after
# all tests are registered
if(QUARISMA_ENABLE_VALGRIND AND QUARISMA_BUILD_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

# Include summary at the end
include(summary)
print_configuration_summary()
