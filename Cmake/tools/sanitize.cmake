# =============================================================================
# Quarisma
# Sanitizer Configuration Module
#
# Configures per-module compiler sanitizers for runtime error detection. Supports AddressSanitizer,
# UndefinedBehaviorSanitizer, ThreadSanitizer, MemorySanitizer, and LeakSanitizer (Clang/GCC only).
# Adapted from smtk (https://gitlab.kitware.com/cmb/smtk)
#
# Usage in each module's CMakeLists.txt:
#
# option(XXX_ENABLE_SANITIZER "..." OFF) set(XXX_SANITIZER_TYPE "address" CACHE STRING "...")
# set_property(CACHE XXX_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak)
# mark_as_advanced(XXX_ENABLE_SANITIZER XXX_SANITIZER_TYPE) ... include(sanitize)
# quarisma_configure_sanitizer(XXX)

include_guard(GLOBAL)

# quarisma_configure_sanitizer(module_name)
#
# Applies sanitizer flags for the given module using that module's XXX_ENABLE_SANITIZER and
# XXX_SANITIZER_TYPE cache variables. Adding or removing a module requires no change to this file.
function(quarisma_configure_sanitizer module_name)
  # Compiler check
  set(_is_clang FALSE)
  set(_is_gnu FALSE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    set(_is_clang TRUE)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(_is_gnu TRUE)
  endif()

  if(NOT _is_clang AND NOT _is_gnu)
    message(WARNING "${module_name}: Sanitizers require GCC or Clang — no flags applied")
    return()
  endif()

  set(_san_type "${${module_name}_SANITIZER_TYPE}")

  message(STATUS "${module_name}: sanitizer enabled (${_san_type})")

  # Linux: tests that launch external binaries may need the ASan runtime preloaded
  if(UNIX AND NOT APPLE)
    if(_san_type STREQUAL "address" OR _san_type STREQUAL "undefined")
      find_library(
        ${module_name}_ASAN_LIBRARY NAMES libasan.so.6 libasan.so.5
        DOC "ASan runtime library for ${module_name}"
      )
      mark_as_advanced(${module_name}_ASAN_LIBRARY)
    endif()
  endif()

  set(_san_compile_flags -O1 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls
                         "-fsanitize=${_san_type}"
  )

  if(_is_clang)
    # Generate the suppression list once (idempotent across multiple module calls)
    if(NOT _quarisma_sanitizer_blacklist_configured)
      configure_file(
        "${PROJECT_SOURCE_DIR}/Scripts/suppressions/sanitizer_ignore.txt.in"
        "${PROJECT_BINARY_DIR}/sanitizer_ignore.txt" @ONLY
      )
      set(_quarisma_sanitizer_blacklist_configured TRUE
          CACHE INTERNAL "Sanitizer blacklist already configured"
      )
    endif()
    list(APPEND _san_compile_flags "-fsanitize-blacklist=${PROJECT_BINARY_DIR}/sanitizer_ignore.txt")
  endif()

  list(JOIN _san_compile_flags " " _san_compile_str)

  string(APPEND CMAKE_C_FLAGS " ${_san_compile_str}")
  string(APPEND CMAKE_CXX_FLAGS " ${_san_compile_str}")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fsanitize=${_san_type}")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fsanitize=${_san_type}")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
endfunction()
