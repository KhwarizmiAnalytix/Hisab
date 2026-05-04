# =============================================================================
# Quarisma Main
# CMake Configuration
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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Library/Vectorization/Cmake")
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

# Always use bundled googletest — external shared gtest (e.g. from conda) causes rpath failures at
# runtime because its dylib install path is not in our test executables' rpath.
set(CMAKE_DISABLE_FIND_PACKAGE_GTest TRUE)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PROJECT_VERSION "${PROJECT_VERSION}")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_subdirectory(ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# Optional: build only one Library/* module and its CMake dependencies (see Scripts/setup.py
# --project.NAME). When empty or unset, all Library/* projects are enabled (full monolith).
# setup.py always passes -DQUARISMA_LIBRARY_PROJECT=... so omitting --project.* resets any prior
# single-library configure.
set(QUARISMA_LIBRARY_PROJECT ""
    CACHE STRING
    "If non-empty: only this Library module and deps (Logging|Memory|Vectorization|Core|Parallel|Profiler). Empty: all Library/* (default)."
)

string(TOLOWER "${QUARISMA_LIBRARY_PROJECT}" _quarisma_libproj_lc)

set(_quarisma_lib_order Logging Memory Vectorization Core Parallel Profiler)

if(_quarisma_libproj_lc STREQUAL "")
  foreach(_lib IN LISTS _quarisma_lib_order)
    add_subdirectory(Library/${_lib})
  endforeach()
else()
  if(_quarisma_libproj_lc STREQUAL "logging")
    set(_quarisma_lib_enabled Logging)
  elseif(_quarisma_libproj_lc STREQUAL "memory")
    # Memory only — no Library/Logging. Memory supports MEMORY_ENABLE_LOGGING=OFF (no logger headers).
    set(MEMORY_ENABLE_LOGGING OFF CACHE BOOL
        "Link Memory with the Logging library when available" FORCE)
    set(_quarisma_lib_enabled Memory)
  elseif(_quarisma_libproj_lc STREQUAL "vectorization")
    set(MEMORY_ENABLE_TESTING OFF CACHE BOOL
        "Link Memory with the Logging library when available" FORCE)
    set(MEMORY_ENABLE_BENCHMARK OFF CACHE BOOL
        "Link Memory with the Logging library when available" FORCE)
    set(_quarisma_lib_enabled Memory Vectorization)
  elseif(_quarisma_libproj_lc STREQUAL "core")
    set(_quarisma_lib_enabled Parallel Profiler Logging Memory Vectorization Core)
  elseif(_quarisma_libproj_lc STREQUAL "parallel")
    set(_quarisma_lib_enabled Parallel)
  elseif(_quarisma_libproj_lc STREQUAL "profiler")
    set(_quarisma_lib_enabled Profiler)
  else()
    message(
      FATAL_ERROR
        "QUARISMA_LIBRARY_PROJECT='${QUARISMA_LIBRARY_PROJECT}' is invalid. "
        "Use one of: Logging, Memory, Vectorization, Core, Parallel, Profiler, or leave empty for all."
    )
  endif()

  message(STATUS "Quarisma: single-library mode — enabling: ${_quarisma_lib_enabled}")

  foreach(_lib IN LISTS _quarisma_lib_order)
    if(_lib IN_LIST _quarisma_lib_enabled)
      add_subdirectory(Library/${_lib})
    endif()
  endforeach()
endif()

quarisma_clang_windows_gtest_workaround()

include(summary)
print_configuration_summary()
