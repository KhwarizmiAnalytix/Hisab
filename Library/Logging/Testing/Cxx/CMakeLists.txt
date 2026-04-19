# ============================================================================= Logging Library C++
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
