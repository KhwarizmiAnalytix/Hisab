# =============================================================================
# Logging Backend Configuration
#
# Declares LOGGING_BACKEND and derives LOGGING_ENABLE_* booleans from it.
# Included by the root CMakeLists.txt *before* ThirdParty so ThirdParty can
# gate loguru/glog on LOGGING_ENABLE_LOGURU / LOGGING_ENABLE_GLOG.
#
# Library/Logging/CMakeLists.txt re-declares the same CACHE var (no-op once
# cached) and uses the derived booleans to configure the Logging target.
# =============================================================================

set(LOGGING_BACKEND "LOGURU"
    CACHE STRING "Logging backend to use. Options are NATIVE, LOGURU, or GLOG")
set_property(CACHE LOGGING_BACKEND PROPERTY STRINGS NATIVE LOGURU GLOG)

if(NOT LOGGING_BACKEND MATCHES "^(NATIVE|LOGURU|GLOG)$")
  message(FATAL_ERROR "LOGGING_BACKEND must be NATIVE, LOGURU, or GLOG (got: ${LOGGING_BACKEND})")
endif()

if(LOGGING_BACKEND STREQUAL "LOGURU")
  set(LOGGING_ENABLE_LOGURU ON)
  set(LOGGING_ENABLE_GLOG   OFF)
  set(LOGGING_ENABLE_NATIVE OFF)
elseif(LOGGING_BACKEND STREQUAL "GLOG")
  set(LOGGING_ENABLE_LOGURU OFF)
  set(LOGGING_ENABLE_GLOG   ON)
  set(LOGGING_ENABLE_NATIVE OFF)
elseif(LOGGING_BACKEND STREQUAL "NATIVE")
  set(LOGGING_ENABLE_LOGURU OFF)
  set(LOGGING_ENABLE_GLOG   OFF)
  set(LOGGING_ENABLE_NATIVE ON)
endif()

message(STATUS "Logging backend: ${LOGGING_BACKEND}")
