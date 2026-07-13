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
                   "texture-intrinsics incompatibility")
  else()
    file(GLOB_RECURSE cuda_test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu")
    list(APPEND test_sources ${cuda_test_sources})

    # TestTensorGpu.cpp: processed by nvcc to instantiate GPU expression-template kernels.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp")
      set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp"
        PROPERTIES LANGUAGE CUDA
      )
    endif()
  endif()
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
if((VECTORIZATION_GPU_BACKEND STREQUAL "cuda" AND CMAKE_CUDA_COMPILER_ID STREQUAL "Clang")
   OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  message(STATUS "LibTorch not probed — GNU-frontend Clang is incompatible with "
                 "LibTorch's CUDA detection (nvcc requires MSVC cl.exe or clang-cl on Windows)")
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
else()
  message(STATUS "LibTorch not found — LibTorch tests skipped (add install dir to CMAKE_PREFIX_PATH to enable)")
endif()

if(Torch_FOUND)
  target_link_libraries(VectorizationCxxTests PRIVATE torch)
  target_compile_definitions(VectorizationCxxTests PRIVATE VECTORIZATION_HAS_LIBTORCH=1)
  target_compile_options(VectorizationCxxTests PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations -Wno-unused-parameter>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations -Wno-unused-parameter>
  )
  if(WIN32)
    file(GLOB _torch_dlls "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET VectorizationCxxTests POST_BUILD
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
  target_compile_definitions(VectorizationCxxTests PRIVATE VECTORIZATION_HAS_LIBTORCH=0
                                                           VECTORIZATION_LIBTORCH_CUDA_REQUIRED=0)
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
if(VECTORIZATION_GPU_BACKEND STREQUAL "cuda" AND
   (cuda_test_sources OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp"))
  set_target_properties(VectorizationCxxTests PROPERTIES
    CUDA_ARCHITECTURES "${VECTORIZATION_CUDA_ARCHITECTURES}"
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
    target_compile_options(VectorizationCxxTests PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:-Xclang -fno-cuda-host-device-constexpr>
    )
    # Clang 22 + -O3 crashes in the always-inline pass when compiling
    # TestTensorGpu.cpp (heavy tensor.h expression templates).  -O1 is sufficient
    # for integration tests and avoids the ICE.
    set_source_files_properties(
      "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorGpu.cpp"
      PROPERTIES COMPILE_OPTIONS "$<$<COMPILE_LANGUAGE:CUDA>:-O1>"
    )
    # Clang on Windows does not populate CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES,
    # so lld-link cannot find cudadevrt.lib / cudart_static.lib that Clang
    # adds automatically via -l flags. Point the linker at the CUDA lib dir.
    if(WIN32)
      target_link_directories(VectorizationCxxTests PRIVATE
        "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib/x64"
      )
    else()
      target_link_directories(VectorizationCxxTests PRIVATE
        "${CMAKE_CUDA_COMPILER_TOOLKIT_LIBRARY_ROOT}/lib64"
      )
    endif()
  else()
    # nvcc (cudafe) #3328: fmt 12.x uses unsigned __int128 as carrier_uint for
    # long double formatting; nvcc rejects ~(inc-1) on __int128 in device-code
    # parsing even though the fmt path is host-only.  Suppress the diagnostic.
    target_compile_options(VectorizationCxxTests PRIVATE
      $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=3328>
    )
  endif()
  message(STATUS "VectorizationCxxTests: CUDA simd tests enabled (archs: ${VECTORIZATION_CUDA_ARCHITECTURES})")
endif()

add_test(NAME VectorizationCxxTests COMMAND VectorizationCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Clang 20 + -O3 crashes (optimizer DSE / copy-propagation / instruction-selector)
# on large functions in test files that heavily instantiate tensor<T> expression
# templates: the LLVM inliner expands SIMD constructors into monolithic functions
# that overflow internal data structures. -O0 avoids the ICE; correctness tests
# don't need aggressive optimization (matching the TestTensorGpu.cpp precedent).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorF2.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorch2D.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchBinaryFns.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchBinaryOps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchElementwise.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchExpLog.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchGpu.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchHyperbolic.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchMultiDim.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchPowerRoot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchRounding.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchTensor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTensorLibTorchTrig.cpp"
    PROPERTIES COMPILE_OPTIONS "-O0"
  )
endif()
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

  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    add_executable(${target_name} "${_bench_source}")

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
  # ---------------------------------------------------------------------------
  if(TARGET benchmark_tensorlibtorch)
    if(Torch_FOUND)
      target_link_libraries(benchmark_tensorlibtorch PRIVATE torch)
      target_compile_definitions(benchmark_tensorlibtorch PRIVATE VECTORIZATION_HAS_LIBTORCH=1)
      target_compile_options(benchmark_tensorlibtorch PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-deprecated-declarations -Wno-unused-parameter>
        $<$<CXX_COMPILER_ID:GNU>:-Wno-deprecated-declarations -Wno-unused-parameter>
      )
      if(WIN32)
        file(GLOB _torch_dlls "${TORCH_INSTALL_PREFIX}/lib/*.dll")
        add_custom_command(TARGET benchmark_tensorlibtorch POST_BUILD
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
endif()
