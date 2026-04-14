# Parallel module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Note: Threading configuration is handled centrally by Cmake/tools/threads.cmake.
# The variables PARALLEL_USE_PTHREADS, PARALLEL_USE_WIN32_THREADS, and
# PARALLEL_MAX_THREADS are set by threads.cmake and available here.
# Backend selection (PARALLEL_PARALLEL_BACKEND) is set by parallel_backend.cmake.

set(parallel_headers)
set(parallel_sources)

if(NOT DEFINED PARALLEL_CXX_STANDARD OR PARALLEL_CXX_STANDARD STREQUAL "")
  set(PARALLEL_CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif()

file(
  GLOB_RECURSE parallel_headers
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(
  GLOB_RECURSE parallel_sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Exclude test and CMake-generated files from the library glob
list(FILTER parallel_headers EXCLUDE REGEX ".*/Testing/.*")
list(FILTER parallel_sources EXCLUDE REGEX ".*/Testing/.*")

# Remove TBB backend files if TBB is not enabled
if(NOT PARALLEL_ENABLE_TBB)
  file(GLOB_RECURSE TBB_PARALLEL_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tbb/*.cpp")
  file(GLOB_RECURSE TBB_PARALLEL_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tbb/*.h")
  list(REMOVE_ITEM parallel_sources ${TBB_PARALLEL_SOURCE_FILES})
  list(REMOVE_ITEM parallel_headers ${TBB_PARALLEL_HEADER_FILES})
  message(STATUS "TBB disabled: Excluding TBB parallel source files from build")
endif()

# Remove OpenMP backend files if OpenMP is not enabled
if(NOT PARALLEL_ENABLE_OPENMP)
  file(GLOB_RECURSE OPENMP_PARALLEL_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/openmp/*.cpp")
  file(GLOB_RECURSE OPENMP_PARALLEL_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/openmp/*.h")
  list(REMOVE_ITEM parallel_sources ${OPENMP_PARALLEL_SOURCE_FILES})
  list(REMOVE_ITEM parallel_headers ${OPENMP_PARALLEL_HEADER_FILES})
  message(STATUS "OpenMP disabled: Excluding OpenMP parallel source files from build")
endif()

# Create the Parallel library. Respect BUILD_SHARED_LIBS setting for library type.
add_library(Parallel ${parallel_sources} ${parallel_headers})

# Set appropriate compile definitions based on library type
if(BUILD_SHARED_LIBS)
  target_compile_definitions(Parallel PUBLIC PARALLEL_SHARED_DEFINE PRIVATE PARALLEL_BUILDING_DLL)
  if(COMMAND parallel_set_target_output_directories)
    parallel_set_target_output_directories(Parallel)
  endif()
else()
  target_compile_definitions(Parallel PUBLIC PARALLEL_STATIC_DEFINE)
endif()

# Create alias for consistent naming
add_library(Parallel::Parallel ALIAS Parallel)

if(PARALLEL_ENABLE_LTO)
  set_target_properties(Parallel PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Parallel library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Parallel library: Link Time Optimization (LTO) disabled")
endif()

# Apply IWYU to the Parallel target (if available)
if(COMMAND parallel_apply_iwyu)
  parallel_apply_iwyu(Parallel)
endif()

# Configure Visual Studio source groups to preserve directory structure
if(COMMAND parallel_module_create_filters)
  parallel_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set_target_properties(
  Parallel PROPERTIES CXX_STANDARD ${PARALLEL_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Parallel PRIVATE /WX)
else()
  target_compile_options(Parallel PRIVATE -include cstdlib)
endif()

# =============================================================================
# Include Directories
# =============================================================================
# Library/Parallel is the include root so that:
#   "common/parallel_export.h"              -> Library/Parallel/common/parallel_export.h
#   "tools/parallel_tools.h"                -> Library/Parallel/tools/parallel_tools.h
#   "common/parallel_tools_api.h"           -> Library/Parallel/common/...

target_include_directories(
  Parallel
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

# Generate per-project test header from template
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/ParallelTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/ParallelTest.h"
  @ONLY
)

if(PARALLEL_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Parallel PUBLIC ${PARALLEL_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Parallel library include directories: ${PARALLEL_DEPENDENCY_INCLUDE_DIRS}")
endif()

# Link with build interface target (compiler flags, C++ standard, etc.)
target_link_libraries(Parallel PUBLIC Parallel::build)

if(COMMAND parallel_target_clang_tidy)
  parallel_target_clang_tidy(Parallel)
endif()

# =============================================================================
# Link with Centralized Dependencies
# =============================================================================
if(PARALLEL_DEPENDENCY_LIBS)
  target_link_libraries(Parallel PUBLIC ${PARALLEL_DEPENDENCY_LIBS})
  message(STATUS "Parallel library linked with centralized dependencies: ${PARALLEL_DEPENDENCY_LIBS}")
endif()

# =============================================================================
# Compile Definitions for Optional Features
# =============================================================================
if(PARALLEL_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Parallel PUBLIC ${PARALLEL_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Parallel library compile definitions: ${PARALLEL_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# Install targets
install(
  TARGETS Parallel
  EXPORT ParallelTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

# Install headers
install(FILES ${parallel_headers} DESTINATION include/Parallel/Parallel COMPONENT Development)

# Configure testing
if(BUILD_TESTING)
  add_subdirectory(Testing/Cxx)
endif()
