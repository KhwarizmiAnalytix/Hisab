# =============================================================================
# Quarisma Main CMake Configuration
# =============================================================================
# Third-party deps: ThirdParty/CMakeLists.txt. Feature flags → compile defs:
# Cmake/flags/compile_definitions.cmake. Library modules: Library/*/CMakeLists.txt.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source build are not supported")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/tools")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/flags")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Cmake/packages")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Logging/Cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Parallel/Cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Memory/Cmake")
include(xcode_toolchain)

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

set(CMAKE_COLOR_DIAGNOSTICS ON)

# =============================================================================
# Global C++ standard
# All five library modules default to C++20.  Each module declares its own
# XXX_CXX_STANDARD cache variable so that a single module can be overridden
# independently; those per-module set(CMAKE_CXX_STANDARD ...) calls are the
# only override point — CMAKE_CXX_STANDARD_REQUIRED and CMAKE_CXX_EXTENSIONS
# are fixed project-wide and must not be duplicated in module CMakeLists.txt.
# =============================================================================
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)



# =============================================================================
# Testing and Quality Assurance Configuration Flags
# =============================================================================

set(BUILD_TESTING "ON" CACHE STRING "Build module testing directories")
set_property(CACHE BUILD_TESTING PROPERTY STRINGS "ON;OFF;WANT")

option(PROJECT_ENABLE_EXAMPLES "Build example programs" OFF)

option(ENABLE_GTEST "Enable google test" ON)

option(ENABLE_BENCHMARK "Enable google benchmark library" OFF)

set(VECTORIZATION_TYPE "avx2"
    CACHE STRING "vectorization implementation to use. Options are no, sse, avx, avx2, or avx512"
)
set_property(CACHE VECTORIZATION_TYPE PROPERTY STRINGS no sse avx avx2 avx512)

option(CORE_ENABLE_MAGICENUM "Enable magic_enum static reflection for enums." ON)
mark_as_advanced(CORE_ENABLE_MAGICENUM)

# =============================================================================
# Global Project Variables
# =============================================================================

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PROJECT_VERSION "${PROJECT_VERSION}")

# =============================================================================
# Build Library Type
# =============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)


include(compression)

add_library(quarismabuild INTERFACE)
add_library(Quarisma::build ALIAS quarismabuild)

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

include(sanitize)

include(mkl)
include(enzyme)
if(CORE_ENABLE_ENZYME AND NOT TARGET Enzyme::enzyme)
  message(STATUS "Enzyme plugin not found - disabling CORE_ENABLE_ENZYME")
  set(CORE_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme" FORCE)
endif()
include(clang_tidy)
include(spell)
include(iwyu)
include(valgrind)
include(helper_macros)

if(NOT DEFINED PARALLEL_LLVM_PROFILE_FILE OR PARALLEL_LLVM_PROFILE_FILE STREQUAL "")
  set(PARALLEL_LLVM_PROFILE_FILE ${PROJECT_LLVM_PROFILE_FILE})
endif()
if(NOT DEFINED PROFILER_LLVM_PROFILE_FILE OR PROFILER_LLVM_PROFILE_FILE STREQUAL "")
  set(PROFILER_LLVM_PROFILE_FILE ${PROJECT_LLVM_PROFILE_FILE})
endif()

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

# =============================================================================
# Third-Party Libraries
# =============================================================================
set(QUARISMA_ENABLE_GTEST      ${ENABLE_GTEST})
set(QUARISMA_ENABLE_BENCHMARK  ${ENABLE_BENCHMARK})
set(CORE_ENABLE_MAGICENUM  ${CORE_ENABLE_MAGICENUM})
set(CORE_ENABLE_ENZYME     ${CORE_ENABLE_ENZYME})

include(logging_backend)

set(_PROJECT_BUILD_TESTING ${BUILD_TESTING})
add_subdirectory(ThirdParty)
set(BUILD_TESTING ${_PROJECT_BUILD_TESTING} CACHE STRING "Build module testing directories" FORCE)
unset(_PROJECT_BUILD_TESTING)

set(LOGGING_DEPENDENCY_LIBS  ${QUARISMA_DEPENDENCY_LIBS})
set(MEMORY_DEPENDENCY_LIBS   ${QUARISMA_DEPENDENCY_LIBS})
set(PARALLEL_DEPENDENCY_LIBS ${QUARISMA_DEPENDENCY_LIBS})
set(PROFILER_DEPENDENCY_LIBS ${QUARISMA_DEPENDENCY_LIBS})
set(CORE_DEPENDENCY_LIBS     ${QUARISMA_DEPENDENCY_LIBS})
set(LOGGING_DEPENDENCY_INCLUDE_DIRS  ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(MEMORY_DEPENDENCY_INCLUDE_DIRS   ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(PARALLEL_DEPENDENCY_INCLUDE_DIRS ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(PROFILER_DEPENDENCY_INCLUDE_DIRS ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})
set(CORE_DEPENDENCY_INCLUDE_DIRS     ${QUARISMA_DEPENDENCY_INCLUDE_DIRS})

include(linker)

if(DEFINED PROJECT_LINKER_FLAGS_COMMON)
  target_link_options(quarismabuild INTERFACE ${PROJECT_LINKER_FLAGS_COMMON})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

if(BUILD_TESTING)
  enable_testing()
endif()

# Forward-declare module LTO options so the PROJECT_ENABLE_LTO aggregate can be
# computed before add_subdirectory().  Each module's CMakeLists.txt re-declares
# the same option(); because CMake cache variables are set on first write, those
# subsequent option() calls are intentional no-ops — not redundancies.
option(LOGGING_ENABLE_LTO "Enable Link Time Optimization for Logging" OFF)
option(MEMORY_ENABLE_LTO "Enable Link Time Optimization for Memory" OFF)
option(PARALLEL_ENABLE_LTO "Enable Link Time Optimization for Parallel" OFF)
option(PROFILER_ENABLE_LTO "Enable Link Time Optimization for Profiler" OFF)
option(CORE_ENABLE_LTO "Enable Link Time Optimization for Core" OFF)

set(PROJECT_ENABLE_LTO OFF)
if(LOGGING_ENABLE_LTO
   OR MEMORY_ENABLE_LTO
   OR PARALLEL_ENABLE_LTO
   OR PROFILER_ENABLE_LTO
   OR CORE_ENABLE_LTO)
  set(PROJECT_ENABLE_LTO ON)
endif()

if(PROJECT_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  message(STATUS "Link Time Optimization (LTO) disabled")
endif()

add_subdirectory(Library/Logging)
add_subdirectory(Library/Memory)
add_subdirectory(Library/Core)
add_subdirectory(Library/Parallel)
add_subdirectory(Library/Profiler)

if(PROJECT_ENABLE_EXAMPLES)
  add_subdirectory("${CMAKE_SOURCE_DIR}/Examples/Profiling" "${CMAKE_BINARY_DIR}/Examples/Profiling")
  message(STATUS "✓ Profiling Examples enabled")
endif()

if(PROJECT_ENABLE_VALGRIND AND BUILD_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

include(summary)
print_configuration_summary()
