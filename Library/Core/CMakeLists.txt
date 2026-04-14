# Core module: utilities, shared ownership, compression — Memory and Logging are //Library/Memory and
# //Library/Logging (CMake + Bazel), same pattern as Parallel.
cmake_minimum_required(VERSION 3.16)

set(quarisma_headers)
set(quarisma_sources)
set(quarisma_templates)

file(
  GLOB_RECURSE headers
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  ${quarisma_headers}
)

file(
  GLOB_RECURSE sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/c17/*.h"
  ${quarisma_sources}
)

file(
  GLOB_RECURSE templates
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/util/*.hxx" "${CMAKE_CURRENT_SOURCE_DIR}/common/*.hxx"
  ${quarisma_templates}
)

if(CORE_ENABLE_COMPRESSION)
  if(CORE_COMPRESSION_TYPE STREQUAL "SNAPPY")
    list(APPEND headers "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.h")
    list(APPEND sources "${CMAKE_CURRENT_SOURCE_DIR}/compression/snappy.cpp")
  endif()
endif()

add_library(Core ${sources} ${headers} ${templates})

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Core PUBLIC QUARISMA_SHARED_DEFINE PRIVATE QUARISMA_BUILDING_DLL)
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Core)
  endif()
else()
  target_compile_definitions(Core PUBLIC QUARISMA_STATIC_DEFINE)
endif()

add_library(Quarisma::Core ALIAS Core)
add_library(Core::Core ALIAS Core)

if(CORE_ENABLE_LTO)
  set_target_properties(Core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "Core library: Link Time Optimization (LTO) enabled")
else()
  message(STATUS "Core library: Link Time Optimization (LTO) disabled")
endif()

if(COMMAND quarisma_apply_iwyu)
  quarisma_apply_iwyu(Core)
endif()

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}, std++=${PROJECT_CXX_STANDARD}")

set_target_properties(
  Core PROPERTIES CXX_STANDARD ${PROJECT_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Core PRIVATE /WX)
else()
  target_compile_options(Core PRIVATE -include cstdlib)
endif()

target_include_directories(
  Core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/CoreTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/CoreTest.h"
  @ONLY
)

if(CORE_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Core PUBLIC ${CORE_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Core library include directories: ${CORE_DEPENDENCY_INCLUDE_DIRS}")
endif()

target_link_libraries(
  Core
  PUBLIC Core::build
         Logging::Logging
         Memory::Memory
)

if(COMMAND quarisma_target_clang_tidy)
  quarisma_target_clang_tidy(Core)
  set_source_files_properties(PROPERTIES SKIP_LINTING ON)
endif()

# Core-local dependency list (e.g. fmt/cpuinfo for util).
if(CORE_DEPENDENCY_LIBS)
  target_link_libraries(Core PUBLIC ${CORE_DEPENDENCY_LIBS})
  message(STATUS "Core library linked with dependencies: ${CORE_DEPENDENCY_LIBS}")
endif()

if(CORE_ENABLE_ENZYME)
  target_link_libraries(Core PRIVATE Core::enzyme)
endif()

if(CORE_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Core PUBLIC ${CORE_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Core library compile definitions: ${CORE_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

if(MEMORY_ENABLE_MIMALLOC AND TARGET Memory::mimalloc)
  add_dependencies(Core Memory::mimalloc)
endif()

install(
  TARGETS Core
  EXPORT QuarismaTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

install(FILES ${headers} DESTINATION include/Quarisma/Core COMPONENT Development)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/quarisma_version_macros.h"
              "${CMAKE_CURRENT_BINARY_DIR}/quarisma_threads.h"
              "${CMAKE_CURRENT_BINARY_DIR}/Testing/CoreTest.h"
        DESTINATION include/Quarisma/Core
        COMPONENT Development
)

if(BUILD_TESTING)
  add_subdirectory(Testing/Cxx)
endif()
