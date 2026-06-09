# =============================================================================
# Library/Vectorization — CMakeLists.txt
#
# Provides the Vectorization library: SIMD packet abstractions, expression templates, and
# architecture-specific kernels for SSE, AVX, AVX2, and AVX-512.
#
# VECTORIZATION_CPU_BACKEND (cache: no|sse|avx|avx2|avx512|neon|sve) selects the CPU SIMD backend.
# VECTORIZATION_GPU_BACKEND (cache: none|cuda|hip) selects the GPU backend (independent of CPU type).
# Both can be active simultaneously — e.g. CPU_TYPE=avx2 + GPU_TYPE=cuda.
# Optional VECTORIZATION_ENABLE_SVML adds SVML headers.
#
# C++ preprocessor defines: sse → HAS_SSE=1; avx/avx2 → HAS_AVX=1; avx2 → HAS_AVX2=1;
# avx512 → HAS_AVX512=1; cuda → HAS_CUDA=1; hip → HAS_HIP=1.
# VECTORIZATION_HAS_SVML=1/0   — SVML active in C++ code
# VECTORIZATION_HAS_GTEST=1/0  — Google Test available to Vectorization tests
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# =============================================================================
# SECTION 1 — Cache
# options (VECTORIZATION_CPU_BACKEND, VECTORIZATION_GPU_BACKEND, tooling) and backend selectors
#
# include(utils) runs compiler feature checks (SIMD flags, exception_ptr); results are published to
# CACHE for Core (VECTORIZATION_COMPILER_FLAGS, PROJECT_*).
#
# VECTORIZATION_CPU_BACKEND selects the CPU SIMD backend (one of: no, sse, avx, avx2, avx512, neon, sve).
# VECTORIZATION_GPU_BACKEND selects the GPU backend (one of: none, cuda, hip).
# Both flags are independent — you can combine any CPU backend with any GPU backend.
# =============================================================================
# Host-aware default CPU SIMD: x86_64 -> avx2, aarch64 -> neon.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _quarisma_vec_proc_default)
set(_quarisma_vectorization_cpu_default "avx2")
if(_quarisma_vec_proc_default MATCHES "^(aarch64|arm64)$")
  set(_quarisma_vectorization_cpu_default "neon")
endif()

set(VECTORIZATION_CPU_BACKEND "${_quarisma_vectorization_cpu_default}"
    CACHE STRING
        "CPU SIMD backend. Options: no, sse, avx, avx2, avx512, neon, sve"
)
set_property(CACHE VECTORIZATION_CPU_BACKEND PROPERTY STRINGS no sse avx avx2 avx512 neon sve)
if(NOT VECTORIZATION_CPU_BACKEND MATCHES "^(no|sse|avx|avx2|avx512|neon|sve)$")
  message(
    FATAL_ERROR
      "VECTORIZATION_CPU_BACKEND must be one of: no, sse, avx, avx2, avx512, neon, sve (got '${VECTORIZATION_CPU_BACKEND}')"
  )
endif()

# GPU backend. Can be set from the command line: -DVECTORIZATION_GPU_BACKEND=cuda|hip|none
set(VECTORIZATION_GPU_BACKEND "none"
    CACHE STRING "GPU backend for Vectorization: none, cuda, hip"
)
set_property(CACHE VECTORIZATION_GPU_BACKEND PROPERTY STRINGS none cuda hip)
if(NOT VECTORIZATION_GPU_BACKEND MATCHES "^(none|cuda|hip)$")
  message(FATAL_ERROR "VECTORIZATION_GPU_BACKEND must be one of: none, cuda, hip (got '${VECTORIZATION_GPU_BACKEND}')")
endif()

set(VECTORIZATION_PACKET_SIZE "4"
    CACHE STRING
        "SIMD packet lane count (vectorization::packet_size<T>::value); must match your ISA and kernels"
)
if(NOT VECTORIZATION_PACKET_SIZE MATCHES "^[1-9][0-9]*$")
  message(
    FATAL_ERROR
      "VECTORIZATION_PACKET_SIZE must be a positive decimal integer (got '${VECTORIZATION_PACKET_SIZE}')"
  )
endif()
if(VECTORIZATION_PACKET_SIZE GREATER 256)
  message(
    FATAL_ERROR
      "VECTORIZATION_PACKET_SIZE must be at most 256 (got '${VECTORIZATION_PACKET_SIZE}')"
  )
endif()

include(lto)
quarisma_lto_compute_default(_lto_default PROFILE PERFORMANCE)
set(VECTORIZATION_LTO_MODE "${_lto_default}"
    CACHE STRING "LTO mode for Vectorization: off | thin | full | ipo | auto")
set_property(CACHE VECTORIZATION_LTO_MODE PROPERTY STRINGS off thin full ipo auto)
option(VECTORIZATION_ENABLE_COVERAGE "Enable coverage instrumentation for Vectorization" OFF)
option(VECTORIZATION_ENABLE_TESTING "Build Vectorization test suite" ON)
option(VECTORIZATION_ENABLE_EXAMPLES "Build Vectorization example programs" OFF)
option(VECTORIZATION_ENABLE_GTEST "Enable GoogleTest for Vectorization" ON)
option(VECTORIZATION_ENABLE_BENCHMARK "Enable Google Benchmark for Vectorization" ON)

# Clang/GCC: append -march=native for extra instruction scheduling / tuning on the build host.
# Improves runtime on this machine; the binary may not run on older CPUs. MSVC ignores this (no flag
# added).
option(USE_NATIVE_ARCH "Prefer host CPU instruction tuning (-march=native, Clang/GCC only)" OFF)

option(VECTORIZATION_ENABLE_SVML "Enable Intel SVML short vector math library" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SVML)

# MKL VML backend — statically linked Intel MKL; mutually exclusive with SVML/SLEEF math paths.
# Enable by setting VECTORIZATION_ENABLE_MKL=ON and ensuring MKLROOT is set or MKL is
# discoverable via find_package(MKL CONFIG).
option(VECTORIZATION_ENABLE_MKL "Enable Intel MKL VML vector math backend (x86 only, static link)" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_MKL)

option(VECTORIZATION_ENABLE_SLEEF "Enable SLEEF SIMD math library (SSE/AVX/AVX2/AVX512/NEON); mutually exclusive with SVML" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_SLEEF)

option(VECTORIZATION_ENABLE_ACCELERATE "Enable Apple Accelerate vForce on AArch64 NEON" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_ACCELERATE)

# Memory presence — detected from the CMake target; not a user option.
if(TARGET Memory::Memory)
  set(_vec_has_memory 1)
else()
  set(_vec_has_memory 0)
endif()

# Logging presence — detected from the CMake target; not a user option.
if(TARGET Logging::Logging)
  set(_vec_has_logging 1)
else()
  set(_vec_has_logging 0)
endif()

option(VECTORIZATION_ENABLE_ICECC "Use Icecream distributed compiler for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_ICECC)
option(VECTORIZATION_ENABLE_CACHE "Enable compiler caching for the Vectorization target" ON)
mark_as_advanced(VECTORIZATION_ENABLE_CACHE)
option(VECTORIZATION_ENABLE_CLANGTIDY "Enable clang-tidy static analysis for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_CLANGTIDY)
option(VECTORIZATION_ENABLE_FIX "Enable clang-tidy fix options for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_FIX)
option(VECTORIZATION_ENABLE_IWYU "Enable include-what-you-use analysis for Vectorization" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_IWYU)
option(VECTORIZATION_ENABLE_SANITIZER "Build Vectorization with sanitizer support (Clang/GCC only)"
       OFF
)
set(VECTORIZATION_SANITIZER_TYPE "address"
    CACHE STRING
          "Sanitizer type for Vectorization. Options: address, undefined, thread, memory, leak"
)
set_property(
  CACHE VECTORIZATION_SANITIZER_TYPE PROPERTY STRINGS address undefined thread memory leak
)
mark_as_advanced(VECTORIZATION_ENABLE_SANITIZER VECTORIZATION_SANITIZER_TYPE)
option(VECTORIZATION_ENABLE_SPELL
       "Enable spell checking for Vectorization (WARNING: modifies source files)" OFF
)
mark_as_advanced(VECTORIZATION_ENABLE_SPELL)
option(VECTORIZATION_ENABLE_VALGRIND "Execute Vectorization test suite with Valgrind" OFF)
mark_as_advanced(VECTORIZATION_ENABLE_VALGRIND)
set(VECTORIZATION_LINKER_CHOICE "default"
    CACHE STRING "Linker for Vectorization: default, lld, mold, gold, lld-link"
)
set_property(CACHE VECTORIZATION_LINKER_CHOICE PROPERTY STRINGS default lld mold gold lld-link)
mark_as_advanced(VECTORIZATION_LINKER_CHOICE)
set(VECTORIZATION_CACHE_BACKEND "none"
    CACHE STRING
          "Compiler cache backend for Vectorization. Options: none, ccache, sccache, buildcache"
)
set_property(CACHE VECTORIZATION_CACHE_BACKEND PROPERTY STRINGS none ccache sccache buildcache)
mark_as_advanced(VECTORIZATION_CACHE_BACKEND)
set(VECTORIZATION_CXX_STANDARD "20" CACHE STRING
                                          "C++ standard for Vectorization. Options: 11 14 17 20 23"
)
set_property(CACHE VECTORIZATION_CXX_STANDARD PROPERTY STRINGS 11 14 17 20 23)
if(NOT VECTORIZATION_CXX_STANDARD MATCHES "^(11|14|17|20|23)$")
  message(FATAL_ERROR "VECTORIZATION_CXX_STANDARD must be one of: 11 14 17 20 23")
endif()

# SVE vector register width. The current backend/sve/ implementation only supports 128 bits.
# Set to 256 or 512 once those backends are extended to support scalable widths.
set(VECTORIZATION_SVE_VECTOR_BITS "128"
    CACHE STRING "SVE vector register width in bits: 128, 256, 512"
)
set_property(CACHE VECTORIZATION_SVE_VECTOR_BITS PROPERTY STRINGS 128 256 512)
if(NOT VECTORIZATION_SVE_VECTOR_BITS MATCHES "^(128|256|512)$")
  message(FATAL_ERROR "VECTORIZATION_SVE_VECTOR_BITS must be 128, 256, or 512")
endif()

# =============================================================================
# SECTION 3 — CMake
# module includes
# =============================================================================
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckSymbolExists)

include(compiler_checks)
include(checks)
include(build_type)
include(platform)
include(compile_definitions)
include(CMakeDependentOption)
include(cache)
include(utils)

# Auto-downgrade to "no" if the requested CPU SIMD tier is unsupported
# (e.g. ARM64 has no x86 SSE/AVX).  HIP/CUDA are GPU backends — skip this check for them.
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "no" AND NOT VECTORIZATION)
  message(
    WARNING
      "VECTORIZATION_CPU_BACKEND '${VECTORIZATION_CPU_BACKEND}' is not supported by this compiler/architecture. "
      "Falling back to VECTORIZATION_CPU_BACKEND=no."
  )
  set(VECTORIZATION_CPU_BACKEND "no"
      CACHE STRING "CPU SIMD backend. Options: no, sse, avx, avx2, avx512, neon, sve" FORCE
  )
endif()

# Other libraries must not include Vectorization CMake modules; expose probes for Core et al.
if(HAS_EXCEPTION_PTR)
  set(PROJECT_HAS_EXCEPTION_PTR 1 CACHE INTERNAL
                                        "Quarisma: std::exception_ptr (Vectorization utils)" FORCE
  )
else()
  set(PROJECT_HAS_EXCEPTION_PTR 0 CACHE INTERNAL
                                        "Quarisma: std::exception_ptr (Vectorization utils)" FORCE
  )
endif()
foreach(_simd IN ITEMS SSE AVX AVX2 AVX512 NEON SVE)
  if(HAS_${_simd})
    set(PROJECT_${_simd} 1 CACHE INTERNAL "Quarisma: SIMD (Vectorization utils)" FORCE)
  else()
    set(PROJECT_${_simd} 0 CACHE INTERNAL "Quarisma: SIMD (Vectorization utils)" FORCE)
  endif()
endforeach()
set(VECTORIZATION_COMPILER_FLAGS "${VECTORIZATION_COMPILER_FLAGS}"
    CACHE INTERNAL "Quarisma: SIMD flags from Vectorization utils" FORCE
)


message("-- SIMD compiler flags: ${VECTORIZATION_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VECTORIZATION_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VECTORIZATION_COMPILER_FLAGS}")

# =============================================================================
# SECTION 2 —
# VECTORIZATION_CPU_BACKEND / VECTORIZATION_GPU_BACKEND → VECTORIZATION_HAS_* defines and backend trees
# =============================================================================
if(VECTORIZATION_ENABLE_SANITIZER)
  include(sanitize)
endif()
if(VECTORIZATION_ENABLE_SPELL)
  include(spell)
endif()
if(VECTORIZATION_ENABLE_VALGRIND)
  include(valgrind)
endif()
include(helper_macros)

if(VECTORIZATION_ENABLE_COVERAGE)
  include(coverage)
elseif(MSVC)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG:FULL")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG:FULL")
endif()

# --- CPU backend → VECTORIZATION_HAS_* (one-hot, driven by VECTORIZATION_CPU_BACKEND) ---
set(VECTORIZATION_COMPILE_DEFINITIONS "")

if(VECTORIZATION_CPU_BACKEND STREQUAL "sse")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SSE=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SSE=0)
endif()

# AVX2 implies AVX; set once to avoid duplicate -DVECTORIZATION_HAS_AVX on the command line.
if(VECTORIZATION_CPU_BACKEND STREQUAL "avx" OR VECTORIZATION_CPU_BACKEND STREQUAL "avx2")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX=0)
endif()

if(VECTORIZATION_CPU_BACKEND STREQUAL "avx2")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX2=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX2=0)
endif()

if(VECTORIZATION_CPU_BACKEND STREQUAL "avx512")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX512=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_AVX512=0)
endif()

if(VECTORIZATION_CPU_BACKEND STREQUAL "neon")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_NEON=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_NEON=0)
endif()

if(VECTORIZATION_CPU_BACKEND STREQUAL "sve")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SVE=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_SVE=0)
endif()

# --- GPU backend → VECTORIZATION_HAS_* (driven by VECTORIZATION_GPU_BACKEND) ---
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_CUDA=1 VECTORIZATION_HAS_HIP=0)
elseif(VECTORIZATION_GPU_BACKEND STREQUAL "hip")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_CUDA=0 VECTORIZATION_HAS_HIP=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_CUDA=0 VECTORIZATION_HAS_HIP=0)
endif()

compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SVML)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_SLEEF)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_ACCELERATE)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_MKL)
compile_definition(VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_ENABLE_GTEST)
list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_MEMORY=${_vec_has_memory})
list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_HAS_LOGGING=${_vec_has_logging})
# VECTORIZATION_VECTORIZED: 1 when any CPU SIMD backend is active (GPU does not affect this).
if(VECTORIZATION_CPU_BACKEND MATCHES "^(sse|avx|avx2|avx512|neon|sve)$")
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_VECTORIZED=1)
else()
  list(APPEND VECTORIZATION_COMPILE_DEFINITIONS VECTORIZATION_VECTORIZED=0)
endif()

list(APPEND VECTORIZATION_COMPILE_DEFINITIONS
     VECTORIZATION_PACKET_SIZE=${VECTORIZATION_PACKET_SIZE}
)

set(VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "")
list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS ${VECTORIZATION_COMPILE_DEFINITIONS})
set(VECTORIZATION_DEPENDENCY_LINK_DIRS "")


# Build/link ThirdParty SLEEF for AArch64 NEON when requested.
if(VECTORIZATION_ENABLE_SLEEF)
  set(_tpdir "${CMAKE_SOURCE_DIR}/ThirdParty")
  set(_sleef_build_dir "${CMAKE_BINARY_DIR}/ThirdParty/sleef_build")
  set(_sleef_install_prefix "${CMAKE_BINARY_DIR}/ThirdParty/install")

  # SLEEF requires a non-default install prefix even for subdirectory builds.
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${_sleef_install_prefix}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)
  endif()

  set(SLEEF_BUILD_TESTS OFF CACHE BOOL "Disable SLEEF tests" FORCE)
  set(SLEEF_BUILD_DFT OFF CACHE BOOL "Disable SLEEF DFT library" FORCE)
  set(SLEEF_BUILD_SCALAR_LIB OFF CACHE BOOL "Disable SLEEF scalar library" FORCE)
  set(SLEEF_BUILD_QUAD OFF CACHE BOOL "Disable SLEEF quad library" FORCE)
  set(SLEEF_BUILD_GNUABI_LIBS OFF CACHE BOOL "Disable SLEEF GNU ABI variants" FORCE)
  set(SLEEF_DISABLE_OPENMP ON CACHE BOOL "Disable SLEEF OpenMP usage" FORCE)
  set(SLEEF_BUILD_INLINE_HEADERS OFF CACHE BOOL "Disable SLEEF inline headers" FORCE)
  set(SLEEF_ENABLE_TESTER OFF CACHE BOOL "Disable SLEEF tester binaries" FORCE)
  set(SLEEF_ENABLE_TESTER4 OFF CACHE BOOL "Disable SLEEF tester4 binaries" FORCE)
  set(SLEEF_ENABLE_SSL OFF CACHE BOOL "Disable SLEEF SSL-backed tester checks" FORCE)
  set(SLEEF_SHOW_CONFIG OFF CACHE BOOL "Suppress SLEEF config output" FORCE)
  set(SLEEF_BUILD_SHARED_LIBS OFF CACHE BOOL "Build SLEEF as static library" FORCE)

  add_third_party_library(
    sleef
    LIBRARY_TYPE
    STATIC
    SOURCE_DIR
    "${_tpdir}/sleef"
    BINARY_DIR
    "${_sleef_build_dir}"
    QUARISMA_TARGETS
    "Sleef::sleef=sleef"
    INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/include"
    EXCLUDE_FROM_ALL
    AUTO_ADD_TO_DEPENDENCIES
    OFF
  )

  list(APPEND VECTORIZATION_DEPENDENCY_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/include")
  list(APPEND VECTORIZATION_DEPENDENCY_INCLUDE_DIRS "${_sleef_build_dir}/include")
  if(TARGET sleef)
    list(APPEND VECTORIZATION_DEPENDENCY_LIBS sleef)
  endif()
  list(APPEND VECTORIZATION_DEPENDENCY_LINK_DIRS "${_sleef_install_prefix}/lib")
  # sleef.h uses SLEEF_STATIC_LIBS to suppress __declspec(dllimport) on Windows.
  # The generated sleef-config.h defines it, but sleef.h does not include that header,
  # so we propagate the definition explicitly to all consumers.
  list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS SLEEF_STATIC_LIBS)

endif()

# Detect and configure Intel MKL VML (x86 only, statically linked) when requested.
if(VECTORIZATION_ENABLE_MKL)
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _mkl_proc)
  if(NOT _mkl_proc MATCHES "^(x86_64|amd64|i686|i386)$")
    message(WARNING
      "VECTORIZATION_ENABLE_MKL=ON is only supported on x86_64; disabling."
    )
    set(VECTORIZATION_ENABLE_MKL OFF
        CACHE BOOL "Enable Intel MKL VML vector math backend (x86 only, static link)" FORCE)
    list(REMOVE_ITEM VECTORIZATION_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=1")
    list(APPEND VECTORIZATION_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=0")
    list(REMOVE_ITEM VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=1")
    list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=0")
  else()
    # Sequential (single-threaded) VML — no mandatory OpenMP runtime dependency.
    set(MKL_THREADING "SEQ"
        CACHE STRING "MKL threading flavour used by Vectorization" FORCE)
    # Use shared MKL libraries (pip-installed MKL ships .so only, no .a archives).
    # Set ENABLE_STATIC_MKL=ON manually if you have static archives available.
    set(ENABLE_STATIC_MKL OFF
        CACHE BOOL "Link MKL statically for Vectorization" FORCE)

    find_package(MKL QUIET)

    if(MKL_FOUND)
      message(STATUS "Vectorization: MKL VML found (version ${MKL_VERSION}). Include: ${MKL_INCLUDE_DIR}")

      # Umbrella INTERFACE target — wraps include dir, library list, and the
      # --start-group / --end-group pair needed on Linux to resolve circular
      # references between the three static MKL archives.
      if(NOT TARGET MKL::MKL)
        add_library(MKL::MKL INTERFACE IMPORTED GLOBAL)
        target_include_directories(MKL::MKL INTERFACE "${MKL_INCLUDE_DIR}")
        # --start-group / --end-group is only needed for static archives to resolve
        # circular references.  With shared libraries it is harmless but unnecessary.
        if(UNIX AND NOT APPLE AND ENABLE_STATIC_MKL)
          target_link_libraries(MKL::MKL INTERFACE
            -Wl,--start-group ${MKL_LIBRARIES} -Wl,--end-group
            ${CMAKE_DL_LIBS}
          )
        else()
          target_link_libraries(MKL::MKL INTERFACE
            ${MKL_LIBRARIES}
            ${CMAKE_DL_LIBS}
          )
        endif()
      endif()
    else()
      message(WARNING
        "VECTORIZATION_ENABLE_MKL=ON but MKL was not found — disabling MKL VML backend. "
        "Set MKLROOT or INTEL_MKL_DIR to your MKL installation root "
        "(e.g. /opt/intel/oneapi/mkl/latest)."
      )
      set(VECTORIZATION_ENABLE_MKL OFF
          CACHE BOOL "Enable Intel MKL VML vector math backend (x86 only, static link)" FORCE)
      # Patch the compile-definition lists that were populated before this detection block.
      list(REMOVE_ITEM VECTORIZATION_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=1")
      list(APPEND VECTORIZATION_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=0")
      list(REMOVE_ITEM VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=1")
      list(APPEND VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS "VECTORIZATION_HAS_MKL=0")
    endif()
  endif()
  unset(_mkl_proc)
endif()

# =============================================================================
# SECTION 4 — Source
# discovery with SIMD-tier filtering
# =============================================================================
set(CMAKE_CXX_STANDARD ${VECTORIZATION_CXX_STANDARD})

file(GLOB_RECURSE headers LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                                                 "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)
file(GLOB_RECURSE sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
                                                 "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE templates LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.hxx")

list(FILTER sources EXCLUDE REGEX ".*/Testing/.*")
list(FILTER headers EXCLUDE REGEX ".*/Testing/.*")
list(FILTER templates EXCLUDE REGEX ".*/Testing/.*")

# Exclude CPU SIMD backend trees not selected by VECTORIZATION_CPU_BACKEND.
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "sse")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/sse/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/sse/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/cpu/sse/.*")
endif()
if(NOT VECTORIZATION_CPU_BACKEND MATCHES "^(avx|avx2)$")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/avx/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/avx/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/cpu/avx/.*")
endif()
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "avx512")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/avx512/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/avx512/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/cpu/avx512/.*")
endif()
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "neon" AND NOT VECTORIZATION_CPU_BACKEND STREQUAL "sve")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/neon/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/neon/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/cpu/neon/.*")
endif()
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "sve")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/sve/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/sve/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/cpu/sve/.*")
endif()

# Exclude GPU backend trees not selected by VECTORIZATION_GPU_BACKEND.
if(NOT VECTORIZATION_GPU_BACKEND STREQUAL "hip")
  list(FILTER headers EXCLUDE REGEX ".*/backend/gpu/hip/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/gpu/hip/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/gpu/hip/.*")
endif()
if(NOT VECTORIZATION_GPU_BACKEND STREQUAL "cuda")
  list(FILTER headers EXCLUDE REGEX ".*/backend/gpu/cuda/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/gpu/cuda/.*")
  list(FILTER templates EXCLUDE REGEX ".*/backend/gpu/cuda/.*")
endif()

# Exclude x86 svml.h when neither SVML nor SLEEF is active.
# NEON svml.h is handled separately — it always compiles for NEON (has scalar fallbacks).
if(NOT VECTORIZATION_ENABLE_SVML AND NOT VECTORIZATION_ENABLE_SLEEF)
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/(sse|avx|avx512)/svml\\.h$")
endif()

# Always exclude NEON svml.h when not targeting NEON.
if(NOT VECTORIZATION_CPU_BACKEND STREQUAL "neon")
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/neon/svml\\.h$")
endif()

# Exclude the MKL backend tree when MKL is not enabled.
if(NOT VECTORIZATION_ENABLE_MKL)
  list(FILTER headers EXCLUDE REGEX ".*/backend/cpu/mkl/.*")
  list(FILTER sources EXCLUDE REGEX ".*/backend/cpu/mkl/.*")
endif()

# expressions_functors_mkl.h is a stub (content merged into expressions_functors.h).
# Exclude it from the build unconditionally to keep the source list clean.
list(FILTER headers EXCLUDE REGEX ".*/expressions_functors_mkl\\.h$")

# =============================================================================
# SECTION 5 — Target
# definition
# =============================================================================
add_library(Vectorization ${sources} ${headers} ${templates})
quarisma_target_apply_lto(Vectorization VECTORIZATION)
if(VECTORIZATION_ENABLE_SANITIZER)
  quarisma_configure_sanitizer(VECTORIZATION)
endif()
quarisma_find_linker(${VECTORIZATION_LINKER_CHOICE} Vectorization VECTORIZATION_LTO_MODE)
quarisma_target_apply_cache(Vectorization VECTORIZATION_ENABLE_CACHE VECTORIZATION_CACHE_BACKEND)
quarisma_target_optional_icecc(Vectorization VECTORIZATION_ENABLE_ICECC)

# --- DLL-export macros (VECTORIZATION_API in source-tree vectorization_export.h) ---
if(BUILD_SHARED_LIBS)
  target_compile_definitions(
    Vectorization PUBLIC VECTORIZATION_SHARED_DEFINE PRIVATE VECTORIZATION_BUILDING_DLL
  )
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(Vectorization)
  endif()
else()
  target_compile_definitions(Vectorization PUBLIC VECTORIZATION_STATIC_DEFINE)
endif()

add_library(Vectorization::Vectorization ALIAS Vectorization)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Testing/VectorizationTest.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Testing/VectorizationTest.h" @ONLY
)

set_target_properties(
  Vectorization PROPERTIES CXX_STANDARD ${VECTORIZATION_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
                           CXX_EXTENSIONS OFF
)

# ---- HIP GPU backend -------------------------------------------------------
if(VECTORIZATION_GPU_BACKEND STREQUAL "hip")
  # Prefer ROCm HIP; fall back to CUDA when ROCM_PATH is absent.
  if(NOT DEFINED CMAKE_HIP_COMPILER AND DEFINED ENV{ROCM_PATH})
    set(CMAKE_HIP_COMPILER "$ENV{ROCM_PATH}/bin/hipcc" CACHE FILEPATH "HIP compiler")
  endif()
  enable_language(HIP)

  set(VECTORIZATION_GPU_ARCHITECTURES "gfx900;gfx906;gfx908;gfx90a;gfx1030;gfx1100"
      CACHE STRING "HIP GPU architectures (gfxXXX for AMD, smXX for CUDA offload via hipcc)")
  set_target_properties(Vectorization PROPERTIES
    HIP_STANDARD    ${VECTORIZATION_CXX_STANDARD}
    HIP_ARCHITECTURES "${VECTORIZATION_GPU_ARCHITECTURES}"
  )
  target_compile_options(Vectorization PRIVATE
    $<$<COMPILE_LANGUAGE:HIP>:-ffast-math>
  )
  message(STATUS "Vectorization: HIP GPU backend enabled (archs: ${VECTORIZATION_GPU_ARCHITECTURES})")
endif()

# ---- CUDA GPU backend -------------------------------------------------------
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda")
  enable_language(CUDA)

  # sm_70 (Volta) was removed in CUDA 12.8; CUDA 13.x starts at sm_75.
  # Force-update to evict any stale cached value that contains sm_70.
  set(VECTORIZATION_CUDA_ARCHITECTURES "75;80;86;89;90"
      CACHE STRING "CUDA compute capability list (e.g. 75;80;86 for Turing/Ampere/Ada)" FORCE)
  set_target_properties(Vectorization PROPERTIES
    CUDA_STANDARD              ${VECTORIZATION_CXX_STANDARD}
    CUDA_STANDARD_REQUIRED     ON
    CUDA_ARCHITECTURES         "${VECTORIZATION_CUDA_ARCHITECTURES}"
    CUDA_SEPARABLE_COMPILATION OFF
  )
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Vectorization PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:-ffast-math>
    )
  else()
    target_compile_options(Vectorization PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math -Xcompiler=-fPIC>
    )
  endif()
  message(STATUS "Vectorization: CUDA GPU backend enabled (archs: ${VECTORIZATION_CUDA_ARCHITECTURES}, compiler: ${CMAKE_CUDA_COMPILER_ID})")
endif()

if(MSVC)
  target_compile_options(Vectorization PRIVATE /WX)
  if(EXISTS "${PROJECT_SOURCE_DIR}/PRETORIAN.natvis")
    target_sources(Vectorization PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/PRETORIAN.natvis>)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND VECTORIZATION_CPU_BACKEND STREQUAL "sse")
    target_compile_definitions(Vectorization PUBLIC VECTORIZATION_CLANG_CL)
  endif()
else()
  target_compile_options(Vectorization PRIVATE -include cstdlib)
endif()

# Apply SIMD flags as PUBLIC target options so all consumers inherit the correct ISA.
if(VECTORIZATION_COMPILER_FLAGS)
  if(MSVC)
    separate_arguments(_vec_simd_flags_list WINDOWS_COMMAND "${VECTORIZATION_COMPILER_FLAGS}")
  else()
    separate_arguments(_vec_simd_flags_list UNIX_COMMAND "${VECTORIZATION_COMPILER_FLAGS}")
  endif()
  target_compile_options(Vectorization PUBLIC ${_vec_simd_flags_list})
endif()

# =============================================================================
# SECTION 6 — Include
# directories and link libraries
# =============================================================================
target_include_directories(
  Vectorization
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Testing>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Testing>
         $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

if(VECTORIZATION_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS})
  message(STATUS "Vectorization: extra include dirs: ${VECTORIZATION_DEPENDENCY_INCLUDE_DIRS}")
endif()

if(_vec_has_logging)
  target_link_libraries(Vectorization PUBLIC Logging::Logging)
endif()

if(_vec_has_memory)
  target_link_libraries(Vectorization PUBLIC Memory::Memory)
endif()

if(VECTORIZATION_DEPENDENCY_LIBS)
  target_link_libraries(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_LIBS})
  message(STATUS "Vectorization: linked dependencies: ${VECTORIZATION_DEPENDENCY_LIBS}")
endif()

if(VECTORIZATION_DEPENDENCY_LINK_DIRS)
  target_link_directories(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_LINK_DIRS})
  message(STATUS "Vectorization: dependency link dirs: ${VECTORIZATION_DEPENDENCY_LINK_DIRS}")
endif()

# Link Apple Accelerate framework when vForce is enabled.
if(VECTORIZATION_ENABLE_ACCELERATE AND APPLE)
  target_link_libraries(Vectorization PUBLIC "-framework Accelerate")
  message(STATUS "Vectorization: linked Apple Accelerate framework (vForce).")
endif()

if(VECTORIZATION_ENABLE_SLEEF AND TARGET Sleef::sleef)
  target_link_libraries(Vectorization PUBLIC Sleef::sleef)
endif()

# Link ThirdParty SVML and stage its runtime binaries next to the output library.
if(VECTORIZATION_ENABLE_SVML AND TARGET SVML::SVML)
  target_link_libraries(Vectorization PUBLIC SVML::SVML)
  if(TARGET SVML::IRC)
    target_link_libraries(Vectorization PUBLIC SVML::IRC)
  endif()
  if(TARGET SVML::INTLC)
    target_link_libraries(Vectorization PUBLIC SVML::INTLC)
  endif()
  foreach(_svml_rt IN LISTS SVML_RUNTIME_LIBRARIES)
    if(_svml_rt)
      add_custom_command(TARGET Vectorization POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_svml_rt}"
          "$<TARGET_FILE_DIR:Vectorization>"
        COMMENT "Staging SVML runtime: ${_svml_rt}"
        VERBATIM
      )
    endif()
  endforeach()
  unset(_svml_rt)
  message(STATUS "Vectorization: linked SVML::SVML (ThirdParty); runtime files: ${SVML_RUNTIME_LIBRARIES}")
endif()

# Link Intel MKL VML when VECTORIZATION_ENABLE_MKL=ON.
# The MKL::MKL umbrella target was created above (find_package + target setup).
if(VECTORIZATION_ENABLE_MKL)
  if(MKL_FOUND AND TARGET MKL::MKL)
    target_link_libraries(Vectorization PUBLIC MKL::MKL)
    message(STATUS "Vectorization: linked MKL VML via MKL::MKL.")
  else()
    message(FATAL_ERROR
      "VECTORIZATION_ENABLE_MKL=ON but the MKL::MKL target is not available. "
      "Set MKLROOT or INTEL_MKL_DIR to your MKL installation root."
    )
  endif()
endif()

if(VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS)
  target_compile_definitions(Vectorization PUBLIC ${VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS})
  message(
    STATUS "Vectorization: compile definitions: ${VECTORIZATION_DEPENDENCY_COMPILE_DEFINITIONS}"
  )
endif()

if(VECTORIZATION_ENABLE_CLANGTIDY)
  include(clang_tidy)
  quarisma_target_clang_tidy(Vectorization ${VECTORIZATION_ENABLE_FIX})
endif()

if(VECTORIZATION_ENABLE_IWYU)
  include(iwyu)
  quarisma_apply_iwyu(Vectorization)
endif()

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

# =============================================================================
# SECTION 7 —
# Installation
# =============================================================================
install(
  TARGETS Vectorization
  EXPORT VectorizationTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)
install(FILES ${headers} DESTINATION include/Vectorization COMPONENT Development)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/vectorization_export.h"
              "${CMAKE_CURRENT_SOURCE_DIR}/Testing/VectorizationTest.h"
        DESTINATION include/Vectorization COMPONENT Development
)

# =============================================================================
# SECTION 8 — Testing
# =============================================================================
if(VECTORIZATION_ENABLE_TESTING)
  add_subdirectory(Testing/Cxx)
endif()

if(VECTORIZATION_ENABLE_VALGRIND AND VECTORIZATION_ENABLE_TESTING)
  quarisma_apply_valgrind_timeouts()
endif()

# =============================================================================
# SECTION 9 — Summary
# message
# =============================================================================
message("")
message("******** Vectorization module (CMake flags) ********")
message("  CPU backend          : ${VECTORIZATION_CPU_BACKEND}")
message("  GPU backend          : ${VECTORIZATION_GPU_BACKEND}")
message("  Packet size          : ${VECTORIZATION_PACKET_SIZE}")
message("  Svml                 : ${VECTORIZATION_ENABLE_SVML}")
message("  Sleef                : ${VECTORIZATION_ENABLE_SLEEF}")
message("  Accelerate (vForce)  : ${VECTORIZATION_ENABLE_ACCELERATE}")
message("  Mkl VML              : ${VECTORIZATION_ENABLE_MKL}")
message("  Memory (HAS)         : ${_vec_has_memory}")
message("  Logging (HAS)        : ${_vec_has_logging}")
message("  Cxx standard         : ${VECTORIZATION_CXX_STANDARD}")
message("  Cache                : ${VECTORIZATION_ENABLE_CACHE}")
message("  Cache backend        : ${VECTORIZATION_CACHE_BACKEND}")
message("  Icecc                : ${VECTORIZATION_ENABLE_ICECC}")
quarisma_print_lto_linker_summary(VECTORIZATION)
message("  Coverage             : ${VECTORIZATION_ENABLE_COVERAGE}")
message("  Testing              : ${VECTORIZATION_ENABLE_TESTING}")
message("  Gtest                : ${VECTORIZATION_ENABLE_GTEST}")
message("  Benchmark            : ${VECTORIZATION_ENABLE_BENCHMARK}")

message("  Clang-tidy           : ${VECTORIZATION_ENABLE_CLANGTIDY}")
message("  Fix                  : ${VECTORIZATION_ENABLE_FIX}")
message("  Iwyu                 : ${VECTORIZATION_ENABLE_IWYU}")
message("  Sanitizer            : ${VECTORIZATION_ENABLE_SANITIZER}")
message("  Sanitizer type       : ${VECTORIZATION_SANITIZER_TYPE}")
message("  Spell                : ${VECTORIZATION_ENABLE_SPELL}")
message("  Valgrind             : ${VECTORIZATION_ENABLE_VALGRIND}")
message("")
message("Build:")
message("  C++ standard             : ${CMAKE_CXX_STANDARD}")
message("  C flags                  : ${CMAKE_C_FLAGS}")
message("  CXX flags                : ${CMAKE_CXX_FLAGS}")
message("  Shared linker flags      : ${CMAKE_SHARED_LINKER_FLAGS}")
message("")
