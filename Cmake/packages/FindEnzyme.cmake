# FindEnzyme.cmake
# ----------------
# Locate the Enzyme AD LLVM/clang plugin (LLDEnzyme / ClangEnzyme / LLVMEnzyme).
#
# Use with module mode (default for this name):
#   find_package(Enzyme [REQUIRED] [QUIET])
#
# CMAKE_MODULE_PATH must include the directory containing this file (e.g. Cmake/packages).
# CONFIG mode: use EnzymeConfig.cmake in the same directory, e.g.
#   find_package(Enzyme CONFIG REQUIRED PATHS "${CMAKE_SOURCE_DIR}/Cmake/packages")
#
# Input cache variable (Quarisma defines it in enzyme.cmake before find_package):
#   ENZYME_PLUGIN_PATH - Optional explicit path to the plugin (.so/.dylib/.dll)
#
# Result variables:
#   Enzyme_FOUND
#   Enzyme_PLUGIN_LIBRARY - Path to the Enzyme plugin shared library
#
# Legacy compatibility (set when Enzyme_FOUND):
#   ENZYME_PLUGIN_LIBRARY

include(FindPackageHandleStandardArgs)

set(Enzyme_PLUGIN_LIBRARY "Enzyme_PLUGIN_LIBRARY-NOTFOUND")

if(ENZYME_PLUGIN_PATH AND EXISTS "${ENZYME_PLUGIN_PATH}")
  set(Enzyme_PLUGIN_LIBRARY "${ENZYME_PLUGIN_PATH}")
  if(NOT Enzyme_FIND_QUIETLY)
    message(STATUS "Using user-specified Enzyme plugin: ${Enzyme_PLUGIN_LIBRARY}")
  endif()
else()
  if(WIN32)
    set(_enzyme_search_paths
      "$ENV{LLVM_DIR}/bin"
      "$ENV{LLVM_DIR}/lib"
      "C:/Program Files/LLVM/bin"
      "C:/Program Files/LLVM/lib"
      "C:/Program Files (x86)/LLVM/bin"
      "C:/Program Files (x86)/LLVM/lib"
    )
  else()
    set(_enzyme_search_paths
      "/usr/lib"
      "/usr/local/lib"
      "/opt/homebrew/lib"
      "/opt/homebrew/opt/llvm/lib"
      "$ENV{LLVM_DIR}/lib"
      "${CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN}/lib"
    )
  endif()

  get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(_llvm_root "${_compiler_dir}" DIRECTORY)
  if(WIN32)
    list(APPEND _enzyme_search_paths "${_llvm_root}/bin" "${_llvm_root}/lib")
  else()
    list(APPEND _enzyme_search_paths "${_llvm_root}/lib")
  endif()

  string(REGEX MATCH "^([0-9]+)" _llvm_major_version "${CMAKE_CXX_COMPILER_VERSION}")

  if(NOT Enzyme_FIND_QUIETLY)
    message(STATUS "Searching for Enzyme plugin...")
    message(STATUS "  Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "  LLVM major version: ${_llvm_major_version}")
    message(STATUS "  Search paths: ${_enzyme_search_paths}")
  endif()

  set(_found_enzyme_files)
  foreach(_search_path ${_enzyme_search_paths})
    if(WIN32)
      file(GLOB _enzyme_candidates
        "${_search_path}/LLDEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dll"
        "${_search_path}/LLDEnzyme-${_llvm_major_version}.dll"
        "${_search_path}/LLDEnzyme.dll"
        "${_search_path}/ClangEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dll"
        "${_search_path}/ClangEnzyme-${_llvm_major_version}.dll"
        "${_search_path}/ClangEnzyme.dll"
        "${_search_path}/LLVMEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dll"
        "${_search_path}/LLVMEnzyme-${_llvm_major_version}.dll"
        "${_search_path}/LLVMEnzyme.dll"
      )
    elseif(APPLE)
      file(GLOB _enzyme_candidates
        "${_search_path}/LLDEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dylib"
        "${_search_path}/LLDEnzyme-${_llvm_major_version}.dylib"
        "${_search_path}/LLDEnzyme.dylib"
        "${_search_path}/ClangEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dylib"
        "${_search_path}/ClangEnzyme-${_llvm_major_version}.dylib"
        "${_search_path}/ClangEnzyme.dylib"
        "${_search_path}/LLVMEnzyme-${CMAKE_CXX_COMPILER_VERSION}.dylib"
        "${_search_path}/LLVMEnzyme-${_llvm_major_version}.dylib"
        "${_search_path}/LLVMEnzyme.dylib"
      )
    else()
      file(GLOB _enzyme_candidates
        "${_search_path}/LLDEnzyme-${CMAKE_CXX_COMPILER_VERSION}.so"
        "${_search_path}/LLDEnzyme-${_llvm_major_version}.so"
        "${_search_path}/LLDEnzyme.so"
        "${_search_path}/ClangEnzyme-${CMAKE_CXX_COMPILER_VERSION}.so"
        "${_search_path}/ClangEnzyme-${_llvm_major_version}.so"
        "${_search_path}/ClangEnzyme.so"
        "${_search_path}/LLVMEnzyme-${CMAKE_CXX_COMPILER_VERSION}.so"
        "${_search_path}/LLVMEnzyme-${_llvm_major_version}.so"
        "${_search_path}/LLVMEnzyme.so"
      )
    endif()
    if(_enzyme_candidates)
      list(APPEND _found_enzyme_files ${_enzyme_candidates})
    endif()
  endforeach()

  if(_found_enzyme_files)
    list(GET _found_enzyme_files 0 Enzyme_PLUGIN_LIBRARY)
    if(NOT Enzyme_FIND_QUIETLY)
      message(STATUS "  Found candidates: ${_found_enzyme_files}")
      message(STATUS "  Result: ${Enzyme_PLUGIN_LIBRARY}")
      message(STATUS "Found Enzyme plugin: ${Enzyme_PLUGIN_LIBRARY}")
    endif()
  elseif(NOT Enzyme_FIND_QUIETLY)
    message(STATUS "  Result: ")
  endif()
endif()

find_package_handle_standard_args(Enzyme
  FOUND_VAR Enzyme_FOUND
  REQUIRED_VARS Enzyme_PLUGIN_LIBRARY
)

if(Enzyme_FOUND)
  set(ENZYME_PLUGIN_LIBRARY "${Enzyme_PLUGIN_LIBRARY}" CACHE FILEPATH
    "Enzyme plugin library path" FORCE)
  mark_as_advanced(ENZYME_PLUGIN_LIBRARY)
endif()
