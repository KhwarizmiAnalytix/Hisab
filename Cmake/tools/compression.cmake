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
if(CORE_ENABLE_COMPRESSION)
  if(CORE_COMPRESSION_TYPE STREQUAL "SNAPPY")
    set(PROJECT_COMPRESSION_TYPE_SNAPPY ON)
    message(STATUS "Compression enabled: Snappy")
  elseif(CORE_COMPRESSION_TYPE STREQUAL "NONE")
    set(CORE_ENABLE_COMPRESSION OFF)
    message(STATUS "Compression type set to NONE - disabling compression")
  else()
    message(
      FATAL_ERROR
        "Invalid CORE_COMPRESSION_TYPE: ${CORE_COMPRESSION_TYPE}. Valid options are: NONE, SNAPPY"
    )
  endif()
else()
  message(STATUS "Compression disabled")
endif()
