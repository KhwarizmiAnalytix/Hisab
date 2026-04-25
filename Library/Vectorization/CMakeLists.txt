# =============================================================================
# Library/Vectorization — CMakeLists.txt
#
# Provides the Vectorization library: SIMD packet abstractions, expression
# templates, and architecture-specific kernels for SSE, AVX, AVX2, and AVX-512.
#
# CMake feature flags (VECTORIZATION_ENABLE_*):
#   VECTORIZATION_ENABLE_SSE    — request SSE SIMD tier
#   VECTORIZATION_ENABLE_AVX    — request AVX SIMD tier
#   VECTORIZATION_ENABLE_AVX2   — request AVX2 SIMD tier (default ON when hardware supports it)
#   VECTORIZATION_ENABLE_AVX512 — request AVX-512 SIMD tier
#   VECTORIZATION_ENABLE_SVML   — request Intel SVML short-vector math library
#
# C++ preprocessor defines emitted (VECTORIZATION_HAS_*):
#   VECTORIZATION_HAS_SSE=1/0    — SSE tier active in C++ code
#   VECTORIZATION_HAS_AVX=1/0    — AVX tier active in C++ code
#   VECTORIZATION_HAS_AVX2=1/0   — AVX2 tier active in C++ code
#   VECTORIZATION_HAS_AVX512=1/0 — AVX-512 tier active in C++ code
#   VECTORIZATION_HAS_SVML=1/0   — SVML active in C++ code
#   VECTORIZATION_HAS_GTEST=1/0  — Google Test available to Vectorization tests
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# =============================================================================
# SECTION 1 — Feature flags (VECTORIZATION_ENABLE_*) and backend selectors
#
# SIMD tier defaults are hardware-detected from TMP_COMPILER_SUPPORTS_* CACHE
# booleans written by check_cxx_source_compiles in Library/Vectorization/Cmake/utils.cmake.
# VECTORIZATION_TYPE (Core cache variable, default "avx2") controls which tier
# is active by default. Individual flags can be overridden on the command line.
# =============================================================================

option(VECTORIZATION_ENABLE_LTO "Enable Link Time Optimization for Vectorization" OFF)
option(VECTORIZATION_ENABLE_COVERAGE "Enable coverage instrumentation for Vectorization" OFF)
option(VECTORIZATION_ENABLE_TESTING "Build Vectorization test suite" ON)
option(VECTORIZATION_ENABLE_EXAMPLES "Build Vectorization example programs" OFF)
option(VECTORIZATION_ENABLE_GTEST "Enable GoogleTest for Vectorization" ON)
option(VECTORIZATION_ENABLE_BENCHMARK "Enable Google Benchmark for Vectorization" OFF)

# --- SIMD tier flags ---
# Defaults are derived from VECTORIZATION_TYPE + hardware-detection cache results.
if(TMP_COMPILER_SUPPORTS_SSE_EXTENSIONS AND VECTORIZATION_TYPE MATCHES "^(sse|avx|avx2|avx512)$")
  set(_def_sse ON)
else()
  set(_def_sse OFF)
endif()
if(TMP_COMPILER_SUPPORTS_AVX_EXTENSIONS AND VECTORIZATION_TYPE MATCHES "^(avx|avx2|avx512)$")
  set(_def_avx ON)
else()
  set(_def_avx OFF)
endif()
if(TMP_COMPILER_SUPPORTS_AVX2_EXTENSIONS AND VECTORIZATION_TYPE MATCHES "^(avx2|avx512)$")
  set(_def_avx2 ON)
else()
  set(_def_avx2 OFF)
endif()
if(TMP_COMPILER_SUPPORTS_AVX512_EXTENSIONS AND VECTORIZATION_TYPE STREQUAL "avx512")
  set(_def_avx512 ON)
else()
  set(_def_avx512 OFF)
endif()

option(VECTORIZATION_ENABLE_SSE    "Enable SSE SIMD tier"    ${_def_sse})
option(VECTORIZATION_ENABLE_AVX    "Enable AVX SIMD tier"    ${_def_avx})
option(VECTORIZATION_ENABLE_AVX2   "Enable AVX2 SIMD tier"   ${_def_avx2})
option(VECTORIZATION_ENABLE_AVX512 "Enable AVX-512 SIMD tier" ${_def_avx512})

# Intel SVML — short vector math library for fast sin/cos/exp/log on SIMD lanes.
option(VECTORIZATION_ENABLE_SVML "Enable Intel SVML short vector math library" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SVML)

option(VECTORIZATION_ENABLE_ICECC "Use Icecream distributed compiler for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_ICECC)
option(VECTORIZATION_ENABLE_CACHE "Enable compiler caching for the Vectorization target" ON)
mark_as_advanced(VECTORIZATION_ENABLE_CACHE)
option(VECTORIZATION_ENABLE_CLANGTIDY "Enable clang-tidy static analysis for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_CLANGTIDY)
option(VECTORIZATION_ENABLE_FIX "Enable clang-tidy fix options for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_FIX)
option(VECTORIZATION_ENABLE_IWYU "Enable include-what-you-use analysis for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_IWYU)
option(VECTORIZATION_ENABLE_SANITIZER
       "Build Vectorization with sanitizer support (Clang/GCC only)" OFF)
set(VECTORIZATION_SANITIZER_TYPE "address"
    CACHE STRING
          "Sanitizer type for Vectorization. Options: address, undefined, thread, memory, leak"
)
set_property(CACHE VECTORIZATION_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory
                                                                  leak)
mark_as_advanced(VECTORIZATION_ENABLE_SANITIZER VECTORIZATION_SANITIZER_TYPE)
option(VECTORIZATION_ENABLE_SPELL
       "Enable spell checking for Vectorization (WARNING: modifies source files)" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SPELL)
option(VECTORIZATION_ENABLE_VALGRIND "Execute Vectorization test suite with Valgrind" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_VALGRIND)
set(VECTORIZATION_LINKER_CHOICE "default"
    CACHE STRING "Linker for Vectorization: default, lld, mold, gold, lld-link"
)
set_property(CACHE VECTORIZATION_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)
mark_as_advanced(VECTORIZATION_LINKER_CHOICE)
set(VECTORIZATION_CACHE_BACKEND "none"
    CACHE STRING
          "Compiler cache backend for Vectorization. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE VECTORIZATION_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(VECTORIZATION_CACHE_BACKEND)
set(VECTORIZATION_CXX_STANDARD "20"
    CACHE STRING "C++ standard for Vectorization. Options: 11 14 17 20 23"
)
set_property(CACHE VECTORIZATION_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT VECTORIZATION_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "VECTORIZATION_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# =============================================================================
# SECTION 2 — Derive SIMD compiler flags from VECTORIZATION_ENABLE_* flags
#
# Flags are target-scoped (applied via target_compile_options PUBLIC so all
# consumers inherit the correct ISA). The highest active tier wins.
# =============================================================================

set(_vec_simd_flags "")
if(VECTORIZATION_ENABLE_AVX512)
  if(MSVC)
    set(_vec_simd_flags "/D__AVX512F__ /D__AVX512DQ__ /D__AVX512VL__ /D__F16C__")
  else()
    set(_vec_simd_flags "-mavx512f -mavx512dq -mavx512vl -mf16c")
  endif()
elseif(VECTORIZATION_ENABLE_AVX2)
  if(MSVC)
    set(_vec_simd_flags "/arch:AVX2 /D__F16C__")
  else()
    set(_vec_simd_flags "-mavx2 -mf16c")
  endif()
elseif(VECTORIZATION_ENABLE_AVX)
  if(MSVC)
    set(_vec_simd_flags "/arch:AVX /D__F16C__")
  else()
    set(_vec_simd_flags "-mavx -mf16c")
  endif()
elseif(VECTORIZATION_ENABLE_SSE)
  if(NOT MSVC)
    set(_vec_simd_flags "-msse4.2 -msse4.1 -msse2 -msse")
  endif()
endif()
if(_vec_simd_flags AND TMP_COMPILER_SUPPORTS_FMA_EXTENSIONS)
  if(MSVC)
    string(APPEND _vec_simd_flags " /D__FMA__")
  else()
    string(APPEND _vec_simd_flags " -mfma")
  endif()
endif()

# =============================================================================
# SECTION 3 — CMake module includes
# =============================================================================

if(VECTORIZATION_ENABLE_COVERAGE OR VECTORIZATION_ENABLE_SANITIZER)
  set(VECTORIZATION_ENABLE_LTO OFF)
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

include(compiler_checks)
include(checks)
include(build_type)
include(platform)
include(compile_definitions)
include(CMakeDependentOption)
include(cache)

if(VECTORIZATION_ENABLE_SANITIZER)
  include(sanitize)
  quarisma_configure_sanitizer(VECTORIZATION)
endif()
if(VECTORIZATION_ENABLE_SPELL)
  include(spell)
endif()
if(VECTORIZATION_ENABLE_VALGRIND)
  include(valgrind)
endif()
include(helper_macros)

if(VECTORIZATION_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FULL")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FULL")
endif()

if(VECTORIZATION_ENABLE_LTO AND NOT VECTORIZATION_ENABLE_COVERAGE
   AND NOT VECTORIZATION_ENABLE_SANITIZER)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  message(STATUS "Link Time Optimization (LTO) enabled for Vectorization")
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
endif()

include(linker)

# --- VECTORIZATION_ENABLE_* → VECTORIZATION_HAS_* compile definitions ---
# compile_definition() reads the ENABLE_ option and emits HAS_=1 or HAS_=0.
set(VECTORIZATION_COMPILE_DEFINITIONS "")
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SSE)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_AVX)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_AVX2)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_AVX512)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SVML)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_GTEST)

set(VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "")
list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS ${VECTORIZATION_COMPILE_DEFINITIONS})

# =============================================================================
# SECTION 4 — Source discovery with SIMD-tier filtering
# =============================================================================

set(CMAKE_CXX_STANDARD ${VECTORIZATION_CXX_STANDARD})

file(GLOB_RECURSE headers LIST_DIRECTORIES false
     "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)
file(GLOB_RECURSE sources   LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx")
file(GLOB_RECURSE templates LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")

list(FILTER sources   EXCLUDE REGEX ".*/Testing/.*")
list(FILTER headers   EXCLUDE REGEX ".*/Testing/.*")
list(FILTER templates EXCLUDE REGEX ".*/Testing/.*")

# Exclude SIMD subdirectories whose tier flag is OFF.
if(NOT VECTORIZATION_ENABLE_SSE)
  list(FILTER headers   EXCLUDE REGEX ".*/sse/.*")
  list(FILTER sources   EXCLUDE REGEX ".*/sse/.*")
  list(FILTER templates EXCLUDE REGEX ".*/sse/.*")
endif()

# avx/ serves both AVX and AVX2; include it when either (or AVX512) is active.
if(NOT VECTORIZATION_ENABLE_AVX AND NOT VECTORIZATION_ENABLE_AVX2
   AND NOT VECTORIZATION_ENABLE_AVX512)
  list(FILTER headers   EXCLUDE REGEX ".*/avx/.*")
  list(FILTER sources   EXCLUDE REGEX ".*/avx/.*")
  list(FILTER templates EXCLUDE REGEX ".*/avx/.*")
endif()

if(NOT VECTORIZATION_ENABLE_AVX512)
  list(FILTER headers   EXCLUDE REGEX ".*/avx512/.*")
  list(FILTER sources   EXCLUDE REGEX ".*/avx512/.*")
  list(FILTER templates EXCLUDE REGEX ".*/avx512/.*")
endif()

if(NOT VECTORIZATION_ENABLE_SVML)
  list(FILTER headers EXCLUDE REGEX ".*/svml\\.h$")
endif()

# =============================================================================
# SECTION 5 — Target definition
# =============================================================================

add_library(Vectorization ${sources} ${headers} ${templates})
quarisma_find_linker(${VECTORIZATION_LINKER_CHOICE} Vectorization)
quarisma_target_apply_cache(Vectorization VECTORIZATION_ENABLE_CACHE VECTORIZATION_CACHE_BACKEND)
quarisma_target_optional_icecc(Vectorization VECTORIZATION_ENABLE_ICECC)

# --- DLL-export macros (VECTORIZATION_API declared in generated vectorization_export.h) ---
if(BUILD_SHARED_LIBS)
  target_compile_definitions(
    Vectorization PUBLIC VECTORIZATION_SHARED_DEFINE PRIVATE VECTORIZATION_BUILDING_DLL
  )
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Vectorization)
  endif()
else()
  target_compile_definitions(Vectorization PUBLIC VECTORIZATION_STATIC_DEFINE)
endif()

include(GenerateExportHeader)
generate_export_header(
  Vectorization
  BASE_NAME         VECTORIZATION
  EXPORT_MACRO_NAME VECTORIZATION_API
  EXPORT_FILE_NAME  "${CMAKE_CURRENT_BINARY_DIR}/vectorization_export.h"
  STATIC_DEFINE     VECTORIZATION_STATIC_DEFINE
)

add_library(Vectorization::Vectorization ALIAS Vectorization)
add_library(Quarisma::Vectorization      ALIAS Vectorization)

set_target_properties(
  Vectorization PROPERTIES
  CXX_STANDARD          ${VECTORIZATION_CXX_STANDARD}
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS        OFF
)

if(MSVC)
  target_compile_options(Vectorization PRIVATE /WX)
  if(EXISTS "${PROJECT_SOURCE_DIR}/PRETORIAN.natvis")
    target_sources(
      Vectorization PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/PRETORIAN.natvis>
    )
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND VECTORIZATION_ENABLE_SSE)
    target_compile_definitions(Vectorization PUBLIC VECTORIZATION_CLANG_CL)
  endif()
else()
  target_compile_options(Vectorization PRIVATE -include cstdlib)
endif()

# Apply SIMD flags as PUBLIC target options so all consumers inherit the correct ISA.
if(_vec_simd_flags)
  if(MSVC)
    separate_arguments(_vec_simd_flags_list WINDOWS_COMMAND "${_vec_simd_flags}")
  else()
    separate_arguments(_vec_simd_flags_list UNIX_COMMAND "${_vec_simd_flags}")
  endif()
  target_compile_options(Vectorization PUBLIC ${_vec_simd_flags_list})
endif()

# =============================================================================
# SECTION 6 — Include directories and link libraries
# =============================================================================

target_include_directories(
  Vectorization
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(VECTORIZATION_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Vectorization: extra include dirs: ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS}")
endif()

target_link_libraries(Vectorization PUBLIC Core::Core)

if(VECTORIZATION_DEPENDENCY_LIBS)
  target_link_libraries(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_LIBS})
  message(STATUS "Vectorization: linked dependencies: ${VECTORIZATION_DEPENDENCY_LIBS}")
endif()

if(VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS})
  message(
    STATUS "Vectorization: compile definitions: ${VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS}"
  )
endif()

if(VECTORIZATION_ENABLE_CLANGTIDY)
  include(clang_tidy)
  quarisma_target_clang_tidy(Vectorization ${VECTORIZATION_ENABLE_FIX})
endif()

if(VECTORIZATION_ENABLE_IWYU)
  include(iwyu)
  quarisma_apply_iwyu(Vectorization)
endif()

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

# =============================================================================
# SECTION 7 — Installation
# =============================================================================

install(
  TARGETS Vectorization
  EXPORT  QuarismaTargets
  LIBRARY   DESTINATION lib
  ARCHIVE   DESTINATION lib
  RUNTIME   DESTINATION bin
  INCLUDES  DESTINATION include
)
install(FILES ${headers} DESTINATION include/Quarisma/Vectorization COMPONENT Development)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vectorization_export.h"
  DESTINATION include/Quarisma/Vectorization
  COMPONENT Development
)

# =============================================================================
# SECTION 8 — Testing
# =============================================================================

if(VECTORIZATION_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(VECTORIZATION_ENABLE_VALGRIND AND VECTORIZATION_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

# =============================================================================
# SECTION 9 — Summary message
# =============================================================================

message("")
message("******** Vectorization module (CMake flags) ********")
message("  Vectorization type   : ${VECTORIZATION_TYPE}")
message("  SIMD flags           : ${_vec_simd_flags}")
message("  Enable SSE           : ${VECTORIZATION_ENABLE_SSE}")
message("  Enable AVX           : ${VECTORIZATION_ENABLE_AVX}")
message("  Enable AVX2          : ${VECTORIZATION_ENABLE_AVX2}")
message("  Enable AVX512        : ${VECTORIZATION_ENABLE_AVX512}")
message("  Svml                 : ${VECTORIZATION_ENABLE_SVML}")
message("  Cxx standard         : ${VECTORIZATION_CXX_STANDARD}")
message("  Cache                : ${VECTORIZATION_ENABLE_CACHE}")
message("  Cache backend        : ${VECTORIZATION_CACHE_BACKEND}")
message("  Icecc                : ${VECTORIZATION_ENABLE_ICECC}")
message("  Linker               : ${VECTORIZATION_LINKER_CHOICE}")
message("  Lto                  : ${VECTORIZATION_ENABLE_LTO}")
message("  Coverage             : ${VECTORIZATION_ENABLE_COVERAGE}")
message("  Testing              : ${VECTORIZATION_ENABLE_TESTING}")
message("  Gtest                : ${VECTORIZATION_ENABLE_GTEST}")
message("  Benchmark            : ${VECTORIZATION_ENABLE_BENCHMARK}")
message("  Clang-tidy           : ${VECTORIZATION_ENABLE_CLANGTIDY}")
message("  Fix                  : ${VECTORIZATION_ENABLE_FIX}")
message("  Iwyu                 : ${VECTORIZATION_ENABLE_IWYU}")
message("  Sanitizer            : ${VECTORIZATION_ENABLE_SANITIZER}")
message("  Sanitizer type       : ${VECTORIZATION_SANITIZER_TYPE}")
message("  Spell                : ${VECTORIZATION_ENABLE_SPELL}")
message("  Valgrind             : ${VECTORIZATION_ENABLE_VALGRIND}")
message("")
message("Build:")
message("  C++ standard             : ${CMAKE_CXX_STANDARD}")
message("  C flags                  : ${CMAKE_C_FLAGS}")
message("  CXX flags                : ${CMAKE_CXX_FLAGS}")
message("  Shared linker flags      : ${CMAKE_SHARED_LINKER_FLAGS}")
message("  IPO / LTO (CMake)        : ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
message("")
