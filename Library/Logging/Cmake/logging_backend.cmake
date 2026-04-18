# ============================================================================= Logging Backend
# Configuration
#
# Declares LOGGING_BACKEND and derives LOGGING_ENABLE_* booleans from it. Included by the root
# CMakeLists.txt *before* ThirdParty so ThirdParty can gate loguru/glog on LOGGING_ENABLE_LOGURU /
# LOGGING_ENABLE_GLOG. Also runs the early pass of Library/Profiler/CMakeLists.txt so Kineto gating
# sees PROFILER_ENABLE_KINETO.
#
# Library/Logging/CMakeLists.txt re-declares the same CACHE var (no-op once cached) and uses the
# derived booleans to configure the Logging target.
#
# Compiler / platform modules run here (and from each Library/*/CMakeLists.txt; include_guard skips
# repeats) so add_subdirectory(ThirdParty) sees global flags. Root CMakeLists no longer includes
# these directly.
# =============================================================================

include(compiler_checks)
include(cache)
include(build_type)
include(checks)
include(utils)
include(platform)

set(LOGGING_BACKEND "LOGURU" CACHE STRING
                                   "Logging backend to use. Options are NATIVE, LOGURU, or GLOG"
)
set_property(CACHE LOGGING_BACKEND PROPERTY STRINGS NATIVE LOGURU GLOG)

if(NOT LOGGING_BACKEND MATCHES "^(NATIVE|LOGURU|GLOG)$")
  message(FATAL_ERROR "LOGGING_BACKEND must be NATIVE, LOGURU, or GLOG (got: ${LOGGING_BACKEND})")
endif()

if(LOGGING_BACKEND STREQUAL "LOGURU")
  set(LOGGING_ENABLE_LOGURU ON)
  set(LOGGING_ENABLE_GLOG OFF)
  set(LOGGING_ENABLE_NATIVE OFF)
elseif(LOGGING_BACKEND STREQUAL "GLOG")
  set(LOGGING_ENABLE_LOGURU OFF)
  set(LOGGING_ENABLE_GLOG ON)
  set(LOGGING_ENABLE_NATIVE OFF)
elseif(LOGGING_BACKEND STREQUAL "NATIVE")
  set(LOGGING_ENABLE_LOGURU OFF)
  set(LOGGING_ENABLE_GLOG OFF)
  set(LOGGING_ENABLE_NATIVE ON)
endif()

message(STATUS "Logging backend: ${LOGGING_BACKEND}")

# Profiler: PROFILER_BACKEND / PROFILER_ENABLE_KINETO must exist before ThirdParty (Kineto gate).
# Library/Profiler/CMakeLists.txt handles this via an early include pass (see
# PROFILER_INCLUDE_GATE_ONLY).
set(PROFILER_INCLUDE_GATE_ONLY ON)
include("${CMAKE_SOURCE_DIR}/Library/Profiler/CMakeLists.txt")
unset(PROFILER_INCLUDE_GATE_ONLY)

# compile_definitions.cmake sets PROJECT_DEPENDENCY_COMPILE_DEFINITIONS in the root scope so every
# Library/* subdirectory inherits the same baseline flags.
include(compile_definitions)
