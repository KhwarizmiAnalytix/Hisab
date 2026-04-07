# ============================================================================= Quarisma Faster Linker
# Configuration Module
# =============================================================================
# Configures faster linker selection for improved build performance. Supports mold, lld, gold, and
# lld-link linkers across different platforms.
#
# NOTE: Linker flags are applied ONLY to the quarismabuild interface target. This ensures that
# third-party dependencies use their default linker settings and are not affected by Quarisma's linker
# choices.
# =============================================================================

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

if(QUARISMA_ENABLE_COVERAGE)
  return()
endif()
# Determine which linker to use based on platform and compiler
set(QUARISMA_LINKER_CHOICE "default" CACHE STRING "Linker to use: default, lld, mold, gold, lld-link")
mark_as_advanced(QUARISMA_LINKER_CHOICE)

set_property(CACHE QUARISMA_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)

function(quarisma_find_linker)
  set(LINKER_FOUND FALSE)
  set(LINKER_NAME "")
  set(LINKER_FLAGS)

  # Skip faster linker selection if LTO is enabled LTO with faster linkers (especially gold) can
  # cause out-of-memory errors
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    message("LTO is enabled - skipping faster linker configuration to avoid memory issues")
    return()
  endif()

  # Platform and compiler specific linker selection
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
        # Use ld.gold as it's more compatible across LLVM versions
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
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       AND NOT CMAKE_CXX_COMPILER MATCHES "msys64"
       AND NOT CMAKE_CXX_COMPILER MATCHES "mingw")
      # Windows + native LLVM Clang: prefer lld-link.
      # MSYS2/MinGW Clang uses GNU ABI and its own linker; skip lld-link for it.
      find_program(LLD_LINK_LINKER lld-link)
      if(LLD_LINK_LINKER)
        set(LINKER_NAME "${LLD_LINK_LINKER}")
        set(LINKER_FOUND TRUE)
        message("Windows/Clang: Using lld-link linker for faster linking")
      else()
        message("Windows/Clang: lld-link not found - using default linker")
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      # Windows + MSVC: use default linker with optimizations
      message("Windows/MSVC: Using default linker with optimizations")
    endif()
  endif()

  # Apply linker flags to quarismabuild target if found
  if(LINKER_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      # Windows + Clang: lld-link does not understand -fuse-ld=; set the linker directly.
      set(CMAKE_LINKER "${LINKER_NAME}" CACHE FILEPATH "Linker executable")
      message("Linker configured: ${LINKER_NAME}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      # For Clang and GCC on non-Windows, use -fuse-ld flag. Extract just the linker name
      # (e.g., "mold" from "/usr/bin/mold")
      get_filename_component(LINKER_BASENAME "${LINKER_NAME}" NAME)

      # Convert linker names to -fuse-ld compatible format ld.gold -> gold, ld.lld -> lld, mold ->
      # mold
      string(REPLACE "ld." "" LINKER_SHORTNAME "${LINKER_BASENAME}")
      set(LINKER_FLAGS "-fuse-ld=${LINKER_SHORTNAME}")

      if(TARGET quarismabuild)
        target_link_options(quarismabuild INTERFACE ${LINKER_FLAGS})
        message("Applied linker flag to quarismabuild: ${LINKER_FLAGS}")
      else()
        message(WARNING "quarismabuild target not found - linker flag will not be applied")
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND LINKER_NAME MATCHES "lld-link")
      # For MSVC with lld-link, set the linker directly
      set(CMAKE_LINKER "${LINKER_NAME}" CACHE FILEPATH "Linker executable")
      message("Linker configured: ${LINKER_NAME}")
    endif()
  else()
    message("No faster linker found - using default linker")
  endif()
endfunction()

# Call the linker detection function
quarisma_find_linker()

# if(QUARISMA_ENABLE_GOLD_LINKER) if(USE_DISTRIBUTED AND USE_MPI) # Same issue as here with default
# MPI on Ubuntu # https://bugs.launchpad.net/ubuntu/+source/deal.ii/+bug/1841577 message(WARNING
# "Refusing to use gold when USE_MPI=1") else() execute_process( COMMAND "${CMAKE_C_COMPILER}"
# -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION) if(NOT "${LD_VERSION}" MATCHES
# "GNU gold") message( WARNING "USE_GOLD_LINKER was set but ld.gold isn't available, turning it off"
# ) set(USE_GOLD_LINKER OFF) else() message("ld.gold is available, using it to link")
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
# set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=gold") endif() endif()
# endif()
# ============================================================================
# Summary
# ============================================================================

message("  - Faster linker: Automatically detected and applied to quarismabuild target")
message("  - Third-party dependencies: Not affected by linker configuration")
