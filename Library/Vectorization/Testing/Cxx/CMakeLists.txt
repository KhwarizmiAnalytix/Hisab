# =============================================================================
# Vectorization Library C++ Tests
# =============================================================================

# --- GoogleTest setup --- Core/Testing/Cxx builds gtest first in a full-project build; this block
# is a no-op in that case (TARGET gtest already exists). It activates when Vectorization tests are
# built in isolation.
if(NOT TARGET gtest)
  set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "Disable Google Test install" FORCE)
  set(gtest_build_samples OFF CACHE BOOL "Disable Google Test samples" FORCE)
  set(gtest_build_tests OFF CACHE BOOL "Disable Google Test tests" FORCE)
  if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  endif()
  add_subdirectory(
    "${QUARISMA_THIRDPARTY_DIR}/googletest" "${CMAKE_BINARY_DIR}/ThirdParty/googletest_build"
    EXCLUDE_FROM_ALL
  )
endif()
_create_third_party_interface_targets(
  "Gtest::gtest=GTest::gtest|gtest" "Gtest::gtest_main=GTest::gtest_main|gtest_main"
)

# =============================================================================
# VectorizationCxxTests executable
# =============================================================================

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

add_executable(VectorizationCxxTests ${test_sources})

target_compile_definitions(
  VectorizationCxxTests PRIVATE QUARISMA_HAS_GTEST=1 QUARISMA_GOOGLE_TEST USE_GTEST
                                _VARIADIC_MAX=10
)

target_link_libraries(
  VectorizationCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Vectorization::Vectorization
)

set_target_properties(
  VectorizationCxxTests
  PROPERTIES FOLDER "Tests"
             CXX_STANDARD ${VECTORIZATION_CXX_STANDARD}
             CXX_STANDARD_REQUIRED ON
             VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

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

  if(VECTORIZATION_TYPE STREQUAL "no")
    list(FILTER bench_sources EXCLUDE REGEX ".*BenchmarkSimd.*\\.cpp$")
    message(
      STATUS
        "Vectorization: VECTORIZATION_TYPE=no — BenchmarkSimd*.cpp excluded (no SIMD backend active)"
    )
  endif()

  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    add_executable(${target_name} "${_bench_source}")

    # Clang 22 ICE: LiveDebugValues and GVN crash on heavy template instantiation TUs when
    # -gcodeview + AVX2 (+ SVML) are active.  -O1 reduces IR complexity (avoids GVN);
    # -g0 disables CodeView debug-info emission (avoids LiveDebugValues).
    # Coverage builds: skip -g0 here; combining -fcoverage-mapping
    # with -O1 -g0 can ICE fmt-heavy TUs.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND WIN32)
      get_filename_component(_vectorization_bench_file "${_bench_source}" NAME)
      if(_vectorization_bench_file MATCHES "^BenchmarkSimd.*\\.cpp$"
         AND NOT VECTORIZATION_ENABLE_COVERAGE)
        set_source_files_properties("${_bench_source}" PROPERTIES COMPILE_FLAGS "-O1 -g0")
      endif()
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
endif()
