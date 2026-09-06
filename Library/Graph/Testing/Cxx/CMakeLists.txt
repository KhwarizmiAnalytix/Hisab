# =============================================================================
# Graph Library C++ Tests
# =============================================================================
xsigma_add_googletest()

file(GLOB_RECURSE test_sources LIST_DIRECTORIES false "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp")

# Custom main (not Gtest::gtest_main) -- see GraphTestMain.cpp for why: on macOS+ASan this
# binary needs to bypass a known third-party (TBB) static-destructor crash at exit that is
# unrelated to anything these tests check.
add_executable(GraphCxxTests ${test_sources} GraphTestMain.cpp)

target_compile_definitions(
  GraphCxxTests PRIVATE GRAPH_HAS_GTEST=1 GRAPH_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

target_link_libraries(GraphCxxTests PRIVATE Gtest::gtest Graph::Graph)

set_target_properties(
  GraphCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME GraphCxxTests COMMAND GraphCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(GraphCxxTests PROPERTIES TIMEOUT 120 LABELS "Graph;Unit")

if(COMMAND graph_set_target_output_directories)
  graph_set_target_output_directories(GraphCxxTests)
endif()
