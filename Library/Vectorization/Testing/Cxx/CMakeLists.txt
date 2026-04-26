# =============================================================================
# Vectorization Library C++ Tests
# =============================================================================

# --- GoogleTest setup ---
# Core/Testing/Cxx builds gtest first in a full-project build; this block is
# a no-op in that case (TARGET gtest already exists). It activates when
# Vectorization tests are built in isolation.
if(NOT TARGET gtest)
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
endif()
_create_third_party_interface_targets(
  "Gtest::gtest=GTest::gtest|gtest"
  "Gtest::gtest_main=GTest::gtest_main|gtest_main"
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
  VectorizationCxxTests PROPERTIES
  FOLDER                        "Tests"
  CXX_STANDARD                  ${VECTORIZATION_CXX_STANDARD}
  CXX_STANDARD_REQUIRED         ON
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME VectorizationCxxTests COMMAND VectorizationCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
set_tests_properties(VectorizationCxxTests PROPERTIES TIMEOUT 120 LABELS "Vectorization;Unit")

quarisma_module_create_filters("${CMAKE_CURRENT_SOURCE_DIR}")

if(COMMAND quarisma_set_target_output_directories)
  quarisma_set_target_output_directories(VectorizationCxxTests)
endif()

message(STATUS "VectorizationCxxTests registered with CTest")

# =============================================================================
# Benchmark executable (optional)
# =============================================================================

if(VECTORIZATION_ENABLE_BENCHMARK)
  if(NOT TARGET benchmark)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
    set(BENCHMARK_ENABLE_EXCEPTIONS ON CACHE BOOL "Enable benchmark exceptions" FORCE)
    set(BENCHMARK_ENABLE_LTO OFF CACHE BOOL "Disable benchmark LTO" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable benchmark gtest tests" FORCE)
    add_subdirectory(
      "${QUARISMA_THIRDPARTY_DIR}/benchmark"
      "${CMAKE_BINARY_DIR}/ThirdParty/benchmark_build"
      EXCLUDE_FROM_ALL
    )
  endif()
  _create_third_party_interface_targets(
    "Benchmark::benchmark=benchmark::benchmark|benchmark"
    "Benchmark::benchmark_main=benchmark::benchmark_main|benchmark_main"
  )

  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp"
  )
  if(bench_sources)
    add_executable(VectorizationCxxBenchmark ${bench_sources})
    target_link_libraries(
      VectorizationCxxBenchmark PRIVATE Benchmark::benchmark Benchmark::benchmark_main
                                        Vectorization::Vectorization
    )
    set_target_properties(
      VectorizationCxxBenchmark PROPERTIES
      CXX_STANDARD          ${VECTORIZATION_CXX_STANDARD}
      CXX_STANDARD_REQUIRED ON
      FOLDER                "Benchmarks"
    )
    add_test(
      NAME    VectorizationCxxBenchmark
      COMMAND VectorizationCxxBenchmark --benchmark_min_time=0.01s
      WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    set_tests_properties(
      VectorizationCxxBenchmark PROPERTIES TIMEOUT 600 LABELS "Benchmark;Vectorization"
    )
    if(COMMAND quarisma_set_target_output_directories)
      quarisma_set_target_output_directories(VectorizationCxxBenchmark)
    endif()
    message(STATUS "VectorizationCxxBenchmark registered with CTest")
  endif()
endif()
