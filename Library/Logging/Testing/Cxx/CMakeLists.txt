# =============================================================================
# Logging Library C++
# Tests
# =============================================================================
quarisma_add_googletest()

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
  LoggingCxxTests PRIVATE LOGGING_HAS_GTEST=1 LOGGING_GOOGLE_TEST LOGGING_HAS_GTEST=1
                          LOGGING_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
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
