# =============================================================================
# Quarisma
# Clang-Tidy Static Analysis Configuration Module

# This module configures clang-tidy for static code analysis and automated fixes. It enables code
# quality checks and optional automatic error correction.

# Include guard to prevent multiple inclusions
include_guard(GLOBAL)

# Quarisma ClangTidy Configuration
find_program(CLANG_TIDY_PATH NAMES clang-tidy DOC "Path to clang-tidy.")

if(NOT CLANG_TIDY_PATH)
  message(FATAL_ERROR "Could not find clang-tidy.")
endif()
set(CLANG_TIDY_FOUND ON CACHE BOOL "Found clang-tidy.")
mark_as_advanced(CLANG_TIDY_FOUND)

# enable_fix — pass the caller's XXX_ENABLE_FIX variable value as the second argument. WARNING: fix
# mode modifies source files. Use with caution in version control.
function(quarisma_target_clang_tidy target_name enable_fix)
  set(QUARISMA_CLANG_TIDY_HEADER_FILTER "^${PROJECT_SOURCE_DIR}/(Library|Cmake|Tools|Examples)/.*")
  set(QUARISMA_CLANG_TIDY_EXCLUDE_FILTER ".*/(ThirdParty|third_party|3rdparty|third-party)/.*")

  if(enable_fix)
    message(WARNING "Applying clang-tidy fix to target: ${target_name}")
    set_target_properties(
      ${target_name}
      PROPERTIES
        C_CLANG_TIDY
        "${CLANG_TIDY_PATH};-fix-errors;-fix;-warnings-as-errors=*;--header-filter=${QUARISMA_CLANG_TIDY_HEADER_FILTER};--exclude-header-filter=${QUARISMA_CLANG_TIDY_EXCLUDE_FILTER}"
        CXX_CLANG_TIDY
        "${CLANG_TIDY_PATH};-fix-errors;-fix;-warnings-as-errors=*;--header-filter=${QUARISMA_CLANG_TIDY_HEADER_FILTER};--exclude-header-filter=${QUARISMA_CLANG_TIDY_EXCLUDE_FILTER}"
    )
  else()
    set_target_properties(
      ${target_name}
      PROPERTIES
        C_CLANG_TIDY
        "${CLANG_TIDY_PATH};-warnings-as-errors=*;--header-filter=${QUARISMA_CLANG_TIDY_HEADER_FILTER};--exclude-header-filter=${QUARISMA_CLANG_TIDY_EXCLUDE_FILTER}"
        CXX_CLANG_TIDY
        "${CLANG_TIDY_PATH};-warnings-as-errors=*;--header-filter=${QUARISMA_CLANG_TIDY_HEADER_FILTER};--exclude-header-filter=${QUARISMA_CLANG_TIDY_EXCLUDE_FILTER}"
    )
  endif()
endfunction()

function(disable_clang_tidy_for_target target_name)
  set_target_properties(${target_name} PROPERTIES C_CLANG_TIDY "" CXX_CLANG_TIDY "")
endfunction()
