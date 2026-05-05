# =============================================================================
# Library/Vectorization — CMakeLists.txt
#
# Provides the Vectorization library: SIMD packet abstractions, expression templates, and
# architecture-specific kernels for SSE, AVX, AVX2, and AVX-512.
#
# VECTORIZATION_TYPE (cache: no | sse | avx | avx2 | avx512 | neon | sve) selects which backend/ source trees are
# compiled in. Optional VECTORIZATION_ENABLE_SVML adds SVML headers.
#
# C++ preprocessor defines when vectorized: sse → HAS_SSE=1; avx → HAS_AVX=1; avx2 → HAS_AVX=1 and
# HAS_AVX2=1 (AVX2 extends AVX; same backend/avx/ tree); avx512 → HAS_AVX512=1 (HAS_AVX may be 0).
# VECTORIZATION_HAS_SVML=1/0   — SVML active in C++ code VECTORIZATION_HAS_GTEST=1/0  — Google Test
# available to Vectorization tests
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# =============================================================================
# SECTION 1 — Cache
# options (VECTORIZATION_TYPE, tooling) and backend selectors
#
# include(utils) runs compiler feature checks (SIMD flags, exception_ptr); results are published to
# CACHE for Core (VECTORIZATION_COMPILER_FLAGS, PROJECT_*). VECTORIZATION_TYPE drives
# backend source filters and VECTORIZATION_HAS_* one-to-one.
# =============================================================================
set(VECTORIZATION_TYPE "avx2"
    CACHE STRING
        "vectorization implementation to use. Options are no, sse, avx, avx2, avx512, neon, sve"
)
set_property(CACHE VECTORIZATION_TYPE PROPERTY STRINGS no sse avx avx2 avx512 neon sve)
if(NOT VECTORIZATION_TYPE MATCHES "^(no|sse|avx|avx2|avx512|neon|sve)$")
  message(
    FATAL_ERROR
      "VECTORIZATION_TYPE must be one of: no, sse, avx, avx2, avx512, neon, sve (got '${VECTORIZATION_TYPE}')"
  )
endif()

set(VECTORIZATION_PACKET_SIZE "4"
    CACHE STRING
        "SIMD packet lane count (vectorization::packet_size<T>::value); must match your ISA and kernels"
)
if(NOT VECTORIZATION_PACKET_SIZE MATCHES "^[1-9][0-9]*$")
  message(
    FATAL_ERROR
      "VECTORIZATION_PACKET_SIZE must be a positive decimal integer (got '${VECTORIZATION_PACKET_SIZE}')"
  )
endif()
if(VECTORIZATION_PACKET_SIZE GREATER 256)
  message(
    FATAL_ERROR
      "VECTORIZATION_PACKET_SIZE must be at most 256 (got '${VECTORIZATION_PACKET_SIZE}')"
  )
endif()

include(lto)
quarisma_lto_compute_default(_lto_default PROFILE PERFORMANCE)
set(VECTORIZATION_LTO_MODE "${_lto_default}"
    CACHE STRING "LTO mode for Vectorization: off | thin | full | ipo | auto")
set_property(CACHE VECTORIZATION_LTO_MODE PROPERTY STRINGS off thin full ipo auto)
option(VECTORIZATION_ENABLE_COVERAGE "Enable coverage instrumentation for Vectorization" OFF)
option(VECTORIZATION_ENABLE_TESTING "Build Vectorization test suite" ON)
option(VECTORIZATION_ENABLE_EXAMPLES "Build Vectorization example programs" OFF)
option(VECTORIZATION_ENABLE_GTEST "Enable GoogleTest for Vectorization" ON)
option(VECTORIZATION_ENABLE_BENCHMARK "Enable Google Benchmark for Vectorization" ON)

# Clang/GCC: append -march=native for extra instruction scheduling / tuning on the build host.
# Improves runtime on this machine; the binary may not run on older CPUs. MSVC ignores this (no flag
# added).
option(USE_NATIVE_ARCH "Prefer host CPU instruction tuning (-march=native, Clang/GCC only)" OFF)

option(VECTORIZATION_ENABLE_SVML "Enable Intel SVML short vector math library" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SVML)

option(VECTORIZATION_ENABLE_SLEEF "Enable SLEEF SIMD math library for NEON" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SLEEF)

option(VECTORIZATION_ENABLE_ACCELERATE "Enable Apple Accelerate vForce on AArch64 NEON" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_ACCELERATE)

# Memory presence — detected from the CMake target; not a user option.
if(TARGET Memory::Memory)
  set(_vec_has_memory 1)
else()
  set(_vec_has_memory 0)
endif()

# Logging presence — detected from the CMake target; not a user option.
if(TARGET Logging::Logging)
  set(_vec_has_logging 1)
else()
  set(_vec_has_logging 0)
endif()

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
option(VECTORIZATION_ENABLE_SANITIZER "Build Vectorization with sanitizer support (Clang/GCC only)"
       OFF
)
set(VECTORIZATION_SANITIZER_TYPE "address"
    CACHE STRING
          "Sanitizer type for Vectorization. Options: address, undefined, thread, memory, leak"
)
set_property(
  CACHE VECTORIZATION_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak
)
mark_as_advanced(VECTORIZATION_ENABLE_SANITIZER VECTORIZATION_SANITIZER_TYPE)
option(VECTORIZATION_ENABLE_SPELL
       "Enable spell checking for Vectorization (WARNING: modifies source files)" OFF
)
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
set(VECTORIZATION_CXX_STANDARD "20" CACHE STRING
                                          "C++ standard for Vectorization. Options: 11 14 17 20 23"
)
set_property(CACHE VECTORIZATION_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT VECTORIZATION_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "VECTORIZATION_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# SVE vector register width. The current backend/sve/ implementation only supports 128 bits.
# Set to 256 or 512 once those backends are extended to support scalable widths.
set(VECTORIZATION_SVE_VECTOR_BITS "128"
    CACHE STRING "SVE vector register width in bits: 128, 256, 512"
)
set_property(CACHE VECTORIZATION_SVE_VECTOR_BITS PROPERTY STRINGS 128 256 512)
if(NOT VECTORIZATION_SVE_VECTOR_BITS MATCHES "^(128|256|512)$")
  message(FATAL_ERROR "VECTORIZATION_SVE_VECTOR_BITS must be 128, 256, or 512")
endif()

# =============================================================================
# SECTION 3 — CMake
# module includes
# =============================================================================
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
include(utils)

# Auto-downgrade to "no" if the requested SIMD tier is unsupported (e.g. ARM64 has no x86 SSE/AVX).
if(NOT VECTORIZATION_TYPE STREQUAL "no" AND NOT VECTORIZATION)
  message(
    WARNING
      "VECTORIZATION_TYPE '${VECTORIZATION_TYPE}' is not supported by this compiler/architecture. "
      "Falling back to VECTORIZATION_TYPE=no."
  )
  set(VECTORIZATION_TYPE "no"
      CACHE STRING
          "vectorization implementation to use. Options are no, sse, avx, avx2, avx512, neon, sve"
      FORCE
  )
endif()

# Other libraries must not include Vectorization CMake modules; expose probes for Core et al.
if(HAS_EXCEPTION_PTR)
  set(PROJECT_HAS_EXCEPTION_PTR 1 CACHE INTERNAL
                                        "Quarisma: std::exception_ptr (Vectorization utils)" FORCE
  )
else()
  set(PROJECT_HAS_EXCEPTION_PTR 0 CACHE INTERNAL
                                        "Quarisma: std::exception_ptr (Vectorization utils)" FORCE
  )
endif()
foreach(_simd IN ITEMS SSE AVX AVX2 AVX512 NEON SVE)
  if(HAS_${_simd})
    set(PROJECT_${_simd} 1 CACHE INTERNAL "Quarisma: SIMD (Vectorization utils)" FORCE)
  else()
    set(PROJECT_${_simd} 0 CACHE INTERNAL "Quarisma: SIMD (Vectorization utils)" FORCE)
  endif()
endforeach()
set(VECTORIZATION_COMPILER_FLAGS "${VECTORIZATION_COMPILER_FLAGS}"
    CACHE INTERNAL "Quarisma: SIMD flags from Vectorization utils" FORCE
)


message("-- SIMD compiler flags: ${VECTORIZATION_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VECTORIZATION_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VECTORIZATION_COMPILER_FLAGS}")

# =============================================================================
# SECTION 2 —
# VECTORIZATION_TYPE ↔ exactly one VECTORIZATION_HAS_* and backend trees
# =============================================================================
if(VECTORIZATION_ENABLE_SANITIZER)
  include(sanitize)
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

# --- VECTORIZATION_TYPE → VECTORIZATION_HAS_* (one hot) ---
set(VECTORIZATION_COMPILE_DEFINITIONS "")

if(VECTORIZATION_TYPE STREQUAL "sse")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SSE=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SSE=0)
endif()

# AVX2 implies AVX; set once to avoid duplicate -DVECTORIZATION_HAS_AVX on the command line.
if(VECTORIZATION_TYPE STREQUAL "avx" OR VECTORIZATION_TYPE STREQUAL "avx2")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX=0)
endif()

if(VECTORIZATION_TYPE STREQUAL "avx2")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX2=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX2=0)
endif()

if(VECTORIZATION_TYPE STREQUAL "avx512")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX512=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX512=0)
endif()

if(VECTORIZATION_TYPE STREQUAL "neon")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_NEON=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_NEON=0)
endif()

if(VECTORIZATION_TYPE STREQUAL "sve")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SVE=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SVE=0)
endif()

compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SVML)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SLEEF)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_ACCELERATE)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_GTEST)
list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_MEMORY=${_vec_has_memory})
list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_LOGGING=${_vec_has_logging})
if(VECTORIZATION_TYPE MATCHES "^(sse|avx|avx2|avx512|neon|sve)$")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_VECTORIZED=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_VECTORIZED=0)
endif()

list(APPEND VECTORIZATION_COMPILE_DEFINITIONS
     VECTORIZATION_PACKET_SIZE=${VECTORIZATION_PACKET_SIZE}
)

set(VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "")
list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS ${VECTORIZATION_COMPILE_DEFINITIONS})

# =============================================================================
# SECTION 4 — Source
# discovery with SIMD-tier filtering
# =============================================================================
set(CMAKE_CXX_STANDARD ${VECTORIZATION_CXX_STANDARD})

file(GLOB_RECURSE headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                                                 "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)
file(GLOB_RECURSE sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
                                                 "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE templates LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")

list(FILTER sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER headers EXCLUDE REGEX ".*/Testing/.*")
list(FILTER templates EXCLUDE REGEX ".*/Testing/.*")

# Exclude SIMD backend trees not selected by VECTORIZATION_TYPE.
if(NOT VECTORIZATION_TYPE STREQUAL "sse")
  list(FILTER headers EXCLUDE REGEX ".*/backend/sse/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/sse/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/sse/.*")
endif()
if(NOT VECTORIZATION_TYPE MATCHES "^(avx|avx2)$")
  list(FILTER headers EXCLUDE REGEX ".*/backend/avx/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/avx/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/avx/.*")
endif()
if(NOT VECTORIZATION_TYPE STREQUAL "avx512")
  list(FILTER headers EXCLUDE REGEX ".*/backend/avx512/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/avx512/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/avx512/.*")
endif()
if(NOT VECTORIZATION_TYPE STREQUAL "neon" AND NOT VECTORIZATION_TYPE STREQUAL "sve")
  list(FILTER headers EXCLUDE REGEX ".*/backend/neon/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/neon/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/neon/.*")
endif()
if(NOT VECTORIZATION_TYPE STREQUAL "sve")
  list(FILTER headers EXCLUDE REGEX ".*/backend/sve/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/sve/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/sve/.*")
endif()

if(NOT VECTORIZATION_ENABLE_SVML)
  list(FILTER headers EXCLUDE REGEX ".*/svml\\.h$")
endif()

if(NOT VECTORIZATION_TYPE STREQUAL "neon")
  list(FILTER headers EXCLUDE REGEX ".*/backend/neon/svml\\.h$")
endif()

# =============================================================================
# SECTION 5 — Target
# definition
# =============================================================================
add_library(Vectorization ${sources} ${headers} ${templates})
quarisma_target_apply_lto(Vectorization VECTORIZATION)
if(VECTORIZATION_ENABLE_SANITIZER)
  quarisma_configure_sanitizer(VECTORIZATION)
endif()
quarisma_find_linker(${VECTORIZATION_LINKER_CHOICE} Vectorization VECTORIZATION_LTO_MODE)
quarisma_target_apply_cache(Vectorization VECTORIZATION_ENABLE_CACHE VECTORIZATION_CACHE_BACKEND)
quarisma_target_optional_icecc(Vectorization VECTORIZATION_ENABLE_ICECC)

# --- DLL-export macros (VECTORIZATION_API in source-tree vectorization_export.h) ---
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

add_library(Vectorization::Vectorization ALIAS Vectorization)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/VectorizationTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/VectorizationTest.h" @ONLY
)

set_target_properties(
  Vectorization PROPERTIES CXX_STANDARD ${VECTORIZATION_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
                           CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(Vectorization PRIVATE /WX)
  if(EXISTS "${PROJECT_SOURCE_DIR}/PRETORIAN.natvis")
    target_sources(Vectorization PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/PRETORIAN.natvis>)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND VECTORIZATION_TYPE STREQUAL "sse")
    target_compile_definitions(Vectorization PUBLIC VECTORIZATION_CLANG_CL)
  endif()
else()
  target_compile_options(Vectorization PRIVATE -include cstdlib)
endif()

# Apply SIMD flags as PUBLIC target options so all consumers inherit the correct ISA.
if(VECTORIZATION_COMPILER_FLAGS)
  if(MSVC)
    separate_arguments(_vec_simd_flags_list WINDOWS_COMMAND "${VECTORIZATION_COMPILER_FLAGS}")
  else()
    separate_arguments(_vec_simd_flags_list UNIX_COMMAND "${VECTORIZATION_COMPILER_FLAGS}")
  endif()
  target_compile_options(Vectorization PUBLIC ${_vec_simd_flags_list})
endif()

# =============================================================================
# SECTION 6 — Include
# directories and link libraries
# =============================================================================
target_include_directories(
  Vectorization
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

if(VECTORIZATION_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Vectorization: extra include dirs: ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS}")
endif()

if(_vec_has_logging)
  target_link_libraries(Vectorization PUBLIC Logging::Logging)
endif()

if(_vec_has_memory)
  target_link_libraries(Vectorization PUBLIC Memory::Memory)
endif()

if(VECTORIZATION_DEPENDENCY_LIBS)
  target_link_libraries(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_LIBS})
  message(STATUS "Vectorization: linked dependencies: ${VECTORIZATION_DEPENDENCY_LIBS}")
endif()

# Link Apple Accelerate framework when vForce is enabled.
if(VECTORIZATION_ENABLE_ACCELERATE AND APPLE)
  target_link_libraries(Vectorization PUBLIC "-framework Accelerate")
  message(STATUS "Vectorization: linked Apple Accelerate framework (vForce).")
endif()

# Link ThirdParty SVML and stage its runtime binaries next to the output library.
if(VECTORIZATION_ENABLE_SVML AND TARGET SVML::SVML)
  target_link_libraries(Vectorization PUBLIC SVML::SVML)
  if(TARGET SVML::IRC)
    target_link_libraries(Vectorization PUBLIC SVML::IRC)
  endif()
  if(TARGET SVML::INTLC)
    target_link_libraries(Vectorization PUBLIC SVML::INTLC)
  endif()
  foreach(_svml_rt IN LISTS SVML_RUNTIME_LIBRARIES)
    if(_svml_rt)
      add_custom_command(TARGET Vectorization POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_svml_rt}"
          "$<TARGET_FILE_DIR:Vectorization>"
        COMMENT "Staging SVML runtime: ${_svml_rt}"
        VERBATIM
      )
    endif()
  endforeach()
  unset(_svml_rt)
  message(STATUS "Vectorization: linked SVML::SVML (ThirdParty); runtime files: ${SVML_RUNTIME_LIBRARIES}")
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
# SECTION 7 —
# Installation
# =============================================================================
install(
  TARGETS Vectorization
  EXPORT VectorizationTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)
install(FILES ${headers} DESTINATION include/Vectorization COMPONENT Development)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vectorization_export.h"
              "${CMAKE_CURRENT_SOURCE_DIR}/Testing/VectorizationTest.h"
        DESTINATION include/Vectorization COMPONENT Development
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
# SECTION 9 — Summary
# message
# =============================================================================
message("")
message("******** Vectorization module (CMake flags) ********")
message("  Vectorization type   : ${VECTORIZATION_TYPE}")
message("  Packet size          : ${VECTORIZATION_PACKET_SIZE}")
message("  Svml                 : ${VECTORIZATION_ENABLE_SVML}")
message("  Sleef                : ${VECTORIZATION_ENABLE_SLEEF}")
message("  Accelerate (vForce)  : ${VECTORIZATION_ENABLE_ACCELERATE}")
message("  Memory (HAS)         : ${_vec_has_memory}")
message("  Logging (HAS)        : ${_vec_has_logging}")
message("  Cxx standard         : ${VECTORIZATION_CXX_STANDARD}")
message("  Cache                : ${VECTORIZATION_ENABLE_CACHE}")
message("  Cache backend        : ${VECTORIZATION_CACHE_BACKEND}")
message("  Icecc                : ${VECTORIZATION_ENABLE_ICECC}")
quarisma_print_lto_linker_summary(VECTORIZATION)
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
message("")
