# =============================================================================
# Quarisma Enzyme Automatic Differentiation Integration Module
# =============================================================================
# This module integrates Enzyme AD (https://enzyme.mit.edu/) for automatic
# differentiation. Enzyme works by loading as an LLVM plugin and requires
# special compiler flags to enable differentiation capabilities.
#
# Enzyme provides:
# - Forward-mode automatic differentiation
# - Reverse-mode automatic differentiation (backpropagation)
# - High-performance AD with minimal overhead
# - Works with C and C++ code
#
# Requirements:
# - Clang/LLVM compiler (GCC not supported)
# - Installed Enzyme CMake package (EnzymeConfig.cmake), e.g. from Homebrew or
#   a source build with install; use Enzyme_DIR or CMAKE_PREFIX_PATH if needed
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Enzyme AD Support Flag
# Controls whether Enzyme automatic differentiation is enabled.
# When enabled, Enzyme provides high-performance AD capabilities.
# Note: Already defined in main CMakeLists.txt, just documenting here
if(NOT DEFINED QUARISMA_ENABLE_ENZYME)
  option(QUARISMA_ENABLE_ENZYME "Enable Enzyme automatic differentiation support" OFF)
  mark_as_advanced(QUARISMA_ENABLE_ENZYME)
endif()

# Only proceed if Enzyme is enabled
if(NOT QUARISMA_ENABLE_ENZYME)
  message(STATUS "Enzyme automatic differentiation support is disabled (QUARISMA_ENABLE_ENZYME=OFF)")
  return()
endif()

message(STATUS "Configuring Enzyme automatic differentiation support...")

# =============================================================================
# Step 1: Verify compiler compatibility
# =============================================================================
# Enzyme requires Clang/LLVM compiler
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING
    "\n"
    "================================================================================\n"
    "WARNING: Enzyme AD requires Clang/LLVM compiler\n"
    "================================================================================\n"
    "\n"
    "Current compiler: ${CMAKE_CXX_COMPILER_ID}\n"
    "Enzyme automatic differentiation only works with Clang/LLVM.\n"
    "\n"
    "To use Enzyme:\n"
    "1. Install Clang/LLVM:\n"
    "   - macOS: brew install llvm\n"
    "   - Ubuntu: apt install clang llvm\n"
    "\n"
    "2. Reconfigure with Clang:\n"
    "   cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..\n"
    "\n"
    "Disabling Enzyme support.\n"
    "================================================================================\n"
  )
  set(QUARISMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

message(STATUS "✅ Compiler check passed: ${CMAKE_CXX_COMPILER_ID}")

# =============================================================================
# Step 2: find_package(Enzyme)
# =============================================================================
# Upstream Enzyme installs EnzymeConfig.cmake and imported targets such as
# ClangEnzymeFlags and LLDEnzymeFlags (see enzyme/test/test_find_package).

set(QUARISMA_ENZYME_FLAGS_TARGET
    "ClangEnzymeFlags"
    CACHE STRING
          "Enzyme imported flags target: ClangEnzymeFlags (clang) or LLDEnzymeFlags (lld)"
)
mark_as_advanced(QUARISMA_ENZYME_FLAGS_TARGET)

find_package(Enzyme CONFIG)

if(NOT Enzyme_FOUND)
  message(WARNING
    "\n"
    "================================================================================\n"
    "WARNING: Enzyme CMake package not found\n"
    "================================================================================\n"
    "\n"
    "Enzyme automatic differentiation requires an installed Enzyme CMake config\n"
    "(EnzymeConfig.cmake), typically under <prefix>/lib/cmake/Enzyme.\n"
    "\n"
    "INSTALLATION:\n"
    "\n"
    "1. macOS (Homebrew):\n"
    "   brew install enzyme\n"
    "   # If needed:\n"
    "   cmake -DEnzyme_DIR=/opt/homebrew/opt/enzyme/lib/cmake/Enzyme ..\n"
    "\n"
    "2. Build from source:\n"
    "   git clone https://github.com/EnzymeAD/Enzyme.git\n"
    "   cd Enzyme/enzyme && mkdir build && cd build\n"
    "   cmake -G Ninja .. -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm\n"
    "   cmake --build . && cmake --install . --prefix <install-prefix>\n"
    "   cmake -DCMAKE_PREFIX_PATH=<install-prefix> ..\n"
    "\n"
    "For more information: https://enzyme.mit.edu/\n"
    "\n"
    "Disabling Enzyme support.\n"
    "================================================================================\n"
  )
  set(QUARISMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

if(NOT TARGET "${QUARISMA_ENZYME_FLAGS_TARGET}")
  message(WARNING
    "\n"
    "================================================================================\n"
    "WARNING: Enzyme flags target \"${QUARISMA_ENZYME_FLAGS_TARGET}\" not found\n"
    "================================================================================\n"
    "\n"
    "Valid options are typically ClangEnzymeFlags or LLDEnzymeFlags (from the\n"
    "installed Enzyme package). Set QUARISMA_ENZYME_FLAGS_TARGET accordingly.\n"
    "\n"
    "Disabling Enzyme support.\n"
    "================================================================================\n"
  )
  set(QUARISMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

message(STATUS "Found Enzyme: ${Enzyme_DIR}")
message(STATUS "Using Enzyme flags target: ${QUARISMA_ENZYME_FLAGS_TARGET}")

# =============================================================================
# Step 3: Optional extra compiler flags
# =============================================================================
# NOTE: -fno-exceptions and -fno-rtti are disabled by default because they
# conflict with code that uses exceptions and RTTI (which Quarisma does).
# Enzyme can work with exceptions and RTTI enabled, though it may have
# slightly lower performance.
option(ENZYME_ENABLE_OPTIMIZATIONS "Enable Enzyme-specific optimizations (disables exceptions/RTTI)" OFF)
mark_as_advanced(ENZYME_ENABLE_OPTIMIZATIONS)

set(_quarisma_enzyme_extra_flags "")
if(ENZYME_ENABLE_OPTIMIZATIONS)
  list(APPEND _quarisma_enzyme_extra_flags "-fno-exceptions" "-fno-rtti")
  message(WARNING "Enzyme optimizations enabled: -fno-exceptions and -fno-rtti will be applied. This may break code that uses exceptions or RTTI.")
endif()

# Legacy cache entry: full compiler flag line is no longer constructed here;
# the Enzyme package supplies -fpass-plugin via the imported flags target.
set(ENZYME_COMPILE_OPTIONS "${_quarisma_enzyme_extra_flags}" CACHE STRING "Extra Enzyme-related compiler flags (in addition to find_package targets)" FORCE)

# =============================================================================
# Step 4: Create Quarisma::enzyme interface target
# =============================================================================

if(NOT TARGET quarisma_enzyme_iface)
  add_library(quarisma_enzyme_iface INTERFACE)
  add_library(Quarisma::enzyme ALIAS quarisma_enzyme_iface)
  message(STATUS "✅ Created Quarisma::enzyme interface target")
endif()

set_target_properties(
  quarisma_enzyme_iface
  PROPERTIES INTERFACE_LINK_LIBRARIES "${QUARISMA_ENZYME_FLAGS_TARGET}" INTERFACE_COMPILE_DEFINITIONS
             QUARISMA_HAS_ENZYME=1
)

if(_quarisma_enzyme_extra_flags)
  set_target_properties(
    quarisma_enzyme_iface
    PROPERTIES INTERFACE_COMPILE_OPTIONS "${_quarisma_enzyme_extra_flags}" INTERFACE_LINK_OPTIONS
               "${_quarisma_enzyme_extra_flags}"
  )
else()
  set_target_properties(quarisma_enzyme_iface PROPERTIES INTERFACE_COMPILE_OPTIONS "" INTERFACE_LINK_OPTIONS "")
endif()

# =============================================================================
# Step 5: Export Enzyme information
# =============================================================================

set(ENZYME_FOUND TRUE CACHE BOOL "Enzyme was found successfully" FORCE)
# Plugin path is owned by the Enzyme imported targets; kept for compatibility with older docs/tools.
set(ENZYME_PLUGIN_LIBRARY "" CACHE FILEPATH "Enzyme plugin path (unused with find_package; see Enzyme imported targets)" FORCE)

message(STATUS "Enzyme automatic differentiation configuration complete")
message(STATUS "  Enzyme_DIR: ${Enzyme_DIR}")
message(STATUS "  Flags target: ${QUARISMA_ENZYME_FLAGS_TARGET}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
