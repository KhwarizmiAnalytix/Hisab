#=============================================================================
# Quarisma Intel MKL
# (Math Kernel Library) Integration Module

# This module provides robust MKL integration with automatic fallback support: 1. First attempts to
# find system-installed MKL (vcpkg, apt, homebrew, Intel oneAPI) 2. If not found, automatically
# downloads and builds MKL from source 3. Creates consistent Mkl::mkl target regardless of source

cmake_minimum_required(VERSION 3.16)

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

option(CORE_ENABLE_STATIC_MKL "Prefer to link with MKL statically (Unix only)" OFF)
mark_as_advanced(CORE_ENABLE_STATIC_MKL)

if(NOT CORE_ENABLE_MKL)
  return()
endif()

find_package(MKL QUIET)

if(TARGET Mkl::mkl)
  return()
endif()

if(NOT MKL_FOUND)
  message(FATAL_ERROR "MKL not found! Please set MKL_ROOT or CMAKE_PREFIX_PATH")
endif()

add_library(Mkl::mkl INTERFACE IMPORTED)
target_include_directories(Mkl::mkl INTERFACE ${MKL_INCLUDE_DIR})
target_link_libraries(Mkl::mkl INTERFACE ${MKL_LIBRARIES})

foreach(MKL_LIB IN LISTS MKL_LIBRARIES)
  if(EXISTS "${MKL_LIB}")
    get_filename_component(MKL_LINK_DIR "${MKL_LIB}" DIRECTORY)
    if(IS_DIRECTORY "${MKL_LINK_DIR}")
      target_link_directories(Mkl::mkl INTERFACE "${MKL_LINK_DIR}")
    endif()
  endif()
endforeach()

# TODO: This is a hack, it will not pick up architecture dependent
# MKL libraries correctly; see https://github.com/pytorch/pytorch/issues/73008
set_property(
  TARGET Mkl::mkl PROPERTY INTERFACE_LINK_DIRECTORIES
  ${MKL_ROOT}/lib ${MKL_ROOT}/lib/intel64 ${MKL_ROOT}/lib/intel64_win ${MKL_ROOT}/lib/win-x64)

if(UNIX)
  if(CORE_ENABLE_STATIC_MKL)
    foreach(MKL_LIB_PATH IN LISTS MKL_LIBRARIES)
      if(NOT EXISTS "${MKL_LIB_PATH}")
        continue()
      endif()

      get_filename_component(MKL_LIB_NAME "${MKL_LIB_PATH}" NAME)

      # Match archive libraries starting with "libmkl_"
      if(MKL_LIB_NAME MATCHES "^libmkl_" AND MKL_LIB_NAME MATCHES ".a$")
        target_link_options(Mkl::mkl INTERFACE "-Wl,--exclude-libs,${MKL_LIB_NAME}")
      endif()
    endforeach()
  endif()
endif()


