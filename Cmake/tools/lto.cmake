# =============================================================================
# Quarisma Per-Target LTO + Linker Module
#
# LTO API:
#
# quarisma_lto_compute_default(<out_var> [PROFILE <PERFORMANCE|SUPPORT>])
#   Computes a smart LTO default for the calling module and stores it in
#   <out_var>.  Call this BEFORE the set(XXX_LTO_MODE CACHE ...) declaration
#   so the computed value becomes the first-configure default.
#
#   PERFORMANCE (default) — hot-path libraries (Vectorization, Core, Memory, Parallel):
#     Debug build         → off   (LTO wastes compile time with no runtime benefit)
#     Clang (GNU driver)  → thin  (ThinLTO: Linux, macOS, and Windows clang++ + Ninja)
#     GCC                 → ipo   (CMake-managed monolithic LTO)
#     MSVC (cl)           → ipo   (/GL /LTCG via INTERPROCEDURAL_OPTIMIZATION)
#     clang-cl            → ipo   (MSVC link.exe / LTCG stack; same as cl for defaults)
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
#   auto  — thin on GNU Clang, ipo on GCC / cl / clang-cl
#
# Linker API:
#
# quarisma_find_linker(<linker_choice> <target_name> [lto_mode_var])
#   Selects a linker compatible with the active toolchain and LTO mode.
#   When LTO is off, prefers fast linkers (mold, gold, lld) as before.
#   When LTO is on, MUST NOT leave the default Linux bfd ld with Clang Thin/full
#   LTO (unsupported IR) — prefers ld.lld, then mold, then gold.
#   Windows: cl / clang-cl use the MSVC link stack (no -fuse-ld).  Windows GNU
#   Clang (+ Ninja) uses lld like Unix (-fuse-ld=lld).
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
#   quarisma_find_linker(${FOO_LINKER_CHOICE} Foo FOO_LTO_MODE)
# =============================================================================
include_guard(GLOBAL)

include(CheckIPOSupported)
include(CheckCXXCompilerFlag)

# True when Windows builds use cl or clang-cl (MSVC linker / LTCG), not GNU clang++.
function(_quarisma_windows_msvc_link_stack out_var)
  set(_result FALSE)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      set(_result TRUE)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(_result TRUE)
      elseif(
        CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL ""
        AND CMAKE_CXX_COMPILER MATCHES "clang-cl"
      )
        set(_result TRUE)
      endif()
    endif()
  endif()
  set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

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
  _quarisma_windows_msvc_link_stack(_q_wmsvc_link)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(_q_wmsvc_link)
      set(${out_var} "ipo" PARENT_SCOPE)   # clang-cl → link.exe / CMake IPO
    else()
      set(${out_var} "thin" PARENT_SCOPE) # GNU Clang (incl. Windows clang++ + Ninja)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(${out_var} "ipo" PARENT_SCOPE)    # GCC CMake-managed LTO (-flto via IPO property)
  elseif(MSVC)
    set(${out_var} "ipo" PARENT_SCOPE)    # cl.exe /GL /LTCG via CMake IPO
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
  _quarisma_windows_msvc_link_stack(_q_wmsvc_link)
  if(_lto_mode STREQUAL "auto")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT _q_wmsvc_link)
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

# Apply -fuse-ld=... (or MSVC CMAKE_LINKER) for a resolved linker executable path.
function(_quarisma_apply_linker_to_target target_name LINKER_NAME)
  if(NOT LINKER_NAME)
    return()
  endif()
  _quarisma_windows_msvc_link_stack(_q_wmsvc_link)
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND _q_wmsvc_link)
    # clang-cl: lld-link is typically already the default. CMake can inject -fuse-ld=lld-link,
    # which the GNU driver rejects; skip overrides for the MSVC frontend only.
    message(STATUS "Windows/clang-cl: default link.exe / lld-link stack, no -fuse-ld override")
    return()
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    get_filename_component(LINKER_BASENAME "${LINKER_NAME}" NAME)
    string(REPLACE "ld." "" LINKER_SHORTNAME "${LINKER_BASENAME}")
    set(LINKER_FLAGS "-fuse-ld=${LINKER_SHORTNAME}")
    if(TARGET "${target_name}")
      target_link_options("${target_name}" PUBLIC ${LINKER_FLAGS})
      message(STATUS "Applied linker flag to ${target_name}: ${LINKER_FLAGS}")
    else()
      message(WARNING "Target '${target_name}' not found - linker flag will not be applied")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND LINKER_NAME MATCHES "lld-link")
    set(CMAKE_LINKER "${LINKER_NAME}" CACHE FILEPATH "Linker executable")
    message(STATUS "Linker configured: ${LINKER_NAME}")
  endif()
endfunction()

# When LTO is active, pick a linker that understands LTO IR (Clang ThinLTO cannot use bfd ld).
function(_quarisma_find_lto_compatible_linker linker_choice target_name)
  set(LINKER_FOUND FALSE)
  set(LINKER_NAME "")

  if(NOT linker_choice STREQUAL "default")
    find_program(
      _EXPLICIT_LINKER NAMES "${linker_choice}" "ld.${linker_choice}" "${linker_choice}-link"
    )
    if(_EXPLICIT_LINKER)
      set(LINKER_NAME "${_EXPLICIT_LINKER}")
      set(LINKER_FOUND TRUE)
      message(STATUS "LTO active: using requested linker '${linker_choice}': ${_EXPLICIT_LINKER}")
    else()
      message(
        WARNING
        "LTO active: requested linker '${linker_choice}' not found — auto-selecting LTO-capable linker"
      )
    endif()
    unset(_EXPLICIT_LINKER CACHE)
  endif()

  if(NOT LINKER_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # Apple ld64 handles LTO from Xcode/Apple Clang; optional ld64.lld if present.
      find_program(LD64_LLD_LINKER ld64.lld)
      if(LD64_LLD_LINKER)
        set(LINKER_NAME "${LD64_LLD_LINKER}")
        set(LINKER_FOUND TRUE)
        message(STATUS "LTO active (Darwin): using ld64.lld")
      else()
        message(STATUS "LTO active (Darwin): using platform linker (ld64)")
        return()
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
        # Thin/full LTO objects need LLVM-aware linker: prefer lld, then mold, then gold.
        find_program(LD_LLD_LINKER ld.lld)
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)
        if(LD_LLD_LINKER)
          set(LINKER_NAME "${LD_LLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + Clang: using ld.lld (LTO-compatible)")
        elseif(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + Clang: using mold (LTO-compatible)")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + Clang: using ld.gold (higher memory use than lld)")
        endif()
      elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)
        find_program(LD_LLD_LINKER ld.lld)
        if(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + GCC: using mold")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + GCC: using ld.gold")
        elseif(LD_LLD_LINKER)
          set(LINKER_NAME "${LD_LLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + GCC: using ld.lld")
        endif()
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        find_program(WIN_GNU_LLD NAMES ld.lld lld)
        if(WIN_GNU_LLD)
          set(LINKER_NAME "${WIN_GNU_LLD}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + Windows (GNU Clang): using ${WIN_GNU_LLD} for ThinLTO")
        endif()
      elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        find_program(WIN_GNU_LLD NAMES ld.lld lld)
        if(WIN_GNU_LLD)
          set(LINKER_NAME "${WIN_GNU_LLD}")
          set(LINKER_FOUND TRUE)
          message(STATUS "LTO + Windows (GCC): using ${WIN_GNU_LLD}")
        endif()
      endif()
    endif()
  endif()

  if(LINKER_FOUND)
    _quarisma_apply_linker_to_target("${target_name}" "${LINKER_NAME}")
  else()
    message(
      WARNING
      "LTO active but no LTO-capable linker found — linking may fail (e.g. Clang ThinLTO + bfd ld). "
      "Install ld.lld or mold."
    )
  endif()
endfunction()

# quarisma_find_linker(<linker_choice> <target_name> [lto_mode_var])
#   linker_choice  — "default" (auto-detect) | "lld" | "mold" | "gold" | "lld-link"
#   target_name    — CMake target to receive the -fuse-ld= link option
#   lto_mode_var   — optional: ${PREFIX}_LTO_MODE; when not off/empty, use LTO-safe linker path.
function(quarisma_find_linker linker_choice target_name)
  set(LINKER_CHOICE "${linker_choice}")

  set(LINKER_FOUND FALSE)
  set(LINKER_NAME "")

  set(_lto_active FALSE)
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION OR "$CACHE{CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
    set(_lto_active TRUE)
  endif()
  if(NOT _lto_active AND ARGC GREATER 2)
    set(_lto_mode_var "${ARGV2}")
    if(DEFINED "${_lto_mode_var}")
      string(TOLOWER "${${_lto_mode_var}}" _lto_mode_val)
      if(NOT _lto_mode_val STREQUAL "off" AND NOT _lto_mode_val STREQUAL "")
        set(_lto_active TRUE)
      endif()
    endif()
  endif()

  if(_lto_active)
    _quarisma_windows_msvc_link_stack(_q_wmsvc_link)
    if(_q_wmsvc_link OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      message(STATUS "LTO enabled — MSVC toolset (cl or clang-cl); link.exe / LTCG — no -fuse-ld override")
      return()
    endif()
    _quarisma_find_lto_compatible_linker("${LINKER_CHOICE}" "${target_name}")
    return()
  endif()

  # --- LTO off: faster linkers (original behavior) ---
  if(NOT LINKER_CHOICE STREQUAL "default")
    find_program(
      _EXPLICIT_LINKER NAMES "${LINKER_CHOICE}" "ld.${LINKER_CHOICE}" "${LINKER_CHOICE}-link"
    )
    if(_EXPLICIT_LINKER)
      set(LINKER_NAME "${_EXPLICIT_LINKER}")
      set(LINKER_FOUND TRUE)
      message(STATUS "Using explicitly requested linker '${LINKER_CHOICE}': ${_EXPLICIT_LINKER}")
    else()
      message(WARNING "Requested linker '${LINKER_CHOICE}' not found — falling back to auto-detect")
    endif()
    unset(_EXPLICIT_LINKER CACHE)
  endif()

  if(NOT LINKER_FOUND)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)
        find_program(LD_LLD_LINKER ld.lld)

        if(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "Linux/Clang: Using mold linker for faster linking")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "Linux/Clang: Using ld.gold linker for faster linking (more compatible)")
        elseif(LD_LLD_LINKER)
          set(LINKER_NAME "${LD_LLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "Linux/Clang: Using ld.lld linker for faster linking")
        endif()
      elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        find_program(MOLD_LINKER mold)
        find_program(GOLD_LINKER ld.gold)

        if(MOLD_LINKER)
          set(LINKER_NAME "${MOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "Linux/GCC: Using mold linker for faster linking")
        elseif(GOLD_LINKER)
          set(LINKER_NAME "${GOLD_LINKER}")
          set(LINKER_FOUND TRUE)
          message(STATUS "Linux/GCC: Using gold linker for faster linking")
        endif()
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      find_program(LD64_LLD_LINKER ld64.lld)
      if(LD64_LLD_LINKER)
        set(LINKER_NAME "${LD64_LLD_LINKER}")
        set(LINKER_FOUND TRUE)
        message(STATUS "macOS: Using ld64.lld linker for faster linking")
      else()
        message(STATUS "macOS: Using system linker (ld64.lld not found)")
        return()
      endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      _quarisma_windows_msvc_link_stack(_q_wmsvc_link)
      if(_q_wmsvc_link)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER MATCHES "msys64"
           AND NOT CMAKE_CXX_COMPILER MATCHES "mingw"
        )
          find_program(LLD_LINK_LINKER lld-link)
          if(LLD_LINK_LINKER)
            set(LINKER_NAME "${LLD_LINK_LINKER}")
            set(LINKER_FOUND TRUE)
            message(STATUS "Windows/clang-cl: Using lld-link for faster linking")
          else()
            message(STATUS "Windows/clang-cl: lld-link not found — using default linker")
            return()
          endif()
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
          message(STATUS "Windows/MSVC (cl): Using default linker with optimizations")
          return()
        endif()
      else()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          find_program(WIN_GNU_LLD NAMES ld.lld lld)
          if(WIN_GNU_LLD)
            set(LINKER_NAME "${WIN_GNU_LLD}")
            set(LINKER_FOUND TRUE)
            message(STATUS "Windows (GNU Clang/GCC): Using lld for faster linking")
          else()
            message(STATUS "Windows (GNU Clang/GCC): lld not found — using default linker")
            return()
          endif()
        endif()
      endif()
    endif()
  endif()

  if(LINKER_FOUND)
    _quarisma_apply_linker_to_target("${target_name}" "${LINKER_NAME}")
  else()
    message(STATUS "No faster linker found - using default linker")
  endif()
endfunction()
