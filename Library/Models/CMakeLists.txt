# =============================================================================
# Library/Models — CMakeLists.txt
#
# Stochastic-vol teachers (ZABR/SABR) and a QA-trained MLP calibrator.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

include(lto)
quarisma_lto_compute_default(_lto_default PROFILE SUPPORT)
set(MODELS_LTO_MODE "${_lto_default}" CACHE STRING "LTO mode for Models: off | thin | full | ipo | auto")
set_property(CACHE MODELS_LTO_MODE PROPERTY STRINGS off thin full ipo auto)

option(MODELS_ENABLE_COVERAGE "Enable coverage instrumentation for Models" OFF)
option(MODELS_ENABLE_TESTING "Build Models test suite" ON)
option(MODELS_ENABLE_EXAMPLES "Build Models example programs" OFF)
option(MODELS_ENABLE_GTEST "Enable GoogleTest for Models" ON)
option(MODELS_ENABLE_BENCHMARK "Enable Google Benchmark for Models" ON)

option(MODELS_ENABLE_ICECC "Use Icecream (icecc) distributed compiler for Models" OFF)
mark_as_advanced(MODELS_ENABLE_ICECC)
option(MODELS_ENABLE_CACHE "Enable compiler caching for the Models target" ON)
mark_as_advanced(MODELS_ENABLE_CACHE)
option(MODELS_ENABLE_CLANGTIDY "Enable clang-tidy static analysis for Models" OFF)
mark_as_advanced(MODELS_ENABLE_CLANGTIDY)
option(MODELS_ENABLE_FIX "Enable clang-tidy fix-errors and fix options for Models" OFF)
mark_as_advanced(MODELS_ENABLE_FIX)
option(MODELS_ENABLE_IWYU "Enable include-what-you-use analysis for Models" OFF)
mark_as_advanced(MODELS_ENABLE_IWYU)
option(MODELS_ENABLE_SANITIZER "Build Models with sanitizer support (Clang/GCC only)" OFF)
set(MODELS_SANITIZER_TYPE "address"
    CACHE STRING "Sanitizer type for Models. Options: address, undefined, thread, memory, leak"
)
set_property(CACHE MODELS_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak)
mark_as_advanced(MODELS_ENABLE_SANITIZER MODELS_SANITIZER_TYPE)
option(
  MODELS_ENABLE_SPELL
  "Enable spell checking with automatic corrections for Models (WARNING: modifies source files)"
  OFF
)
mark_as_advanced(MODELS_ENABLE_SPELL)
option(MODELS_ENABLE_VALGRIND "Execute Models test suite with Valgrind" OFF)
mark_as_advanced(MODELS_ENABLE_VALGRIND)
set(MODELS_LINKER_CHOICE "default" CACHE STRING "Linker for Models: default, lld, mold, gold, lld-link")
set_property(CACHE MODELS_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)
mark_as_advanced(MODELS_LINKER_CHOICE)
set(MODELS_CACHE_BACKEND "none"
    CACHE STRING "Compiler cache backend for Models. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE MODELS_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(MODELS_CACHE_BACKEND)

set(MODELS_CXX_STANDARD "20" CACHE STRING "C++ standard for Models. Options: 11 14 17 20 23")
set_property(CACHE MODELS_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT MODELS_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "MODELS_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)
include(compiler_checks)
include(checks)
include(build_type)
include(platform)
include(cache)
include(CMakeDependentOption)
include(helper_macros)
if(MODELS_ENABLE_SANITIZER)
  include(sanitize)
endif()
if(MODELS_ENABLE_SPELL)
  include(spell)
endif()
if(MODELS_ENABLE_VALGRIND)
  include(valgrind)
endif()
if(MODELS_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FULL")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FULL")
endif()

set(models_headers "")
set(models_sources "")
set(CMAKE_CXX_STANDARD ${MODELS_CXX_STANDARD})

file(GLOB_RECURSE models_headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE models_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER models_headers EXCLUDE REGEX ".*/Testing/.*")
list(FILTER models_sources EXCLUDE REGEX ".*/Testing/.*")

add_library(Models ${models_sources} ${models_headers})
quarisma_target_apply_lto(Models MODELS)
if(MODELS_ENABLE_SANITIZER)
  quarisma_configure_sanitizer(MODELS)
endif()
quarisma_find_linker(${MODELS_LINKER_CHOICE} Models MODELS_LTO_MODE)
quarisma_target_apply_cache(Models MODELS_ENABLE_CACHE MODELS_CACHE_BACKEND)
quarisma_target_optional_icecc(Models MODELS_ENABLE_ICECC)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(Models PUBLIC MODELS_SHARED_DEFINE PRIVATE MODELS_BUILDING_DLL)
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Models)
  endif()
else()
  target_compile_definitions(Models PUBLIC MODELS_STATIC_DEFINE)
endif()

add_library(Models::Models ALIAS Models)

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(
  Models PROPERTIES CXX_STANDARD ${MODELS_CXX_STANDARD} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Models PRIVATE /WX)
else()
  target_compile_options(Models PRIVATE -include cstdlib)
endif()

target_include_directories(
  Models
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/ModelsTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/ModelsTest.h" @ONLY
)

if(MODELS_ENABLE_CLANGTIDY)
  include(clang_tidy)
  quarisma_target_clang_tidy(Models ${MODELS_ENABLE_FIX})
endif()
if(MODELS_ENABLE_IWYU)
  include(iwyu)
  quarisma_apply_iwyu(Models)
endif()

install(
  TARGETS Models
  EXPORT ModelsTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)
install(FILES ${models_headers} DESTINATION include/Models/Models COMPONENT Development)

if(MODELS_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()
if(MODELS_ENABLE_VALGRIND AND MODELS_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

message("")
message("******** Models module (CMake flags) ********")
message("  Cxx standard     : ${MODELS_CXX_STANDARD}")
message("  Cache            : ${MODELS_ENABLE_CACHE}")
message("  Cache backend    : ${MODELS_CACHE_BACKEND}")
message("  Icecc            : ${MODELS_ENABLE_ICECC}")
quarisma_print_lto_linker_summary(MODELS)
message("  Coverage         : ${MODELS_ENABLE_COVERAGE}")
message("  Testing          : ${MODELS_ENABLE_TESTING}")
message("  Gtest            : ${MODELS_ENABLE_GTEST}")
message("  Benchmark        : ${MODELS_ENABLE_BENCHMARK}")
message("  Clang-tidy       : ${MODELS_ENABLE_CLANGTIDY}")
message("  Sanitizer        : ${MODELS_ENABLE_SANITIZER}")
message("  Valgrind         : ${MODELS_ENABLE_VALGRIND}")
message("")
