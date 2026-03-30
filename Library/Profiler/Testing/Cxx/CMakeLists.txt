# ============================================================================= Profiler C++ tests
# =============================================================================
file(GLOB_RECURSE profiler_test_sources LIST_DIRECTORIES false
     "${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp"
)

set(TestFiles)
foreach(_test_source IN ITEMS ${profiler_test_sources})
  get_filename_component(file_name "${_test_source}" NAME)

  if(NOT QUARISMA_ENABLE_KINETO)
    if(file_name MATCHES "TestKineto")
      message(STATUS "Excluding Kineto profiler test (Kineto disabled): ${file_name}")
      continue()
    endif()
  endif()

  if(NOT QUARISMA_ENABLE_ENZYME)
    if(file_name MATCHES "TestEnzyme")
      message(STATUS "Excluding Enzyme test (Enzyme disabled): ${file_name}")
      continue()
    endif()
  endif()

  list(APPEND TestFiles "${_test_source}")
endforeach()

set(QUARISMA_NATIVE_PROFILER_TESTS
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendIntegration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerPlatform.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerXPlane.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerTimespan.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestEnhancedProfiler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnnotationParsing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerExporters.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnnotationStack.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerXPlaneVisitor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerFormatUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerChromeTraceHierarchical.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneSchema.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerAnalysis.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerHeavyFunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestTraceme.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfiler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerSessionEdgeCases.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestScopedMemoryDebugAnnotation.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerStatsCalculator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerUtils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerMemoryAndStats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestXPlaneBuilder.cpp"
)

if(NOT QUARISMA_ENABLE_NATIVE_PROFILER)
  list(REMOVE_ITEM TestFiles ${QUARISMA_NATIVE_PROFILER_TESTS})
  message(STATUS "Native profiler disabled: excluding native-only Profiler tests")
endif()

add_executable(ProfilerCxxTests ${TestFiles})

target_compile_definitions(ProfilerCxxTests PRIVATE QUARISMA_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10)

target_link_libraries(ProfilerCxxTests PRIVATE Quarisma::gtest Quarisma::gtest_main Quarisma::Profiler)

set_target_properties(
  ProfilerCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                             "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME ProfilerCxxTests COMMAND ProfilerCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(ProfilerCxxTests PROPERTIES TIMEOUT 300 LABELS "Profiler;Unit")

if(QUARISMA_ENABLE_COVERAGE AND DEFINED QUARISMA_LLVM_PROFILE_FILE)
  set_tests_properties(
    ProfilerCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${QUARISMA_LLVM_PROFILE_FILE}"
  )
endif()

if(QUARISMA_ENABLE_ENZYME)
  target_link_libraries(ProfilerCxxTests PRIVATE Quarisma::enzyme)
endif()

if(COMMAND quarisma_set_target_output_directories)
  quarisma_set_target_output_directories(ProfilerCxxTests)
endif()

message(STATUS "ProfilerCxxTests registered with CTest")
