# =============================================================================
# Models Library C++ Tests
# =============================================================================
xsigma_add_googletest()

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

add_executable(ModelsCxxTests ${test_sources})

target_compile_definitions(
  ModelsCxxTests PRIVATE MODELS_HAS_GTEST=1 MODELS_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

target_link_libraries(ModelsCxxTests PRIVATE Gtest::gtest Gtest::gtest_main Models::Models)

set_target_properties(
  ModelsCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                           "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME ModelsCxxTests COMMAND ModelsCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(ModelsCxxTests PROPERTIES TIMEOUT 120 LABELS "Models;Unit")

if(COMMAND models_set_target_output_directories)
  models_set_target_output_directories(ModelsCxxTests)
endif()
