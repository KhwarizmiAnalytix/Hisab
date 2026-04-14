# ============================================================================= Quarisma System
# Validation and Checks Module
# =============================================================================
# This module performs efficient system validation with aggressive caching to minimize CMake
# reconfiguration overhead while ensuring all required dependencies and capabilities are available.
#
# Performance Optimizations: - Cached validation results to avoid redundant checks - Fast platform
# detection with cached results - Efficient compiler capability validation - Streamlined dependency
# checking
# =============================================================================

# Guard against multiple inclusions for performance
if(PROJECT_CHECKS_CONFIGURED)
  return()
endif()
set(PROJECT_CHECKS_CONFIGURED TRUE CACHE INTERNAL "Checks module loaded")

# Create cache key for validation results
set(PROJECT_VALIDATION_CACHE_KEY
    "${CMAKE_CXX_COMPILER_ID}_${CMAKE_CXX_COMPILER_VERSION}_${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_VERSION}"
    CACHE INTERNAL "Validation cache key"
)

# Check if validation has already been completed for this configuration
if(PROJECT_VALIDATION_COMPLETED STREQUAL PROJECT_VALIDATION_CACHE_KEY)
  message(STATUS "Quarisma: Using cached validation results")
  return()
endif()

message(STATUS "Quarisma: Performing system validation...")

# ============================================================================= Platform Detection
# with Caching
# =============================================================================

# Fast platform detection with cached results
if(NOT DEFINED PROJECT_PLATFORM_DETECTED)
  if(WIN32)
    set(PROJECT_PLATFORM "Windows" CACHE INTERNAL "Detected platform")
    set(PROJECT_PLATFORM_WINDOWS TRUE CACHE INTERNAL "Windows platform detected")
  elseif(APPLE)
    set(PROJECT_PLATFORM "macOS" CACHE INTERNAL "Detected platform")
    set(PROJECT_PLATFORM_MACOS TRUE CACHE INTERNAL "macOS platform detected")
  elseif(UNIX)
    set(PROJECT_PLATFORM "Linux" CACHE INTERNAL "Detected platform")
    set(PROJECT_PLATFORM_LINUX TRUE CACHE INTERNAL "Linux platform detected")
  else()
    message(WARNING "Quarisma: Unknown platform detected")
    set(PROJECT_PLATFORM "Unknown" CACHE INTERNAL "Detected platform")
  endif()
  set(PROJECT_PLATFORM_DETECTED TRUE CACHE INTERNAL "Platform detection completed")
  message(STATUS "Quarisma: Platform detected: ${PROJECT_PLATFORM}")
endif()

# ============================================================================= Compiler Version
# Validation (Updated for Modern Requirements)
# =============================================================================

# Updated minimum compiler versions for C++17 support and modern optimizations
set(PROJECT_MIN_GCC_VERSION "7.0")
set(PROJECT_MIN_CLANG_VERSION "5.0")
set(PROJECT_MIN_APPLE_CLANG_VERSION "9.0")
set(PROJECT_MIN_MSVC_VERSION "19.14") # VS 2017 15.7
set(PROJECT_MIN_INTEL_VERSION "18.0")

set(PROJECT_COMPILER_ID ${CMAKE_CXX_COMPILER_ID} CACHE INTERNAL "Compiler ID")
mark_as_advanced(PROJECT_COMPILER_ID)

# GCC version check
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${PROJECT_MIN_GCC_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires GCC ${PROJECT_MIN_GCC_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(PROJECT_COMPILER_GCC TRUE CACHE INTERNAL "GCC compiler detected")
  message(STATUS "Quarisma: GCC ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(PROJECT_COMPILER_ID "gcc" CACHE INTERNAL "Compiler ID")

  # Clang version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${PROJECT_MIN_CLANG_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Clang ${PROJECT_MIN_CLANG_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(PROJECT_COMPILER_CLANG TRUE CACHE INTERNAL "Clang compiler detected")
  message(STATUS "Quarisma: Clang ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(PROJECT_COMPILER_ID "clang" CACHE INTERNAL "Compiler ID")

  # Apple Clang version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${PROJECT_MIN_APPLE_CLANG_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Apple Clang ${PROJECT_MIN_APPLE_CLANG_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(PROJECT_COMPILER_APPLE_CLANG TRUE CACHE INTERNAL "Apple Clang compiler detected")
  message(STATUS "Quarisma: Apple Clang ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(PROJECT_COMPILER_ID "clang" CACHE INTERNAL "Compiler ID")

  # MSVC version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${PROJECT_MIN_MSVC_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires MSVC ${PROJECT_MIN_MSVC_VERSION} or later (Visual Studio 2017 15.7+) for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(PROJECT_COMPILER_MSVC TRUE CACHE INTERNAL "MSVC compiler detected")
  message(STATUS "Quarisma: MSVC ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(PROJECT_COMPILER_ID "msvc" CACHE INTERNAL "Compiler ID")

  # Intel C++ version check
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${PROJECT_MIN_INTEL_VERSION})
    message(
      FATAL_ERROR
        "Quarisma requires Intel C++ ${PROJECT_MIN_INTEL_VERSION} or later for C++17 support and modern optimizations. Found: ${CMAKE_CXX_COMPILER_VERSION}"
    )
  endif()
  set(PROJECT_COMPILER_INTEL TRUE CACHE INTERNAL "Intel compiler detected")
  message(STATUS "Quarisma: Intel C++ ${CMAKE_CXX_COMPILER_VERSION} validated")
  set(PROJECT_COMPILER_ID "intel" CACHE INTERNAL "Compiler ID")

else()
  message(WARNING "Quarisma: Unknown compiler '${CMAKE_CXX_COMPILER_ID}'. Build may fail.")
endif()

# ============================================================================= C++ Standard
# Validation
# =============================================================================

# Ensure C++17 is properly configured (updated from C++11)
if(NOT PROJECT_IGNORE_CMAKE_CXX_STANDARD_CHECKS)
  # Validate that the requested C++ standard is supported
  if(PROJECT_CXX_STANDARD LESS 11)
    message(
      FATAL_ERROR "Quarisma requires C++11 or later. Current setting: C++${PROJECT_CXX_STANDARD}"
    )
  endif()

  # Ensure standard is properly set
  set(CMAKE_CXX_STANDARD ${PROJECT_CXX_STANDARD})
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  message(STATUS "Quarisma: C++${PROJECT_CXX_STANDARD} standard validated and configured")
endif()

# ============================================================================= Essential System
# Dependencies Validation
# =============================================================================

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)

# Threading support validation (cached)
if(NOT DEFINED PROJECT_THREADING_VALIDATED)
  find_package(Threads REQUIRED)
  if(NOT Threads_FOUND)
    message(FATAL_ERROR "Quarisma: Threading support is required but not found")
  endif()
  set(PROJECT_THREADING_VALIDATED TRUE CACHE INTERNAL "Threading validation completed")
  message(STATUS "Quarisma: Threading support validated")
endif()

# Math library validation (cached)
if(NOT DEFINED PROJECT_MATH_LIB_VALIDATED)
  if(NOT WIN32)
    check_library_exists(m sin "" HAVE_LIBM)
    if(NOT HAVE_LIBM)
      message(FATAL_ERROR "Quarisma: Math library (libm) is required but not found")
    endif()
  endif()
  set(PROJECT_MATH_LIB_VALIDATED TRUE CACHE INTERNAL "Math library validation completed")
  message(STATUS "Quarisma: Math library support validated")
endif()

# ============================================================================= Compiler Capability
# Validation (Cached)
# =============================================================================

include(CheckCXXSourceCompiles)

# C++17 features validation (cached)
if(NOT DEFINED PROJECT_CXX17_FEATURES_VALIDATED)
  # Test structured bindings
  check_cxx_source_compiles(
    "
        #include <tuple>
        int main() {
            auto [a, b] = std::make_tuple(1, 2);
            return a + b - 3;
        }
    "
    PROJECT_HAS_STRUCTURED_BINDINGS
  )

  # Test if constexpr
  check_cxx_source_compiles(
    "
        constexpr int test_func(int x) {
            if constexpr (true) {
                return x * 2;
            } else {
                return x;
            }
        }
        int main() {
            constexpr int result = test_func(5);
            return result - 10;
        }
    "
    PROJECT_HAS_IF_CONSTEXPR
  )

  # Test std::optional
  check_cxx_source_compiles(
    "
        #include <optional>
        int main() {
            std::optional<int> opt = 42;
            return opt.value_or(0) - 42;
        }
    "
    PROJECT_HAS_STD_OPTIONAL
  )

  if(NOT PROJECT_HAS_STRUCTURED_BINDINGS OR NOT PROJECT_HAS_IF_CONSTEXPR OR NOT
                                                                          PROJECT_HAS_STD_OPTIONAL
  )
    message(WARNING "Quarisma: Compiler does not support required C++17 features")
  endif()

  set(PROJECT_CXX17_FEATURES_VALIDATED TRUE CACHE INTERNAL "C++17 features validation completed")
  message(STATUS "Quarisma: C++17 features validated")
endif()

# Exception handling validation (cached)
if(NOT DEFINED PROJECT_EXCEPTION_HANDLING_VALIDATED)
  check_cxx_source_compiles(
    "
        #include <stdexcept>
        int main() {
            try {
                throw std::runtime_error(\"test\");
            } catch (const std::exception& e) {
                return 0;
            }
            return 1;
        }
    "
    PROJECT_HAS_EXCEPTION_HANDLING
  )

  if(NOT PROJECT_HAS_EXCEPTION_HANDLING)
    message(WARNING "Quarisma: Exception handling not available - some features may be limited")
  endif()

  set(PROJECT_EXCEPTION_HANDLING_VALIDATED TRUE CACHE INTERNAL
                                                     "Exception handling validation completed"
  )
  message(STATUS "Quarisma: Exception handling validated")
endif()

# ============================================================================= Platform-Specific
# Validations
# =============================================================================

# Windows-specific checks
if(PROJECT_PLATFORM_WINDOWS)
  if(NOT DEFINED PROJECT_WINDOWS_VALIDATED)
    check_include_file("windows.h" HAVE_WINDOWS_H)
    if(NOT HAVE_WINDOWS_H)
      message(FATAL_ERROR "Quarisma: Windows.h header not found on Windows platform")
    endif()
    set(PROJECT_WINDOWS_VALIDATED TRUE CACHE INTERNAL "Windows validation completed")
    message(STATUS "Quarisma: Windows platform validation completed")
  endif()
endif()

# Unix-specific checks
if(PROJECT_PLATFORM_LINUX OR PROJECT_PLATFORM_MACOS)
  if(NOT DEFINED PROJECT_UNIX_VALIDATED)
    check_include_file("unistd.h" HAVE_UNISTD_H)
    check_include_file("pthread.h" HAVE_PTHREAD_H)
    if(NOT HAVE_UNISTD_H OR NOT HAVE_PTHREAD_H)
      message(FATAL_ERROR "Quarisma: Required Unix headers not found")
    endif()
    set(PROJECT_UNIX_VALIDATED TRUE CACHE INTERNAL "Unix validation completed")
    message(STATUS "Quarisma: Unix platform validation completed")
  endif()
endif()

# ============================================================================= Validation
# Completion and Caching
# =============================================================================

# Mark validation as completed for this configuration
set(PROJECT_VALIDATION_COMPLETED "${PROJECT_VALIDATION_CACHE_KEY}"
    CACHE INTERNAL "Validation completed for configuration"
)

# Store validation summary
set(PROJECT_VALIDATION_SUMMARY
    "Platform: ${PROJECT_PLATFORM}, Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}, C++: ${PROJECT_CXX_STANDARD}"
    CACHE INTERNAL "Validation summary"
)

message(STATUS "Quarisma: System validation completed successfully")
message(STATUS "Quarisma: ${PROJECT_VALIDATION_SUMMARY}")

# ============================================================================= End of checks.cmake
# =============================================================================
