# =============================================================================
# Vectorization Library C++ Tests
# =============================================================================

quarisma_add_googletest()

# =============================================================================
# VectorizationCxxTests executable
# =============================================================================

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

# Include CUDA simd unit tests when the CUDA GPU backend is active.
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda")
  # Clang as CUDA compiler: Clang 20 + CUDA 13.x has texture-intrinsics
  # incompatibilities (missing texture_fetch_functions.h, __tex_op_hash
  # host-only constexpr called from device code). All Test*.cu files fail.
  # TestTensorGpu.cpp also fails (tensor<T> uses std::vector in device code).
  # Exclude all CUDA-specific test sources when using Clang as the compiler.
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # TestTensorGpu.cpp is in the Test*.cpp glob above; remove it so it is not
    # linked into VectorizationCxxTests. Without LANGUAGE CUDA its GPU kernels
    # would be absent and any TensorGpu test would segfault at runtime.
    list(REMOVE_ITEM test_sources "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
    message(STATUS "VectorizationCxxTests: CUDA-specific tests (Test*.cu, TestTensorGpu.cpp) "
                   "excluded — Clang ${CMAKE_CXX_COMPILER_VERSION} + CUDA ${CUDAToolkit_VERSION} "
                   "texture-intrinsics incompatibility"
    )
  else()
    file(GLOB_RECURSE cuda_test_sources LIST_DIRECTORIES false
         "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu"
    )
    list(APPEND test_sources ${cuda_test_sources})

    # TestTensorGpu.cpp: processed by nvcc to instantiate GPU expression-template kernels.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
      set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp" PROPERTIES LANGUAGE CUDA
      )
    endif()
  endif()
endif()

# Include HIP simd unit tests when the HIP GPU backend is active.
# TestGpuSimd.cu / TestTensorGpu.cpp are shared CUDA+HIP sources (guarded by
# VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP at the C++ level); here we
# just need hipcc to actually process them. CMake's HIP language only
# auto-detects sources by the .hip extension, so LANGUAGE HIP must be set
# explicitly on both — including the .cu file, whose extension would
# otherwise route it to the (unloaded) CUDA language rules.
if(VECTORIZATION_GPU_BACKEND STREQUAL "hip")
  file(GLOB_RECURSE hip_test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu")
  list(APPEND test_sources ${hip_test_sources})
  foreach(_hip_test_src IN LISTS hip_test_sources)
    set_source_files_properties("${_hip_test_src}" PROPERTIES LANGUAGE HIP)
  endforeach()
  unset(_hip_test_src)

  # TestTensorGpu.cpp: processed by hipcc to instantiate GPU expression-template kernels.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
    set_source_files_properties(
      "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp" PROPERTIES LANGUAGE HIP
    )
  endif()
  message(STATUS "VectorizationCxxTests: HIP-specific tests (Test*.cu, TestTensorGpu.cpp) included")
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
  VectorizationCxxTests PRIVATE QUARISMA_HAS_GTEST=1 QUARISMA_GOOGLE_TEST USE_GTEST
                                _VARIADIC_MAX=10
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
  # Clang CUDA + MSVC C++20: std::lerp is constexpr, so Clang implicitly marks it
  # __host__ __device__, making the internal std::fma call ambiguous (both host and
  # device versions are in std:: via __clang_cuda_math_forward_declares.h).
  # -fno-cuda-host-device-constexpr disables the implicit annotation.
  # CMAKE_CUDA_COMPILER_ID is "unknown" when CMAKE_CUDA_COMPILER_FORCED=TRUE (the
  # Clang+CMake 3.28 workaround in cuda.cmake skips the compiler probe). Use
  # CMAKE_CXX_COMPILER_ID as the reliable fallback: when Clang is the CUDA compiler
  # it is always also the CXX compiler, so CMAKE_CXX_COMPILER_ID is always "Clang".
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(
      VectorizationCxxTests PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xclang
                                    -fno-cuda-host-device-constexpr>
    )
    # Clang 22 + -O3 crashes in the always-inline pass when compiling
    # TestTensorGpu.cpp (heavy tensor.h expression templates).  -O1 is sufficient
    # for integration tests and avoids the ICE.
    set_source_files_properties(
      "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp" PROPERTIES COMPILE_OPTIONS
                                                                 "$<$<COMPILE_LANGUAGE:CUDA>:-O1>"
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
    # integration tests and avoids both crashes.
    set_source_files_properties(
      "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/TestGpuSimd.cu"
      PROPERTIES COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CUDA>:-O1>"
    )
  endif()
  message(
    STATUS
      "VectorizationCxxTests: CUDA simd tests enabled (archs: ${VECTORIZATION_CUDA_ARCHITECTURES})"
  )
endif()
unset(_vectorization_has_cuda_gpu_test_source)

# HIP-specific properties for the test executable when any HIP source is present
# (.cu files or .cpp files compiled as HIP via LANGUAGE HIP).
set(_vectorization_has_hip_gpu_test_source FALSE)
if(hip_test_sources OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
  set(_vectorization_has_hip_gpu_test_source TRUE)
endif()
if(VECTORIZATION_GPU_BACKEND STREQUAL "hip" AND _vectorization_has_hip_gpu_test_source)
  set_target_properties(
    VectorizationCxxTests PROPERTIES HIP_ARCHITECTURES "${VECTORIZATION_GPU_ARCHITECTURES}"
  )
  message(
    STATUS
      "VectorizationCxxTests: HIP simd tests enabled (archs: ${VECTORIZATION_GPU_ARCHITECTURES})"
  )
endif()
unset(_vectorization_has_hip_gpu_test_source)

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

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

if(COMMAND quarisma_set_target_output_directories)
  quarisma_set_target_output_directories(VectorizationCxxTests)
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
    if(QUARISMA_ENABLE_EXTERNAL)
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
        "${QUARISMA_THIRDPARTY_DIR}/benchmark" "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build"
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

  # BenchmarkTensorGpu.cpp instantiates GPU expression-template kernels the same
  # way TestTensorGpu.cpp does, so it inherits the same Clang+CUDA13
  # texture-intrinsics incompatibility (see the TestTensorGpu.cpp exclusion
  # above). Without LANGUAGE CUDA the tensor<T> device-dispatch branch in
  # expressions_evaluator.h is silently compiled out (no __CUDACC__), which
  # would make the benchmark dereference device pointers from host code.
  if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda" AND (CMAKE_CUDA_COMPILER_ID STREQUAL "Clang"
                                                    OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  )
    list(FILTER bench_sources EXCLUDE REGEX ".*BenchmarkTensorGpu\\.cpp$")
    message(STATUS "Vectorization: BenchmarkTensorGpu.cpp excluded — Clang CUDA compiler "
                   "incompatibility (matches TestTensorGpu.cpp exclusion)"
    )
  endif()

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
    if(bench_name STREQUAL "BenchmarkTensorGpu" AND VECTORIZATION_GPU_BACKEND STREQUAL "cuda")
      set(_bench_is_gpu TRUE)
      set(_bench_gpu_lang "CUDA")
      # Processed by nvcc to instantiate GPU expression-template kernels
      # (same treatment as TestTensorGpu.cpp in VectorizationCxxTests above).
      set_source_files_properties("${_bench_source}" PROPERTIES LANGUAGE CUDA)
    elseif(bench_name STREQUAL "BenchmarkTensorGpu" AND VECTORIZATION_GPU_BACKEND STREQUAL "hip")
      set(_bench_is_gpu TRUE)
      set(_bench_gpu_lang "HIP")
      # Processed by hipcc to instantiate GPU expression-template kernels
      # (same treatment as TestTensorGpu.cpp in VectorizationCxxTests above).
      set_source_files_properties("${_bench_source}" PROPERTIES LANGUAGE HIP)
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

    if(COMMAND quarisma_set_target_output_directories)
      quarisma_set_target_output_directories(${target_name})
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
