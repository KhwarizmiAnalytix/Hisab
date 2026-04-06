# ============================================================================= Core Library C++
# Tests
# =============================================================================
# Following cpuinfo's simple pattern for test integration

# ============================================================================= Test Source Files
# Discovery
# =============================================================================
file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp"
                                                      "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cu"
)

# Exclude special test files that need separate handling
set(TestFiles)
foreach(_test_source IN ITEMS ${test_sources})
  get_filename_component(file_name "${_test_source}" NAME)

  # Always exclude TestLoggerDisableSignalHandler
  if(file_name MATCHES "TestLoggerDisableSignalHandler")
    continue()
  endif()

  # Exclude GPU-related test files based on compile flags When both CUDA and HIP are disabled,
  # exclude all GPU tests
  if(NOT QUARISMA_ENABLE_CUDA AND NOT QUARISMA_ENABLE_HIP)
    if(file_name MATCHES "TestGpu" OR file_name MATCHES "TestGPU")
      message(STATUS "Excluding GPU test (no GPU support): ${file_name}")
      continue()
    endif()
  endif()

  # When CUDA is disabled, exclude CUDA-specific tests
  if(NOT QUARISMA_ENABLE_CUDA)
    if(file_name MATCHES "TestCuda")
      message(STATUS "Excluding CUDA test (CUDA disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When HIP is disabled, exclude HIP-specific tests
  if(NOT QUARISMA_ENABLE_HIP)
    if(file_name MATCHES "TestHip")
      message(STATUS "Excluding HIP test (HIP disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When Kineto is disabled, exclude Kineto-specific tests
  if(NOT PROFILER_ENABLE_KINETO)
    if(file_name MATCHES "TestKineto")
      message(STATUS "Excluding Kineto test (Kineto disabled): ${file_name}")
      continue()
    endif()
  endif()

  # When Enzyme is disabled, exclude Enzyme-specific tests
  if(NOT QUARISMA_ENABLE_ENZYME)
    if(file_name MATCHES "TestEnzyme")
      message(STATUS "Excluding Enzyme test (Enzyme disabled): ${file_name}")
      continue()
    endif()
  endif()

  # TestEnzymeAD.cu is a standalone executable with its own main() — keep it out of CoreCxxTests
  if(file_name STREQUAL "TestEnzymeAD.cu")
    continue()
  endif()

  list(APPEND TestFiles "${_test_source}")
endforeach()

# ============================================================================= CoreCxxTests
# Executable
# =============================================================================
add_executable(CoreCxxTests ${TestFiles})

# Compile definitions
target_compile_definitions(CoreCxxTests PRIVATE QUARISMA_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10)

# Workaround for Clang 20/21.x compiler crash in register allocator The crash occurs in the Greedy
# Register Allocator pass when compiling with -O3 This is a known Clang bug:
# https://github.com/llvm/llvm-project/issues/ Disable optimization for affected files to work
# around the ICE
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/TestPointer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestAsyncParallel.cpp" PROPERTIES COMPILE_FLAGS "-O1"
  )
  message(
    STATUS
      "Applied Clang ICE workaround: TestPointer.cpp and TestAsyncParallel.cpp compiled with -O1"
  )
endif()

# Link libraries Core library is linked which transitively brings in all centralized dependencies
# via QUARISMA_DEPENDENCY_LIBS
target_link_libraries(CoreCxxTests PRIVATE Quarisma::gtest Quarisma::gtest_main Quarisma::Core)

# Set target properties
set_target_properties(
  CoreCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                         "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# ============================================================================= Google Test
# Discovery for CTest and Visual Studio Test Explorer
# =============================================================================

# Register test with CTest
add_test(NAME CoreCxxTests COMMAND CoreCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Set test properties
# Unit tests finish in seconds when healthy; keep timeout modest for faster feedback.
set_tests_properties(CoreCxxTests PROPERTIES TIMEOUT 120 LABELS "Core;Unit")

# Set coverage environment variable if enabled
if(QUARISMA_ENABLE_COVERAGE AND DEFINED QUARISMA_LLVM_PROFILE_FILE)
  set_tests_properties(
    CoreCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${QUARISMA_LLVM_PROFILE_FILE}"
  )
endif()

if(QUARISMA_ENABLE_ENZYME)
  target_link_libraries(CoreCxxTests PRIVATE Quarisma::enzyme)
endif()

# TestEnzymeAD.cu: standalone CUDA executable (has its own main(), not GTest-based)
if(QUARISMA_ENABLE_CUDA AND QUARISMA_ENABLE_ENZYME)
  add_executable(TestEnzymeAD_CUDA "${CMAKE_CURRENT_SOURCE_DIR}/TestEnzymeAD.cu")
  target_link_libraries(TestEnzymeAD_CUDA PRIVATE Quarisma::Core Quarisma::enzyme CUDA::cudart)
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(TestEnzymeAD_CUDA)
  endif()
  add_test(NAME EnzymeAD_CUDA COMMAND TestEnzymeAD_CUDA)
  message(STATUS "✓ TestEnzymeAD_CUDA registered with CTest")
endif()

message(STATUS "✓ CoreCxxTests registered with CTest")
message(STATUS "  - Working directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# ============================================================================= Output Directory
# Configuration
# =============================================================================
if(COMMAND quarisma_set_target_output_directories)
  quarisma_set_target_output_directories(CoreCxxTests)
endif()

# ============================================================================= Individual Benchmark
# Executables
# =============================================================================
# Each benchmark is built as a separate executable for: - Parallel compilation (faster builds) -
# Selective rebuilding (only rebuild changed benchmarks) - Independent execution (run specific
# benchmarks in isolation) - Better CTest integration (individual test reporting)
# =============================================================================
if(QUARISMA_ENABLE_BENCHMARK)

  # Discover all benchmark source files
  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cu"
  )

  # Exclude CUDA-related benchmarks when GPU support is disabled
  set(filtered_bench_sources)
  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(file_name "${_bench_source}" NAME)

    # Exclude CUDA-related benchmarks when GPU support is disabled
    if(NOT QUARISMA_ENABLE_CUDA AND NOT QUARISMA_ENABLE_HIP)
      if(file_name MATCHES ".*[Cc]uda.*")
        message(STATUS "Excluding CUDA benchmark (no GPU support): ${file_name}")
        continue()
      endif()
    endif()

    list(APPEND filtered_bench_sources "${_bench_source}")
  endforeach()
  set(bench_sources ${filtered_bench_sources})

  # Create a separate executable target for each benchmark file
  foreach(_bench_source IN ITEMS ${bench_sources})
    # Extract benchmark name from filename (remove .cpp/.cu extension)
    get_filename_component(bench_name "${_bench_source}" NAME_WE)

    # Create executable target with consistent naming: benchmark_<name> Example: BenchmarkSMP.cpp ->
    # benchmark_parallel
    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_${bench_suffix_lower}")

    # Create the executable
    add_executable(${target_name} "${_bench_source}")

    # Link libraries - Quarisma::benchmark: Google Benchmark library - Quarisma::benchmark_main:
    # Provides main() function - Quarisma::Core: Core library with all dependencies
    target_link_libraries(
      ${target_name} PRIVATE Quarisma::benchmark Quarisma::benchmark_main Quarisma::Core
    )

    # Set target properties
    set_target_properties(
      ${target_name} PROPERTIES FOLDER "Benchmarks" OUTPUT_NAME "${target_name}"
    )

    # Apply Quarisma output directory configuration This ensures benchmarks are placed in
    # ${QUARISMA_BINARY_DIR}/bin
    if(COMMAND quarisma_set_target_output_directories)
      quarisma_set_target_output_directories(${target_name})
    else()
      # Fallback: use standard output directories
      set_target_properties(
        ${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
      )
    endif()

    # Register with CTest for integrated test execution This allows running benchmarks via: ctest -R
    # benchmark_ Or specific benchmark: ctest -R benchmark_parallel
    add_test(NAME ${target_name} COMMAND ${target_name} --benchmark_min_time=0.01s
             WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    # Set test properties
    set_tests_properties(
      ${target_name} PROPERTIES TIMEOUT 600 # 10 minutes timeout for benchmarks
                                LABELS "Benchmark;Performance;Core"
    )

    message(STATUS "✓ Benchmark target configured: ${target_name} (from ${bench_name}.cpp)")
  endforeach()

  # Report total number of benchmarks configured
  list(LENGTH bench_sources bench_count)
  message(STATUS "✓ Individual benchmark executables configured (${bench_count} total)")
endif()
