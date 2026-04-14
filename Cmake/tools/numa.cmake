#=============================================================================
# Quarisma NUMA
# (Non-Uniform Memory Access) Configuration Module

# This module configures NUMA support for multi-socket systems. It enables memory-aware thread
# scheduling and allocation on NUMA architectures.

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# NUMA Support Flag Controls whether NUMA (Non-Uniform Memory Access) support is enabled. When
# enabled on Unix systems, provides memory-aware scheduling for multi-socket systems. Automatically
# disabled on non-Unix platforms (Windows, macOS).
option(MEMORY_ENABLE_NUMA "Enable numa node" OFF)
mark_as_advanced(MEMORY_ENABLE_NUMA)

if(NOT MEMORY_ENABLE_NUMA)
  return()
endif()

if(UNIX)
  find_package(Numa)
  if(NUMA_FOUND)
    include_directories(SYSTEM ${Numa_INCLUDE_DIR})
    list(APPEND PROJECT_DEPENDENCY_LIBS "${Numa_LIBRARIES}")
  else()
    message(WARNING "Not compiling with NUMA. Suppress this warning with -DPROJECT_ENABLE_NUMA=OFF")
  endif()
endif()
