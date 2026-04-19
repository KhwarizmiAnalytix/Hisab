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

enable_testing()

# Always use bundled googletest — external shared gtest (e.g. from conda) causes rpath failures
# at runtime because its dylib install path is not in our test executables' rpath.
set(CMAKE_DISABLE_FIND_PACKAGE_GTest TRUE)


set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PROJECT_VERSION "${PROJECT_VERSION}")


option(BUILD_SHARED_LIBS "Build shared libraries" ON)

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
