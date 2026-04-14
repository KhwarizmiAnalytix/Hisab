#=============================================================================
# Quarisma Sanitizer
# Configuration Module

# This module configures compiler sanitizers for runtime error detection. Supports AddressSanitizer,
# UndefinedBehaviorSanitizer, ThreadSanitizer, MemorySanitizer, and LeakSanitizer (Clang/GCC only).
# Adapted from smtk (https://gitlab.kitware.com/cmb/smtk)

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Sanitizer Support Flag Controls whether compiler sanitizers are enabled for runtime error
# detection. When enabled, instruments code to detect memory errors, undefined behavior, and data
# races. Clang and GCC only. Automatically disabled when using mimalloc.
option(PROJECT_ENABLE_SANITIZER "Build with sanitizer support (Clang only)" OFF)
mark_as_advanced(PROJECT_ENABLE_SANITIZER)

if(PROJECT_ENABLE_SANITIZER)
  message(STATUS "Sanitizer enabled")
else()
  message(STATUS "Sanitizer disabled")
  return()
endif()

# Sanitizer Type Configuration Specifies which sanitizer to use: address, undefined, thread, memory,
# or leak.
set(PROJECT_SANITIZER_TYPE "address"
    CACHE STRING "The sanitizer to use. Options are address, undefined, thread, memory, leak"
)
set_property(CACHE PROJECT_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak)
mark_as_advanced(PROJECT_SANITIZER_TYPE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
  if(PROJECT_ENABLE_SANITIZER)
    # Use PROJECT_SANITIZER_TYPE instead of hardcoded "address"
    set(PROJECT_SANITIZER "${PROJECT_SANITIZER_TYPE}" CACHE STRING "The sanitizer to use")
    mark_as_advanced(PROJECT_SANITIZER)

    if(UNIX AND NOT APPLE)
      # Tests using external binaries need additional help to load the ASan runtime when in use.
      if(PROJECT_SANITIZER STREQUAL "address" OR PROJECT_SANITIZER STREQUAL "undefined")
        find_library(PROJECT_ASAN_LIBRARY NAMES libasan.so.6 libasan.so.5 DOC "ASan library")
        mark_as_advanced(PROJECT_ASAN_LIBRARY)

        set(_quarisma_testing_ld_preload "${PROJECT_ASAN_LIBRARY}")
      endif()
    endif()

    set(quarisma_sanitize_args "-fsanitize=${PROJECT_SANITIZER}")

    if(CMAKE_COMPILER_IS_CLANGXX)
      configure_file(
        "${PROJECT_SOURCE_DIR}/Scripts/suppressions/sanitizer_ignore.txt.in"
        "${PROJECT_BINARY_DIR}/sanitizer_ignore.txt" @ONLY
      )
      list(APPEND quarisma_sanitize_args
           "SHELL:-fsanitize-blacklist=${PROJECT_BINARY_DIR}/sanitizer_ignore.txt"
      )
    endif()

    target_compile_options(quarismabuild INTERFACE "$<BUILD_INTERFACE:${quarisma_sanitize_args}>")
    target_link_options(quarismabuild INTERFACE "$<BUILD_INTERFACE:${quarisma_sanitize_args}>")
  endif()
endif()

if(PROJECT_ENABLE_SANITIZER)
  set(PROJECT_ENABLE_MIMALLOC OFF)
  # Disable LTO for sanitizer builds to avoid linker crashes with Clang
  set(PROJECT_ENABLE_LTO OFF)
endif()
