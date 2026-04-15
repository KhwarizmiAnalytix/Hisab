# Logging module (standalone; same layout pattern as Library/Parallel).
cmake_minimum_required(VERSION 3.16)

option(LOGGING_ENABLE_LTO "Enable Link Time Optimization for Logging" OFF)

# LOGGING_BACKEND, LOGGING_ENABLE_LOGURU/GLOG/NATIVE are declared in
# Library/Logging/Cmake/logging_backend.cmake (included by the root
# CMakeLists.txt before ThirdParty so loguru/glog targets are gated correctly).

# Map LOGGING_BACKEND to LOGGING_HAS_* compile definitions
if(LOGGING_BACKEND STREQUAL "LOGURU")
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=1 LOGGING_HAS_GLOG=0 LOGGING_HAS_NATIVE=0)
elseif(LOGGING_BACKEND STREQUAL "GLOG")
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=1 LOGGING_HAS_NATIVE=0)
elseif(LOGGING_BACKEND STREQUAL "NATIVE")
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=0 LOGGING_HAS_NATIVE=1)
endif()
list(APPEND LOGGING_DEPENDENCY_COMPILE_DEFINITIONS ${LOGGING_COMPILE_DEFINITIONS})

set(logging_headers)
set(logging_sources)

if(NOT DEFINED LOGGING_CXX_STANDARD OR LOGGING_CXX_STANDARD STREQUAL "")
  set(LOGGING_CXX_STANDARD ${CMAKE_CXX_STANDARD})
endif()

file(
  GLOB_RECURSE logging_headers
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
)

file(
  GLOB_RECURSE logging_sources
  LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# Keep test sources out of the Logging library target.
list(FILTER logging_sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER logging_headers EXCLUDE REGEX ".*/Testing/.*")

if(LOGGING_ENABLE_NATIVE)
  list(REMOVE_ITEM logging_sources "${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp")
  list(REMOVE_ITEM logging_headers "${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.h")
  message(
    STATUS
      "NATIVE logging backend: Excluding logger.cpp and logger.h (fmt-based implementation)"
  )
endif()

add_library(Logging ${logging_sources} ${logging_headers})

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Logging PUBLIC LOGGING_SHARED_DEFINE PRIVATE LOGGING_BUILDING_DLL)
  if(COMMAND logging_set_target_output_directories)
    logging_set_target_output_directories(Logging)
  endif()
else()
  target_compile_definitions(Logging PUBLIC LOGGING_STATIC_DEFINE)
endif()

add_library(Logging::Logging ALIAS Logging)

if(LOGGING_ENABLE_LTO)
  set_target_properties(Logging PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(COMMAND logging_apply_iwyu)
  logging_apply_iwyu(Logging)
endif()

if(COMMAND logging_module_create_filters)
  logging_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set_target_properties(
  Logging PROPERTIES CXX_STANDARD ${LOGGING_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Logging PRIVATE /WX)
else()
  target_compile_options(Logging PRIVATE -include cstdlib)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/LoggingTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/LoggingTest.h"
  @ONLY
)

# Include roots: Library/Logging → "util/exception.h", "common/logging_export.h"
#                Library/Logging/logger → "logger.h", "back_trace.h", "logger_verbosity_enum.h"
target_include_directories(
  Logging
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/logger>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

if(LOGGING_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Logging PUBLIC ${LOGGING_DEPENDENCY_INCLUDE_DIRS})
endif()

target_link_libraries(Logging PUBLIC Logging::build)

if(LOGGING_ENABLE_LOGURU AND TARGET Loguru::loguru)
  list(APPEND LOGGING_DEPENDENCY_LIBS Loguru::loguru)
endif()
if(LOGGING_ENABLE_GLOG AND TARGET Glog::glog)
  list(APPEND LOGGING_DEPENDENCY_LIBS Glog::glog)
endif()

if(COMMAND logging_target_clang_tidy)
  logging_target_clang_tidy(Logging)
endif()

if(LOGGING_DEPENDENCY_LIBS)
  target_link_libraries(Logging PUBLIC ${LOGGING_DEPENDENCY_LIBS})
endif()

if(LOGGING_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Logging PUBLIC ${LOGGING_DEPENDENCY_COMPILE_DEFINITIONS})
endif()

if(WIN32 AND NOT MSVC)
  target_link_libraries(Logging PRIVATE -ldbghelp)
endif()

install(
  TARGETS Logging
  EXPORT LoggingTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

install(FILES ${logging_headers} DESTINATION include/Logging/Logging COMPONENT Development)

if(BUILD_TESTING)
  add_subdirectory(Testing/Cxx)
endif()
