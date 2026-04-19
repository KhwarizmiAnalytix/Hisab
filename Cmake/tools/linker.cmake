# ============================================================================= Quarisma Faster
# Linker Configuration Module

# Configures faster linker selection for improved build performance. Supports mold, lld, gold, and
# lld-link linkers across different platforms.
#
# NOTE: Linker flags are applied directly to the named project target via target_link_options,
# keeping third-party dependencies unaffected by Quarisma's linker choices.

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# quarisma_find_linker(<linker_choice> <target_name>)
#   linker_choice: "default" (auto-detect) | "lld" | "mold" | "gold" | "lld-link"
#   target_name:   CMake target to receive the -fuse-ld= link option (e.g. Logging, Core, …)
function(quarisma_find_linker linker_choice target_name)
  set(LINKER_CHOICE "${linker_choice}")

  set(LINKER_FOUND FALSE)
  set(LINKER_NAME "")
  set(LINKER_FLAGS)

  # Skip faster linker selection if LTO is enabled — LTO with faster linkers (especially gold) can
  # cause out-of-memory errors
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message("LTO is enabled - skipping faster linker configuration to avoid memory issues")
    return()
  endif()

  # Explicit override: honour the caller's XXX_LINKER_CHOICE when not "default"
  if(NOT LINKER_CHOICE STREQUAL "default")
    find_program(
      _EXPLICIT_LINKER NAMES "${LINKER_CHOICE}" "ld.${LINKER_CHOICE}" "${LINKER_CHOICE}-link"
    )
    if(_EXPLICIT_LINKER)
      set(LINKER_NAME "${_EXPLICIT_LINKER}")
      set(LINKER_FOUND TRUE)
      message("Using explicitly requested linker '${LINKER_CHOICE}': ${_EXPLICIT_LINKER}")
    else()
      message(WARNING "Requested linker '${LINKER_CHOICE}' not found — falling back to auto-detect")
    endif()
    unset(_EXPLICIT_LINKER CACHE)
  endif()

  # Auto-detect when still not resolved
  if(NOT LINKER_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Linux + Clang: prefer mold, then ld.gold (more compatible), then ld.lld
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)
        find_program(LD_LLD_LINKER ld.lld)

        if(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Linux/Clang: Using mold linker for faster linking")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Linux/Clang: Using ld.gold linker for faster linking (more compatible)")
        elseif(LD_LLD_LINKER)
          set(LINKER_NAME "${LD_LLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Linux/Clang: Using ld.lld linker for faster linking")
        endif()
      elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Linux + GCC: prefer mold or gold
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)

        if(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Linux/GCC: Using mold linker for faster linking")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Linux/GCC: Using gold linker for faster linking")
        endif()
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # macOS: use system linker or ld64.lld if available (not generic lld)
      find_program(LD64_LLD_LINKER ld64.lld)
      if(LD64_LLD_LINKER)
        set(LINKER_NAME "${LD64_LLD_LINKER}")
        set(LINKER_FOUND TRUE)
        message("macOS: Using ld64.lld linker for faster linking")
      else()
        message("macOS: Using system linker (ld64.lld not found)")
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER MATCHES "msys64"
         AND NOT CMAKE_CXX_COMPILER MATCHES "mingw"
      )
        find_program(LLD_LINK_LINKER lld-link)
        if(LLD_LINK_LINKER)
          set(LINKER_NAME "${LLD_LINK_LINKER}")
          set(LINKER_FOUND TRUE)
          message("Windows/Clang: Using lld-link linker for faster linking")
        else()
          message("Windows/Clang: lld-link not found - using default linker")
        endif()
      elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message("Windows/MSVC: Using default linker with optimizations")
      endif()
    endif()
  endif()

  # Apply linker flags to the named project target
  if(LINKER_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(CMAKE_LINKER "${LINKER_NAME}" CACHE FILEPATH "Linker executable")
      message("Linker configured: ${LINKER_NAME}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      get_filename_component(LINKER_BASENAME "${LINKER_NAME}" NAME)
      string(REPLACE "ld." "" LINKER_SHORTNAME "${LINKER_BASENAME}")
      set(LINKER_FLAGS "-fuse-ld=${LINKER_SHORTNAME}")

      if(TARGET "${target_name}")
        target_link_options("${target_name}" PUBLIC ${LINKER_FLAGS})
        message("Applied linker flag to ${target_name}: ${LINKER_FLAGS}")
      else()
        message(WARNING "Target '${target_name}' not found - linker flag will not be applied")
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND LINKER_NAME MATCHES "lld-link")
      set(CMAKE_LINKER "${LINKER_NAME}" CACHE FILEPATH "Linker executable")
      message("Linker configured: ${LINKER_NAME}")
    endif()
  else()
    message("No faster linker found - using default linker")
  endif()
endfunction()
