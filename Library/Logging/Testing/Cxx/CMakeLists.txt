# =============================================================================
# Logging Library C++
# Tests
# =============================================================================
xsigma_add_googletest()

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

set(TestFiles)
foreach(_test_source IN ITEMS ${test_sources})
  get_filename_component(file_name "${_test_source}" NAME)
  if(file_name MATCHES "TestLoggerDisableSignalHandler")
    continue()
  endif()
  list(APPEND TestFiles "${_test_source}")
endforeach()

add_executable(LoggingCxxTests ${TestFiles})

# Testing/ (sources + generated *Test.h) and third-party includes match the Logging::Logging
# interface.
target_include_directories(
  LoggingCxxTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/.."
)
if(LOGGING_DEPENDENCY_INCLUDE_DIRS)
  target_include_directories(LoggingCxxTests PRIVATE ${LOGGING_DEPENDENCY_INCLUDE_DIRS})
endif()

target_compile_definitions(
  LoggingCxxTests PRIVATE LOGGING_HAS_GTEST=1 LOGGING_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

target_link_libraries(LoggingCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Logging::Logging)
if(LOGGING_DEPENDENCY_LIBS)
  target_link_libraries(LoggingCxxTests PRIVATE ${LOGGING_DEPENDENCY_LIBS})
endif()

set_target_properties(
  LoggingCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME LoggingCxxTests COMMAND LoggingCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(LoggingCxxTests PROPERTIES TIMEOUT 120 LABELS "Logging;Unit")

if(COMMAND logging_set_target_output_directories)
  logging_set_target_output_directories(LoggingCxxTests)
endif()

if(LOGGING_ENABLE_BENCHMARK)
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
  if(TARGET Benchmark::benchmark AND NOT TARGET Logging::benchmark)
    add_library(Logging::benchmark ALIAS benchmark)
    add_library(Logging::benchmark_main ALIAS benchmark_main)
  endif()

  file(GLOB_RECURSE bench_sources LIST_DIRECTORIES false
       "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark*.cpp"
  )

  foreach(_bench_source IN ITEMS ${bench_sources})
    get_filename_component(bench_name "${_bench_source}" NAME_WE)
    string(REPLACE "Benchmark" "" bench_suffix "${bench_name}")
    string(TOLOWER "${bench_suffix}" bench_suffix_lower)
    set(target_name "benchmark_logging_${bench_suffix_lower}")

    add_executable(${target_name} "${_bench_source}")
    target_include_directories(
      ${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/.."
    )
    if(LOGGING_DEPENDENCY_INCLUDE_DIRS)
      target_include_directories(${target_name} PRIVATE ${LOGGING_DEPENDENCY_INCLUDE_DIRS})
    endif()
    target_link_libraries(
      ${target_name} PRIVATE Logging::benchmark Logging::benchmark_main Logging::Logging
    )
    if(LOGGING_DEPENDENCY_LIBS)
      target_link_libraries(${target_name} PRIVATE ${LOGGING_DEPENDENCY_LIBS})
    endif()
    if(COMMAND logging_set_target_output_directories)
      logging_set_target_output_directories(${target_name})
    endif()

    add_test(NAME ${target_name} COMMAND ${target_name} --benchmark_min_time=0.01s
             WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    set_tests_properties(
      ${target_name} PROPERTIES TIMEOUT 600 LABELS "Benchmark;Performance;Logging"
    )
  endforeach()
endif()
