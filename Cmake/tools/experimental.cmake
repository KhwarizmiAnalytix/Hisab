#=============================================================================
# Quarisma Experimental
# Features Configuration Module

# This module configures experimental features for the Quarisma project. Experimental features
# are under development and may not be stable or fully tested. They are disabled by default
# and should only be enabled for development and testing purposes.

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Experimental Features Support Flag Controls whether experimental features are enabled.
# When enabled, provides access to features that are under active development and may
# change or be removed in future releases. Use with caution in production environments.
option(CORE_ENABLE_EXPERIMENTAL "Enable experimental features (use with caution)" OFF)
mark_as_advanced(CORE_ENABLE_EXPERIMENTAL)

# Only proceed if experimental features are enabled
if(NOT CORE_ENABLE_EXPERIMENTAL)
  message(WARNING "Experimental features are disabled (CORE_ENABLE_EXPERIMENTAL=OFF)")
  return()
endif()

message(STATUS "Configuring experimental features...")

#=============================================================================
# Experimental
# Features Configuration

# Set flag to indicate experimental features are available
set(PROJECT_EXPERIMENTAL_FOUND TRUE CACHE BOOL "Experimental features are enabled" FORCE)

message(WARNING "✅ Experimental features enabled")
message(WARNING "   WARNING: Experimental features may be unstable or incomplete")
message(WARNING "   WARNING: API and behavior may change without notice")
message(WARNING "   WARNING: Not recommended for production use")
message(WARNING "Experimental features configuration complete")

