cmake_minimum_required(VERSION 3.16)

set(profiler_headers)
set(profiler_sources)

if(NOT DEFINED PROFILER_CXX_STANDARD OR PROFILER_CXX_STANDARD STREQUAL "")
  set(PROFILER_CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif()

# =============================================================================
# Common utilities (always compiled: approximate_clock, small_vector, etc.)
# =============================================================================
file(
  GLOB common_util_headers
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
)
file(
  GLOB common_util_sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
)
list(APPEND profiler_headers ${common_util_headers})
list(APPEND profiler_sources ${common_util_sources})

# =============================================================================
# Kineto profiler backend
# base/ is shared infrastructure for Kineto and ITT — included with either.
# =============================================================================
if(PROFILER_ENABLE_KINETO)
  file(
    GLOB_RECURSE kineto_headers
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h"
  )
  file(
    GLOB_RECURSE kineto_sources
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp"
  )
  # Exclude unwind sources from kineto build (platform-specific unwinder, not needed for kineto)
  file(GLOB_RECURSE KINETO_UNWIND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/unwind/*.cpp")
  list(REMOVE_ITEM kineto_sources ${KINETO_UNWIND_SOURCES})

  list(APPEND profiler_headers ${kineto_headers})
  list(APPEND profiler_sources ${kineto_sources})
  message(STATUS "Kineto profiler enabled: including base, kineto, and common sources")
endif()

# =============================================================================
# Native profiler backend (standalone)
# =============================================================================
if(PROFILER_ENABLE_NATIVE_PROFILER)
  file(
    GLOB_RECURSE native_headers
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/native/*.h"
  )
  file(
    GLOB_RECURSE native_sources
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/native/*.cpp"
  )
  list(APPEND profiler_headers ${native_headers})
  list(APPEND profiler_sources ${native_sources})
  message(STATUS "Native profiler enabled: including native sources (standalone)")
endif()

# =============================================================================
# ITT profiler backend
# base/ is shared infrastructure for Kineto and ITT — included with either.
# Guard against double-inclusion when both Kineto and ITT are enabled.
# =============================================================================
if(PROFILER_ENABLE_ITT)
  if(NOT PROFILER_ENABLE_KINETO)
    file(
      GLOB_RECURSE itt_base_headers
      LIST_DIRECTORIES false
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h"
    )
    file(
      GLOB_RECURSE itt_base_sources
      LIST_DIRECTORIES false
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp"
    )
    list(APPEND profiler_headers ${itt_base_headers})
    list(APPEND profiler_sources ${itt_base_sources})
  endif()
  file(
    GLOB_RECURSE itt_headers
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.h"
  )
  file(
    GLOB_RECURSE itt_sources
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.cpp"
  )
  list(APPEND profiler_headers ${itt_headers})
  list(APPEND profiler_sources ${itt_sources})
  message(STATUS "ITT profiler enabled: including base and ITT sources")
endif()

# Create the Profiler library Respect BUILD_SHARED_LIBS setting for library type
add_library(Profiler ${profiler_sources} ${profiler_headers})

# Set appropriate compile definitions based on library type
if(BUILD_SHARED_LIBS)
  target_compile_definitions(Profiler PUBLIC PROFILER_SHARED_DEFINE PRIVATE PROFILER_BUILDING_DLL)
  # Set output directories using Profiler's standardized approach
  if(COMMAND profiler_set_target_output_directories)
    profiler_set_target_output_directories(Profiler)
  endif()
else()
  target_compile_definitions(Profiler PUBLIC PROFILER_STATIC_DEFINE)
endif()

# Create alias for consistent naming
add_library(Profiler::Profiler ALIAS Profiler)

# Enable Link Time Optimization (LTO) for Profiler library if enabled LTO is disabled for third-party
# libraries via FILTER_FLAGS to avoid conflicts
if(PROFILER_ENABLE_LTO)
  set_target_properties(Profiler PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Profiler library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Profiler library: Link Time Optimization (LTO) disabled")
endif()

# Apply IWYU to the Profiler target (if available)
if(COMMAND profiler_apply_iwyu)
  profiler_apply_iwyu(Profiler)
endif()

# Configure Visual Studio source groups to preserve directory structure
if(COMMAND profiler_module_create_filters)
  profiler_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set_target_properties(
  Profiler PROPERTIES CXX_STANDARD ${PROFILER_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)
target_include_directories(
  Profiler
  BEFORE
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
    $<INSTALL_INTERFACE:include>
)

# Generate per-project test header from template
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/ProfilerTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/ProfilerTest.h"
  @ONLY
)

# Add centralized third-party include directories as PUBLIC
if(PROFILER_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Profiler PUBLIC ${PROFILER_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Profiler library include directories: ${PROFILER_DEPENDENCY_INCLUDE_DIRS}")
endif()

target_link_libraries(Profiler PUBLIC Profiler::build)

# Link with centralized dependency list
if(PROFILER_DEPENDENCY_LIBS)
  target_link_libraries(Profiler PUBLIC ${PROFILER_DEPENDENCY_LIBS})
  message(STATUS "Profiler library linked with centralized dependencies: ${PROFILER_DEPENDENCY_LIBS}")
endif()

# Apply centralized compile definitions
if(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Profiler PUBLIC ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Profiler library compile definitions: ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()
# =============================================================================
# Testing
# =============================================================================
if(BUILD_TESTING)
  add_subdirectory(Testing/Cxx)
endif()
