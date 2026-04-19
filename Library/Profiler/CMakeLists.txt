# =============================================================================
# Library/Profiler — CMakeLists.txt
#
# Activity tracing, timing primitives, and pluggable profiler-backend support
# (Kineto, Intel ITT/VTune, Native).
#
# Public compile definitions:
#   PROFILER_HAS_KINETO=1/0  — Kineto (PyTorch-compatible) backend active
#   PROFILER_HAS_ITT=1/0     — Intel ITT/VTune backend active
#   PROFILER_HAS_NATIVE=1/0  — Lightweight standalone native backend active
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# --- Early-exit gate ---
# When PROFILER_INCLUDE_GATE_ONLY=ON, only set PROFILER_ENABLE_KINETO and return early
# so the root can gate ThirdParty/Kineto before add_subdirectory runs.
if(PROFILER_INCLUDE_GATE_ONLY)
  set(PROFILER_BACKEND "KINETO" CACHE STRING "Profiler backend. Options: KINETO, NATIVE, ITT")
  set_property(CACHE PROFILER_BACKEND PROPERTY STRINGS KINETO NATIVE ITT)
  if(PROFILER_BACKEND STREQUAL "KINETO")
    set(PROFILER_ENABLE_KINETO ON)
  else()
    set(PROFILER_ENABLE_KINETO OFF)
  endif()
  return()
endif()

# --- Section 1: Feature flags (PROFILER_ENABLE_*) and backend selector ---
# PROFILER_BACKEND is the single control point; derived booleans are mutually exclusive.

# Link-Time Optimization — disabled when PROFILER_ENABLE_COVERAGE or PROFILER_ENABLE_SANITIZER is ON
# (LTO conflicts with instrumentation / sanitizers).
option(PROFILER_ENABLE_LTO "Enable Link Time Optimization for Profiler" OFF)

# Coverage instrumentation — forces PROFILER_ENABLE_LTO=OFF and activates -fprofile-instr-generate /
# --coverage compiler flags for this module only.
option(PROFILER_ENABLE_COVERAGE "Enable coverage instrumentation for Profiler tests" OFF)

# Testing — gates add_subdirectory(Testing/Cxx).
option(PROFILER_ENABLE_TESTING "Build Profiler test suite" ON)

# Examples — gates the Profiling examples under Examples/Profiling.
option(PROFILER_ENABLE_EXAMPLES "Build Profiler example programs" OFF)

# Google Test integration for Profiler tests.
option(PROFILER_ENABLE_GTEST "Enable GoogleTest for Profiler" ON)

# Google Benchmark integration for Profiler micro-benchmarks.
option(PROFILER_ENABLE_BENCHMARK "Enable Google Benchmark for Profiler" OFF)

# Icecream distributed compilation launcher (rarely needed; hidden by default).
option(PROFILER_ENABLE_ICECC "Use Icecream (icecc) distributed compiler for Profiler" OFF)
mark_as_advanced(PROFILER_ENABLE_ICECC)

# Per-target compiler cache — wraps every compile command with ccache / sccache etc. to speed up
# incremental and CI rebuilds. ON by default.
option(PROFILER_ENABLE_CACHE "Enable compiler caching for the Profiler target" ON)
mark_as_advanced(PROFILER_ENABLE_CACHE)

option(PROFILER_ENABLE_CLANGTIDY "Enable clang-tidy static analysis for Profiler" OFF)
mark_as_advanced(PROFILER_ENABLE_CLANGTIDY)
option(PROFILER_ENABLE_FIX "Enable clang-tidy fix-errors and fix options for Profiler" OFF)
mark_as_advanced(PROFILER_ENABLE_FIX)
option(PROFILER_ENABLE_IWYU "Enable include-what-you-use analysis for Profiler" OFF)
mark_as_advanced(PROFILER_ENABLE_IWYU)
option(PROFILER_ENABLE_SANITIZER "Build Profiler with sanitizer support (Clang/GCC only)" OFF)
set(PROFILER_SANITIZER_TYPE "address"
    CACHE STRING "Sanitizer type for Profiler. Options: address, undefined, thread, memory, leak"
)
set_property(CACHE PROFILER_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak)
mark_as_advanced(PROFILER_ENABLE_SANITIZER PROFILER_SANITIZER_TYPE)
option(
  PROFILER_ENABLE_SPELL
  "Enable spell checking with automatic corrections for Profiler (WARNING: modifies source files)"
  OFF
)
mark_as_advanced(PROFILER_ENABLE_SPELL)
option(PROFILER_ENABLE_VALGRIND "Execute Profiler test suite with Valgrind" OFF)
mark_as_advanced(PROFILER_ENABLE_VALGRIND)
set(PROFILER_LINKER_CHOICE "default"
    CACHE STRING "Linker for Profiler: default, lld, mold, gold, lld-link"
)
set_property(CACHE PROFILER_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)
mark_as_advanced(PROFILER_LINKER_CHOICE)
set(PROFILER_CACHE_BACKEND "none"
    CACHE STRING "Compiler cache backend for Profiler. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE PROFILER_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(PROFILER_CACHE_BACKEND)

# Backend selector — the single control point for profiler backend choice. Derived booleans
# (PROFILER_ENABLE_KINETO etc.) are set immediately below. KINETO — full PyTorch-compatible activity
# tracing (default) ITT    — Intel Instrumentation and Tracing Technology / VTune NATIVE —
# lightweight standalone backend with no external dependencies
set(PROFILER_BACKEND "KINETO" CACHE STRING "Profiler backend. Options: KINETO, NATIVE, ITT")
set_property(CACHE PROFILER_BACKEND PROPERTY STRINGS KINETO NATIVE ITT)
if(NOT PROFILER_BACKEND MATCHES "^(KINETO|NATIVE|ITT)$")
  message(
    FATAL_ERROR "Invalid PROFILER_BACKEND '${PROFILER_BACKEND}'. Valid options: KINETO, NATIVE, ITT"
  )
endif()

# Derive mutually-exclusive enable booleans from the backend selector.
if(PROFILER_BACKEND STREQUAL "KINETO")
  set(PROFILER_ENABLE_KINETO ON)
  set(PROFILER_ENABLE_ITT OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(PROFILER_BACKEND STREQUAL "ITT")
  set(PROFILER_ENABLE_KINETO OFF)
  set(PROFILER_ENABLE_ITT ON)
  set(PROFILER_ENABLE_NATIVE_PROFILER OFF)
elseif(PROFILER_BACKEND STREQUAL "NATIVE")
  set(PROFILER_ENABLE_KINETO OFF)
  set(PROFILER_ENABLE_ITT OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER ON)
endif()

# C++ standard — applied to every TU in this module via CMAKE_CXX_STANDARD.
set(PROFILER_CXX_STANDARD "20" CACHE STRING "C++ standard for Profiler. Options: 11 14 17 20 23")
set_property(CACHE PROFILER_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT PROFILER_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "PROFILER_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# --- Section 2: PROFILER_ENABLE_* → PROFILER_HAS_* compile definitions ---
# Exactly one HAS_* will be 1; the others 0.
set(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})

if(PROFILER_ENABLE_KINETO)
  # Kineto — full PyTorch-compatible activity-tracing and timeline recording.
  list(APPEND PROFILER_COMPILE_DEFINITIONS PROFILER_HAS_KINETO=1 PROFILER_HAS_ITT=0
       PROFILER_HAS_NATIVE=0
  )
elseif(PROFILER_ENABLE_ITT)
  # ITT/VTune — Intel instrumentation API; integrates with VTune Amplifier.
  list(APPEND PROFILER_COMPILE_DEFINITIONS PROFILER_HAS_KINETO=0 PROFILER_HAS_ITT=1
       PROFILER_HAS_NATIVE=0
  )
elseif(PROFILER_ENABLE_NATIVE_PROFILER)
  # Native — lightweight standalone backend; no external library dependency.
  list(APPEND PROFILER_COMPILE_DEFINITIONS PROFILER_HAS_KINETO=0 PROFILER_HAS_ITT=0
       PROFILER_HAS_NATIVE=1
  )
endif()

list(APPEND PROFILER_DEPENDENCY_COMPILE_DEFINITIONS ${PROFILER_COMPILE_DEFINITIONS})

# --- Section 3: CMake module includes ---
# Order: introspection → toolchain helpers → coverage/sanitizer → LTO/linker.

# LTO is incompatible with coverage/sanitizer — clear before any include().
if(PROFILER_ENABLE_COVERAGE OR PROFILER_ENABLE_SANITIZER)
  set(PROFILER_ENABLE_LTO OFF)
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
if(PROFILER_ENABLE_SANITIZER)
  include(sanitize)
  quarisma_configure_sanitizer(PROFILER)
endif()
if(PROFILER_ENABLE_SPELL)
  include(spell)
endif()
if(PROFILER_ENABLE_VALGRIND)
  include(valgrind)
endif()
include(helper_macros)

# c) Coverage gate — appends -fprofile-instr-generate / --coverage; LTO already cleared above.
if(PROFILER_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FASTLINK")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FASTLINK")
endif()

if(PROFILER_ENABLE_LTO AND NOT PROFILER_ENABLE_COVERAGE AND NOT PROFILER_ENABLE_SANITIZER)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  if(PROFILER_ENABLE_LTO AND (PROFILER_ENABLE_COVERAGE OR PROFILER_ENABLE_SANITIZER))
    message(
      STATUS "Link Time Optimization (LTO) disabled (incompatible with coverage and/or sanitizer)"
    )
  else()
    message(STATUS "Link Time Optimization (LTO) disabled")
  endif()
endif()

include(linker)
quarisma_find_linker(${PROFILER_LINKER_CHOICE})

# --- Third-party dependencies owned by Profiler ---
set(_tpdir "${CMAKE_SOURCE_DIR}/ThirdParty")

# Kineto: configure GPU-backend knobs before building the library.
if(PROFILER_ENABLE_KINETO)
  set(LIBKINETO_NOAIUPTI ON CACHE STRING "" FORCE)
  if(NOT MEMORY_ENABLE_CUDA)
    set(LIBKINETO_NOCUPTI ON CACHE STRING "" FORCE)
  else()
    set(LIBKINETO_NOCUPTI OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with CUPTI support")
  endif()
  if(NOT QUARISMA_ENABLE_ROCM)
    set(LIBKINETO_NOROCTRACER ON CACHE STRING "" FORCE)
  else()
    set(LIBKINETO_NOROCTRACER OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with Roctracer support")
  endif()
  if(LIBKINETO_NOCUPTI AND LIBKINETO_NOROCTRACER AND LIBKINETO_NOXPUPTI)
    message(STATUS "Using CPU-only version of Kineto")
  endif()
  set(KINETO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(KINETO_LIBRARY_TYPE "static" CACHE STRING "" FORCE)
  # Reuse Quarisma's fmt build to avoid duplicate symbols.
  set(FMT_SOURCE_DIR "${_tpdir}/fmt" CACHE STRING "Use Quarisma's fmt library" FORCE)
endif()

add_third_party_library(kineto
    CONDITION PROFILER_ENABLE_KINETO
    LIBRARY_TYPE STATIC
    SOURCE_DIR "${_tpdir}/kineto/libkineto"
    DISABLE_WARNINGS
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Kineto::kineto=kineto"
    INCLUDE_DIR "${_tpdir}/kineto/libkineto/include"
    AUTO_ADD_TO_DEPENDENCIES OFF
)

if(PROFILER_ENABLE_KINETO AND NOT TARGET Kineto::kineto)
  message(STATUS "Kineto not found — disabling PROFILER_ENABLE_KINETO")
  set(PROFILER_ENABLE_KINETO OFF CACHE BOOL "Enable Quarisma Kineto profiling library." FORCE)
endif()

list(APPEND PROFILER_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/fmt/include")
if(PROFILER_ENABLE_KINETO)
  list(APPEND PROFILER_DEPENDENCY_INCLUDE_DIRS "${_tpdir}/kineto/libkineto/include")
endif()

# ITT — Intel VTune API (external system dependency, not bundled).
if(PROFILER_ENABLE_ITT)
  find_package(ITT)
  if(ITT_FOUND)
    if(TARGET Itt::itt)
      list(APPEND PROFILER_DEPENDENCY_LIBS Itt::itt)
      if(TARGET Itt::jitprofiling)
        list(APPEND PROFILER_DEPENDENCY_LIBS Itt::jitprofiling)
      endif()
      message(STATUS "Dependency: Itt::itt added to PROFILER_DEPENDENCY_LIBS")
    endif()
  else()
    message(STATUS "ITT API not found — disabling PROFILER_ENABLE_ITT")
    set(PROFILER_ENABLE_ITT OFF CACHE BOOL "Enable Intel ITT API for VTune profiling." FORCE)
  endif()
endif()

# --- Runtime library wiring ---
if(TARGET Fmt::fmt)
  list(APPEND PROFILER_DEPENDENCY_LIBS Fmt::fmt)
endif()
if(PROFILER_ENABLE_KINETO AND TARGET Kineto::kineto)
  list(APPEND PROFILER_DEPENDENCY_LIBS Kineto::kineto)
endif()

# --- Source discovery: only sources for the active backend are compiled ---
set(profiler_headers "")
set(profiler_sources "")
set(CMAKE_CXX_STANDARD ${PROFILER_CXX_STANDARD})

# Common utilities — always compiled (approximate_clock, small_vector, etc.)
file(GLOB common_util_headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h")
file(GLOB common_util_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp")
list(APPEND profiler_headers ${common_util_headers})
list(APPEND profiler_sources ${common_util_sources})

# Kineto backend — base/ is shared infrastructure between Kineto and ITT.
if(PROFILER_ENABLE_KINETO)
  file(
    GLOB_RECURSE kineto_headers LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h"
  )
  file(
    GLOB_RECURSE kineto_sources
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp"
  )
  # Platform-specific unwinder is not used by Kineto; exclude it.
  file(GLOB_RECURSE KINETO_UNWIND_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/unwind/*.cpp")
  list(REMOVE_ITEM kineto_sources ${KINETO_UNWIND_SOURCES})
  list(APPEND profiler_headers ${kineto_headers})
  list(APPEND profiler_sources ${kineto_sources})
  message(STATUS "Kineto profiler enabled: including base, kineto, and common sources")
endif()

# Native backend — standalone, no external dependencies.
if(PROFILER_ENABLE_NATIVE_PROFILER)
  file(GLOB_RECURSE native_headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/native/*.h")
  file(GLOB_RECURSE native_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/native/*.cpp"
  )
  list(APPEND profiler_headers ${native_headers})
  list(APPEND profiler_sources ${native_sources})
  message(STATUS "Native profiler enabled: including native sources (standalone)")
endif()

# ITT backend — base/ is shared with Kineto; guard against double-inclusion.
if(PROFILER_ENABLE_ITT)
  if(NOT PROFILER_ENABLE_KINETO)
    file(GLOB_RECURSE itt_base_headers LIST_DIRECTORIES false
         "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h"
    )
    file(GLOB_RECURSE itt_base_sources LIST_DIRECTORIES false
         "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp"
    )
    list(APPEND profiler_headers ${itt_base_headers})
    list(APPEND profiler_sources ${itt_base_sources})
  endif()
  file(GLOB_RECURSE itt_headers LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.h"
  )
  file(GLOB_RECURSE itt_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.cpp"
  )
  list(APPEND profiler_headers ${itt_headers})
  list(APPEND profiler_sources ${itt_sources})
  message(STATUS "ITT profiler enabled: including base and ITT sources")
endif()

# --- Target definition ---
add_library(Profiler ${profiler_sources} ${profiler_headers})
quarisma_target_apply_cache(Profiler PROFILER_ENABLE_CACHE PROFILER_CACHE_BACKEND)
quarisma_target_optional_icecc(Profiler PROFILER_ENABLE_ICECC)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Profiler PUBLIC PROFILER_SHARED_DEFINE PRIVATE PROFILER_BUILDING_DLL)
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Profiler)
  endif()
else()
  target_compile_definitions(Profiler PUBLIC PROFILER_STATIC_DEFINE)
endif()

add_library(Profiler::Profiler ALIAS Profiler)

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(
  Profiler PROPERTIES CXX_STANDARD ${PROFILER_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS
                                                                                     OFF
)

if(MSVC)
  target_compile_options(Profiler PRIVATE /WX)
else()
  target_compile_options(Profiler PRIVATE -include cstdlib)
endif()

# Include roots: Library/Profiler → "common/profiler_export.h", "bespoke/…"
target_include_directories(
  Profiler BEFORE
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing> $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/ProfilerTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/ProfilerTest.h" @ONLY
)

if(PROFILER_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Profiler PUBLIC ${PROFILER_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Profiler: extra include dirs: ${PROFILER_DEPENDENCY_INCLUDE_DIRS}")
endif()

target_link_libraries(Profiler PUBLIC Profiler::build)

if(PROFILER_DEPENDENCY_LIBS)
  target_link_libraries(Profiler PUBLIC ${PROFILER_DEPENDENCY_LIBS})
  message(STATUS "Profiler: linked dependencies: ${PROFILER_DEPENDENCY_LIBS}")
endif()

if(PROFILER_ENABLE_CLANGTIDY)
  include(clang_tidy)
  quarisma_target_clang_tidy(Profiler ${PROFILER_ENABLE_FIX})
endif()

if(PROFILER_ENABLE_IWYU)
  include(iwyu)
  quarisma_apply_iwyu(Profiler)
endif()

if(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Profiler PUBLIC ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Profiler: compile definitions: ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# --- Examples ---
if(PROFILER_ENABLE_EXAMPLES)
  add_subdirectory(
    "${CMAKE_SOURCE_DIR}/Examples/Profiling" "${CMAKE_BINARY_DIR}/Examples/Profiling"
  )
  message(STATUS "Profiling Examples enabled")
endif()

# --- Testing ---
if(PROFILER_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(PROFILER_ENABLE_VALGRIND AND PROFILER_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

message("")
message("******** Profiler module (CMake flags) ********")
message("  PROFILER_CXX_STANDARD      : ${PROFILER_CXX_STANDARD}")
message("  PROFILER_BACKEND           : ${PROFILER_BACKEND}")
message("  PROFILER_ENABLE_CACHE      : ${PROFILER_ENABLE_CACHE}")
message("  PROFILER_CACHE_BACKEND     : ${PROFILER_CACHE_BACKEND}")
message("  PROFILER_LINKER_CHOICE     : ${PROFILER_LINKER_CHOICE}")
message("  PROFILER_ENABLE_LTO        : ${PROFILER_ENABLE_LTO}")
message("  PROFILER_ENABLE_COVERAGE   : ${PROFILER_ENABLE_COVERAGE}")
message("  PROFILER_ENABLE_TESTING    : ${PROFILER_ENABLE_TESTING}")
message("  PROFILER_ENABLE_EXAMPLES   : ${PROFILER_ENABLE_EXAMPLES}")
message("  PROFILER_ENABLE_GTEST      : ${PROFILER_ENABLE_GTEST}")
message("  PROFILER_ENABLE_BENCHMARK  : ${PROFILER_ENABLE_BENCHMARK}")
message("  PROFILER_ENABLE_ICECC      : ${PROFILER_ENABLE_ICECC}")
message("  PROFILER_ENABLE_CLANGTIDY  : ${PROFILER_ENABLE_CLANGTIDY}")
message("  PROFILER_ENABLE_FIX        : ${PROFILER_ENABLE_FIX}")
message("  PROFILER_ENABLE_IWYU       : ${PROFILER_ENABLE_IWYU}")
message("  PROFILER_ENABLE_SANITIZER  : ${PROFILER_ENABLE_SANITIZER}")
message("  PROFILER_SANITIZER_TYPE    : ${PROFILER_SANITIZER_TYPE}")
message("  PROFILER_ENABLE_SPELL      : ${PROFILER_ENABLE_SPELL}")
message("  PROFILER_ENABLE_VALGRIND   : ${PROFILER_ENABLE_VALGRIND}")
message("")
