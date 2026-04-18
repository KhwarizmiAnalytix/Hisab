# =============================================================================
# Library/Profiler — CMakeLists.txt
#
# Provides the Profiler library: activity tracing, timing primitives, and
# pluggable profiler-backend support (Kineto, Intel ITT/VTune, Native).
#
# Public compile definitions emitted by this file:
#   PROFILER_HAS_KINETO=1/0  — Kineto (PyTorch-compatible) backend active
#   PROFILER_HAS_ITT=1/0     — Intel ITT/VTune backend active
#   PROFILER_HAS_NATIVE=1/0  — Lightweight standalone native backend active
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# =============================================================================
# Early-exit gate
#
# logging_backend.cmake includes this file with QUARISMA_INCLUDE_PROFILER_GATE_ONLY=ON
# before add_subdirectory(ThirdParty) so that PROFILER_ENABLE_KINETO is set in
# time for the Kineto third-party target to be conditionally built.
# The return() prevents the remainder of this file from executing during
# that early pass.
# =============================================================================
if(QUARISMA_INCLUDE_PROFILER_GATE_ONLY)
  set(PROFILER_BACKEND "KINETO"
      CACHE STRING "Profiler backend. Options: KINETO, NATIVE, ITT")
  set_property(CACHE PROFILER_BACKEND PROPERTY STRINGS KINETO NATIVE ITT)
  if(PROFILER_BACKEND STREQUAL "KINETO")
    set(PROFILER_ENABLE_KINETO ON)
  else()
    set(PROFILER_ENABLE_KINETO OFF)
  endif()
  return()
endif()

# =============================================================================
# SECTION 1 — Feature flags (PROFILER_ENABLE_*) and backend selector
#
# These cache variables are the public configuration API for this module.
# Pass them on the CMake command line: -DPROFILER_BACKEND=NATIVE
#
# PROFILER_BACKEND is the single control point for backend selection.
# The derived booleans PROFILER_ENABLE_KINETO / PROFILER_ENABLE_ITT /
# PROFILER_ENABLE_NATIVE_PROFILER are mutually exclusive — do not set them
# directly; change PROFILER_BACKEND instead.
# =============================================================================

# Link-Time Optimization — automatically disabled when PROFILER_ENABLE_COVERAGE=ON
# because LTO and LLVM profiling instrumentation are mutually incompatible.
option(PROFILER_ENABLE_LTO
  "Enable Link Time Optimization for Profiler" OFF)

# Coverage instrumentation — forces PROFILER_ENABLE_LTO=OFF and activates
# -fprofile-instr-generate / --coverage compiler flags for this module only.
option(PROFILER_ENABLE_COVERAGE
  "Enable coverage instrumentation for Profiler tests" OFF)

# Testing — gates add_subdirectory(Testing/Cxx).
option(PROFILER_ENABLE_TESTING
  "Build Profiler test suite" ON)

# Examples — gates the Profiling examples under Examples/Profiling.
option(PROFILER_ENABLE_EXAMPLES
  "Build Profiler example programs" OFF)

# Google Test integration for Profiler tests.
option(PROFILER_ENABLE_GTEST
  "Enable GoogleTest for Profiler" ON)

# Google Benchmark integration for Profiler micro-benchmarks.
option(PROFILER_ENABLE_BENCHMARK
  "Enable Google Benchmark for Profiler" OFF)

# Icecream distributed compilation launcher (rarely needed; hidden by default).
option(PROFILER_ENABLE_ICECC
  "Use Icecream (icecc) distributed compiler for Profiler" OFF)
mark_as_advanced(PROFILER_ENABLE_ICECC)

# Per-target compiler cache — wraps every compile command with ccache / sccache
# etc. to speed up incremental and CI rebuilds. ON by default.
option(PROFILER_ENABLE_CACHE
  "Enable compiler caching for the Profiler target" ON)
mark_as_advanced(PROFILER_ENABLE_CACHE)
set(PROFILER_CACHE_BACKEND "none"
    CACHE STRING
    "Compiler cache backend for Profiler. Options: none, ccache, sccache, buildcache")
set_property(CACHE PROFILER_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(PROFILER_CACHE_BACKEND)

# Backend selector — the single control point for profiler backend choice.
# Derived booleans (PROFILER_ENABLE_KINETO etc.) are set immediately below.
#   KINETO — full PyTorch-compatible activity tracing (default)
#   ITT    — Intel Instrumentation and Tracing Technology / VTune
#   NATIVE — lightweight standalone backend with no external dependencies
set(PROFILER_BACKEND "KINETO"
    CACHE STRING "Profiler backend. Options: KINETO, NATIVE, ITT")
set_property(CACHE PROFILER_BACKEND PROPERTY STRINGS KINETO NATIVE ITT)
if(NOT PROFILER_BACKEND MATCHES "^(KINETO|NATIVE|ITT)$")
  message(FATAL_ERROR
    "Invalid PROFILER_BACKEND '${PROFILER_BACKEND}'. Valid options: KINETO, NATIVE, ITT")
endif()

# Derive mutually-exclusive enable booleans from the backend selector.
if(PROFILER_BACKEND STREQUAL "KINETO")
  set(PROFILER_ENABLE_KINETO           ON)
  set(PROFILER_ENABLE_ITT              OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER  OFF)
elseif(PROFILER_BACKEND STREQUAL "ITT")
  set(PROFILER_ENABLE_KINETO           OFF)
  set(PROFILER_ENABLE_ITT              ON)
  set(PROFILER_ENABLE_NATIVE_PROFILER  OFF)
elseif(PROFILER_BACKEND STREQUAL "NATIVE")
  set(PROFILER_ENABLE_KINETO           OFF)
  set(PROFILER_ENABLE_ITT              OFF)
  set(PROFILER_ENABLE_NATIVE_PROFILER  ON)
endif()

# C++ standard — applied to every TU in this module via CMAKE_CXX_STANDARD.
set(PROFILER_CXX_STANDARD "20"
    CACHE STRING "C++ standard for Profiler. Options: 11 14 17 20 23")
set_property(CACHE PROFILER_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT PROFILER_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "PROFILER_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# =============================================================================
# SECTION 2 — PROFILER_ENABLE_* → PROFILER_HAS_* compile definitions
#
# Maps each feature flag to a PROFILER_HAS_* token consumed by source via #if.
# Exactly one HAS_* will be 1; the others will be 0.
# =============================================================================
set(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS ${PROJECT_DEPENDENCY_COMPILE_DEFINITIONS})

if(PROFILER_ENABLE_KINETO)
  # Kineto — full PyTorch-compatible activity-tracing and timeline recording.
  list(APPEND PROFILER_COMPILE_DEFINITIONS
    PROFILER_HAS_KINETO=1 PROFILER_HAS_ITT=0 PROFILER_HAS_NATIVE=0)
elseif(PROFILER_ENABLE_ITT)
  # ITT/VTune — Intel instrumentation API; integrates with VTune Amplifier.
  list(APPEND PROFILER_COMPILE_DEFINITIONS
    PROFILER_HAS_KINETO=0 PROFILER_HAS_ITT=1 PROFILER_HAS_NATIVE=0)
elseif(PROFILER_ENABLE_NATIVE_PROFILER)
  # Native — lightweight standalone backend; no external library dependency.
  list(APPEND PROFILER_COMPILE_DEFINITIONS
    PROFILER_HAS_KINETO=0 PROFILER_HAS_ITT=0 PROFILER_HAS_NATIVE=1)
endif()

list(APPEND PROFILER_DEPENDENCY_COMPILE_DEFINITIONS ${PROFILER_COMPILE_DEFINITIONS})

# =============================================================================
# SECTION 3 — CMake module includes
#
# Inclusion order:
#   a) Standard CMake introspection helpers
#   b) Shared toolchain helpers  (Cmake/tools/, Cmake/flags/)
#   c) Coverage gate — must follow toolchain includes so that
#      PROFILER_ENABLE_LTO is in scope when coverage forces it OFF
# =============================================================================

# a) Standard CMake introspection
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

# b) Shared toolchain helpers
include(compiler_checks)    # Detects compiler capabilities; sets PROJECT_* flags
include(cache)              # Configures per-target compiler cache launcher
include(build_type)         # Validates / normalises CMAKE_BUILD_TYPE
include(checks)             # Additional feature-check macros
include(utils)              # Utility macros shared across all modules
include(platform)           # Platform flags (macOS rpath, Windows /utf-8, …)
include(CMakeDependentOption)
include(sanitize)
include(clang_tidy)
include(spell)
include(iwyu)
include(valgrind)
include(helper_macros)
include(linker)
# c) Coverage gate
#    When enabled, appends -fprofile-instr-generate / --coverage to global
#    CMAKE_CXX_FLAGS for this module.  LTO is forced OFF here because LTO
#    can move or eliminate profile-counter stores, corrupting the .profraw data.
if(PROFILER_ENABLE_COVERAGE)
  set(PROFILER_ENABLE_LTO OFF)
  include(coverage)
endif()

# =============================================================================
# Runtime library wiring
# Kineto uses AUTO_ADD_TO_DEPENDENCIES=OFF so it must be appended explicitly.
# =============================================================================
if(PROFILER_ENABLE_KINETO AND TARGET Kineto::kineto)
  list(APPEND PROFILER_DEPENDENCY_LIBS Kineto::kineto)
endif()
if(QUARISMA_PROFILER_DEPENDENCY_LIBS)
  list(APPEND PROFILER_DEPENDENCY_LIBS ${QUARISMA_PROFILER_DEPENDENCY_LIBS})
endif()

# =============================================================================
# Source discovery — only sources for the active backend are compiled.
# =============================================================================
set(profiler_headers "")
set(profiler_sources "")

# Per-module override — propagates to all targets in this directory scope,
# including test targets in Testing/Cxx.  CMAKE_CXX_STANDARD_REQUIRED and
# CMAKE_CXX_EXTENSIONS are set globally in the root CMakeLists.txt.
set(CMAKE_CXX_STANDARD ${PROFILER_CXX_STANDARD})

# Common utilities — always compiled (approximate_clock, small_vector, etc.)
file(GLOB common_util_headers LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.h")
file(GLOB common_util_sources LIST_DIRECTORIES false
  "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp")
list(APPEND profiler_headers ${common_util_headers})
list(APPEND profiler_sources ${common_util_sources})

# Kineto backend — base/ is shared infrastructure between Kineto and ITT.
if(PROFILER_ENABLE_KINETO)
  file(GLOB_RECURSE kineto_headers LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h")
  file(GLOB_RECURSE kineto_sources LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/kineto/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp")
  # Platform-specific unwinder is not used by Kineto; exclude it.
  file(GLOB_RECURSE KINETO_UNWIND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/unwind/*.cpp")
  list(REMOVE_ITEM kineto_sources ${KINETO_UNWIND_SOURCES})
  list(APPEND profiler_headers ${kineto_headers})
  list(APPEND profiler_sources ${kineto_sources})
  message(STATUS "Kineto profiler enabled: including base, kineto, and common sources")
endif()

# Native backend — standalone, no external dependencies.
if(PROFILER_ENABLE_NATIVE_PROFILER)
  file(GLOB_RECURSE native_headers LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/native/*.h")
  file(GLOB_RECURSE native_sources LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/native/*.cpp")
  list(APPEND profiler_headers ${native_headers})
  list(APPEND profiler_sources ${native_sources})
  message(STATUS "Native profiler enabled: including native sources (standalone)")
endif()

# ITT backend — base/ is shared with Kineto; guard against double-inclusion.
if(PROFILER_ENABLE_ITT)
  if(NOT PROFILER_ENABLE_KINETO)
    file(GLOB_RECURSE itt_base_headers LIST_DIRECTORIES false
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.h")
    file(GLOB_RECURSE itt_base_sources LIST_DIRECTORIES false
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/base/*.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/common/*.cpp")
    list(APPEND profiler_headers ${itt_base_headers})
    list(APPEND profiler_sources ${itt_base_sources})
  endif()
  file(GLOB_RECURSE itt_headers LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.h")
  file(GLOB_RECURSE itt_sources LIST_DIRECTORIES false
    "${CMAKE_CURRENT_SOURCE_DIR}/bespoke/itt/*.cpp")
  list(APPEND profiler_headers ${itt_headers})
  list(APPEND profiler_sources ${itt_sources})
  message(STATUS "ITT profiler enabled: including base and ITT sources")
endif()

# =============================================================================
# Target definition
# =============================================================================
if(PROFILER_ENABLE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  message(STATUS "Link Time Optimization (LTO) disabled")
endif()

add_library(Profiler ${profiler_sources} ${profiler_headers})
quarisma_target_apply_cache(Profiler PROFILER_ENABLE_CACHE PROFILER_CACHE_BACKEND)
quarisma_target_optional_icecc(Profiler PROFILER_ENABLE_ICECC)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Profiler
    PUBLIC  PROFILER_SHARED_DEFINE
    PRIVATE PROFILER_BUILDING_DLL)
  if(COMMAND profiler_set_target_output_directories)
    profiler_set_target_output_directories(Profiler)
  endif()
else()
  target_compile_definitions(Profiler PUBLIC PROFILER_STATIC_DEFINE)
endif()

add_library(Profiler::Profiler ALIAS Profiler)


if(COMMAND profiler_apply_iwyu)
  profiler_apply_iwyu(Profiler)
endif()

if(COMMAND profiler_module_create_filters)
  profiler_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set_target_properties(Profiler PROPERTIES
  CXX_STANDARD          ${PROFILER_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS        OFF
)

if(MSVC)
  target_compile_options(Profiler PRIVATE /WX)
else()
  target_compile_options(Profiler PRIVATE -include cstdlib)
endif()

# Include roots: Library/Profiler → "common/profiler_export.h", "bespoke/…"
target_include_directories(Profiler
  BEFORE PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
          $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/ProfilerTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/ProfilerTest.h"
  @ONLY
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

if(PROFILER_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Profiler PUBLIC ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS})
  message(STATUS "Profiler: compile definitions: ${PROFILER_DEPENDENCY_COMPILE_DEFINITIONS}")
endif()

# =============================================================================
# Examples
# =============================================================================
if(PROFILER_ENABLE_EXAMPLES)
  add_subdirectory("${CMAKE_SOURCE_DIR}/Examples/Profiling" "${CMAKE_BINARY_DIR}/Examples/Profiling")
  message(STATUS "✓ Profiling Examples enabled")
endif()

# =============================================================================
# Testing
# =============================================================================
if(PROFILER_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(PROJECT_ENABLE_VALGRIND AND PROFILER_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()
