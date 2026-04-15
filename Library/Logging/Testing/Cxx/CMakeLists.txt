# =============================================================================
# Logging Library C++ Tests
# =============================================================================

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

target_compile_definitions(
  LoggingCxxTests PRIVATE LOGGING_HAS_GTEST=1 LOGGING_GOOGLE_TEST LOGGING_HAS_GTEST=1
                          LOGGING_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

target_include_directories(
  LoggingCxxTests PRIVATE "${CMAKE_SOURCE_DIR}/Library/Core/Testing" "${CMAKE_SOURCE_DIR}/Library/Core"
)

target_link_libraries(
  LoggingCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Logging::Logging Logging::build
)

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
