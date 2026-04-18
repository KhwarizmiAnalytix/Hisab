# ============================================================================= Library/Logging —
# CMakeLists.txt
#
# Provides the Logging library: structured log levels, back-trace capture, runtime format strings,
# and pluggable backend support (Loguru, GLog, Native).
#
# Public compile definitions emitted by this file: LOGGING_HAS_LOGURU=1/0  — Loguru backend active
# LOGGING_HAS_GLOG=1/0    — GLog backend active LOGGING_HAS_NATIVE=1/0  — Native (fmt-based) backend
# active
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# ============================================================================= SECTION 1 — Feature
# flags (LOGGING_ENABLE_*) and backend selector
#
# These cache variables are the public configuration API for this module. Pass them on the CMake
# command line: -DLOGGING_ENABLE_LTO=ON
#
# Note: LOGGING_BACKEND and the derived LOGGING_ENABLE_LOGURU/GLOG/NATIVE are declared in
# Library/Logging/Cmake/logging_backend.cmake, which the root CMakeLists.txt includes before
# add_subdirectory(ThirdParty) so that the correct backend third-party target (Loguru, GLog) is
# built first.
# =============================================================================

# Link-Time Optimization — disabled when LOGGING_ENABLE_COVERAGE or LOGGING_ENABLE_SANITIZER is ON
# (LTO conflicts with instrumentation / sanitizers).
option(LOGGING_ENABLE_LTO "Enable Link Time Optimization for Logging" OFF)

# Coverage instrumentation — forces LOGGING_ENABLE_LTO=OFF and activates -fprofile-instr-generate /
# --coverage compiler flags for this module only.
option(LOGGING_ENABLE_COVERAGE "Enable coverage instrumentation for Logging" OFF)

# Testing — gates add_subdirectory(Testing/Cxx).
option(LOGGING_ENABLE_TESTING "Build Logging test suite" ON)

# Examples — gates add_subdirectory(Examples) when Logging examples exist.
option(LOGGING_ENABLE_EXAMPLES "Build Logging example programs" OFF)

# Google Test integration for Logging tests.
option(LOGGING_ENABLE_GTEST "Enable GoogleTest for Logging" ON)

# Google Benchmark integration for Logging micro-benchmarks.
option(LOGGING_ENABLE_BENCHMARK "Enable Google Benchmark for Logging" OFF)

# Icecream distributed compilation launcher (rarely needed; hidden by default).
option(LOGGING_ENABLE_ICECC "Use Icecream (icecc) distributed compiler for Logging" OFF)
mark_as_advanced(LOGGING_ENABLE_ICECC)

# Per-target compiler cache — wraps every compile command with ccache / sccache etc. to speed up
# incremental and CI rebuilds. ON by default.
option(LOGGING_ENABLE_CACHE "Enable compiler caching for the Logging target" ON)
mark_as_advanced(LOGGING_ENABLE_CACHE)

option(LOGGING_ENABLE_CLANGTIDY "Enable clang-tidy static analysis for Logging" OFF)
mark_as_advanced(LOGGING_ENABLE_CLANGTIDY)
option(LOGGING_ENABLE_FIX "Enable clang-tidy fix-errors and fix options for Logging" OFF)
mark_as_advanced(LOGGING_ENABLE_FIX)
option(LOGGING_ENABLE_IWYU "Enable include-what-you-use analysis for Logging" OFF)
mark_as_advanced(LOGGING_ENABLE_IWYU)
option(LOGGING_ENABLE_SANITIZER "Build Logging with sanitizer support (Clang/GCC only)" OFF)
set(LOGGING_SANITIZER_TYPE "address"
    CACHE STRING "Sanitizer type for Logging. Options: address, undefined, thread, memory, leak"
)
set_property(CACHE LOGGING_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak)
mark_as_advanced(LOGGING_ENABLE_SANITIZER LOGGING_SANITIZER_TYPE)
option(
  LOGGING_ENABLE_SPELL
  "Enable spell checking with automatic corrections for Logging (WARNING: modifies source files)"
  OFF
)
mark_as_advanced(LOGGING_ENABLE_SPELL)
option(LOGGING_ENABLE_VALGRIND "Execute Logging test suite with Valgrind" OFF)
mark_as_advanced(LOGGING_ENABLE_VALGRIND)
set(LOGGING_LINKER_CHOICE "default" CACHE STRING
                                          "Linker for Logging: default, lld, mold, gold, lld-link"
)
set_property(CACHE LOGGING_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)
mark_as_advanced(LOGGING_LINKER_CHOICE)
set(LOGGING_CACHE_BACKEND "none"
    CACHE STRING "Compiler cache backend for Logging. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE LOGGING_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(LOGGING_CACHE_BACKEND)

# C++ standard — applied to every TU in this module via CMAKE_CXX_STANDARD.
set(LOGGING_CXX_STANDARD "20" CACHE STRING "C++ standard for Logging. Options: 11 14 17 20 23")
set_property(CACHE LOGGING_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT LOGGING_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "LOGGING_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# ============================================================================= SECTION 2 —
# LOGGING_ENABLE_* → LOGGING_HAS_* compile definitions
#
# Maps the LOGGING_BACKEND selector (set by logging_backend.cmake) to the LOGGING_HAS_* boolean
# tokens consumed by source code via #if. Exactly one HAS_* will be 1; the others will be 0.
# =============================================================================
set(LOGGING_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})

if(LOGGING_BACKEND STREQUAL "LOGURU")
  # Loguru — header-only, supports custom verbosity levels and callbacks.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=1 LOGGING_HAS_GLOG=0
       LOGGING_HAS_NATIVE=0
  )
elseif(LOGGING_BACKEND STREQUAL "GLOG")
  # GLog — Google's production logging library; thread-safe sink management.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=1
       LOGGING_HAS_NATIVE=0
  )
elseif(LOGGING_BACKEND STREQUAL "NATIVE")
  # Native — fmt-based minimal backend with no external dependencies.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=0
       LOGGING_HAS_NATIVE=1
  )
endif()
list(APPEND LOGGING_DEPENDENCY_COMPILE_DEFINITIONS ${LOGGING_COMPILE_DEFINITIONS})

# ============================================================================= SECTION 3 — CMake
# module includes
#
# Inclusion order: (preflight) LTO cleared when coverage or sanitizer — before any include() a)
# Standard CMake introspection helpers b) Shared toolchain helpers  (Cmake/tools/, Cmake/flags/) c)
# Coverage / sanitizer / … then LTO + linker so IPO matches before quarisma_find_linker()
# =============================================================================

# LTO is incompatible with coverage/sanitizer — clear before any include().
if(LOGGING_ENABLE_COVERAGE OR LOGGING_ENABLE_SANITIZER)
  set(LOGGING_ENABLE_LTO OFF)
endif()

# a) Standard CMake introspection
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

# b) Shared toolchain helpers
include(compiler_checks) # Detects compiler capabilities; sets PROJECT_* flags
include(cache) # Configures per-target compiler cache launcher
include(build_type) # Validates / normalises CMAKE_BUILD_TYPE
include(checks) # Additional feature-check macros
include(utils) # Utility macros shared across all modules
include(platform) # Platform flags (macOS rpath, Windows /utf-8, …)
include(CMakeDependentOption)
include(helper_macros)
if(LOGGING_ENABLE_SANITIZER)
  include(sanitize)
  quarisma_configure_sanitizer(LOGGING)
endif()
if(LOGGING_ENABLE_SPELL)
  include(spell)
endif()
if(LOGGING_ENABLE_VALGRIND)
  include(valgrind)
endif()
# c) Coverage gate When enabled, appends -fprofile-instr-generate / --coverage to global
# CMAKE_CXX_FLAGS for this module.  LTO is cleared before Section 3 includes when coverage or
# sanitizer is ON.
if(LOGGING_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FASTLINK")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FASTLINK")
endif()

if(LOGGING_ENABLE_LTO AND NOT LOGGING_ENABLE_COVERAGE AND NOT LOGGING_ENABLE_SANITIZER)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  if(LOGGING_ENABLE_LTO AND (LOGGING_ENABLE_COVERAGE OR LOGGING_ENABLE_SANITIZER))
    message(
      STATUS "Link Time Optimization (LTO) disabled (incompatible with coverage and/or sanitizer)"
    )
  else()
    message(STATUS "Link Time Optimization (LTO) disabled")
  endif()
endif()

include(linker)
quarisma_find_linker(${LOGGING_LINKER_CHOICE})

# ============================================================================= Source discovery
# =============================================================================
set(logging_headers "")
set(logging_sources "")

file(GLOB_RECURSE logging_headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
)
file(GLOB_RECURSE logging_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Keep test sources out of the Logging library target.
list(FILTER logging_sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER logging_headers EXCLUDE REGEX ".*/Testing/.*")

# The NATIVE backend provides its own fmt-based logger; exclude the Loguru-backed logger.cpp /
# logger.h to avoid duplicate symbols.
if(LOGGING_ENABLE_NATIVE)
  list(REMOVE_ITEM logging_sources "${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.cpp")
  list(REMOVE_ITEM logging_headers "${CMAKE_CURRENT_SOURCE_DIR}/logger/logger.h")
  message(STATUS "NATIVE logging backend: excluding logger.cpp / logger.h (Loguru implementation)")
endif()

# ============================================================================= Target definition
# =============================================================================
# Per-module override — propagates to all targets in this directory scope, including test targets in
# Testing/Cxx.  CMAKE_CXX_STANDARD_REQUIRED and CMAKE_CXX_EXTENSIONS are set globally in the root
# CMakeLists.txt.
set(CMAKE_CXX_STANDARD ${LOGGING_CXX_STANDARD})

add_library(Logging ${logging_sources} ${logging_headers})
quarisma_target_apply_cache(Logging LOGGING_ENABLE_CACHE LOGGING_CACHE_BACKEND)
quarisma_target_optional_icecc(Logging LOGGING_ENABLE_ICECC)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Logging PUBLIC LOGGING_SHARED_DEFINE PRIVATE LOGGING_BUILDING_DLL)
  if(COMMAND logging_set_target_output_directories)
    logging_set_target_output_directories(Logging)
  endif()
else()
  target_compile_definitions(Logging PUBLIC LOGGING_STATIC_DEFINE)
endif()

add_library(Logging::Logging ALIAS Logging)

if(COMMAND logging_apply_iwyu)
  logging_apply_iwyu(Logging)
endif()

if(COMMAND logging_module_create_filters)
  logging_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set_target_properties(
  Logging PROPERTIES CXX_STANDARD ${LOGGING_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS
                                                                                   OFF
)

if(MSVC)
  target_compile_options(Logging PRIVATE /WX)
else()
  target_compile_options(Logging PRIVATE -include cstdlib)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/LoggingTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/LoggingTest.h" @ONLY
)

# Include roots: Library/Logging        → "util/exception.h", "common/logging_export.h"
# Library/Logging/logger → "logger.h", "back_trace.h", "logger_verbosity_enum.h"
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

if(LOGGING_ENABLE_CLANGTIDY)
  include(clang_tidy)
  quarisma_target_clang_tidy(Logging ${LOGGING_ENABLE_FIX})
endif()

if(LOGGING_ENABLE_IWYU)
  include(iwyu)
  quarisma_apply_iwyu(Logging)
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

# ============================================================================= Installation
# =============================================================================
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

# ============================================================================= Testing
# =============================================================================
if(LOGGING_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(LOGGING_ENABLE_VALGRIND AND LOGGING_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()
