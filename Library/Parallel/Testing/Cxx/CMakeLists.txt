# ============================================================================= Parallel Library C++
# Tests
# =============================================================================

# --- GoogleTest setup ---
if(NOT TARGET gtest)
  set(_gtest_external_found FALSE)
  if(QUARISMA_ENABLE_EXTERNAL)
    find_package(GTest QUIET)
    if(GTest_FOUND)
      message(STATUS "Found external Google Test")
      _create_third_party_interface_targets("Gtest::gtest=GTest::gtest|gtest"
                                            "Gtest::gtest_main=GTest::gtest_main|gtest_main")
      set(_gtest_external_found TRUE)
    endif()
  endif()
  if(NOT _gtest_external_found)
    set(BUILD_GMOCK OFF CACHE BOOL "Disable Google Mock" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Disable Google Test install" FORCE)
    set(gtest_build_samples OFF CACHE BOOL "Disable Google Test samples" FORCE)
    set(gtest_build_tests OFF CACHE BOOL "Disable Google Test tests" FORCE)
    if(MSVC)
      set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(
      "${QUARISMA_THIRDPARTY_DIR}/googletest"
      "${CMAKE_BINARY_DIR}/ThirdParty/googletest_build"
      EXCLUDE_FROM_ALL
    )
    _set_third_party_folder_properties("googletest"
                                       "${CMAKE_BINARY_DIR}/ThirdParty/googletest_build")
  endif()
endif()
_create_third_party_interface_targets("Gtest::gtest=GTest::gtest|gtest"
                                      "Gtest::gtest_main=GTest::gtest_main|gtest_main")

# ============================================================================= Test Source Files
# Discovery
# =============================================================================
file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

set(TestFiles)
foreach(_test_source IN ITEMS ${test_sources})
  get_filename_component(file_name "${_test_source}" NAME)

  # When TBB is disabled, exclude TBB-specific parallel tests
  if(NOT PARALLEL_ENABLE_TBB)
    if(file_name MATCHES "TestTbb" OR file_name MATCHES "TestTBB")
      message(STATUS "Excluding TBB parallel test (TBB disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When OpenMP is disabled, exclude OpenMP-specific parallel tests
  if(NOT PARALLEL_ENABLE_OPENMP)
    if(file_name MATCHES "TestOpenMP" OR file_name MATCHES "TestOmp")
      message(STATUS "Excluding OpenMP parallel test (OpenMP disabled): ${file_name}")
      continue()
    endif()
  endif()

  list(APPEND TestFiles "${_test_source}")
endforeach()

# ============================================================================= ParallelCxxTests
# Executable
# =============================================================================
add_executable(ParallelCxxTests ${TestFiles})

# PARALLEL_HAS_GTEST must be set so ParallelTest.h includes gtest (streaming EXPECT_EQ, SUCCEED,
# etc.).
target_compile_definitions(
  ParallelCxxTests PRIVATE PARALLEL_HAS_GTEST=1 PARALLEL_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

# Workaround for Clang 20/21.x compiler crash in register allocator
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestAsyncParallel.cpp" PROPERTIES COMPILE_FLAGS "-O1"
  )
endif()

target_link_libraries(ParallelCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Parallel::Parallel)
if(EXISTS "${CMAKE_SOURCE_DIR}/ThirdParty/loguru")
  target_include_directories(ParallelCxxTests PRIVATE "${CMAKE_SOURCE_DIR}/ThirdParty/loguru")
endif()
if(TARGET Loguru::loguru)
  target_link_libraries(ParallelCxxTests PRIVATE Loguru::loguru)
endif()
if(TARGET Fmt::fmt)
  target_link_libraries(ParallelCxxTests PRIVATE Fmt::fmt)
endif()

set_target_properties(
  ParallelCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                             "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME ParallelCxxTests COMMAND ParallelCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(ParallelCxxTests PROPERTIES TIMEOUT 120 LABELS "Parallel;Unit")

if(PARALLEL_ENABLE_COVERAGE AND DEFINED PARALLEL_LLVM_PROFILE_FILE)
  set_tests_properties(
    ParallelCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${PARALLEL_LLVM_PROFILE_FILE}"
  )
endif()

if(COMMAND parallel_set_target_output_directories)
  parallel_set_target_output_directories(ParallelCxxTests)
endif()

message(STATUS "✓ ParallelCxxTests registered with CTest")
message(STATUS "  - Working directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# ============================================================================= Individual Benchmark
# Executables
# =============================================================================
if(PARALLEL_ENABLE_BENCHMARK)
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
        "${QUARISMA_THIRDPARTY_DIR}/benchmark"
        "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build"
        EXCLUDE_FROM_ALL
      )
      _set_third_party_folder_properties("benchmark"
                                         "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build")
    endif()
  endif()
  _create_third_party_interface_targets(
    "Benchmark::benchmark=benchmark::benchmark|benchmark"
    "Benchmark::benchmark_main=benchmark::benchmark_main|benchmark_main"
  )
  if(TARGET Benchmark::benchmark AND NOT TARGET Parallel::benchmark)
    add_library(Parallel::benchmark ALIAS benchmark)
    add_library(Parallel::benchmark_main ALIAS benchmark_main)
  endif()

  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp"
  )

  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    add_executable(${target_name} "${_bench_source}")

    target_link_libraries(
      ${target_name} PRIVATE Parallel::benchmark Parallel::benchmark_main Parallel::Parallel
    )

    set_target_properties(
      ${target_name} PROPERTIES FOLDER "Benchmarks" OUTPUT_NAME "${target_name}"
    )

    if(COMMAND parallel_set_target_output_directories)
      parallel_set_target_output_directories(${target_name})
    else()
      set_target_properties(
        ${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      )
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name} --benchmark_min_time=0.01s
             WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    set_tests_properties(
      ${target_name} PROPERTIES TIMEOUT 600 LABELS "Benchmark;Performance;Parallel"
    )

    message(STATUS "✓ Benchmark target configured: ${target_name} (from ${bench_name}.cpp)")
  endforeach()

  list(LENGTH bench_sources bench_count)
  message(STATUS "✓ Individual benchmark executables configured (${bench_count} total)")
endif()
