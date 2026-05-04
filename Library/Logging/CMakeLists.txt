# =============================================================================
# Library/Logging —
# CMakeLists.txt
#
# Structured log levels, back-trace capture, and pluggable backend support (Loguru, GLog, Native).
#
# Public compile definitions: LOGGING_HAS_LOGURU=1/0  — Loguru backend active LOGGING_HAS_GLOG=1/0 —
# GLog backend active LOGGING_HAS_NATIVE=1/0  — Native (fmt-based) backend active
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# --- Section 1: Feature flags (LOGGING_ENABLE_*) and backend selector ---
set(LOGGING_BACKEND "LOGURU" CACHE STRING
                                   "Logging backend to use. Options are NATIVE, LOGURU, GLOG, or SPDLOG"
)
set_property(CACHE LOGGING_BACKEND PROPERTY STRINGS NATIVE LOGURU GLOG SPDLOG)

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
option(LOGGING_ENABLE_BENCHMARK "Enable Google Benchmark for Logging" ON)

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

# --- Section 2: LOGGING_ENABLE_* → LOGGING_HAS_* compile definitions --- Exactly one HAS_* will be
# 1; the others 0.

if(LOGGING_BACKEND STREQUAL "LOGURU")
  # Loguru — header-only, supports custom verbosity levels and callbacks.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=1 LOGGING_HAS_GLOG=0
       LOGGING_HAS_NATIVE=0 LOGGING_HAS_SPDLOG=0
  )
elseif(LOGGING_BACKEND STREQUAL "GLOG")
  # GLog — Google's production logging library; thread-safe sink management.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=1
       LOGGING_HAS_NATIVE=0 LOGGING_HAS_SPDLOG=0
  )
elseif(LOGGING_BACKEND STREQUAL "NATIVE")
  # Native — fmt-based minimal backend with no external dependencies.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=0
       LOGGING_HAS_NATIVE=1 LOGGING_HAS_SPDLOG=0
  )
elseif(LOGGING_BACKEND STREQUAL "SPDLOG")
  # spdlog — header-only, fast, feature-rich backend with colored output and async support.
  list(APPEND LOGGING_COMPILE_DEFINITIONS LOGGING_HAS_LOGURU=0 LOGGING_HAS_GLOG=0
       LOGGING_HAS_NATIVE=0 LOGGING_HAS_SPDLOG=1 SPDLOG_FMT_EXTERNAL=1
  )
endif()
list(APPEND LOGGING_DEPENDENCY_COMPILE_DEFINITIONS ${LOGGING_COMPILE_DEFINITIONS})

# --- Section 3: CMake module includes --- Order: introspection → toolchain helpers →
# coverage/sanitizer → LTO/linker.

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
include(checks) # Additional feature-check macros
include(build_type) # Validates / normalises CMAKE_BUILD_TYPE
include(platform) # Platform flags (macOS rpath, Windows /utf-8, …)

include(cache) # Configures per-target compiler cache launcher
include(CMakeDependentOption)
include(helper_macros)
if(LOGGING_ENABLE_SANITIZER)
  include(sanitize)
endif()
if(LOGGING_ENABLE_SPELL)
  include(spell)
endif()
if(LOGGING_ENABLE_VALGRIND)
  include(valgrind)
endif()
# c) Coverage gate — appends -fprofile-instr-generate / --coverage; LTO already cleared above.
if(LOGGING_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FULL")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FULL")
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

# --- Third-party dependencies owned by Logging ---
set(_tpdir "${CMAKE_SOURCE_DIR}/ThirdParty")

set(BUILD_TESTING OFF CACHE BOOL "Disable testing for third-party libraries" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples for third-party libraries" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable benchmarks for third-party libraries" FORCE)

# loguru-specific options
set(LOGURU_BUILD_EXAMPLES OFF CACHE BOOL "Disable loguru examples" FORCE)
# set(LOGURU_USE_FMTLIB ON CACHE BOOL "Enable fmtlib formatting in loguru" FORCE)
set(LOGURU_WITH_STREAMS ON CACHE BOOL "Enable streams in loguru" FORCE)

# glog-specific options
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable glog examples" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "Disable gflags dependency for glog" FORCE)
set(WITH_GTEST OFF CACHE BOOL "Disable gtest for glog" FORCE)
set(WITH_GMOCK OFF CACHE BOOL "Disable gmock for glog" FORCE)

# fmt — Logging uses fmt directly and is processed before Core, so we initialise fmt here. Core's
# identical add_third_party_library(fmt...) call is a no-op (processed flag).
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
set(FMT_DOC OFF CACHE BOOL "Disable fmt documentation" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "Disable fmt install" FORCE)
set(FMT_HEADER_ONLY ON CACHE BOOL "Use header-only fmt library" FORCE)

add_third_party_library(
  fmt
  LIBRARY_TYPE
  STATIC
  SOURCE_DIR
  "${_tpdir}/fmt"
  ALIASES
  "fmt::fmt-header-only"
  QUARISMA_TARGETS
  "Fmt::fmt=fmt::fmt-header-only|fmt::fmt|fmt"
  INCLUDE_DIR
  "${_tpdir}/fmt/include"
  AUTO_ADD_TO_DEPENDENCIES
  OFF
)

# fmt is used by Logging source files directly; Fmt::fmt is not available here (it is created by
# Core, which is processed after Logging).
list(APPEND LOGGING_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/fmt/include")
if(LOGGING_BACKEND STREQUAL "LOGURU")
  add_third_party_library(
    loguru
    LIBRARY_TYPE
    STATIC
    SOURCE_DIR
    "${_tpdir}/loguru"
    FILTER_FLAGS
    "-flto=thin"
    "-flto=full"
    QUARISMA_TARGETS
    "Loguru::loguru=loguru::loguru|loguru"
    INCLUDE_DIR
    "${_tpdir}/loguru"
    AUTO_ADD_TO_DEPENDENCIES
    OFF
  )
  if(TARGET loguru)
    set_property(TARGET loguru PROPERTY POSITION_INDEPENDENT_CODE ON)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(loguru PRIVATE -Wno-format-truncation)
    endif()
  endif()
  list(APPEND LOGGING_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/loguru")
endif()
if(LOGGING_BACKEND STREQUAL "GLOG")
  add_third_party_library(
    glog
    LIBRARY_TYPE
    STATIC
    SOURCE_DIR
    "${_tpdir}/glog"
    FILTER_FLAGS
    "-flto=thin"
    "-flto=full"
    QUARISMA_TARGETS
    "Glog::glog=glog::glog|glog"
    INCLUDE_DIR
    "${_tpdir}/glog/src"
    AUTO_ADD_TO_DEPENDENCIES
    OFF
  )
  list(APPEND LOGGING_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/glog/src")
endif()
if(LOGGING_BACKEND STREQUAL "SPDLOG")
  # spdlog as HEADER_ONLY: add_third_party_library only skips compilation for LIBRARY_TYPE
  # HEADER_ONLY (INTERFACE is treated as "default" and would add_subdirectory + build spdlog).
  add_third_party_library(
    spdlog
    LIBRARY_TYPE
    HEADER_ONLY
    SOURCE_DIR
    "${_tpdir}/spdlog"
    QUARISMA_TARGETS
    "Spdlog::spdlog=spdlog::spdlog|spdlog"
    INCLUDE_DIR
    "${_tpdir}/spdlog/include"
    AUTO_ADD_TO_DEPENDENCIES
    OFF
  )
  list(APPEND LOGGING_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/spdlog/include")
endif()

# --- Source discovery ---
set(logging_headers "")
set(logging_sources "")

file(GLOB_RECURSE logging_headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                                                         "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx"
)
file(GLOB_RECURSE logging_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Keep test sources out of the Logging library target.
list(FILTER logging_sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER logging_headers EXCLUDE REGEX ".*/Testing/.*")

# --- Target definition ---
set(CMAKE_CXX_STANDARD ${LOGGING_CXX_STANDARD})

add_library(Logging ${logging_sources} ${logging_headers})
if(LOGGING_ENABLE_SANITIZER)
  quarisma_configure_sanitizer(LOGGING)
endif()
quarisma_find_linker(${LOGGING_LINKER_CHOICE} Logging)
quarisma_target_apply_cache(Logging LOGGING_ENABLE_CACHE LOGGING_CACHE_BACKEND)
quarisma_target_optional_icecc(Logging LOGGING_ENABLE_ICECC)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Logging PUBLIC LOGGING_SHARED_DEFINE PRIVATE LOGGING_BUILDING_DLL)
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Logging)
  endif()
else()
  target_compile_definitions(Logging PUBLIC LOGGING_STATIC_DEFINE)
endif()

add_library(Logging::Logging ALIAS Logging)

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(
  Logging PROPERTIES CXX_STANDARD ${LOGGING_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS
                                                                                   OFF
)

if(MSVC)
  target_compile_options(Logging PRIVATE /WX)
else()
  target_compile_options(Logging PRIVATE -include cstdlib)
endif()

# Clang-on-Windows (GNU driver + CodeView) can ICE in codegen when instantiating spdlog at -O3.
if(WIN32
   AND NOT MSVC
   AND CMAKE_CXX_COMPILER_ID MATCHES "Clang"
   AND LOGGING_BACKEND STREQUAL "SPDLOG"
)
  target_compile_options(
    Logging
    PRIVATE
      $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-O1>
  )
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/LoggingTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/LoggingTest.h" @ONLY
)

# Include roots: Library/Logging        → "util/logging_exception.h", "common/logging_export.h"
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

if(TARGET Fmt::fmt)
  list(APPEND LOGGING_DEPENDENCY_LIBS Fmt::fmt)
endif()
if(TARGET Loguru::loguru)
  list(APPEND LOGGING_DEPENDENCY_LIBS Loguru::loguru)
endif()
if(TARGET Glog::glog)
  list(APPEND LOGGING_DEPENDENCY_LIBS Glog::glog)
endif()
if(TARGET Spdlog::spdlog)
  list(APPEND LOGGING_DEPENDENCY_LIBS Spdlog::spdlog)
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

# --- Installation ---
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

# --- Testing ---
if(LOGGING_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(LOGGING_ENABLE_VALGRIND AND LOGGING_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

get_property(_logging_summary_backend CACHE LOGGING_BACKEND PROPERTY VALUE)
message("")
message("******** Logging module (CMake flags) ********")
message("  Backend          : ${_logging_summary_backend} (options: LOGURU, GLOG, NATIVE, SPDLOG)")
message("  Cxx standard     : ${LOGGING_CXX_STANDARD}")
message("  Cache            : ${LOGGING_ENABLE_CACHE}")
message("  Cache backend    : ${LOGGING_CACHE_BACKEND}")
message("  Icecc            : ${LOGGING_ENABLE_ICECC}")
message("  Linker           : ${LOGGING_LINKER_CHOICE}")
message("  Lto              : ${LOGGING_ENABLE_LTO}")
message("  Coverage         : ${LOGGING_ENABLE_COVERAGE}")
message("  Testing          : ${LOGGING_ENABLE_TESTING}")
message("  Examples         : ${LOGGING_ENABLE_EXAMPLES}")
message("  Gtest            : ${LOGGING_ENABLE_GTEST}")
message("  Benchmark        : ${LOGGING_ENABLE_BENCHMARK}")
message("  Clang-tidy       : ${LOGGING_ENABLE_CLANGTIDY}")
message("  Fix              : ${LOGGING_ENABLE_FIX}")
message("  Iwyu             : ${LOGGING_ENABLE_IWYU}")
message("  Sanitizer        : ${LOGGING_ENABLE_SANITIZER}")
message("  Sanitizer type   : ${LOGGING_SANITIZER_TYPE}")
message("  Spell            : ${LOGGING_ENABLE_SPELL}")
message("  Valgrind         : ${LOGGING_ENABLE_VALGRIND}")
message("")
message("Build:")
message("  Default C++ standard     : ${CMAKE_CXX_STANDARD}")
message("  C flags                  : ${CMAKE_C_FLAGS}")
message("  CXX flags                : ${CMAKE_CXX_FLAGS}")
message("  Shared linker flags      : ${CMAKE_SHARED_LINKER_FLAGS}")
message("  Static linker flags      : ${CMAKE_STATIC_LINKER_FLAGS}")
message("  Module linker flags      : ${CMAKE_MODULE_LINKER_FLAGS}")
message("  IPO / LTO (CMake)        : ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
get_directory_property(_summary_compile_defs DIRECTORY ${PROJECT_SOURCE_DIR} COMPILE_DEFINITIONS)
message("  Top-level compile defs   : ${_summary_compile_defs}")
message("")
