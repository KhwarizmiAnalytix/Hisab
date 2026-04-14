# ============================================================================= Quarisma Build Type
# Configuration Module
# =============================================================================
# This module optimizes build configurations for maximum performance and minimal CMake
# reconfiguration overhead through aggressive caching.
#
# Performance Optimizations: - Cached compiler flag validation to avoid repeated checks - Build
# type-specific optimization flags for maximum runtime performance - Efficient MSVC runtime library
# selection - Assertion and debug symbol management per build type
# =============================================================================

# Guard against multiple inclusions for performance
if(PROJECT_BUILD_TYPE_CONFIGURED)
  return()
endif()
set(PROJECT_BUILD_TYPE_CONFIGURED TRUE CACHE INTERNAL "Build type module loaded")

# Set default build type with caching for performance
if(NOT CMAKE_BUILD_TYPE)
  message("CMAKE_BUILD_TYPE not set - defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(
    CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
  )
endif()

# Cache build type for performance optimization
set(PROJECT_CACHED_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE INTERNAL "Cached build type")
