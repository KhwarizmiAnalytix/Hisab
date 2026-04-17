include_guard(GLOBAL)

# Compression Support Flag Controls whether compression support is enabled for data serialization.
# When enabled, allows selection of compression library (snappy or none).
option(CORE_ENABLE_COMPRESSION "Enable compression support" OFF)

# Compression Library Selection Specifies which compression library to use: none or snappy.
set(CORE_COMPRESSION_TYPE "none" CACHE STRING
                                         "Compression library to use. Options are  none, snappy"
)
set_property(CACHE CORE_COMPRESSION_TYPE PROPERTY STRINGS none snappy)
mark_as_advanced(CORE_ENABLE_COMPRESSION CORE_COMPRESSION_TYPE)

# Compression configuration validation and setup
# CORE_COMPRESSION_TYPE cache STRINGS are lowercase (none, snappy); compare case-insensitively.
set(CORE_COMPRESSION_TYPE_SNAPPY OFF)
if(CORE_ENABLE_COMPRESSION)
  string(TOUPPER "${CORE_COMPRESSION_TYPE}" _core_compression_type_uc)
  if(_core_compression_type_uc STREQUAL "SNAPPY")
    set(PROJECT_COMPRESSION_TYPE_SNAPPY ON)
    set(CORE_COMPRESSION_TYPE_SNAPPY ON)
    message(STATUS "Compression enabled: Snappy")
  elseif(_core_compression_type_uc STREQUAL "NONE")
    set(CORE_ENABLE_COMPRESSION OFF)
    message(STATUS "Compression type set to NONE - disabling compression")
  else()
    message(
      FATAL_ERROR
        "Invalid CORE_COMPRESSION_TYPE: ${CORE_COMPRESSION_TYPE}. Valid options are: none, snappy (case-insensitive)"
    )
  endif()
else()
  message(STATUS "Compression disabled")
endif()
