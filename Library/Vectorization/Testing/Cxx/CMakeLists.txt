# =============================================================================
# Vectorization Library C++ Tests
# =============================================================================

xsigma_add_googletest()

# =============================================================================
# VectorizationCxxTests executable
# =============================================================================

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

# Include CUDA/HIP unit tests when a GPU backend is active, and compile every
# Test*.cpp that touches tensor<T> expressions as that backend's device
# language — not just the files literally named *Gpu*.
#
# Why more than *Gpu* files: tensor<T> is device-polymorphic at RUNTIME
# (device is a field, not part of the type), so the exact same expression-tree
# type (e.g. binary_expression<tensor<float>,tensor<float>,add_evaluator>)
# gets instantiated by expressions_evaluator::run<E,T>()/fill<E,T>() from both
# GPU-touching files (TestTensorGpu.cpp) and plain CPU-only files (TestTensor.cpp,
# TestTensorF2.cpp, ...) alike. run()'s compiled body depends on whether
# __CUDACC__/__HIPCC__ is defined in the compiling TU (see
# expressions_evaluator.h): with only TestTensorGpu.cpp tagged, that
# preprocessor state — and thus the compiled body — differed across
# translation units for the SAME template instantiation. That's a real ODR
# violation: two definitions of the same external-linkage symbol disagree,
# and the linker silently keeps only one. In practice it always kept the
# CPU-only body, so any GPU-tensor expression whose type was *also* used by
# a plain .cpp file elsewhere in the binary silently fell through to
# check_cpu_reachable() and threw at runtime instead of dispatching to the
# GPU kernel — confirmed via dumpbin: TestTensor.cpp.obj contained its own
# (CPU-only) definition of run<binary_expression<tensor<float>,tensor<float>,
# add_evaluator>, tensor<float>> that the linker preferred over
# TestTensorGpu.cpp.obj's GPU-dispatching one.
#
# Why NOT literally every Test*.cpp: files that never touch tensor<T> (e.g.
# TestSimdUtility.cpp, which exercises simd<T> packet intrinsics directly)
# don't instantiate run<E,T>/fill<E,T> at all, so they carry no ODR risk —
# and tagging them as CUDA anyway actively breaks them, since simd<T>'s
# host-only AVX/SSE members (copy/broadcast/ptranspose/...) aren't available
# under __CUDACC__. Select by content (grep for "tensor<") rather than a
# hardcoded file list so a new tensor-touching test file is covered
# automatically instead of silently reintroducing the ODR gap.
#
# The historical "Clang + CUDA 13.x texture-intrinsics incompatibility"
# exclusion for Test*.cu/TestTensorGpu.cpp no longer reproduces on the
# current toolchain (Clang 22 + CUDA 13.2): both compile and pass cleanly.
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda" OR VECTORIZATION_GPU_BACKEND STREQUAL "hip")
  set(_vec_tensor_test_sources "")
  foreach(_vec_src IN LISTS test_sources)
    file(STRINGS "${_vec_src}" _vec_tensor_hit REGEX "tensor<" LIMIT_COUNT 1)
    if(_vec_tensor_hit)
      list(APPEND _vec_tensor_test_sources "${_vec_src}")
    endif()
    unset(_vec_tensor_hit)
  endforeach()
  unset(_vec_src)

  file(GLOB_RECURSE cuda_test_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu"
  )
  list(APPEND test_sources ${cuda_test_sources})
  list(APPEND _vec_tensor_test_sources ${cuda_test_sources})
  vectorization_tag_gpu_expression_sources(${_vec_tensor_test_sources})

  if(VECTORIZATION_GPU_BACKEND STREQUAL "hip")
    # NOTE: unverified on Windows this session — HIP is Unix-only by project
    # policy (Library/Vectorization/CLAUDE.md); mirrors the CUDA branch above
    # for when this is next built on Linux/ROCm. No longer split into a
    # separate executable (the old rationale was to avoid tagging unrelated
    # host SIMD tests with LANGUAGE HIP / --offload-arch; content-based
    # selection above already excludes them).
    message(STATUS "VectorizationCxxTests: tensor<T>-touching sources compiled as HIP "
                   "(ODR-safety requirement — see comment above; unverified on Windows)"
    )
  else()
    message(STATUS "VectorizationCxxTests: tensor<T>-touching sources compiled as CUDA "
                   "(ODR-safety requirement — see comment above)"
    )
  endif()
  # NOTE: _vec_tensor_test_sources stays set past this block — the CUDA-specific
  # properties section below (heavy-template -O1 crash workaround) needs the same
  # list, since every file in it is compiled as CUDA and can hit the same
  # expression-template ICEs, not just TestTensorGpu.cpp. Unset there instead.
endif()

# Include Metal Objective-C++ unit tests when the Metal GPU backend is active. Unlike
# CUDA/HIP, Metal test sources are ordinary Objective-C++ (no CMake "LANGUAGE Metal" —
# there is no such thing; OBJCXX is already enabled at the Vectorization target level)
# and need no special per-file compiler-workaround properties.
if(VECTORIZATION_GPU_BACKEND STREQUAL "metal")
  # *.mm (not just Test*.mm): also picks up metal_device_probe.mm, the tiny Objective-C++
  # shim TestTensorGpu.cpp calls into for a device-count query (that file itself must
  # stay plain .cpp, shared textually with the CUDA/HIP path).
  file(GLOB_RECURSE metal_test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/*.mm")
  list(APPEND test_sources ${metal_test_sources})
  set_source_files_properties(${metal_test_sources} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
  message(STATUS "VectorizationCxxTests: Metal-specific tests (*.mm) included")
endif()

# LibTorch comparison tests require LibTorch; skip compiling them entirely
# when VECTORIZATION_ENABLE_LIBTORCH is OFF instead of building empty
# translation units guarded by #if VECTORIZATION_HAS_LIBTORCH.
if(NOT VECTORIZATION_ENABLE_LIBTORCH)
  list(FILTER test_sources EXCLUDE REGEX ".*TestTensorLibTorch.*\\.cpp$")
  message(STATUS "VectorizationCxxTests: TestTensorLibTorch*.cpp excluded — "
                 "VECTORIZATION_ENABLE_LIBTORCH is OFF"
  )
endif()

add_executable(VectorizationCxxTests ${test_sources})

target_compile_definitions(
  VectorizationCxxTests PRIVATE XSIGMA_HAS_GTEST=1 XSIGMA_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

target_link_libraries(
  VectorizationCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Vectorization::Vectorization
)

# ---------------------------------------------------------------------------
# LibTorch (PyTorch C++ frontend) — auto-detected via CMAKE_PREFIX_PATH
# ---------------------------------------------------------------------------
# Isolate from our project's custom find-modules so LibTorch's own Caffe2
# dependencies (e.g. FindMKL) use CMake's built-ins, not ours.
# LibTorch's bundled Caffe2/FindCUDA requires the nvcc-only CMake internal
# _CMAKE_CUDA_WHOLE_FLAG which is not set when Clang is the CUDA compiler.
# Skip the probe entirely in that configuration to avoid a fatal CMake error.
#
# Also skip when CMAKE_PREFIX_PATH resolves to a CUDA-enabled LibTorch build
# on Windows with a GNU-frontend Clang host compiler (plain clang.exe, not
# clang-cl): Caffe2Config.cmake unconditionally calls enable_language(CUDA)
# in that case, which drives CMake to nvcc. nvcc only accepts MSVC cl.exe or
# clang-cl.exe as a Windows host compiler — with plain clang.exe it fails
# with "nvcc fatal : Host compiler targets unsupported OS" — regardless of
# our own VECTORIZATION_GPU_BACKEND selection.
if(NOT VECTORIZATION_ENABLE_LIBTORCH)
  message(
    STATUS
      "LibTorch disabled (pass 'torch' to setup.py, or -DVECTORIZATION_ENABLE_LIBTORCH=ON, to enable)"
  )
elseif(
  VECTORIZATION_GPU_BACKEND STREQUAL "cuda"
  AND CMAKE_CUDA_COMPILER_ID STREQUAL "Clang"
  OR WIN32
  AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
  AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"
)
  message(STATUS "LibTorch not probed — GNU-frontend Clang is incompatible with "
                 "LibTorch's CUDA detection (nvcc requires MSVC cl.exe or clang-cl on Windows)"
  )
else()
  set(_saved_module_path "${CMAKE_MODULE_PATH}")
  set(CMAKE_MODULE_PATH "")
  # Force MKL single-dynamic-library mode so MKLConfig.cmake emits only
  # -lmkl_rt instead of the full component set (ScaLAPACK/BLACS/CDFT), which
  # would pull in MPI symbols that may not be installed on this machine.
  set(MKL_LINK "sdl")
  find_package(Torch QUIET)
  unset(MKL_LINK)
  set(CMAKE_MODULE_PATH "${_saved_module_path}")
endif()
if(Torch_FOUND)
  message(STATUS "LibTorch found: ${TORCH_INSTALL_PREFIX} (${Torch_VERSION})")
  # pip-installed MKL ships versioned .so.N files without the unversioned .so
  # development symlinks that the linker needs for -lmkl_NAME resolution.
  # caffe2::mkl (created by LibTorch's mkl.cmake) holds the MKL link-search
  # directories; walk them and create any missing unversioned .so symlinks.
  if(TARGET caffe2::mkl)
    get_target_property(_mkl_link_dirs caffe2::mkl INTERFACE_LINK_DIRECTORIES)
  else()
    set(_mkl_link_dirs "")
  endif()
  foreach(_mkl_dir IN LISTS _mkl_link_dirs)
    file(GLOB _mkl_ver_libs "${_mkl_dir}/libmkl_*.so.[0-9]*")
    foreach(_vlib IN LISTS _mkl_ver_libs)
      string(REGEX REPLACE "\\.so\\.[0-9].*$" ".so" _unversioned "${_vlib}")
      if(NOT EXISTS "${_unversioned}")
        get_filename_component(_vlib_base "${_vlib}" NAME)
        get_filename_component(_unver_base "${_unversioned}" NAME)
        execute_process(
          COMMAND ${CMAKE_COMMAND} -E create_symlink "${_vlib_base}" "${_unversioned}"
          RESULT_VARIABLE _sym_rc
        )
        if(_sym_rc EQUAL 0)
          message(STATUS "MKL: created linker symlink ${_unver_base} -> ${_vlib_base}")
        endif()
      endif()
    endforeach()
  endforeach()
  unset(_mkl_link_dirs)
  unset(_mkl_ver_libs)
elseif(VECTORIZATION_ENABLE_LIBTORCH)
  message(
    STATUS
      "LibTorch not found — LibTorch tests skipped (add install dir to CMAKE_PREFIX_PATH to enable)"
  )
endif()

if(Torch_FOUND)
  target_link_libraries(VectorizationCxxTests PRIVATE torch)
  target_compile_definitions(VectorizationCxxTests PRIVATE VECTORIZATION_HAS_LIBTORCH=1)
  target_compile_options(
    VectorizationCxxTests
    PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations
            -Wno-unused-parameter> $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations
            -Wno-unused-parameter>
  )
  if(WIN32)
    file(GLOB _torch_dlls "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(
      TARGET VectorizationCxxTests POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_torch_dlls}
              $<TARGET_FILE_DIR:VectorizationCxxTests>
    )
  endif()
  if(MEMORY_GPU_BACKEND STREQUAL "cuda")
    target_compile_definitions(VectorizationCxxTests PRIVATE VECTORIZATION_LIBTORCH_CUDA_REQUIRED=1)
    message(STATUS "VectorizationCxxTests: LibTorch CUDA tests REQUIRED (MEMORY_GPU_BACKEND=cuda)")
  else()
    target_compile_definitions(VectorizationCxxTests PRIVATE VECTORIZATION_LIBTORCH_CUDA_REQUIRED=0)
  endif()
  message(STATUS "VectorizationCxxTests: LibTorch comparison tests ENABLED")
else()
  target_compile_definitions(
    VectorizationCxxTests PRIVATE VECTORIZATION_HAS_LIBTORCH=0
                                  VECTORIZATION_LIBTORCH_CUDA_REQUIRED=0
  )
endif()

set_target_properties(
  VectorizationCxxTests
  PROPERTIES FOLDER "Tests"
             CXX_STANDARD ${VECTORIZATION_CXX_STANDARD}
             CXX_STANDARD_REQUIRED ON
             VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# CUDA-specific properties for the test executable when any CUDA source is present
# (.cu files or .cpp files compiled as CUDA via LANGUAGE CUDA).
set(_vectorization_has_cuda_gpu_test_source FALSE)
if(cuda_test_sources OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
  set(_vectorization_has_cuda_gpu_test_source TRUE)
endif()
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda" AND _vectorization_has_cuda_gpu_test_source)
  set_target_properties(
    VectorizationCxxTests PROPERTIES CUDA_ARCHITECTURES "${VECTORIZATION_CUDA_ARCHITECTURES}"
  )
  # CMAKE_CUDA_COMPILER_ID is "unknown" when CMAKE_CUDA_COMPILER_FORCED=TRUE (the
  # Clang+CMake 3.28 workaround in cuda.cmake skips the compiler probe). Use
  # CMAKE_CXX_COMPILER_ID as the reliable fallback: when Clang is the CUDA compiler
  # it is always also the CXX compiler, so CMAKE_CXX_COMPILER_ID is always "Clang".
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # The CUDA_ARCHITECTURES property set above is a no-op for flag generation here:
    # CMAKE_CUDA_COMPILER_FORCED (see cuda.cmake) skips the compiler-probe step that
    # would normally load Clang's CUDA arch-flag-generation rules, so CMake never
    # translates CUDA_ARCHITECTURES into --cuda-gpu-arch flags. Without them Clang
    # falls back to its own default (sm_35), which CUDA 12+ toolkits reject outright.
    # Emit the flags directly instead of relying on the property.
    foreach(_vectorization_cuda_arch IN LISTS VECTORIZATION_CUDA_ARCHITECTURES)
      target_compile_options(
        VectorizationCxxTests
        PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--cuda-gpu-arch=sm_${_vectorization_cuda_arch}>
      )
    endforeach()
    unset(_vectorization_cuda_arch)

    # `-Xclang -fno-cuda-host-device-constexpr` used to be applied here as a
    # target-wide flag to work around a Clang CUDA + C++20 ambiguity where
    # std::lerp's implicit __host__ __device__ annotation collided with the
    # device-only std::fma overloads from __clang_cuda_math_forward_declares.h.
    # That was pulled after it broke __clang_cuda_texture_intrinsics.h
    # (included unconditionally by __clang_cuda_runtime_wrapper.h for every
    # CUDA TU) target-wide. On Clang 22.1.2 + CUDA 13.2 the lerp/fma ambiguity
    # reproduces again (confirmed: TestTensor.cpp, TestTensorF1/F2/F3.cpp,
    # TestTensorGpu.cpp, TestTensorGpuKineto.cpp, TestGpuSimd.cu all fail with
    # "call to 'fma' is ambiguous" instantiating std::_Linear_for_lerp<long
    # double> from MSVC's <cmath>), so it's back — scoped via
    # set_source_files_properties() to just _vec_tensor_test_sources as
    # planned above, instead of target-wide, so it doesn't reach any other
    # CUDA TU in this target that might rely on texture intrinsics.
    #
    # Clang -O3 crashes (ICE in the always-inline / GVN passes, confirmed on both
    # Clang 20 and Clang 22) when compiling heavy tensor.h expression templates.
    # -O1 is sufficient for integration tests and avoids the ICE. This applies to
    # every source in _vec_tensor_test_sources, not just TestTensorGpu.cpp — the
    # ODR-safety tagging above (see comment at the top of this file) compiles
    # TestTensor.cpp, TestTensorF2.cpp, etc. as CUDA too, and they instantiate the
    # same expression-tree templates, so they're equally exposed to the crash.
    set_source_files_properties(
      ${_vec_tensor_test_sources}
      PROPERTIES
        COMPILE_OPTIONS
        "$<$<COMPILE_LANGUAGE:CUDA>:-Xclang>;$<$<COMPILE_LANGUAGE:CUDA>:-fno-cuda-host-device-constexpr>"
    )
    # Clang on Windows does not populate CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES,
    # so lld-link cannot find cudadevrt.lib / cudart_static.lib that Clang
    # adds automatically via -l flags. Point the linker at the CUDA lib dir.
    if(WIN32)
      target_link_directories(
        VectorizationCxxTests PRIVATE "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib/x64"
      )
    else()
      target_link_directories(
        VectorizationCxxTests PRIVATE "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib64"
      )
    endif()
  else()
    # nvcc (cicc) forces fmt's FMT_USE_INT128 fallback (fmt::detail::uint128,
    # no operator~) even for host-only formatting of long double — see
    # cuda_fmt_int128_fix.h for the full explanation. The ADL shim is
    # #include-d directly at the top of TestGpuSimd.cu / TestTensorGpu.cpp
    # rather than force-included via -include here: nvcc reapplies generic
    # compiler options (including -include) when it recompiles its own
    # flattened cudafe1.cpp intermediate, which would re-inject all of <fmt>
    # a second time with no header guards left (they were already consumed
    # during the first preprocessing pass) and corrupt the parser's
    # namespace state — see the git history of this file for the resulting
    # cascade of "conflicting declaration" / "different namespace" errors.

    # GCC 13 + -O3 crashes (internal compiler error in try_forward_edges,
    # cfgcleanup.cc) while recompiling nvcc's generated host stub for the
    # heavily-templated gpu_eval_kernel/tensor.h expression-template
    # instantiations, and cicc (nvcc's device-code frontend) has also been
    # observed to segfault compiling the same GPU test sources at -O3 —
    # both are the compiler backends choking on the generated code volume
    # (the flattened intermediate translation units run 150k+ lines across
    # 5 GPU architectures), not a source-level bug. -O1 is sufficient for
    # integration tests and avoids both crashes. Applies to every source in
    # _vec_tensor_test_sources (already includes TestGpuSimd.cu via the Test*.cu
    # glob), not just TestTensorGpu.cpp — see the ODR-safety comment at the top
    # of this file for why TestTensor.cpp/TestTensorF2.cpp/etc. are equally
    # exposed once they're tagged as CUDA sources.
    set_source_files_properties(
      ${_vec_tensor_test_sources} PROPERTIES COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CUDA>:-O1>"
    )
  endif()
  unset(_vec_tensor_test_sources)
  message(
    STATUS
      "VectorizationCxxTests: CUDA simd tests enabled (archs: ${VECTORIZATION_CUDA_ARCHITECTURES})"
  )
endif()
unset(_vectorization_has_cuda_gpu_test_source)

# HIP GPU tests: own executable so host CXX tests never see --offload-arch.
set(_vectorization_hip_gpu_sources ${hip_test_sources})
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
  list(APPEND _vectorization_hip_gpu_sources "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
endif()
if(VECTORIZATION_GPU_BACKEND STREQUAL "hip" AND _vectorization_hip_gpu_sources)
  add_executable(VectorizationHipCxxTests ${_vectorization_hip_gpu_sources})
  target_compile_definitions(
    VectorizationHipCxxTests PRIVATE XSIGMA_HAS_GTEST=1 XSIGMA_GOOGLE_TEST USE_GTEST
                                     _VARIADIC_MAX=10
  )
  target_link_libraries(
    VectorizationHipCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Vectorization::Vectorization
  )
  if(TARGET hip::device)
    target_link_libraries(VectorizationHipCxxTests PRIVATE hip::device)
  endif()
  set_target_properties(
    VectorizationHipCxxTests PROPERTIES HIP_STANDARD ${VECTORIZATION_CXX_STANDARD}
                                        HIP_ARCHITECTURES "${VECTORIZATION_GPU_ARCHITECTURES}"
  )
  add_test(NAME VectorizationHipCxxTests COMMAND VectorizationHipCxxTests
           WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  )
  set_tests_properties(VectorizationHipCxxTests PROPERTIES TIMEOUT 120 LABELS "Vectorization;Unit")
  message(
    STATUS
      "VectorizationHipCxxTests: HIP simd tests enabled (archs: ${VECTORIZATION_GPU_ARCHITECTURES})"
  )
endif()
unset(_vectorization_hip_gpu_sources)

add_test(NAME VectorizationCxxTests COMMAND VectorizationCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(VectorizationCxxTests PROPERTIES TIMEOUT 120 LABELS "Vectorization;Unit")

# ThinLTO + lld-link OOM: when project-wide IPO is on, INTERPROCEDURAL_OPTIMIZATION=OFF alone is
# not enough — -flto stays on the compile line through PUBLIC options. Force -fno-lto explicitly.
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(VectorizationCxxTests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
  target_compile_options(VectorizationCxxTests PRIVATE -fno-lto)
  target_link_options(VectorizationCxxTests PRIVATE -fno-lto)
  if(TARGET gtest AND TARGET gtest_main)
    set_target_properties(gtest gtest_main PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
    target_compile_options(gtest PRIVATE -fno-lto)
    target_compile_options(gtest_main PRIVATE -fno-lto)
  endif()
endif()

xsigma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

if(COMMAND xsigma_set_target_output_directories)
  xsigma_set_target_output_directories(VectorizationCxxTests)
endif()

message(STATUS "VectorizationCxxTests registered with CTest")

# =============================================================================
# Individual benchmark
# executables (same pattern as Library/Parallel/Testing/Cxx)
# =============================================================================
if(VECTORIZATION_ENABLE_BENCHMARK)
  # --- Google Benchmark setup ---
  if(NOT TARGET benchmark)
    set(_bench_external_found FALSE)
    if(XSIGMA_ENABLE_EXTERNAL)
      find_package(benchmark QUIET)
      if(benchmark_FOUND)
        message(STATUS "Found external Google Benchmark")
        _create_third_party_interface_targets(
          "Benchmark::benchmark=benchmark::benchmark|benchmark"
          "Benchmark::benchmark_main=benchmark::benchmark_main|benchmark_main"
        )
        set(_bench_external_found TRUE)
      endif()
    endif()
    if(NOT _bench_external_found)
      if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        string(APPEND CMAKE_CXX_FLAGS " -Wno-c2y-extensions")
      endif()
      if(NOT DEFINED HAVE_STD_REGEX)
        set(HAVE_STD_REGEX 1 CACHE INTERNAL "")
      endif()
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
      add_subdirectory(
        "${XSIGMA_THIRDPARTY_DIR}/benchmark" "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build"
        EXCLUDE_FROM_ALL
      )
      _set_third_party_folder_properties(
        "benchmark" "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build"
      )
    endif()
  endif()
  _create_third_party_interface_targets(
    "Benchmark::benchmark=benchmark::benchmark|benchmark"
    "Benchmark::benchmark_main=benchmark::benchmark_main|benchmark_main"
  )
  if(TARGET Benchmark::benchmark AND NOT TARGET Vectorization::benchmark)
    add_library(Vectorization::benchmark ALIAS benchmark)
    add_library(Vectorization::benchmark_main ALIAS benchmark_main)
  endif()

  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp"
  )

  if(VECTORIZATION_CPU_BACKEND STREQUAL "no")
    list(FILTER bench_sources EXCLUDE REGEX ".*BenchmarkSimd.*\\.cpp$")
    message(
      STATUS
        "Vectorization: VECTORIZATION_CPU_BACKEND=no — BenchmarkSimd*.cpp excluded (no SIMD backend active)"
    )
  endif()

  # BenchmarkTensorGpu.cpp used to be excluded here for the same (now-stale,
  # see the VectorizationCxxTests comment above) Clang+CUDA13 texture-
  # intrinsics report. Unlike VectorizationCxxTests, each benchmark below is
  # its own single-source executable (add_executable(${target_name}
  # "${_bench_source}")), so there is no cross-file ODR concern here — the
  # existing per-file vectorization_tag_gpu_expression_sources() call further
  # down (bench_name STREQUAL "BenchmarkTensorGpu") is sufficient on its own.

  # LibTorch comparison benchmark requires LibTorch; skip building it entirely
  # (no benchmark_tensorlibtorch target at all) when disabled, instead of
  # building a stub linked without torch.
  if(NOT VECTORIZATION_ENABLE_LIBTORCH)
    list(FILTER bench_sources EXCLUDE REGEX ".*BenchmarkTensorLibTorch\\.cpp$")
    message(STATUS "Vectorization: BenchmarkTensorLibTorch.cpp excluded — "
                   "VECTORIZATION_ENABLE_LIBTORCH is OFF"
    )
  endif()

  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    set(_bench_is_gpu FALSE)
    set(_bench_gpu_lang "")
    if(bench_name STREQUAL "BenchmarkTensorGpu")
      # Processed by nvcc/hipcc to instantiate GPU expression-template kernels
      # (same treatment as TestTensorGpu.cpp in VectorizationCxxTests above).
      vectorization_tag_gpu_expression_sources("${_bench_source}" RESULT_LANGUAGE _bench_gpu_lang)
      if(_bench_gpu_lang)
        set(_bench_is_gpu TRUE)
      endif()
    endif()

    add_executable(${target_name} "${_bench_source}")

    if(_bench_is_gpu AND _bench_gpu_lang STREQUAL "CUDA")
      set_target_properties(
        ${target_name}
        PROPERTIES CUDA_STANDARD ${VECTORIZATION_CXX_STANDARD}
                   CUDA_STANDARD_REQUIRED ON
                   CUDA_ARCHITECTURES "${VECTORIZATION_CUDA_ARCHITECTURES}"
                   CUDA_SEPARABLE_COMPILATION OFF
      )
      # nvcc (cicc) fmt int128 fallback issue — same ADL shim as
      # VectorizationCxxTests above (see cuda_fmt_int128_fix.h).
      target_compile_options(
        ${target_name}
        PRIVATE
          $<$<COMPILE_LANGUAGE:CUDA>:-include${CMAKE_CURRENT_SOURCE_DIR}/cuda_fmt_int128_fix.h>
      )
      # Same Clang CUDA + MSVC C++20 std::fma ambiguity as VectorizationCxxTests
      # above (see the detailed comment there) — Clang implicitly marks
      # std::lerp's internal std::fma call __host__ __device__, colliding with
      # the device-only overloads pulled in via __clang_cuda_math_forward_declares.h.
      if(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # CUDA_ARCHITECTURES set above is a no-op for flag generation here -- same
        # CMAKE_CUDA_COMPILER_FORCED issue as VectorizationCxxTests above. Emit the
        # flags directly instead of relying on the property.
        foreach(_vectorization_cuda_arch IN LISTS VECTORIZATION_CUDA_ARCHITECTURES)
          target_compile_options(
            ${target_name}
            PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--cuda-gpu-arch=sm_${_vectorization_cuda_arch}>
          )
        endforeach()
        unset(_vectorization_cuda_arch)

        target_compile_options(
          ${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xclang
                                 -fno-cuda-host-device-constexpr>
        )
        # Same Clang -O3 ICE (always-inline / GVN passes, confirmed on both Clang 20
        # and Clang 22) as VectorizationCxxTests' _vec_tensor_test_sources above —
        # BenchmarkTensorGpu.cpp instantiates the same heavy tensor.h expression
        # templates. -O1 is sufficient for benchmarking and avoids the ICE.
        #target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-O1>)
        if(WIN32)
          target_link_directories(
            ${target_name} PRIVATE "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib/x64"
          )
        else()
          target_link_directories(
            ${target_name} PRIVATE "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib64"
          )
        endif()
      endif()
    elseif(_bench_is_gpu AND _bench_gpu_lang STREQUAL "HIP")
      set_target_properties(
        ${target_name} PROPERTIES HIP_STANDARD ${VECTORIZATION_CXX_STANDARD}
                                  HIP_ARCHITECTURES "${VECTORIZATION_GPU_ARCHITECTURES}"
      )
    endif()

    target_link_libraries(
      ${target_name} PRIVATE Vectorization::benchmark Vectorization::benchmark_main
                             Vectorization::Vectorization
    )

    # ThinLTO + lld-link OOM: same guard as VectorizationCxxTests above.
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      set_target_properties(${target_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
      target_compile_options(${target_name} PRIVATE -fno-lto)
      target_link_options(${target_name} PRIVATE -fno-lto)
    endif()

    set_target_properties(
      ${target_name} PROPERTIES FOLDER "Benchmarks" OUTPUT_NAME "${target_name}"
    )

    if(COMMAND xsigma_set_target_output_directories)
      xsigma_set_target_output_directories(${target_name})
    else()
      set_target_properties(
        ${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      )
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name} --benchmark_min_time=0.01s
             WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    set_tests_properties(
      ${target_name} PROPERTIES TIMEOUT 600 LABELS "Benchmark;Performance;Vectorization"
    )

    message(STATUS "✓ Benchmark target configured: ${target_name} (from ${bench_name}.cpp)")
  endforeach()

  list(LENGTH bench_sources bench_count)
  message(STATUS "✓ Individual benchmark executables configured (${bench_count} total)")

  # ---------------------------------------------------------------------------
  # BenchmarkTensorLibTorch: link LibTorch when available, otherwise stub out.
  # BenchmarkTensorGpu (Metal): also link LibTorch for LibTorch_MPS_TensorAllocFree.
  # ---------------------------------------------------------------------------
  if(TARGET benchmark_tensorlibtorch)
    if(Torch_FOUND)
      target_link_libraries(benchmark_tensorlibtorch PRIVATE torch)
      target_compile_definitions(benchmark_tensorlibtorch PRIVATE VECTORIZATION_HAS_LIBTORCH=1)
      target_compile_options(
        benchmark_tensorlibtorch
        PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations
                -Wno-unused-parameter> $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations
                -Wno-unused-parameter>
      )
      if(WIN32)
        file(GLOB _torch_dlls "${TORCH_INSTALL_PREFIX}/lib/*.dll")
        add_custom_command(
          TARGET benchmark_tensorlibtorch POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_torch_dlls}
                  $<TARGET_FILE_DIR:benchmark_tensorlibtorch>
        )
      endif()
      message(STATUS "benchmark_tensorlibtorch: LibTorch comparison benchmarks ENABLED")
    else()
      target_compile_definitions(benchmark_tensorlibtorch PRIVATE VECTORIZATION_HAS_LIBTORCH=0)
      message(STATUS "benchmark_tensorlibtorch: LibTorch not found — benchmark stubs only")
    endif()
  endif()

  if(TARGET benchmark_tensorgpu)
    if(Torch_FOUND AND VECTORIZATION_GPU_BACKEND STREQUAL "metal")
      target_link_libraries(benchmark_tensorgpu PRIVATE torch)
      target_compile_definitions(benchmark_tensorgpu PRIVATE VECTORIZATION_HAS_LIBTORCH=1)
      target_compile_options(
        benchmark_tensorgpu
        PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations
                -Wno-unused-parameter> $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations
                -Wno-unused-parameter>
      )
      message(STATUS "benchmark_tensorgpu: LibTorch MPS alloc comparison ENABLED")
    else()
      target_compile_definitions(benchmark_tensorgpu PRIVATE VECTORIZATION_HAS_LIBTORCH=0)
    endif()
  endif()
endif()
