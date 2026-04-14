# ============================================================================= Quarisma Valgrind
# Memory Checking Configuration Module
# =============================================================================
# This module configures Valgrind memory checking for CTest. All Valgrind options and settings are
# centralized here. Provides comprehensive memory leak detection and error tracking.
# =============================================================================

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Valgrind Memory Checking Flag Controls whether Valgrind memory checking is enabled for test
# execution. When enabled, runs all tests under Valgrind with comprehensive memory analysis.
# Automatically adjusts test timeouts to account for Valgrind overhead.
option(PROJECT_ENABLE_VALGRIND "Execute test suite with Valgrind" OFF)
mark_as_advanced(PROJECT_ENABLE_VALGRIND)

if(NOT PROJECT_ENABLE_VALGRIND)
  return()
endif()

message(STATUS "Configuring Valgrind memory checking...")

# ============================================================================= Platform Detection
# and Compatibility Checks
# =============================================================================

# Detect platform architecture
cmake_host_system_information(RESULT PLATFORM_ARCH QUERY OS_PLATFORM)
message(STATUS "Platform architecture: ${PLATFORM_ARCH}")

# Check for Apple Silicon (ARM64) - Valgrind not supported
if(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
  message(WARNING "Valgrind does not support Apple Silicon (ARM64) architecture")
  message(WARNING "Consider using sanitizers instead:")
  message(
    WARNING "  AddressSanitizer: -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=address"
  )
  message(WARNING "  LeakSanitizer:    -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=leak")
  message(
    WARNING "Continuing with Valgrind configuration (will fail if Valgrind is not installed)..."
  )
endif()

# ============================================================================= Find Valgrind
# Executable
# =============================================================================

find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)

if(NOT CMAKE_MEMORYCHECK_COMMAND)
  message(
    FATAL_ERROR "Valgrind not found! Please install Valgrind or disable PROJECT_ENABLE_VALGRIND"
  )
endif()

message(STATUS "Found Valgrind: ${CMAKE_MEMORYCHECK_COMMAND}")

# Get Valgrind version
execute_process(
  COMMAND ${CMAKE_MEMORYCHECK_COMMAND} --version OUTPUT_VARIABLE VALGRIND_VERSION_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Valgrind version: ${VALGRIND_VERSION_OUTPUT}")

# ============================================================================= Valgrind Timeout
# Configuration
# =============================================================================
# Tests run significantly slower under Valgrind (typically 10-50x slower). We need to increase
# timeouts to prevent false failures.
# =============================================================================

# Default test timeout multiplier for Valgrind (20x slower is a safe estimate)
set(PROJECT_VALGRIND_TIMEOUT_MULTIPLIER 20
    CACHE STRING "Timeout multiplier for tests running under Valgrind"
)

# Global CTest timeout for the entire test suite (30 minutes)
set(CTEST_TEST_TIMEOUT 1800 CACHE STRING
                                  "Global timeout in seconds for CTest when running with Valgrind"
)

message(STATUS "Valgrind timeout multiplier: ${PROJECT_VALGRIND_TIMEOUT_MULTIPLIER}x")
message(STATUS "Global CTest timeout: ${CTEST_TEST_TIMEOUT} seconds")

# ============================================================================= Valgrind Command
# Options
# =============================================================================
# All Valgrind configuration options are defined here. These options provide comprehensive memory
# checking and detailed reporting.
# =============================================================================

set(CMAKE_MEMORYCHECK_COMMAND_OPTIONS
    # Core memcheck tool
    "--tool=memcheck"
    # Leak detection settings
    "--leak-check=full" "--show-leak-kinds=all" "--show-reachable=yes"
    # Error tracking
    "--track-origins=yes" "--track-fds=yes"
    # Output verbosity and detail
    "--verbose" "--num-callers=50"
    # Child process handling
    "--trace-children=yes"
    # Exit code on errors (important for CI/CD)
    "--error-exitcode=1"
    # Suppression generation for known false positives
    "--gen-suppressions=all"
    # Log file configuration
    "--log-file=${CMAKE_BINARY_DIR}/Testing/Temporary/valgrind_%p.log"
    # XML output for machine-readable results
    "--xml=yes" "--xml-file=${CMAKE_BINARY_DIR}/Testing/Temporary/valgrind_%p.xml"
)

# ============================================================================= Suppression File
# Configuration
# =============================================================================
# Suppression files allow filtering out known false positives from third-party libraries or system
# libraries.
# =============================================================================

set(CTEST_MEMORYCHECK_SUPPRESSIONS_FILE
    "${PROJECT_SOURCE_DIR}/Scripts/suppressions/valgrind_suppression.txt"
)

if(EXISTS "${CTEST_MEMORYCHECK_SUPPRESSIONS_FILE}")
  message(STATUS "Using Valgrind suppression file: ${CTEST_MEMORYCHECK_SUPPRESSIONS_FILE}")
  list(APPEND CMAKE_MEMORYCHECK_COMMAND_OPTIONS
       "--suppressions=${CTEST_MEMORYCHECK_SUPPRESSIONS_FILE}"
  )
else()
  message(WARNING "Valgrind suppression file not found: ${CTEST_MEMORYCHECK_SUPPRESSIONS_FILE}")
  message(WARNING "Consider creating a suppression file to filter known false positives")
endif()

# ============================================================================= Apply Timeout
# Multiplier to Existing Tests
# =============================================================================
# Automatically increase timeouts for all registered tests when running under Valgrind to prevent
# false timeout failures.
# =============================================================================

# Get all tests that have been registered
get_property(ALL_TESTS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY TESTS)

# Note: This will be applied after all tests are registered via a separate function call in the main
# CMakeLists.txt after enable_testing()

# ============================================================================= Summary and
# Diagnostic Output
# =============================================================================

# Build the complete memcheck command for display
set(memcheck_command "${CMAKE_MEMORYCHECK_COMMAND}")
foreach(opt ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS})
  list(APPEND memcheck_command ${opt})
endforeach()

message(STATUS "Valgrind command: ${memcheck_command}")
message(STATUS "Valgrind configuration complete")
message(STATUS "Use 'ctest -T memcheck' to run tests with Valgrind")

# ============================================================================= Helper Function:
# Apply Valgrind Timeouts to Tests
# =============================================================================
# This function should be called after all tests are registered. It multiplies existing test
# timeouts by PROJECT_VALGRIND_TIMEOUT_MULTIPLIER.
# =============================================================================

function(quarisma_apply_valgrind_timeouts)
  if(NOT PROJECT_ENABLE_VALGRIND)
    return()
  endif()

  # Get all tests in the current directory and subdirectories
  get_property(all_tests DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY TESTS)

  foreach(test_name ${all_tests})
    # Get current timeout
    get_test_property(${test_name} TIMEOUT current_timeout)

    if(current_timeout)
      # Calculate new timeout
      math(EXPR new_timeout "${current_timeout} * ${PROJECT_VALGRIND_TIMEOUT_MULTIPLIER}")

      # Apply new timeout
      set_tests_properties(${test_name} PROPERTIES TIMEOUT ${new_timeout})

      message(STATUS "  ${test_name}: timeout ${current_timeout}s -> ${new_timeout}s")
    else()
      # If no timeout is set, use a default based on global timeout
      set_tests_properties(${test_name} PROPERTIES TIMEOUT ${CTEST_TEST_TIMEOUT})
      message(STATUS "  ${test_name}: timeout not set, using global ${CTEST_TEST_TIMEOUT}s")
    endif()
  endforeach()

  message(STATUS "Applied Valgrind timeout multiplier to ${CMAKE_MATCH_COUNT} tests")
endfunction()
