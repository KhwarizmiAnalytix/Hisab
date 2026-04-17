#=============================================================================
# Code Coverage Configuration Module

# This module configures code coverage instrumentation and automated report generation. Supports
# LLVM (Clang), GCC (gcov), and MSVC (OpenCppCoverage) coverage workflows. Generates coverage
# reports in text and HTML formats.

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    string(APPEND CMAKE_C_FLAGS " --coverage -g -O0  -fprofile-arcs -ftest-coverage")
    string(APPEND CMAKE_CXX_FLAGS " --coverage -g -O0  -fprofile-arcs -ftest-coverage")
  elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    if("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
      # clang-cl uses MSVC-style flags: /Od instead of -O0, /Zi instead of -g
      string(APPEND CMAKE_C_FLAGS " /Zi /Od -fprofile-instr-generate -fcoverage-mapping")
      string(APPEND CMAKE_CXX_FLAGS " /Zi /Od -fprofile-instr-generate -fcoverage-mapping")
    else()
      string(APPEND CMAKE_C_FLAGS " -g -O0  -fprofile-instr-generate -fcoverage-mapping")
      string(APPEND CMAKE_CXX_FLAGS " -g -O0  -fprofile-instr-generate -fcoverage-mapping")
    endif()
    message("Enabling Clang code coverage ${CMAKE_CXX_FLAGS}")
  elseif(MSVC)
    # OpenCppCoverage reads PDB files at runtime — no compiler instrumentation needed.
    # /Zi  : emit full debug info into a separate PDB (compatible with optimized builds)
    # /DEBUG: instruct the linker to produce a PDB next to each binary
    # /OPT:REF /OPT:ICF: keep Release optimisations while writing the PDB
    string(APPEND CMAKE_C_FLAGS_RELEASE   " /Zi")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /Zi")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE    " /DEBUG /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /DEBUG /OPT:REF /OPT:ICF")
    # Place linker PDBs alongside the executables/DLLs so OpenCppCoverage finds them.
    if(NOT CMAKE_PDB_OUTPUT_DIRECTORY)
      set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
    message(STATUS "MSVC coverage: /Zi + /DEBUG added to Release flags; PDBs → ${CMAKE_PDB_OUTPUT_DIRECTORY}")
  else()
    message(
      WARNING " Code coverage for compiler ${CMAKE_CXX_COMPILER_ID} is unsupported natively. "
    )
  endif()
