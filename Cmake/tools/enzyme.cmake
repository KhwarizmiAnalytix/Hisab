# =============================================================================
# Quarisma Enzyme Automatic Differentiation Integration Module
# =============================================================================
# This module integrates Enzyme AD (https://enzyme.mit.edu/) for automatic
# differentiation. Enzyme works by loading as an LLVM plugin and requires
# special compiler flags to enable differentiation capabilities.
#
# Discovery is implemented by Cmake/packages/FindEnzyme.cmake; use
#   find_package(Enzyme [REQUIRED] [QUIET])
# with CMAKE_MODULE_PATH containing Cmake/packages (set in root CMakeLists.txt).
# Module mode is used (not CONFIG); upstream may ship EnzymeConfig.cmake separately.
#
# Windows: if CMake reports an unwanted LLVM prefix (e.g. from LLVM_DIR or the
# compiler path), configure with -DENZYME_RESTRICT_TO_SYSTEM_LLVM_INSTALL=ON
# so Enzyme is searched only under "Program Files\\LLVM". For Bazel helper
# discovery, set the same name in the environment to 1/true/on. Also clear
# LLVM_DIR from the environment or pass -DLLVM_DIR=... to point at the intended
# install when using find_package(LLVM) elsewhere.
#
# Requirements:
# - Clang/LLVM compiler (GCC not supported)
# - Enzyme plugin library (ClangEnzyme-*.so or LLVMEnzyme-*.so)
# =============================================================================

cmake_minimum_required(VERSION 3.16)

include_guard(GLOBAL)

if(NOT DEFINED QUARISMA_ENABLE_ENZYME)
  option(QUARISMA_ENABLE_ENZYME "Enable Enzyme automatic differentiation support" OFF)
  mark_as_advanced(QUARISMA_ENABLE_ENZYME)
endif()

if(NOT QUARISMA_ENABLE_ENZYME)
  message(STATUS "Enzyme automatic differentiation support is disabled (QUARISMA_ENABLE_ENZYME=OFF)")
  return()
endif()

message(STATUS "Configuring Enzyme automatic differentiation support...")

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

message(STATUS "Compiler check passed: ${CMAKE_CXX_COMPILER_ID}")

set(ENZYME_PLUGIN_PATH "" CACHE PATH
  "Path to Enzyme plugin library (ClangEnzyme-*.so or LLVMEnzyme-*.so)")
mark_as_advanced(ENZYME_PLUGIN_PATH)

find_package(Enzyme QUIET)

if(NOT Enzyme_FOUND)
  if(WIN32)
    message(WARNING
      "\n"
      "================================================================================\n"
      "WARNING: Enzyme plugin library not found\n"
      "================================================================================\n"
      "\n"
      "Enzyme automatic differentiation requires the Enzyme LLVM plugin.\n"
      "\n"
      "INSTALLATION METHODS FOR WINDOWS:\n"
      "\n"
      "1. Build from source:\n"
      "   git clone https://github.com/EnzymeAD/Enzyme.git\n"
      "   cd Enzyme\\enzyme\n"
      "   mkdir build && cd build\n"
      "   cmake -G Ninja .. ^\n"
      "     -DLLVM_DIR=\"C:/Program Files/LLVM/lib/cmake/llvm\" ^\n"
      "     -DCMAKE_BUILD_TYPE=Release ^\n"
      "     -DCMAKE_C_COMPILER=\"C:/Program Files/LLVM/bin/clang.exe\" ^\n"
      "     -DCMAKE_CXX_COMPILER=\"C:/Program Files/LLVM/bin/clang++.exe\"\n"
      "   ninja\n"
      "   copy LLVMEnzyme-*.dll \"C:\\Program Files\\LLVM\\bin\\\"\n"
      "\n"
      "2. Specify custom path:\n"
      "   cmake -DENZYME_PLUGIN_PATH=\"C:/path/to/LLVMEnzyme-21.dll\" ..\n"
      "\n"
      "For more information: https://enzyme.mit.edu/\n"
      "\n"
      "Disabling Enzyme support.\n"
      "================================================================================\n"
    )
  else()
    message(WARNING
      "\n"
      "================================================================================\n"
      "WARNING: Enzyme plugin library not found\n"
      "================================================================================\n"
      "\n"
      "Enzyme automatic differentiation requires the Enzyme LLVM plugin.\n"
      "\n"
      "INSTALLATION METHODS:\n"
      "\n"
      "1. Build from source:\n"
      "   git clone https://github.com/EnzymeAD/Enzyme.git\n"
      "   cd Enzyme/enzyme\n"
      "   mkdir build && cd build\n"
      "   cmake -G Ninja .. \\\n"
      "     -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \\\n"
      "     -DCMAKE_BUILD_TYPE=Release\n"
      "   ninja\n"
      "\n"
      "2. Specify custom path:\n"
      "   cmake -DENZYME_PLUGIN_PATH=/path/to/ClangEnzyme-*.so ..\n"
      "\n"
      "3. macOS (Homebrew):\n"
      "   brew install enzyme\n"
      "\n"
      "For more information: https://enzyme.mit.edu/\n"
      "\n"
      "Disabling Enzyme support.\n"
      "================================================================================\n"
    )
  endif()
  set(QUARISMA_ENABLE_ENZYME OFF CACHE BOOL "Enable Enzyme automatic differentiation support" FORCE)
  return()
endif()

set(ENZYME_COMPILER_FLAGS
  "-fpass-plugin=${Enzyme_PLUGIN_LIBRARY}"
)

option(ENZYME_ENABLE_OPTIMIZATIONS "Enable Enzyme-specific optimizations (disables exceptions/RTTI)" OFF)
mark_as_advanced(ENZYME_ENABLE_OPTIMIZATIONS)

if(ENZYME_ENABLE_OPTIMIZATIONS)
  list(APPEND ENZYME_COMPILER_FLAGS
    "-fno-exceptions"
    "-fno-rtti"
  )
  message(WARNING "Enzyme optimizations enabled: -fno-exceptions and -fno-rtti will be applied. This may break code that uses exceptions or RTTI.")
endif()

set(ENZYME_COMPILE_OPTIONS ${ENZYME_COMPILER_FLAGS} CACHE STRING "Enzyme compiler flags" FORCE)

message(STATUS "Enzyme compiler flags: ${ENZYME_COMPILE_OPTIONS}")

if(NOT TARGET Quarisma::enzyme)
  add_library(Quarisma::enzyme INTERFACE IMPORTED GLOBAL)
  target_compile_options(Quarisma::enzyme INTERFACE ${ENZYME_COMPILE_OPTIONS})
  target_link_options(Quarisma::enzyme INTERFACE ${ENZYME_COMPILE_OPTIONS})
  target_compile_definitions(Quarisma::enzyme INTERFACE QUARISMA_HAS_ENZYME=1)
  message(STATUS "Created Quarisma::enzyme interface target")
endif()

set(ENZYME_FOUND TRUE CACHE BOOL "Enzyme was found successfully" FORCE)

message(STATUS "Enzyme automatic differentiation configuration complete")
message(STATUS "  Plugin: ${Enzyme_PLUGIN_LIBRARY}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
