# ============================================================================= Quarisma Main CMake
# Configuration
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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Core/Cmake")
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
set(PROJECT_VERSION_FULL
    "${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_BUILD_VERSION}"
)

set(CMAKE_COLOR_DIAGNOSTICS ON)

# ============================================================================= Global C++ standard
# All five library modules default to C++20.  Each module declares its own XXX_CXX_STANDARD cache
# variable so that a single module can be overridden independently; those per-module
# set(CMAKE_CXX_STANDARD ...) calls are the only override point — CMAKE_CXX_STANDARD_REQUIRED and
# CMAKE_CXX_EXTENSIONS are fixed project-wide and must not be duplicated in module CMakeLists.txt.
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================= Global Project
# Variables
# =============================================================================

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PROJECT_VERSION "${PROJECT_VERSION}")

# ============================================================================= Build Library Type
# =============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

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

# ============================================================================= Third-Party
# Libraries
# =============================================================================

# Aggregate per-module gtest/benchmark flags: ThirdParty builds GoogleTest / Benchmark only when at
# least one module requests them.
set(QUARISMA_ENABLE_GTEST OFF)
if(LOGGING_ENABLE_GTEST
   OR MEMORY_ENABLE_GTEST
   OR CORE_ENABLE_GTEST
   OR PARALLEL_ENABLE_GTEST
   OR PROFILER_ENABLE_GTEST
)
  set(QUARISMA_ENABLE_GTEST ON)
endif()

set(QUARISMA_ENABLE_BENCHMARK OFF)
if(LOGGING_ENABLE_BENCHMARK
   OR MEMORY_ENABLE_BENCHMARK
   OR CORE_ENABLE_BENCHMARK
   OR PARALLEL_ENABLE_BENCHMARK
   OR PROFILER_ENABLE_BENCHMARK
)
  set(QUARISMA_ENABLE_BENCHMARK ON)
endif()

add_subdirectory(ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

add_subdirectory(Library/Logging)
add_subdirectory(Library/Memory)
add_subdirectory(Library/Core)
add_subdirectory(Library/Parallel)
add_subdirectory(Library/Profiler)

include(summary)
print_configuration_summary()
