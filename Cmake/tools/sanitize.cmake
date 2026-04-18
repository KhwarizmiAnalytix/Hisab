#=============================================================================
# Quarisma Sanitizer Configuration Module
#
# Configures per-module compiler sanitizers for runtime error detection.
# Supports AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer,
# MemorySanitizer, and LeakSanitizer (Clang/GCC only).
# Adapted from smtk (https://gitlab.kitware.com/cmb/smtk)

include_guard(GLOBAL)

# =============================================================================
# Per-module sanitizer flags
#
# Each module controls its own sanitizer independently.
# Forward-declared here (sanitize.cmake is included in root before any
# add_subdirectory) so flags are in cache before module CMakeLists.txt run.
# Each module's CMakeLists.txt may re-declare the same option() as a no-op.
# =============================================================================
foreach(_mod LOGGING MEMORY CORE PARALLEL PROFILER)
  option(${_mod}_ENABLE_SANITIZER
    "Build ${_mod} with sanitizer support (Clang/GCC only)" OFF)
  mark_as_advanced(${_mod}_ENABLE_SANITIZER)

  set(${_mod}_SANITIZER_TYPE "address"
      CACHE STRING
      "Sanitizer type for ${_mod}. Options: address, undefined, thread, memory, leak")
  set_property(CACHE ${_mod}_SANITIZER_TYPE
    PROPERTY STRINGS address undefined thread memory leak)
  mark_as_advanced(${_mod}_SANITIZER_TYPE)
endforeach()
unset(_mod)

# =============================================================================
# Early exit if no module has sanitizer enabled
# =============================================================================
set(_quarisma_any_sanitizer OFF)
foreach(_mod LOGGING MEMORY CORE PARALLEL PROFILER)
  if(${_mod}_ENABLE_SANITIZER)
    set(_quarisma_any_sanitizer ON)
    break()
  endif()
endforeach()
unset(_mod)

if(NOT _quarisma_any_sanitizer)
  unset(_quarisma_any_sanitizer)
  return()
endif()
unset(_quarisma_any_sanitizer)

# =============================================================================
# Compiler detection
# =============================================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(NOT CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_COMPILER_IS_CLANGXX)
  message(WARNING "Sanitizers require GCC or Clang; no sanitizer flags applied")
  return()
endif()

# =============================================================================
# Per-module flag application
#
# Flags are applied to each module's interface build target (loggingbuild,
# memorybuild, …) so only the affected module is instrumented.
# =============================================================================
foreach(_mod LOGGING MEMORY CORE PARALLEL PROFILER)
  if(NOT ${_mod}_ENABLE_SANITIZER)
    continue()
  endif()

  string(TOLOWER "${_mod}" _mod_lower)
  set(_build_target "${_mod_lower}build")
  set(_san_type "${${_mod}_SANITIZER_TYPE}")

  message(STATUS "${_mod}: sanitizer enabled (${_san_type})")

  # Linux: tests using external binaries need help loading the ASan runtime.
  if(UNIX AND NOT APPLE)
    if(_san_type STREQUAL "address" OR _san_type STREQUAL "undefined")
      find_library(${_mod}_ASAN_LIBRARY
        NAMES libasan.so.6 libasan.so.5
        DOC "ASan runtime library for ${_mod}")
      mark_as_advanced(${_mod}_ASAN_LIBRARY)
      set(_quarisma_testing_ld_preload "${${_mod}_ASAN_LIBRARY}")
    endif()
  endif()

  set(_san_args
    -O1 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls
    "-fsanitize=${_san_type}"
  )

  if(CMAKE_COMPILER_IS_CLANGXX)
    # Generate the suppression list once (idempotent if called for multiple modules).
    if(NOT _quarisma_sanitizer_blacklist_configured)
      configure_file(
        "${PROJECT_SOURCE_DIR}/Scripts/suppressions/sanitizer_ignore.txt.in"
        "${PROJECT_BINARY_DIR}/sanitizer_ignore.txt" @ONLY
      )
      set(_quarisma_sanitizer_blacklist_configured TRUE)
    endif()
    list(APPEND _san_args
      "SHELL:-fsanitize-blacklist=${PROJECT_BINARY_DIR}/sanitizer_ignore.txt")
  endif()

  target_compile_options(${_build_target} INTERFACE
    "$<BUILD_INTERFACE:${_san_args}>")
  target_link_options(${_build_target} INTERFACE
    "$<BUILD_INTERFACE:-fsanitize=${_san_type}>")

  # mimalloc intercepts malloc/free and conflicts with AddressSanitizer.
  if(_mod STREQUAL "MEMORY" AND MEMORY_ENABLE_MIMALLOC)
    set(MEMORY_ENABLE_MIMALLOC OFF)
    message(STATUS "Memory: mimalloc disabled (incompatible with sanitizer)")
  endif()

  # LTO and sanitizer instrumentation are mutually incompatible.
  if(${_mod}_ENABLE_LTO)
    set(${_mod}_ENABLE_LTO OFF CACHE BOOL
      "Enable Link Time Optimization for ${_mod}" FORCE)
    message(STATUS "${_mod}: LTO disabled (incompatible with sanitizer)")
  endif()

  unset(_san_args)
  unset(_san_type)
  unset(_mod_lower)
  unset(_build_target)
endforeach()
unset(_mod)
