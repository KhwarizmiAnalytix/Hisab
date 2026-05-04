# =============================================================================
# Quarisma Per-Target LTO Module
#
# Provides two public functions:
#
# quarisma_lto_compute_default(<out_var> [PROFILE <PERFORMANCE|SUPPORT>])
#   Computes a smart LTO default for the calling module and stores it in
#   <out_var>.  Call this BEFORE the set(XXX_LTO_MODE CACHE ...) declaration
#   so the computed value becomes the first-configure default.
#
#   PERFORMANCE (default) — hot-path libraries (Vectorization, Core, Memory, Parallel):
#     Debug build         → off   (LTO wastes compile time with no runtime benefit)
#     Clang (non-MSVC)    → thin  (ThinLTO: parallel, incremental, low-overhead)
#     GCC                 → ipo   (CMake-managed monolithic LTO)
#     MSVC                → ipo   (/GL /LTCG via INTERPROCEDURAL_OPTIMIZATION)
#     Multi-config / unknown → off (conservative; user can override per-config)
#
#   SUPPORT — tool libraries (Logging, Profiler):
#     Always → off.  Logging backend templates (spdlog, loguru) and profiler
#     instrumentation are sensitive to symbol visibility changes that LTO can
#     introduce; a conservative default avoids hard-to-diagnose link failures.
#
# quarisma_target_apply_lto(<target> <prefix>)
#   Applies the strategy selected by ${prefix}_LTO_MODE to <target>'s
#   compile/link options as PRIVATE properties.  Never writes
#   CMAKE_INTERPROCEDURAL_OPTIMIZATION (which pollutes all subsequent targets).
#
# LTO modes (set via -D<prefix>_LTO_MODE=<mode>):
#   off   — no LTO
#   thin  — Clang ThinLTO (-flto=thin): parallel, incremental
#   full  — monolithic LTO (-flto / /GL+/LTCG)
#   ipo   — CMake-managed IPO via INTERPROCEDURAL_OPTIMIZATION target property
#   auto  — thin on Clang (non-MSVC), ipo on GCC/MSVC
#
# Incompatibility guards:
#   coverage OR sanitizer ON  → LTO skipped (STATUS message)
#   sccache RULE_LAUNCH_COMPILE → FATAL_ERROR (sccache rejects LTO objects)
#   compiler flag unsupported  → WARNING + fallback to ipo
#
# Typical usage:
#
#   include(lto)
#   quarisma_lto_compute_default(_lto_default PROFILE PERFORMANCE)
#   set(FOO_LTO_MODE "${_lto_default}" CACHE STRING "LTO mode for Foo: off|thin|full|ipo|auto")
#   set_property(CACHE FOO_LTO_MODE PROPERTY STRINGS off thin full ipo auto)
#   ...
#   add_library(Foo ...)
#   quarisma_target_apply_lto(Foo FOO)
# =============================================================================
include_guard(GLOBAL)

include(CheckIPOSupported)
include(CheckCXXCompilerFlag)

# quarisma_lto_compute_default(<out_var> [PROFILE <PERFORMANCE|SUPPORT>])
#
# Stores the recommended first-configure LTO mode in <out_var>.
# Does NOT set any cache variable — the caller does that with the result.
function(quarisma_lto_compute_default out_var)
  cmake_parse_arguments(_lto_def "" "PROFILE" "" ${ARGN})

  # SUPPORT libraries: off unconditionally (see header).
  if(_lto_def_PROFILE STREQUAL "SUPPORT")
    set(${out_var} "off" PARENT_SCOPE)
    return()
  endif()

  # Multi-config generators (VS, Xcode) leave CMAKE_BUILD_TYPE empty; we cannot
  # distinguish Debug from Release at configure time → stay conservative.
  if(NOT CMAKE_BUILD_TYPE)
    set(${out_var} "off" PARENT_SCOPE)
    return()
  endif()

  string(TOUPPER "${CMAKE_BUILD_TYPE}" _bt)

  # Debug builds: LTO adds link time with zero runtime benefit.
  if(_bt STREQUAL "DEBUG")
    set(${out_var} "off" PARENT_SCOPE)
    return()
  endif()

  # Release / RelWithDebInfo / MinSizeRel — pick the best mode for this toolchain.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    set(${out_var} "thin" PARENT_SCOPE)   # ThinLTO: fast, incremental, works with lld/mold
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(${out_var} "ipo" PARENT_SCOPE)    # GCC CMake-managed LTO (-flto via IPO property)
  elseif(MSVC)
    set(${out_var} "ipo" PARENT_SCOPE)    # /GL /LTCG via CMake INTERPROCEDURAL_OPTIMIZATION
  else()
    set(${out_var} "off" PARENT_SCOPE)    # Unknown toolchain: safe default
  endif()
endfunction()

# quarisma_target_apply_lto(<target> <prefix>)
#   target — CMake target name (e.g. Core, Logging, Vectorization)
#   prefix — variable prefix for this module  (e.g. CORE, LOGGING, VECTORIZATION)
#
# Reads ${prefix}_LTO_MODE and applies the appropriate strategy.
# Never sets CMAKE_INTERPROCEDURAL_OPTIMIZATION.
function(quarisma_target_apply_lto target prefix)
  set(_lto_mode "${${prefix}_LTO_MODE}")
  string(TOLOWER "${_lto_mode}" _lto_mode)

  if(_lto_mode STREQUAL "off" OR _lto_mode STREQUAL "")
    message(STATUS "${target}: LTO off")
    return()
  endif()

  # Guard: coverage or sanitizer — instrumentation is incompatible with LTO
  if(${prefix}_ENABLE_COVERAGE OR ${prefix}_ENABLE_SANITIZER)
    message(STATUS "${target}: LTO skipped — incompatible with coverage/sanitizer")
    return()
  endif()

  # Guard: sccache cannot cache LTO IR object files
  get_target_property(_lto_launch "${target}" RULE_LAUNCH_COMPILE)
  if(_lto_launch MATCHES "sccache")
    message(
      FATAL_ERROR
      "${target}: ${prefix}_LTO_MODE=${${prefix}_LTO_MODE} is incompatible with sccache. "
      "Set ${prefix}_CACHE_BACKEND=none or ${prefix}_LTO_MODE=off."
    )
  endif()

  # Resolve auto → preferred mode for this compiler
  if(_lto_mode STREQUAL "auto")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
      set(_lto_mode "thin")
    else()
      set(_lto_mode "ipo")
    endif()
  endif()

  # --- thin ---
  if(_lto_mode STREQUAL "thin")
    check_cxx_compiler_flag("-flto=thin" _quarisma_lto_thin_ok)
    if(_quarisma_lto_thin_ok)
      target_compile_options("${target}" PRIVATE -flto=thin)
      target_link_options("${target}" PRIVATE -flto=thin)
      message(STATUS "${target}: ThinLTO enabled (-flto=thin)")
      return()
    else()
      message(WARNING "${target}: -flto=thin not supported by compiler — falling back to ipo")
      set(_lto_mode "ipo")
    endif()
  endif()

  # --- full ---
  if(_lto_mode STREQUAL "full")
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      target_compile_options("${target}" PRIVATE /GL)
      target_link_options("${target}" PRIVATE /LTCG)
      message(STATUS "${target}: Full LTO enabled (/GL /LTCG)")
      return()
    else()
      check_cxx_compiler_flag("-flto" _quarisma_lto_full_ok)
      if(_quarisma_lto_full_ok)
        target_compile_options("${target}" PRIVATE -flto)
        target_link_options("${target}" PRIVATE -flto)
        message(STATUS "${target}: Full LTO enabled (-flto)")
        return()
      else()
        message(WARNING "${target}: -flto not supported by compiler — falling back to ipo")
        set(_lto_mode "ipo")
      endif()
    endif()
  endif()

  # --- ipo (CMake-managed IPO, target-scoped) ---
  if(_lto_mode STREQUAL "ipo")
    check_ipo_supported(RESULT _quarisma_ipo_ok OUTPUT _quarisma_ipo_err LANGUAGES CXX)
    if(_quarisma_ipo_ok)
      set_target_properties("${target}" PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
      message(STATUS "${target}: IPO/LTO enabled (INTERPROCEDURAL_OPTIMIZATION target property)")
    else()
      message(WARNING "${target}: IPO not supported by this toolchain: ${_quarisma_ipo_err}")
    endif()
    return()
  endif()

  message(
    WARNING
    "${target}: Unknown ${prefix}_LTO_MODE value '${${prefix}_LTO_MODE}' — LTO skipped. "
    "Valid values: off thin full ipo auto"
  )
endfunction()
