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

# Module path + Xcode LLVM xctoolchain before project() so CMAKE_*_COMPILER matches TOOLCHAINS
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/tools")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/flags")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/packages")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Parallel/Cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Memory/Cmake")
include(xcode_toolchain)

# Set project name and version
project(
  Quarisma
  VERSION 1.0.0
  DESCRIPTION "Quarisma High-Performance Computing Library"
  LANGUAGES CXX C
)

# Set version macros
set(PROJECT_MAJOR_VERSION 1)
set(PROJECT_MINOR_VERSION 0)
set(PROJECT_BUILD_VERSION 0)
set(PROJECT_VERSION_FULL "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_BUILD_VERSION}")

# Distributed compilation with Icecream
option(PROJECT_ENABLE_ICECC "Use Icecream distributed compilation" OFF)

if(PROJECT_ENABLE_ICECC)
  find_program(ICECC_EXECUTABLE icecc)
  if(ICECC_EXECUTABLE)
    set(CMAKE_C_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${ICECC_EXECUTABLE})
    message(STATUS "Using Icecream: ${ICECC_EXECUTABLE}")
  endif()
endif()

# Enable colored output for diagnostics
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Build optimization settings (module-local canonical flags)
option(LOGGING_ENABLE_LTO "Enable Link Time Optimization for Logging" OFF)
option(MEMORY_ENABLE_LTO "Enable Link Time Optimization for Memory" OFF)
option(PARALLEL_ENABLE_LTO "Enable Link Time Optimization for Parallel" OFF)
option(PROFILER_ENABLE_LTO "Enable Link Time Optimization for Profiler" OFF)
option(CORE_ENABLE_LTO "Enable Link Time Optimization for Core" OFF)

# Legacy aggregate project flag for shared tooling.
set(PROJECT_ENABLE_LTO OFF)
if(LOGGING_ENABLE_LTO
   OR MEMORY_ENABLE_LTO
   OR PARALLEL_ENABLE_LTO
   OR PROFILER_ENABLE_LTO
   OR CORE_ENABLE_LTO)
  set(PROJECT_ENABLE_LTO ON)
endif()

# Set CMAKE_INTERPROCEDURAL_OPTIMIZATION based on aggregate LTO state
if(PROJECT_ENABLE_LTO)
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
set(PROJECT_CXX_STANDARD "17" CACHE STRING "C++ standard to use. Options are 11, 14, 17, 20, or 23")
mark_as_advanced(PROJECT_CXX_STANDARD)

# Validate C++ standard
if(NOT PROJECT_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "PROJECT_CXX_STANDARD must be 11,14, 17, 20, or 23")
endif()

set(CMAKE_CXX_STANDARD ${PROJECT_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# General Project-Wide
# Configuration Flags
# =============================================================================
# These flags control general project settings that are not specific to individual feature modules.
# Feature-specific flags are declared in their respective module files (Cmake/tools/*.cmake).
# =============================================================================

# Enzyme Automatic Differentiation Support Flag Controls whether Enzyme automatic differentiation is
# enabled. Enzyme provides high-performance automatic differentiation capabilities.
option(CORE_ENABLE_ENZYME "Enable Enzyme" OFF)
mark_as_advanced(CORE_ENABLE_ENZYME)

# GPU Allocation Strategy Configuration This flag controls which GPU allocation functions are used
# by allocator_gpu (CUDA/HIP)
set(MEMORY_GPU_ALLOC "POOL_ASYNC"
    CACHE STRING "GPU allocation strategy to use. Options are SYNC, ASYNC, POOL_ASYNC"
)
set_property(CACHE MEMORY_GPU_ALLOC PROPERTY STRINGS SYNC ASYNC POOL_ASYNC)

# Validate CUDA allocation strategy
if(NOT MEMORY_GPU_ALLOC MATCHES "^(SYNC|ASYNC|POOL_ASYNC)$")
  message(
    FATAL_ERROR
      "Invalid MEMEORY_GPU_ALLOC: ${MEMEORY_GPU_ALLOC}. Valid options are: SYNC, ASYNC, POOL_ASYNC"
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
option(PROJECT_SOBOL_1111 "Build support sobol with 1111 dimension" ON)

# LU Decomposition Pivoting Flag Controls whether LU decomposition uses pivoting for numerical
# stability. When enabled, improves numerical stability at the cost of additional computation.
option(PROJECT_LU_PIVOTING "Enable LU PIVOTING" OFF)

# =============================================================================
# Testing and Quality
# Assurance Configuration Flags
# =============================================================================
# These flags control testing infrastructure, benchmarking, and code quality tools.
# =============================================================================

# Build Testing Configuration Controls whether test directories are built: ON (always), OFF (never),
# or WANT (optional).
set(BUILD_TESTING "ON" CACHE STRING "Build module testing directories")
set_property(CACHE BUILD_TESTING PROPERTY STRINGS "ON;OFF;WANT")

# Build Examples Configuration Controls whether example programs are built. When enabled, builds
# comprehensive examples demonstrating parallel APIs and other features.
option(PROJECT_ENABLE_EXAMPLES "Build example programs" OFF)

# Google Benchmark Support Flag Controls whether Google Benchmark is enabled for performance
# benchmarking. Configure this at module scope.
option(MEMORY_ENABLE_BENCHMARK "Enable google benchmark for Memory" OFF)
option(PARALLEL_ENABLE_BENCHMARK "Enable google benchmark for Parallel" OFF)
option(CORE_ENABLE_BENCHMARK "Enable google benchmark for Core" OFF)

# Legacy aggregate project flag for shared tooling.
set(PROJECT_ENABLE_BENCHMARK OFF)
if(MEMORY_ENABLE_BENCHMARK OR PARALLEL_ENABLE_BENCHMARK OR CORE_ENABLE_BENCHMARK)
  set(PROJECT_ENABLE_BENCHMARK ON)
endif()

# Google Test Support Flag Controls whether Google Test framework is enabled for unit testing. When
# enabled, provides comprehensive testing infrastructure.
option(PROJECT_ENABLE_GTEST "Enable google test" ON)

# =============================================================================
# Performance and
# Optimization Configuration Flags
# =============================================================================
# These flags control performance-related settings and optimization strategies.
# =============================================================================

# Vectorization Strategy Configuration Specifies which SIMD vectorization to use: no, sse, avx,
# avx2, or avx512. Enables platform-specific optimizations for numerical computations.
set(PROJECT_VECTORIZATION_TYPE "avx2"
    CACHE STRING "vectorization implementation to use. Options are no, sse, avx, avx2, or avx512"
)
set_property(CACHE PROJECT_VECTORIZATION_TYPE PROPERTY STRINGS no sse avx avx2 avx512)

# =============================================================================
# Third-Party Library
# and Dependency Configuration Flags
# =============================================================================
# These flags control the use of external libraries and optional dependencies. Mandatory core
# dependencies: fmt (formatting), cpuinfo (CPU detection)
# =============================================================================
# Magic Enum Static Reflection Flag Controls whether magic_enum is enabled for compile-time enum
# reflection. When enabled, provides powerful enum introspection capabilities.
option(PROJECT_ENABLE_MAGICENUM "Enable magic_enum static reflection for enums." ON)
mark_as_advanced(PROJECT_ENABLE_MAGICENUM)

# Microsoft Mimalloc Memory Allocator Flag Controls whether Microsoft's mimalloc high-performance
# allocator is enabled. When enabled, provides faster memory allocation with better performance
# characteristics.
option(MEMORY_ENABLE_MIMALLOC
       "Enable Microsoft mimalloc high-performance memory allocator support." ON
)
mark_as_advanced(MEMORY_ENABLE_MIMALLOC)

# Module-local accelerator and coverage flags.
option(MEMORY_ENABLE_CUDA "Enable CUDA compilation for Memory" OFF)
option(MEMORY_ENABLE_HIP "Enable HIP compilation for Memory" OFF)
option(PARALLEL_ENABLE_COVERAGE "Enable coverage instrumentation for Parallel tests" OFF)
option(PROFILER_ENABLE_COVERAGE "Enable coverage instrumentation for Profiler tests" OFF)

# Profiler backend selection — the single control point for the profiler backend.
# PROFILER_ENABLE_KINETO, PROFILER_ENABLE_ITT, and PROFILER_ENABLE_NATIVE_PROFILER
# are derived from this value and are mutually exclusive. Do not set them directly.
# Defaults to KINETO for downstream compatibility.
set(PROFILER_TYPE "KINETO"
    CACHE STRING "Profiler backend type. Options are KINETO, NATIVE, or ITT")
set_property(CACHE PROFILER_TYPE PROPERTY STRINGS KINETO NATIVE ITT)

if(NOT PROFILER_TYPE MATCHES "^(KINETO|NATIVE|ITT)$")
  message(
    FATAL_ERROR
      "Invalid PROFILER_TYPE: ${PROFILER_TYPE}. Valid options are KINETO, NATIVE, or ITT")
endif()

if(PROFILER_TYPE STREQUAL "KINETO")
  set(PROFILER_ENABLE_KINETO          ON)
  set(PROFILER_ENABLE_ITT             OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(PROFILER_TYPE STREQUAL "ITT")
  set(PROFILER_ENABLE_KINETO          OFF)
  set(PROFILER_ENABLE_ITT             ON)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(PROFILER_TYPE STREQUAL "NATIVE")
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
  set(MEMORY_ENABLE_NUMA OFF)
endif()

# Memkind is only available on Unix systems (not macOS or Windows)
if(NOT UNIX OR APPLE)
  set(MEMORY_ENABLE_MEMKIND OFF)
endif()

# =============================================================================
# Global Project
# Variables
# =============================================================================
# Set global variables used throughout the build system.
# =============================================================================

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PROJECT_VERSION "${PROJECT_VERSION}")

# =============================================================================
# Build Library Type
# Configuration
# =============================================================================
# Controls whether libraries are built as shared or static. Quarisma requires shared libraries (DLLs
# on Windows) for proper functionality.
# =============================================================================

# Build Shared Libraries Flag Controls whether libraries are built as shared (ON) or static (OFF).
# Quarisma requires shared libraries for proper DLL export/import handling.
option(PROJECT_BUILD_SHARED_LIBS "Build shared libraries" ON)
set(BUILD_SHARED_LIBS ${PROJECT_BUILD_SHARED_LIBS})

# Include necessary CMake modules
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

# Include Quarisma-specific CMake modules
include(compiler_checks)
include(cache)
include(build_type)
include(checks)
include(utils)
include(platform)
include(compression)

# Create build interface target for common settings FIRST
# This target is needed by sanitize.cmake and other modules
add_library(quarismabuild INTERFACE)
add_library(Quarisma::build ALIAS quarismabuild)

# Project-scoped build aliases (module-local equivalent of Quarisma::build).
add_library(loggingbuild INTERFACE)
target_link_libraries(loggingbuild INTERFACE quarismabuild)
add_library(Logging::build ALIAS loggingbuild)

add_library(memorybuild INTERFACE)
target_link_libraries(memorybuild INTERFACE quarismabuild)
add_library(Memory::build ALIAS memorybuild)

add_library(parallelbuild INTERFACE)
target_link_libraries(parallelbuild INTERFACE quarismabuild)
add_library(Parallel::build ALIAS parallelbuild)

add_library(profilerbuild INTERFACE)
target_link_libraries(profilerbuild INTERFACE quarismabuild)
add_library(Profiler::build ALIAS profilerbuild)

add_library(corebuild INTERFACE)
target_link_libraries(corebuild INTERFACE quarismabuild)
add_library(Core::build ALIAS corebuild)

# Include sanitizer configuration EARLY to disable incompatible features
# Must be before compile_definitions so MEMORY_HAS_MIMALLOC is set correctly
include(sanitize)

# Configure optional features
include(mkl)
include(numa)
include(enzyme)
# Disable CORE_ENABLE_ENZYME if the Enzyme plugin was not found (enzyme.cmake sets
# PROJECT_ENABLE_ENZYME=OFF on failure but does not touch CORE_ENABLE_ENZYME).
if(CORE_ENABLE_ENZYME AND NOT TARGET Enzyme::enzyme)
  message(STATUS "Enzyme plugin not found - disabling CORE_ENABLE_ENZYME")
  set(CORE_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme" FORCE)
endif()
include(clang_tidy)
include(spell)
include(iwyu)
include(coverage)
include(valgrind)
include(helper_macros)
include(compile_definitions)

# Project-local CMake configuration.
# Module values are canonical; project-level values are fallback defaults only.
if(NOT DEFINED LOGGING_CXX_STANDARD OR LOGGING_CXX_STANDARD STREQUAL "")
  set(LOGGING_CXX_STANDARD ${PROJECT_CXX_STANDARD})
endif()
if(NOT DEFINED MEMORY_CXX_STANDARD OR MEMORY_CXX_STANDARD STREQUAL "")
  set(MEMORY_CXX_STANDARD ${PROJECT_CXX_STANDARD})
endif()
if(NOT DEFINED PARALLEL_CXX_STANDARD OR PARALLEL_CXX_STANDARD STREQUAL "")
  set(PARALLEL_CXX_STANDARD ${PROJECT_CXX_STANDARD})
endif()
if(NOT DEFINED PROFILER_CXX_STANDARD OR PROFILER_CXX_STANDARD STREQUAL "")
  set(PROFILER_CXX_STANDARD ${PROJECT_CXX_STANDARD})
endif()
set(PROFILER_ENABLE_ENZYME ${CORE_ENABLE_ENZYME})
if(NOT DEFINED PARALLEL_LLVM_PROFILE_FILE OR PARALLEL_LLVM_PROFILE_FILE STREQUAL "")
  set(PARALLEL_LLVM_PROFILE_FILE ${PROJECT_LLVM_PROFILE_FILE})
endif()
if(NOT DEFINED PROFILER_LLVM_PROFILE_FILE OR PROFILER_LLVM_PROFILE_FILE STREQUAL "")
  set(PROFILER_LLVM_PROFILE_FILE ${PROJECT_LLVM_PROFILE_FILE})
endif()

# Per-module dependency ownership (no global dependency fan-out).
set(LOGGING_DEPENDENCY_LIBS)
set(MEMORY_DEPENDENCY_LIBS)
set(PARALLEL_DEPENDENCY_LIBS)
set(PROFILER_DEPENDENCY_LIBS)
set(CORE_DEPENDENCY_LIBS)

set(LOGGING_DEPENDENCY_INCLUDE_DIRS)
set(MEMORY_DEPENDENCY_INCLUDE_DIRS)
set(PARALLEL_DEPENDENCY_INCLUDE_DIRS)
set(PROFILER_DEPENDENCY_INCLUDE_DIRS)
set(CORE_DEPENDENCY_INCLUDE_DIRS)

set(LOGGING_DEPENDENCY_COMPILE_DEFINITIONS)
set(MEMORY_DEPENDENCY_COMPILE_DEFINITIONS)
set(PARALLEL_DEPENDENCY_COMPILE_DEFINITIONS)
set(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS)
set(CORE_DEPENDENCY_COMPILE_DEFINITIONS)

# Keep compile-definition propagation from centralized feature flags.
set(LOGGING_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})
set(MEMORY_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})
set(PARALLEL_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})
set(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})
set(CORE_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})

# =============================================================================
# Third-Party Libraries
# =============================================================================
# Map project-level option names to the QUARISMA_ENABLE_* names used inside ThirdParty/.
set(QUARISMA_ENABLE_GTEST      ${PROJECT_ENABLE_GTEST})
set(QUARISMA_ENABLE_BENCHMARK  ${PROJECT_ENABLE_BENCHMARK})
set(QUARISMA_ENABLE_MAGICENUM  ${PROJECT_ENABLE_MAGICENUM})
set(MEMORY_ENABLE_MIMALLOC   ${MEMORY_ENABLE_MIMALLOC})
set(LOGGING_ENABLE_LOGURU     ${LOGGING_ENABLE_LOGURU})
set(LOGGING_ENABLE_GLOG       ${LOGGING_ENABLE_GLOG})
set(QUARISMA_ENABLE_ENZYME     ${CORE_ENABLE_ENZYME})

add_subdirectory(ThirdParty)

# Distribute the ThirdParty dependency list to each module.
# QUARISMA_DEPENDENCY_LIBS is populated by add_third_party_library() for libraries
# that have AUTO_ADD_TO_DEPENDENCIES enabled (fmt, cpuinfo, magic_enum, …).
# loguru, glog, mimalloc, kineto, and ITT use AUTO_ADD_TO_DEPENDENCIES OFF and are wired below.
set(LOGGING_DEPENDENCY_LIBS  ${QUARISMA_DEPENDENCY_LIBS})
set(MEMORY_DEPENDENCY_LIBS   ${QUARISMA_DEPENDENCY_LIBS})
set(PARALLEL_DEPENDENCY_LIBS ${QUARISMA_DEPENDENCY_LIBS})
set(PROFILER_DEPENDENCY_LIBS ${QUARISMA_DEPENDENCY_LIBS})
set(CORE_DEPENDENCY_LIBS     ${QUARISMA_DEPENDENCY_LIBS})

# loguru and glog are exclusively for the Logging module.
if(LOGGING_ENABLE_LOGURU AND TARGET Loguru::loguru)
  list(APPEND LOGGING_DEPENDENCY_LIBS Loguru::loguru)
endif()
if(LOGGING_ENABLE_GLOG AND TARGET Glog::glog)
  list(APPEND LOGGING_DEPENDENCY_LIBS Glog::glog)
endif()

# mimalloc is exclusively for the Memory module.
if(MEMORY_ENABLE_MIMALLOC AND TARGET Mimalloc::mimalloc)
  list(APPEND MEMORY_DEPENDENCY_LIBS Mimalloc::mimalloc)
endif()

# Kineto and ITT are exclusively for the Profiler module.
# Kineto uses AUTO_ADD_TO_DEPENDENCIES OFF and must be appended here explicitly.
# ITT is collected into QUARISMA_PROFILER_DEPENDENCY_LIBS by ThirdParty/CMakeLists.txt.
if(PROFILER_ENABLE_KINETO AND TARGET Kineto::kineto)
  list(APPEND PROFILER_DEPENDENCY_LIBS Kineto::kineto)
endif()
if(QUARISMA_PROFILER_DEPENDENCY_LIBS)
  list(APPEND PROFILER_DEPENDENCY_LIBS ${QUARISMA_PROFILER_DEPENDENCY_LIBS})
endif()
set(LOGGING_DEPENDENCY_INCLUDE_DIRS  ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(MEMORY_DEPENDENCY_INCLUDE_DIRS   ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(PARALLEL_DEPENDENCY_INCLUDE_DIRS ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(PROFILER_DEPENDENCY_INCLUDE_DIRS ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(CORE_DEPENDENCY_INCLUDE_DIRS     ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})

# Per-module gtest aliases so each testing CMakeLists can use Module::gtest.
# (LoggingCxxTests uses Gtest::gtest directly; the others use Module::gtest.)
if(TARGET Gtest::gtest)
  foreach(_mod Memory Core Parallel Profiler)
    if(NOT TARGET ${_mod}::gtest)
      add_library(${_mod}::gtest INTERFACE IMPORTED)
      target_link_libraries(${_mod}::gtest INTERFACE Gtest::gtest)
    endif()
    if(NOT TARGET ${_mod}::gtest_main)
      add_library(${_mod}::gtest_main INTERFACE IMPORTED)
      target_link_libraries(${_mod}::gtest_main INTERFACE Gtest::gtest_main)
    endif()
  endforeach()
endif()

# Include optimization modules AFTER quarismabuild target is created These modules apply flags to the
# quarismabuild target to affect only Quarisma targets, not third-party dependencies
include(linker)

# Set compile features
target_compile_features(quarismabuild INTERFACE cxx_std_${PROJECT_CXX_STANDARD})

# Apply common linker flags
if(DEFINED PROJECT_LINKER_FLAGS_COMMON)
  target_link_options(quarismabuild INTERFACE ${PROJECT_LINKER_FLAGS_COMMON})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

add_subdirectory(Library/Logging)

# Memory depends on Logging; expose it under the Memory:: namespace so
# Library/Memory/CMakeLists.txt can link to Memory::Logging without hard-coding
# a cross-module namespace dependency.
add_library(Memory::Logging ALIAS Logging)

add_subdirectory(Library/Memory)
add_subdirectory(Library/Core)
add_subdirectory(Library/Parallel)
add_subdirectory(Library/Profiler)

# Add examples if enabled
if(PROJECT_ENABLE_EXAMPLES)
  add_subdirectory(Examples/Profiling)
  message(STATUS "✓ Profiling Examples enabled")
endif()

# Apply Valgrind timeout multiplier to all tests (if Valgrind is enabled) This must be called after
# all tests are registered
if(PROJECT_ENABLE_VALGRIND AND BUILD_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

# Include summary at the end
include(summary)
print_configuration_summary()
