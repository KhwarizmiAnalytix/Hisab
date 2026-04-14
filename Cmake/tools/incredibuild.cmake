#=============================================================================
# Quarisma IncrediBuild Distributed Compilation Configuration Module
# Enables IncrediBuild distributed compilation for Windows builds.
# IncrediBuild is a commercial distributed build acceleration tool that
# distributes compilation tasks across multiple machines.
#
# Platform Support:
#   - Windows: Full support (Coordinator + Agents)
#   - Linux: Agent-only support (requires Windows Coordinator)
#   - macOS: Not supported
#
# NOTE: IncrediBuild and Icecream are mutually exclusive. When IncrediBuild
# is enabled, Icecream will be automatically disabled.

include_guard(GLOBAL)

# IncrediBuild Distributed Compilation Flag
# Controls whether IncrediBuild distributed compilation is enabled.
# Only supported on Windows. On Linux/macOS, this option will be ignored
# with a warning message.
option(PROJECT_ENABLE_INCREDIBUILD "Enable IncrediBuild distributed compilation (Windows only)" OFF)
mark_as_advanced(PROJECT_ENABLE_INCREDIBUILD)

if(NOT PROJECT_ENABLE_INCREDIBUILD)
  return()
endif()

# Platform validation: IncrediBuild is Windows-only
if(NOT WIN32)
  message(FATAL_ERROR "IncrediBuild is only supported on Windows. "
    "Current platform: ${CMAKE_SYSTEM_NAME}. "
    "Please disable PROJECT_ENABLE_INCREDIBUILD on non-Windows platforms.")
endif()

# Find XGE executable (IncrediBuild's command-line tool)
find_program(INCREDIBUILD_XGE xge.exe)

if(NOT INCREDIBUILD_XGE)
  message(FATAL_ERROR "IncrediBuild XGE executable (xge.exe) not found. "
    "Please ensure IncrediBuild is installed and xge.exe is in your PATH. "
    "You can download IncrediBuild from: https://www.incredibuild.com/")
endif()

message(STATUS "IncrediBuild XGE found: ${INCREDIBUILD_XGE}")

# Disable Icecream if it was enabled (conflict detection)
if(PROJECT_ENABLE_ICECC)
  message(STATUS "Disabling Icecream (conflicts with IncrediBuild)")
  set(PROJECT_ENABLE_ICECC OFF CACHE BOOL "Use Icecream distributed compilation" FORCE)
  # Clear any Icecream compiler launcher that may have been set
  unset(CMAKE_C_COMPILER_LAUNCHER)
  unset(CMAKE_CXX_COMPILER_LAUNCHER)
endif()

# Configure compiler launchers to use IncrediBuild XGE
# XGE will intercept compilation commands and distribute them across agents
set(CMAKE_C_COMPILER_LAUNCHER "${INCREDIBUILD_XGE}" CACHE STRING "C compiler launcher")
set(CMAKE_CXX_COMPILER_LAUNCHER "${INCREDIBUILD_XGE}" CACHE STRING "CXX compiler launcher")

# Also set CUDA compiler launcher if CUDA is enabled
if(MEMORY_ENABLE_CUDA)
  set(CMAKE_CUDA_COMPILER_LAUNCHER "${INCREDIBUILD_XGE}" CACHE STRING "CUDA compiler launcher")
  message(STATUS "IncrediBuild enabled for CUDA compilation (experimental)")
endif()

message(STATUS "IncrediBuild distributed compilation enabled")
message(STATUS "  Coordinator: Auto-detect (local or network)")
message(STATUS "  Agents: Will be discovered automatically")
message(STATUS "  Note: Ensure IncrediBuild Coordinator is running before building")

