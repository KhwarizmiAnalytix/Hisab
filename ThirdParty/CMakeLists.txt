# =============================================================================
# Quarisma Third-Party Dependency Management
# =============================================================================
# This file manages all third-party dependencies for Quarisma, handling both bundled and
# external (system-installed) libraries via the QUARISMA_ENABLE_EXTERNAL flag. The
# add_third_party_library() function automatically populates QUARISMA_DEPENDENCY_LIBS and
# QUARISMA_DEPENDENCY_INCLUDE_DIRS, which are propagated to the main build system.
# =============================================================================

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# External Library Usage Flag Controls whether external copies of third-party libraries are used by
# default. When enabled, prefers external library versions over bundled versions.
option(QUARISMA_ENABLE_EXTERNAL "Use external copies of third party libraries by default" ON)
mark_as_advanced(QUARISMA_ENABLE_EXTERNAL)


# Global option to control third-party library type (STATIC, SHARED, DEFAULT, or HEADER_ONLY)
# DEFAULT means respect each library's own settings or BUILD_SHARED_LIBS
# HEADER_ONLY means the library is header-only (no compilation/linking needed)
set(QUARISMA_THIRDPARTY_TYPE "DEFAULT" CACHE STRING "Default library type for third-party libraries (STATIC, SHARED, DEFAULT, or HEADER_ONLY)")
set_property(CACHE QUARISMA_THIRDPARTY_TYPE PROPERTY STRINGS "STATIC" "SHARED" "DEFAULT" "HEADER_ONLY")

# Configure Visual Studio folder structure to match file system
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Disable tests, examples, and benchmarks for all third-party libraries
set(BUILD_TESTING OFF CACHE BOOL "Disable testing for third-party libraries" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable examples for third-party libraries" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "Disable benchmarks for third-party libraries" FORCE)


# fmt-specific options
set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
set(FMT_DOC OFF CACHE BOOL "Disable fmt documentation" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "Disable fmt install" FORCE)
set(FMT_HEADER_ONLY ON CACHE BOOL "Use header-only fmt library" FORCE)

# cpuinfo-specific options
set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "Disable cpuinfo tools" FORCE)
set(CPUINFO_BUILD_UNIT_TESTS OFF CACHE BOOL "Disable cpuinfo unit tests" FORCE)
set(CPUINFO_BUILD_MOCK_TESTS OFF CACHE BOOL "Disable cpuinfo mock tests" FORCE)
set(CPUINFO_BUILD_BENCHMARKS OFF CACHE BOOL "Disable cpuinfo benchmarks" FORCE)
set(CPUINFO_LIBRARY_TYPE "static" CACHE STRING "Type of cpuinfo library (shared, static, or default) to build")

# magic_enum-specific options
set(MAGIC_ENUM_OPT_BUILD_EXAMPLES OFF CACHE BOOL "Disable magic_enum examples" FORCE)
set(MAGIC_ENUM_OPT_BUILD_TESTS OFF CACHE BOOL "Disable magic_enum tests" FORCE)

# loguru-specific options
set(LOGURU_BUILD_EXAMPLES OFF CACHE BOOL "Disable loguru examples" FORCE)
#set(LOGURU_USE_FMTLIB ON CACHE BOOL "Enable fmtlib formatting in loguru" FORCE)
set(LOGURU_WITH_STREAMS ON CACHE BOOL "Enable streams in loguru" FORCE)

# glog-specific options
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable glog examples" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "Disable gflags dependency for glog" FORCE)
set(WITH_GTEST OFF CACHE BOOL "Disable gtest for glog" FORCE)
set(WITH_GMOCK OFF CACHE BOOL "Disable gmock for glog" FORCE)

# googletest-specific options
set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable Google Test install" FORCE)
set(gtest_build_samples OFF CACHE BOOL "Disable Google Test samples" FORCE)
set(gtest_build_tests OFF CACHE BOOL "Disable Google Test tests" FORCE)



# Google Benchmark-specific options
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_EXCEPTIONS ON CACHE BOOL "Enable benchmark exceptions" FORCE)
set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "Disable benchmark LTO" FORCE)
set(BENCHMARK_USE_LIBCXX OFF CACHE BOOL "Disable benchmark libcxx" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "Disable benchmark werror" FORCE)
set(BENCHMARK_FORCE_WERROR OFF CACHE BOOL "Disable benchmark force werror" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "Disable benchmark docs install" FORCE)
set(BENCHMARK_ENABLE_DOXYGEN OFF CACHE BOOL "Disable benchmark doxygen" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable benchmark gtest tests" FORCE)
set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "Don't use bundled gtest for benchmark" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Don't download dependencies" FORCE)

# mimalloc-specific options
set(MI_BUILD_TESTS OFF CACHE BOOL "Disable mimalloc tests" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "Disable mimalloc object library" FORCE)
set(MI_INSTALL_TOPLEVEL OFF CACHE BOOL "Disable mimalloc top-level install" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "Disable mimalloc malloc override" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "Build mimalloc as static library" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "Build mimalloc as static library" FORCE)
# Note: MI_BUILD_STATIC is now controlled by add_third_party_library LIBRARY_TYPE parameter

# =============================================================================
# Helper Functions for Third-Party Library Management
# =============================================================================

# -----------------------------------------------------------------------------
# _filter_flags_internal: Remove specified flags from a flag string
# -----------------------------------------------------------------------------
# Internal helper to filter out specific flags from a string of compiler/linker flags
#
# Parameters:
#   input_flags - The flag string to filter
#   flags_to_remove - List of flags to remove (can be regex patterns)
#   output_var - Variable name to store the filtered result
#
function(_filter_flags_internal input_flags flags_to_remove output_var)
    set(result "${input_flags}")

    foreach(flag_pattern ${flags_to_remove})
        # Remove the flag pattern from the string
        string(REGEX REPLACE "${flag_pattern}( |$)" "" result "${result}")
    endforeach()

    # Clean up multiple spaces
    string(REGEX REPLACE "  +" " " result "${result}")
    string(STRIP "${result}" result)

    set(${output_var} "${result}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# _save_third_party_flags: Save and optionally filter compiler/linker flags
# -----------------------------------------------------------------------------
# Saves current CMAKE_CXX_FLAGS, CMAKE_C_FLAGS, and linker flags, then optionally
# filters out problematic flags that shouldn't be applied to third-party libraries
#
# Parameters:
#   name - Library name (for namespacing saved variables)
#   filter_flags - List of flag patterns to remove (optional)
#
function(_save_third_party_flags name filter_flags)
    # Save original flags to parent scope
    set(_SAVED_CMAKE_CXX_FLAGS_${name} "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_C_FLAGS_${name} "${CMAKE_C_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_EXE_LINKER_FLAGS_${name} "${CMAKE_EXE_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_SHARED_LINKER_FLAGS_${name} "${CMAKE_SHARED_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_MODULE_LINKER_FLAGS_${name} "${CMAKE_MODULE_LINKER_FLAGS}" PARENT_SCOPE)
    set(_SAVED_CMAKE_STATIC_LINKER_FLAGS_${name} "${CMAKE_STATIC_LINKER_FLAGS}" PARENT_SCOPE)

    # If filter flags are provided, filter them out
    if(filter_flags)
        _filter_flags_internal("${CMAKE_CXX_FLAGS}" "${filter_flags}" filtered_cxx_flags)
        _filter_flags_internal("${CMAKE_C_FLAGS}" "${filter_flags}" filtered_c_flags)
        _filter_flags_internal("${CMAKE_EXE_LINKER_FLAGS}" "${filter_flags}" filtered_exe_linker_flags)
        _filter_flags_internal("${CMAKE_SHARED_LINKER_FLAGS}" "${filter_flags}" filtered_shared_linker_flags)
        _filter_flags_internal("${CMAKE_MODULE_LINKER_FLAGS}" "${filter_flags}" filtered_module_linker_flags)
        _filter_flags_internal("${CMAKE_STATIC_LINKER_FLAGS}" "${filter_flags}" filtered_static_linker_flags)

        # Apply filtered flags to parent scope
        set(CMAKE_CXX_FLAGS "${filtered_cxx_flags}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${filtered_c_flags}" PARENT_SCOPE)
        set(CMAKE_EXE_LINKER_FLAGS "${filtered_exe_linker_flags}" PARENT_SCOPE)
        set(CMAKE_SHARED_LINKER_FLAGS "${filtered_shared_linker_flags}" PARENT_SCOPE)
        set(CMAKE_MODULE_LINKER_FLAGS "${filtered_module_linker_flags}" PARENT_SCOPE)
        set(CMAKE_STATIC_LINKER_FLAGS "${filtered_static_linker_flags}" PARENT_SCOPE)

        message(STATUS "Filtered flags for ${name}: removed ${filter_flags}")
    endif()
endfunction()

# -----------------------------------------------------------------------------
# _restore_third_party_flags: Restore original compiler/linker flags
# -----------------------------------------------------------------------------
# Restores the compiler and linker flags that were saved before building
# a third-party library
#
# Parameters:
#   name - Library name (for namespacing saved variables)
#
function(_restore_third_party_flags name)
    # Restore original flags to parent scope
    set(CMAKE_CXX_FLAGS "${_SAVED_CMAKE_CXX_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_C_FLAGS "${_SAVED_CMAKE_C_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS "${_SAVED_CMAKE_EXE_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_SHARED_LINKER_FLAGS "${_SAVED_CMAKE_SHARED_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_MODULE_LINKER_FLAGS "${_SAVED_CMAKE_MODULE_LINKER_FLAGS_${name}}" PARENT_SCOPE)
    set(CMAKE_STATIC_LINKER_FLAGS "${_SAVED_CMAKE_STATIC_LINKER_FLAGS_${name}}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# _set_third_party_folder_properties: Set FOLDER property for all targets
# -----------------------------------------------------------------------------
# Automatically discovers all targets created by a third-party library and
# sets their FOLDER property for IDE organization. This function recursively
# traverses all subdirectories to find targets at any nesting level.
#
# IMPORTANT: CMake's BUILDSYSTEM_TARGETS property only returns targets defined
# directly in a directory, not in subdirectories. This function uses recursive
# traversal via the SUBDIRECTORIES property to discover ALL targets.
#
# Parameters:
#   name - Library name
#   binary_dir - Binary directory where the library was built
#
function(_set_third_party_folder_properties name binary_dir)
    # Helper function to recursively collect targets from a directory tree
    # This is necessary because BUILDSYSTEM_TARGETS only returns targets
    # defined directly in the specified directory, not in subdirectories
    function(_collect_targets_recursive dir collected_targets_var)
        # Get targets defined directly in this directory
        get_property(dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)

        if(dir_targets)
            list(APPEND local_targets ${dir_targets})
        endif()

        # Get all subdirectories in this directory
        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

        # Recursively process each subdirectory
        foreach(subdir ${subdirs})
            _collect_targets_recursive("${subdir}" local_targets)
        endforeach()

        # Return the collected targets to parent scope
        set(${collected_targets_var} "${local_targets}" PARENT_SCOPE)
    endfunction()

    # Collect all targets recursively starting from the binary directory
    _collect_targets_recursive("${binary_dir}" all_targets)

    if(all_targets)
        message(STATUS "Setting FOLDER property for ${name} targets: ${all_targets}")
        foreach(target ${all_targets})
            if(TARGET ${target})
                set_target_properties(${target} PROPERTIES FOLDER "ThirdParty/${name}")
            endif()
        endforeach()
    else()
        message(STATUS "No targets found for ${name} in ${binary_dir} (this may be normal for header-only libraries)")
    endif()
endfunction()

# -----------------------------------------------------------------------------
# _create_third_party_aliases: Create library aliases
# -----------------------------------------------------------------------------
# Creates alias targets for a third-party library to provide consistent
# naming conventions (e.g., fmt::fmt as an alias for fmt)
#
# Parameters:
#   name - Library name
#   aliases - List of alias names to create
#
function(_create_third_party_aliases name aliases)
    if(NOT aliases)
        message(WARNING "No aliases provided for ${name}")
        return()
    endif()

    foreach(alias ${aliases})
        # Check if the alias already exists
        if(NOT TARGET ${alias})
            # Check if the base target exists
            if(TARGET ${name})
                add_library(${alias} ALIAS ${name})
                message(STATUS "Created alias ${alias} for ${name}")
            else()
                message(WARNING "Cannot create alias ${alias}: target ${name} does not exist")
            endif()
        else()
            message(STATUS "Alias ${alias} already exists, skipping")
        endif()
    endforeach()
endfunction()

# -----------------------------------------------------------------------------
# _create_third_party_interface_targets: Create Quarisma:: namespace aliases
# -----------------------------------------------------------------------------
# Creates Quarisma:: namespace aliases for third-party library targets.
# This provides a consistent interface for linking against third-party libraries
# throughout the Quarisma project.
#
# The function handles multiple scenarios:
# 1. Target with namespace alias (e.g., fmt::fmt) - resolves the actual target
# 2. Plain target (e.g., fmt) - creates alias directly
# 3. Multiple possible target names - tries each in order
#
# Parameters:
#   quarisma_targets - List of Quarisma target specifications in format:
#                    "Quarisma::name=target1|target2|..."
#                    where target1, target2, etc. are tried in order
#
# Example:
#   _create_third_party_interface_targets(
#       "Quarisma::fmt=fmt::fmt|fmt"
#       "Quarisma::gtest=GTest::gtest|gtest;Quarisma::gtest_main=GTest::gtest_main|gtest_main"
#   )
#
function(_create_third_party_interface_targets)
    set(quarisma_targets ${ARGN})

    if(NOT quarisma_targets)
        return()
    endif()

    foreach(target_spec ${quarisma_targets})
        # Parse the specification: "Quarisma::name=target1|target2|..."
        string(REPLACE "=" ";" spec_parts "${target_spec}")
        list(LENGTH spec_parts spec_parts_len)

        if(NOT spec_parts_len EQUAL 2)
            message(WARNING "Invalid Quarisma target specification: ${target_spec}")
            continue()
        endif()

        list(GET spec_parts 0 quarisma_name)
        list(GET spec_parts 1 source_targets)

        # Check if Quarisma target already exists
        if(TARGET ${quarisma_name})
            message(STATUS "Quarisma target ${quarisma_name} already exists, skipping")
            continue()
        endif()

        # Split source targets by pipe (|) to get alternatives
        string(REPLACE "|" ";" source_target_list "${source_targets}")

        set(target_created FALSE)
        foreach(source_target ${source_target_list})
            if(TARGET ${source_target})
                # Check if source target is IMPORTED (from find_package)
                get_target_property(is_imported ${source_target} IMPORTED)

                if(is_imported)
                    # For IMPORTED targets, create an INTERFACE library that links to it
                    # (ALIAS targets cannot alias IMPORTED targets)
                    add_library(${quarisma_name} INTERFACE IMPORTED GLOBAL)
                    target_link_libraries(${quarisma_name} INTERFACE ${source_target})
                    message(STATUS "Created ${quarisma_name} -> ${source_target} (INTERFACE wrapper for IMPORTED target)")
                else()
                    # Check if source target is an alias and resolve it
                    get_target_property(aliased_target ${source_target} ALIASED_TARGET)

                    if(aliased_target)
                        # Source is an alias, create Quarisma alias to the actual target
                        add_library(${quarisma_name} ALIAS ${aliased_target})
                        message(STATUS "Created ${quarisma_name} -> ${source_target} (resolved to ${aliased_target})")
                    else()
                        # Source is a real target, create alias directly
                        add_library(${quarisma_name} ALIAS ${source_target})
                        message(STATUS "Created ${quarisma_name} -> ${source_target}")
                    endif()
                endif()

                set(target_created TRUE)
                break()
            endif()
        endforeach()

        if(NOT target_created)
            message(WARNING "Could not create ${quarisma_name}: none of the source targets exist (${source_targets})")
        endif()
    endforeach()
endfunction()

# =============================================================================
# Main Function: add_third_party_library
# =============================================================================
# Enhanced function to add third-party libraries with comprehensive configuration
# options including flag filtering, automatic target organization, and alias creation
#
# Parameters:
#   name - Name of the third-party library (required)
#
# Optional Parameters:
#   SOURCE_DIR <path>           - Custom source directory path relative to ThirdParty/
#                                 If not specified, uses the library name as the path
#                                 Example: "kineto/libkineto" for nested structures
#   EXTERNAL_NAME <name>        - Name to use when searching for external package
#   CONDITION <variable>        - CMake variable that must be true to add the library
#   LIBRARY_TYPE <type>         - Type of library: STATIC, SHARED, DEFAULT, or HEADER_ONLY
#                                 HEADER_ONLY: Creates INTERFACE library target with include dirs
#                                 Supports ALIASES and QUARISMA_TARGETS for header-only libraries
#                                 If not specified, uses QUARISMA_THIRDPARTY_TYPE
#   LIBRARY_TYPE_VAR <var>      - CMake variable name for library-specific control
#                                 Example: MI_BUILD_STATIC for mimalloc
#                                 Note: Not applicable for HEADER_ONLY libraries
#   FILTER_FLAGS <flags...>     - List of compiler/linker flags to filter out
#                                 Example: "-fno-exceptions" "-Werror"
#                                 Note: Not applicable for HEADER_ONLY libraries (no compilation)
#   DISABLE_WARNINGS            - Boolean flag to disable all compiler warnings
#                                 Adds -w for Clang/GCC and /w for MSVC
#                                 Note: Not applicable for HEADER_ONLY libraries (no compilation)
#   EXTRA_FLAGS <flags...>      - List of additional compiler flags to add
#                                 Example: "-flto=thin" "-march=native"
#                                 Note: Not applicable for HEADER_ONLY libraries (no compilation)
#   ALIASES <aliases...>        - List of alias names to create for the library
#                                 Example: "fmt::fmt-header-only" for fmt library
#                                 Supported for all library types including HEADER_ONLY
#   QUARISMA_TARGETS <specs...>   - List of Quarisma:: namespace target specifications
#                                 Format: "Quarisma::name=target1|target2|..."
#                                 Example: "Quarisma::fmt=fmt::fmt-header-only|fmt"
#                                 Supported for all library types including HEADER_ONLY
#   INCLUDE_DIR <path>          - Include directory path to add to QUARISMA_DEPENDENCY_INCLUDE_DIRS
#                                 Path is relative to CMAKE_CURRENT_SOURCE_DIR (ThirdParty/)
#                                 Example: "fmt/include" for fmt library
#   AUTO_ADD_TO_DEPENDENCIES    - Boolean flag to automatically add to QUARISMA_DEPENDENCY_LIBS
#                                 Default: ON (automatically adds Quarisma::name target)
#
# Example Usage:
#   add_third_party_library(fmt
#       LIBRARY_TYPE STATIC
#       FILTER_FLAGS "-fno-exceptions" "-Werror"
#       ALIASES "fmt::fmt"
#       QUARISMA_TARGETS "Quarisma::fmt=fmt::fmt|fmt"
#   )
#
#   add_third_party_library(kineto
#       CONDITION PROFILER_ENABLE_KINETO
#       LIBRARY_TYPE STATIC
#       SOURCE_DIR kineto/libkineto
#       DISABLE_WARNINGS
#       EXTRA_FLAGS "-flto=thin"
#   )
#
function(add_third_party_library name)
    # Parse function arguments
    cmake_parse_arguments(
        TPL                                                           # Prefix
        "DISABLE_WARNINGS;AUTO_ADD_TO_DEPENDENCIES"       # Options (boolean flags)
        "SOURCE_DIR;EXTERNAL_NAME;CONDITION;LIBRARY_TYPE;LIBRARY_TYPE_VAR;INCLUDE_DIR" # One-value keywords
        "FILTER_FLAGS;EXTRA_FLAGS;ALIASES;QUARISMA_TARGETS"             # Multi-value keywords
        ${ARGN}                                                       # Arguments to parse
    )

    # Set default values for optional parameters
    if(NOT DEFINED TPL_EXTERNAL_NAME)
        set(TPL_EXTERNAL_NAME ${name})
    endif()

    # Default AUTO_ADD_TO_DEPENDENCIES to ON if not explicitly set to OFF
    # Note: cmake_parse_arguments sets boolean options to FALSE when not specified,
    # so we check if it's in the ARGN list to determine if it was explicitly set
    if("AUTO_ADD_TO_DEPENDENCIES" IN_LIST ARGN)
        # Explicitly specified, use the parsed value
    else()
        # Not specified, default to ON
        set(TPL_AUTO_ADD_TO_DEPENDENCIES ON)
    endif()

    # SOURCE_DIR defaults to the library name if not specified
    if(NOT DEFINED TPL_SOURCE_DIR)
        set(TPL_SOURCE_DIR ${name})
    endif()

    # -------------------------------------------------------------------------
    # Step 1: Check if already processed to avoid duplicates
    # -------------------------------------------------------------------------
    get_property(QUARISMA_${name}_PROCESSED GLOBAL PROPERTY QUARISMA_${name}_PROCESSED)
    if(QUARISMA_${name}_PROCESSED)
        message(STATUS "Third-party library ${name} already processed, skipping")
        return()
    endif()
    set_property(GLOBAL PROPERTY QUARISMA_${name}_PROCESSED TRUE)

    # -------------------------------------------------------------------------
    # Step 2: Determine library type
    # -------------------------------------------------------------------------
    # Priority: LIBRARY_TYPE parameter > QUARISMA_THIRDPARTY_TYPE global > DEFAULT
    set(lib_type "DEFAULT")
    if(DEFINED TPL_LIBRARY_TYPE)
        set(lib_type ${TPL_LIBRARY_TYPE})
    elseif(NOT QUARISMA_THIRDPARTY_TYPE STREQUAL "DEFAULT")
        set(lib_type ${QUARISMA_THIRDPARTY_TYPE})
    endif()

    # -------------------------------------------------------------------------
    # Step 3: Early handling for HEADER_ONLY libraries
    # -------------------------------------------------------------------------
    # Header-only libraries don't need compilation, so we handle them early
    # and return to avoid unnecessary external library searches or condition checks
    if(lib_type STREQUAL "HEADER_ONLY")
        message(STATUS "Processing header-only library ${name}")

        # Verify that INCLUDE_DIR is specified for header-only libraries
        if(NOT DEFINED TPL_INCLUDE_DIR)
            message(WARNING "Header-only library ${name} specified but no INCLUDE_DIR provided - skipping")
            return()
        endif()

        # Create an INTERFACE library target for the header-only library
        # This allows it to be used with target_link_libraries() like any other library
        if(NOT TARGET ${name})
            add_library(${name} INTERFACE)
            message(STATUS "Created INTERFACE library target: ${name}")
        endif()

        # Add include directory to the INTERFACE library
        # This will be propagated to any target that links against this library
        target_include_directories(${name} INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/${TPL_INCLUDE_DIR}"
        )
        message(STATUS "Added include directory to ${name}: ${TPL_INCLUDE_DIR}")

        # Create Quarisma:: namespace alias for the INTERFACE library
        if(NOT TARGET Quarisma::${name})
            add_library(Quarisma::${name} ALIAS ${name})
            message(STATUS "Created alias: Quarisma::${name} -> ${name}")
        endif()

        # Add to QUARISMA_DEPENDENCY_LIBS so it can be linked against
        get_property(_temp_libs GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS)
        list(APPEND _temp_libs Quarisma::${name})
        set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS "${_temp_libs}")
        message(STATUS "Dependency: Quarisma::${name} added to QUARISMA_DEPENDENCY_LIBS (header-only)")

        # Add include directory to global include paths for project-wide access
        get_property(_temp_includes GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS)
        list(APPEND _temp_includes "${CMAKE_CURRENT_SOURCE_DIR}/${TPL_INCLUDE_DIR}")
        set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS "${_temp_includes}")
        message(STATUS "Dependency: ${TPL_INCLUDE_DIR} added to QUARISMA_DEPENDENCY_INCLUDE_DIRS (header-only)")

        # =====================================================================
        # Create library aliases if requested (supported for HEADER_ONLY)
        # =====================================================================
        if(TPL_ALIASES)
            _create_third_party_aliases(${name} "${TPL_ALIASES}")
        endif()

        # =====================================================================
        # Create Quarisma:: namespace interface targets if requested (supported for HEADER_ONLY)
        # =====================================================================
        if(TPL_QUARISMA_TARGETS)
            _create_third_party_interface_targets("${TPL_QUARISMA_TARGETS}")
        endif()

        # Early return - no further processing needed for header-only libraries
        return()
    endif()

    # -------------------------------------------------------------------------
    # Step 4: Try to find external library first if requested
    # -------------------------------------------------------------------------
    if(QUARISMA_ENABLE_EXTERNAL)
        find_package(${TPL_EXTERNAL_NAME} QUIET)
        if(${TPL_EXTERNAL_NAME}_FOUND OR ${name}_FOUND)
            message(STATUS "================================================================")
            message(STATUS "Found external ${TPL_EXTERNAL_NAME}")

            # ADD THIS DEBUG OUTPUT:
            message(STATUS "DEBUG: Checking available targets for ${TPL_EXTERNAL_NAME}:")

            # Check common target naming patterns
            foreach(target_suffix "" "::${name}" "::${TPL_EXTERNAL_NAME}" "-static" "-shared")
                set(target_name "${name}${target_suffix}")
                if(TARGET ${target_name})
                    message(STATUS "  Found target: ${target_name}")
                    get_target_property(target_type ${target_name} TYPE)
                    message(STATUS "    Type: ${target_type}")
                endif()
            endforeach()

            # Print all variables set by find_package
            get_cmake_property(_variableNames VARIABLES)
            foreach(_variableName ${_variableNames})
                if(_variableName MATCHES "^${name}" OR _variableName MATCHES "^${TPL_EXTERNAL_NAME}")
                    message(STATUS "  ${_variableName}=${${_variableName}}")
                endif()
            endforeach()

            message(STATUS "================================================================")

            # Create Quarisma:: namespace target using QUARISMA_TARGETS if provided, otherwise use default pattern
            if(TPL_QUARISMA_TARGETS)
                _create_third_party_interface_targets("${TPL_QUARISMA_TARGETS}")
            else()
                _create_third_party_interface_targets(
                    "Quarisma::${name}=${name}::${name}-header-only||${name}::${name}|${name}|${name}-static"
                )
            endif()

            # Automatically populate dependency lists if requested (default: ON)
            if(TARGET Quarisma::${name})
                # Add to QUARISMA_DEPENDENCY_LIBS using global property
                get_property(_temp_libs GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS)
                list(APPEND _temp_libs Quarisma::${name})
                set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS "${_temp_libs}")
                message(STATUS "Dependency: Quarisma::${name} added to QUARISMA_DEPENDENCY_LIBS (external)")
            endif()

            # Add include directory if specified
            # For external libraries, try TPL_INCLUDE_DIR first, then fall back to ${name}_INCLUDE_DIR
            if(DEFINED ${name}_INCLUDE_DIR)
                # Fall back to the include directory set by find_package
                get_property(_temp_includes GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS)
                list(APPEND _temp_includes ${${name}_INCLUDE_DIR})
                set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS "${_temp_includes}")
                message(STATUS "Dependency: ${${name}_INCLUDE_DIR} added to QUARISMA_DEPENDENCY_INCLUDE_DIRS (external)")
            endif()

            return()
        endif()
    endif()

    # -------------------------------------------------------------------------
    # Step 5: Check condition (early return pattern)
    # -------------------------------------------------------------------------
    if(DEFINED TPL_CONDITION)
        if(NOT ${TPL_CONDITION})
            message(STATUS "Skipping ${name}: condition ${TPL_CONDITION} is not met")
            return()
        endif()
    endif()

    message("================================================================")
    message("Build from source ${TPL_EXTERNAL_NAME}")
    message("================================================================")

    # -------------------------------------------------------------------------
    # Step 6: Save current state (BUILD_SHARED_LIBS and flags)
    # -------------------------------------------------------------------------
    set(_saved_build_shared_libs ${BUILD_SHARED_LIBS})

    # Save and optionally filter compiler/linker flags when we plan to mutate them
    set(_TPL_SAVED_FLAGS FALSE)
    set(_tpl_filter_flags "")
    if(TPL_FILTER_FLAGS)
        set(_tpl_filter_flags "${TPL_FILTER_FLAGS}")
    endif()
    if(TPL_FILTER_FLAGS OR TPL_DISABLE_WARNINGS OR TPL_EXTRA_FLAGS)
        _save_third_party_flags(${name} "${_tpl_filter_flags}")
        set(_TPL_SAVED_FLAGS TRUE)
    endif()

    # -------------------------------------------------------------------------
    # Step 6b: Handle DISABLE_WARNINGS and EXTRA_FLAGS
    # -------------------------------------------------------------------------
    if(TPL_DISABLE_WARNINGS)
        # Remove all existing warning-related flags
        # Patterns: -W*, -Wno-*, /wd* (but keep /W* for MSVC warning level)
        if(MSVC)
            # For MSVC: remove only /wd* (specific warning disables) and -W* flags
            _filter_flags_internal("${CMAKE_CXX_FLAGS}" "-W.*;/wd.*" filtered_cxx_flags_no_warnings)
            _filter_flags_internal("${CMAKE_C_FLAGS}" "-W.*;/wd.*" filtered_c_flags_no_warnings)

            set(CMAKE_CXX_FLAGS "${filtered_cxx_flags_no_warnings}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${filtered_c_flags_no_warnings}")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)

            # Add /W0 to disable all warnings while keeping the warning level infrastructure
            string(APPEND CMAKE_CXX_FLAGS " /W0")
            string(APPEND CMAKE_C_FLAGS " /W0")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        else()
            # For Clang/GCC: remove all -W* and -Wno-* flags
            _filter_flags_internal("${CMAKE_CXX_FLAGS}" "-W.*;-Wno-.*" filtered_cxx_flags_no_warnings)
            _filter_flags_internal("${CMAKE_C_FLAGS}" "-W.*;-Wno-.*" filtered_c_flags_no_warnings)

            set(CMAKE_CXX_FLAGS "${filtered_cxx_flags_no_warnings}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${filtered_c_flags_no_warnings}")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)

            # Add -w to disable all warnings
            string(APPEND CMAKE_CXX_FLAGS " -w")
            string(APPEND CMAKE_C_FLAGS " -w")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        endif()
        message(STATUS "Disabled all warnings for ${name} (removed existing warning flags and added suppression)")
    endif()

    # Add extra compiler flags if provided
    if(TPL_EXTRA_FLAGS)
        foreach(flag ${TPL_EXTRA_FLAGS})
            string(APPEND CMAKE_CXX_FLAGS " ${flag}")
            string(APPEND CMAKE_C_FLAGS " ${flag}")
        endforeach()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" PARENT_SCOPE)
        message(STATUS "Added extra flags for ${name}: ${TPL_EXTRA_FLAGS}")
    endif()

    # -------------------------------------------------------------------------
    # Step 7: Configure library type (STATIC or SHARED)
    # -------------------------------------------------------------------------
    # Note: HEADER_ONLY is handled earlier in Step 3 and returns early
    if(lib_type STREQUAL "STATIC")
        set(BUILD_SHARED_LIBS OFF)
        message(STATUS "Configuring ${name} as STATIC library")

        # Set library-specific variable if provided
        if(DEFINED TPL_LIBRARY_TYPE_VAR)
            set(${TPL_LIBRARY_TYPE_VAR} ON CACHE BOOL "Build ${name} as static library" FORCE)
        endif()
    elseif(lib_type STREQUAL "SHARED")
        set(BUILD_SHARED_LIBS ON)
        message(STATUS "Configuring ${name} as SHARED library")

        # Set library-specific variable if provided
        if(DEFINED TPL_LIBRARY_TYPE_VAR)
            set(${TPL_LIBRARY_TYPE_VAR} OFF CACHE BOOL "Build ${name} as shared library" FORCE)
        endif()
    else()
        message(STATUS "Configuring ${name} with DEFAULT library type")
    endif()

    # -------------------------------------------------------------------------
    # Step 8: Add the third-party library subdirectory
    # -------------------------------------------------------------------------
        # Support both standard (SOURCE_DIR/CMakeLists.txt) and nested (SOURCE_DIR/subdir/CMakeLists.txt) structures
        set(lib_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${TPL_SOURCE_DIR}")
        set(lib_cmakelists_path "${lib_source_dir}/CMakeLists.txt")

        # If CMakeLists.txt doesn't exist at the top level, check if it's in a subdirectory
        # This handles cases like kineto/libkineto where the actual CMakeLists.txt is nested
        if(NOT EXISTS "${lib_cmakelists_path}")
            # Try to find CMakeLists.txt in immediate subdirectories
            file(GLOB subdirs LIST_DIRECTORIES true "${lib_source_dir}/*")
            foreach(subdir ${subdirs})
                if(IS_DIRECTORY "${subdir}")
                    get_filename_component(subdir_name "${subdir}" NAME)
                    set(potential_cmakelists "${subdir}/CMakeLists.txt")
                    if(EXISTS "${potential_cmakelists}")
                        set(lib_source_dir "${subdir}")
                        set(lib_cmakelists_path "${potential_cmakelists}")
                        message(STATUS "Found CMakeLists.txt in nested directory: ${subdir_name}")
                        break()
                    endif()
                endif()
            endforeach()
        endif()

        if(EXISTS "${lib_cmakelists_path}")
            message(STATUS "Using bundled ${name}")

            # Store the binary directory for later use
            # Use the library name (not the source path) for the binary directory
            set(lib_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}_build")

            # Add the library
            add_subdirectory(${lib_source_dir} ${lib_binary_dir})

            # -------------------------------------------------------------------------
            # Step 9: Automatically set FOLDER properties for IDE organization
            # -------------------------------------------------------------------------
            _set_third_party_folder_properties(${name} "${lib_binary_dir}")

            # -------------------------------------------------------------------------
            # Step 10: Create library aliases if requested
            # -------------------------------------------------------------------------
            if(TPL_ALIASES)
                _create_third_party_aliases(${name} "${TPL_ALIASES}")
            endif()

            # -------------------------------------------------------------------------
            # Step 11: Create Quarisma:: namespace interface targets if requested
            # -------------------------------------------------------------------------
            if(TPL_QUARISMA_TARGETS)
                _create_third_party_interface_targets("${TPL_QUARISMA_TARGETS}")
            endif()

            # -------------------------------------------------------------------------
            # Step 12: Automatically populate dependency lists
            # -------------------------------------------------------------------------
            # This step automatically adds the library to QUARISMA_DEPENDENCY_LIBS and
            # QUARISMA_DEPENDENCY_INCLUDE_DIRS if requested (default: ON)

            if(TPL_AUTO_ADD_TO_DEPENDENCIES)
                # Check if the Quarisma:: namespace target was created
                if(TARGET Quarisma::${name})
                    # Add to QUARISMA_DEPENDENCY_LIBS using global property
                    # We use global properties because PARENT_SCOPE doesn't update the variable
                    # in the current scope, so subsequent function calls wouldn't see the changes
                    get_property(_temp_libs GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS)
                    list(APPEND _temp_libs Quarisma::${name})
                    set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS "${_temp_libs}")
                    message(STATUS "Dependency: Quarisma::${name} added to QUARISMA_DEPENDENCY_LIBS")
                endif()

                # Add include directory if specified
                if(DEFINED TPL_INCLUDE_DIR)
                    # Add to QUARISMA_DEPENDENCY_INCLUDE_DIRS using global property
                    get_property(_temp_includes GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS)
                    list(APPEND _temp_includes "${CMAKE_CURRENT_SOURCE_DIR}/${TPL_INCLUDE_DIR}")
                    set_property(GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS "${_temp_includes}")
                    message(STATUS "Dependency: ${TPL_INCLUDE_DIR} added to QUARISMA_DEPENDENCY_INCLUDE_DIRS")
                endif()
            endif()
        else()
            message(WARNING "Third-party library ${name} not found in ${CMAKE_CURRENT_SOURCE_DIR}/${TPL_SOURCE_DIR}")
        endif()

    # -------------------------------------------------------------------------
    # Step 13: Restore original state
    # -------------------------------------------------------------------------
    # Restore BUILD_SHARED_LIBS
    set(BUILD_SHARED_LIBS ${_saved_build_shared_libs})

    # Restore compiler/linker flags if they were filtered
    if(_TPL_SAVED_FLAGS)
        _restore_third_party_flags(${name})
    endif()
endfunction()

# =============================================================================
# Third-Party Library Declarations
# =============================================================================

# Add third-party libraries conditionally based on QUARISMA_ENABLE_XXX options

# fmt - Modern formatting library (header-only mode)
# Using header-only version to avoid shared library issues and ensure compatibility with Kineto
# Creates INTERFACE library target with Quarisma::fmt alias and fmt::fmt-header-only alias
# for Kineto compatibility (Kineto checks for this target to determine if fmt is available)
add_third_party_library(fmt
    LIBRARY_TYPE STATIC
    ALIASES "fmt::fmt-header-only"
    QUARISMA_TARGETS "Quarisma::fmt=fmt::fmt-header-only|fmt::fmt|fmt"
    INCLUDE_DIR "fmt/include"
)

# cpuinfo - CPU feature detection library (mandatory dependency)
add_third_party_library(cpuinfo
    LIBRARY_TYPE STATIC
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::cpuinfo=cpuinfo::cpuinfo|cpuinfo"
    INCLUDE_DIR "cpuinfo/include"
)

# magic_enum - Static reflection for enums in C++
# Header-only library with INTERFACE target and Quarisma::magic_enum alias
# Note: FILTER_FLAGS, DISABLE_WARNINGS, EXTRA_FLAGS, and LIBRARY_TYPE_VAR are not applicable
#       (no compilation occurs for header-only libraries)
add_third_party_library(magic_enum
    CONDITION QUARISMA_ENABLE_MAGICENUM
    LIBRARY_TYPE HEADER_ONLY
    INCLUDE_DIR "magic_enum/include"
)

# loguru - Lightweight C++ logging library (forced to static)
add_third_party_library(loguru
    CONDITION QUARISMA_ENABLE_LOGURU
    LIBRARY_TYPE STATIC
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::loguru=loguru::loguru|loguru"
    INCLUDE_DIR "loguru"
)

# glog - Google logging library
add_third_party_library(glog
    CONDITION QUARISMA_ENABLE_GLOG
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::glog=glog::glog|glog"
    INCLUDE_DIR "glog/src"
)

# mimalloc - Microsoft's high-performance memory allocator (conditional)
# By default builds as static library, controlled by LIBRARY_TYPE parameter
add_third_party_library(mimalloc
    CONDITION QUARISMA_ENABLE_MIMALLOC
    LIBRARY_TYPE STATIC
    LIBRARY_TYPE_VAR MI_BUILD_STATIC
    FILTER_FLAGS "-Wno-strict-prototypes" "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::mimalloc=mimalloc|mimalloc-static"
    INCLUDE_DIR "mimalloc/include"
)

# mimalloc adds Windows system libraries as bare names (e.g. "psapi") via
# PRIVATE target_link_libraries.  MinGW's linker treats bare names as file
# paths; they must have the -l prefix.
#
# CMake stores PRIVATE deps of static libraries in two forms:
#   LINK_LIBRARIES          : plain "psapi"
#   INTERFACE_LINK_LIBRARIES: "$<LINK_ONLY:psapi>"  (generator expression)
# Both must be patched; consumers are generated from INTERFACE_LINK_LIBRARIES.
if(WIN32 AND (MINGW OR CMAKE_CXX_COMPILER MATCHES "msys64") AND TARGET mimalloc-static)
  set(_mi_win_syslibs psapi shell32 user32 advapi32 bcrypt)

  # --- patch LINK_LIBRARIES (plain names) ---
  get_target_property(_mi_ll mimalloc-static LINK_LIBRARIES)
  if(_mi_ll)
    set(_changed FALSE)
    foreach(_lib IN LISTS _mi_win_syslibs)
      if("${_lib}" IN_LIST _mi_ll)
        list(REMOVE_ITEM _mi_ll "${_lib}")
        list(APPEND     _mi_ll "-l${_lib}")
        set(_changed TRUE)
      endif()
    endforeach()
    if(_changed)
      set_property(TARGET mimalloc-static PROPERTY LINK_LIBRARIES "${_mi_ll}")
      message(STATUS "mimalloc: patched LINK_LIBRARIES for MinGW (-l prefix)")
    endif()
  endif()

  # --- patch INTERFACE_LINK_LIBRARIES ($<LINK_ONLY:name> generator exprs) ---
  get_target_property(_mi_il mimalloc-static INTERFACE_LINK_LIBRARIES)
  if(_mi_il)
    set(_changed FALSE)
    foreach(_lib IN LISTS _mi_win_syslibs)
      # bare form (unlikely here, but be safe)
      if("${_lib}" IN_LIST _mi_il)
        list(REMOVE_ITEM _mi_il "${_lib}")
        list(APPEND     _mi_il "-l${_lib}")
        set(_changed TRUE)
      endif()
      # generator-expression form produced by CMake for PRIVATE deps
      set(_ge  "$<LINK_ONLY:${_lib}>")
      set(_ge2 "$<LINK_ONLY:-l${_lib}>")
      if("${_ge}" IN_LIST _mi_il)
        list(REMOVE_ITEM _mi_il "${_ge}")
        list(APPEND     _mi_il "${_ge2}")
        set(_changed TRUE)
      endif()
    endforeach()
    if(_changed)
      set_property(TARGET mimalloc-static PROPERTY INTERFACE_LINK_LIBRARIES "${_mi_il}")
      message(STATUS "mimalloc: patched INTERFACE_LINK_LIBRARIES for MinGW (-l prefix)")
    endif()
  endif()
endif()

# Kineto - Quarisma profiling library
# Aligned with Quarisma's PROFILER_ENABLE_KINETO configuration for feature parity.
# Supports GPU backends: CUPTI (NVIDIA), ROCtracer (AMD), XPUPTI (Intel XPU), AIUPTI (AI accelerators)

# Configure GPU backend support based on enabled platforms
set(LIBKINETO_NOAIUPTI ON CACHE STRING "" FORCE)
if(NOT QUARISMA_ENABLE_CUDA)
    set(LIBKINETO_NOCUPTI ON CACHE STRING "" FORCE)
else()
    set(LIBKINETO_NOCUPTI OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with CUPTI support")
endif()

if(NOT QUARISMA_ENABLE_ROCM)
    set(LIBKINETO_NOROCTRACER ON CACHE STRING "" FORCE)
else()
    set(LIBKINETO_NOROCTRACER OFF CACHE STRING "" FORCE)
    message(STATUS "Using Kineto with Roctracer support")
endif()

if(LIBKINETO_NOCUPTI AND LIBKINETO_NOROCTRACER AND LIBKINETO_NOXPUPTI)
    message(STATUS "Using CPU-only version of Kineto")
endif()

# Configure Kineto library
set(KINETO_BUILD_TESTS OFF CACHE BOOL "")
set(KINETO_LIBRARY_TYPE "static" CACHE STRING "")

# Tell Kineto to use Quarisma's fmt library instead of its embedded one
# This prevents duplicate symbol errors when linking
# Set FMT_SOURCE_DIR to Quarisma's fmt directory so Kineto can build from it
# This ensures Kineto uses the same fmt version as Quarisma
set(FMT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fmt" CACHE STRING "Use Quarisma's fmt library" FORCE)

message(STATUS "Configuring Kineto dependency:")
message(STATUS "  KINETO_BUILD_TESTS = ${KINETO_BUILD_TESTS}")
message(STATUS "  KINETO_LIBRARY_TYPE = ${KINETO_LIBRARY_TYPE}")
message(STATUS "  FMT_SOURCE_DIR = ${FMT_SOURCE_DIR} (using Quarisma's fmt)")

add_third_party_library(kineto
    CONDITION PROFILER_ENABLE_KINETO
    LIBRARY_TYPE STATIC
    SOURCE_DIR kineto/libkineto
    DISABLE_WARNINGS
    FILTER_FLAGS "-flto=thin" "-flto=full"
    QUARISMA_TARGETS "Quarisma::kineto=kineto"
    INCLUDE_DIR "kineto/libkineto/include"
    AUTO_ADD_TO_DEPENDENCIES OFF  # Kineto has special handling in manual section
)

# Disable Kineto if the library was not found
if(PROFILER_ENABLE_KINETO AND NOT TARGET Quarisma::kineto)
    message(STATUS "Kineto library not found - disabling PROFILER_ENABLE_KINETO")
    set(PROFILER_ENABLE_KINETO OFF CACHE BOOL "Enable Quarisma Kineto profiling library." FORCE)
endif()

# =============================================================================
# Google Test and Google Benchmark Integration
# =============================================================================

# Note: googletest and benchmark require special handling due to EXCLUDE_FROM_ALL
# and MSVC-specific settings, so they use manual add_subdirectory calls

# Google Test - Unit testing framework
if(QUARISMA_ENABLE_GTEST)
    # Check if gtest target already exists (avoid duplicate add_subdirectory)
    if(NOT TARGET gtest)
        set(gtest_external_found FALSE)
        if(QUARISMA_ENABLE_EXTERNAL)
            find_package(GTest QUIET)

            if(GTest_FOUND)
                message(STATUS "Found external Google Test")
                # Create Quarisma:: namespace aliases for googletest
                _create_third_party_interface_targets(
                    "Quarisma::gtest=GTest::gtest|gtest"
                    "Quarisma::gtest_main=GTest::gtest_main|gtest_main"
                )
                set(gtest_external_found TRUE)
            endif()
        endif()

        if(NOT gtest_external_found)
            message(STATUS "Adding Google Test from ThirdParty/googletest")

            # Set options before adding subdirectory
            # Force shared CRT on MSVC to match Quarisma's runtime library settings
            if(MSVC)
                set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            endif()

            # Add googletest subdirectory
            # This creates targets: gtest, gtest_main, gmock, gmock_main
            add_subdirectory(
                "${CMAKE_CURRENT_SOURCE_DIR}/googletest"
                "${CMAKE_CURRENT_BINARY_DIR}/googletest"
                EXCLUDE_FROM_ALL
            )

            # Set folder properties for all googletest targets
            _set_third_party_folder_properties("googletest" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

            message(STATUS "Google Test targets created: gtest, gtest_main")
        endif()
    else()
        message(STATUS "Google Test targets already exist, skipping add_subdirectory")
    endif()

    # Create Quarisma:: namespace aliases for googletest
    _create_third_party_interface_targets(
        "Quarisma::gtest=GTest::gtest|gtest"
        "Quarisma::gtest_main=GTest::gtest_main|gtest_main"
    )
endif()

# Google Benchmark - Microbenchmarking framework
if(QUARISMA_ENABLE_BENCHMARK)
    # Check if benchmark target already exists (avoid duplicate add_subdirectory)
    if(NOT TARGET benchmark)
        set(benchmark_external_found FALSE)
        if(QUARISMA_ENABLE_EXTERNAL)
            find_package(benchmark QUIET)

            if(benchmark_FOUND)
                message("Found external Google Benchmark")
                # Create Quarisma:: namespace alias for benchmark
                _create_third_party_interface_targets(
                    "Quarisma::benchmark=benchmark::benchmark|benchmark"
                    "Quarisma::benchmark_main=benchmark::benchmark_main|benchmark_main"
                )
                set(benchmark_external_found TRUE)
            endif()
        endif()

        if(NOT benchmark_external_found)
            message(STATUS "Adding Google Benchmark from ThirdParty/benchmark")

            # Add benchmark subdirectory
            # This creates targets: benchmark, benchmark_main
            add_subdirectory(
                "${CMAKE_CURRENT_SOURCE_DIR}/benchmark"
                "${CMAKE_CURRENT_BINARY_DIR}/benchmark"
                EXCLUDE_FROM_ALL
            )

            # Set folder properties for all benchmark targets
            _set_third_party_folder_properties("benchmark" "${CMAKE_CURRENT_BINARY_DIR}/benchmark")

            message(STATUS "Google Benchmark targets created: benchmark, benchmark_main")
        endif()
    else()
        message(STATUS "Google Benchmark targets already exist, skipping add_subdirectory")
    endif()

    # Create Quarisma:: namespace alias for benchmark
    _create_third_party_interface_targets(
        "Quarisma::benchmark=benchmark::benchmark|benchmark"
        "Quarisma::benchmark_main=benchmark::benchmark_main|benchmark_main"
    )
endif()

# -----------------------------------------------------------------------------
# ITT - Intel VTune API (External System Dependency)
# -----------------------------------------------------------------------------
# ITT is an external system dependency that uses find_package, not bundled
# in ThirdParty, so it's handled separately here.
if(PROFILER_ENABLE_ITT)
  find_package(ITT)
  if(ITT_FOUND)
    list(APPEND QUARISMA_DEPENDENCY_LIBS ${ITT_LIBRARIES})
    list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS ${ITT_INCLUDE_DIR})
    message(STATUS "Dependency: ITT libraries added to QUARISMA_DEPENDENCY_LIBS")
  else()
    message(STATUS "ITT API not found - disabling PROFILER_ENABLE_ITT")
    set(PROFILER_ENABLE_ITT OFF CACHE BOOL "Enable Intel ITT API for VTune profiling." FORCE)
  endif()
endif()

# -----------------------------------------------------------------------------
# TBB - Threading Building Blocks (Handled by tbb.cmake)
# -----------------------------------------------------------------------------
# TBB is configured by tbb.cmake (included before ThirdParty subdirectory)
# which creates TBB::tbb and TBB::tbbmalloc targets. We add them to the
# dependency list here.
if(QUARISMA_ENABLE_TBB)
  if(TARGET Quarisma::tbb)
    list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::tbb)
    message(STATUS "Dependency: TBB::tbb added to QUARISMA_DEPENDENCY_LIBS")
  endif()

  if(TARGET Quarisma::tbbmalloc)
    list(APPEND QUARISMA_DEPENDENCY_LIBS Quarisma::tbbmalloc)
    message(STATUS "Dependency: TBB::tbbmalloc added to QUARISMA_DEPENDENCY_LIBS")
  endif()
endif()

# -----------------------------------------------------------------------------
# Enzyme - Automatic Differentiation (Handled by enzyme.cmake)
# -----------------------------------------------------------------------------
# Enzyme is configured by enzyme.cmake (included before ThirdParty subdirectory)
# which creates Quarisma::enzyme interface target.
# NOTE: Enzyme is NOT added to QUARISMA_DEPENDENCY_LIBS because it should only be applied
# to specific source files that use it (via set_source_files_properties), not globally.
# The Enzyme plugin can cause compilation crashes on files that don't use it.
if(QUARISMA_ENABLE_ENZYME)
  if(TARGET Quarisma::enzyme)
    message(STATUS "Enzyme: Quarisma::enzyme target available (apply selectively to source files)")
  endif()
endif()

# -----------------------------------------------------------------------------
# Propagate dependency lists to parent scope
# -----------------------------------------------------------------------------
# Read from global properties (populated by add_third_party_library function)
# and set as regular variables in parent scope for use by the main build system

# Read from global properties and merge with any manually added dependencies
get_property(_global_libs GLOBAL PROPERTY _QUARISMA_DEPENDENCY_LIBS)
get_property(_global_includes GLOBAL PROPERTY _QUARISMA_DEPENDENCY_INCLUDE_DIRS)

if(_global_libs)
    list(APPEND QUARISMA_DEPENDENCY_LIBS ${_global_libs})
endif()
if(_global_includes)
    list(APPEND QUARISMA_DEPENDENCY_INCLUDE_DIRS ${_global_includes})
endif()

# Propagate to parent scope
set(QUARISMA_DEPENDENCY_LIBS "${QUARISMA_DEPENDENCY_LIBS}" PARENT_SCOPE)
set(QUARISMA_DEPENDENCY_INCLUDE_DIRS "${QUARISMA_DEPENDENCY_INCLUDE_DIRS}" PARENT_SCOPE)

message(STATUS "ThirdParty: QUARISMA_DEPENDENCY_LIBS populated with: ${QUARISMA_DEPENDENCY_LIBS}")
message(STATUS "ThirdParty: QUARISMA_DEPENDENCY_INCLUDE_DIRS populated with: ${QUARISMA_DEPENDENCY_INCLUDE_DIRS}")
