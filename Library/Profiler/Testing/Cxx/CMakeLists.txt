# =============================================================================
# Profiler C++ tests
# =============================================================================
quarisma_add_googletest()

# Explicit list (keep in sync with BUILD.bazel). Real-use-case coverage only:
# backend function / memory / output formats, Chrome trace hierarchy,
# threadpool+tracing end-to-end, XPlane pipeline, native hotspot report,
# heavy-function stress, native GpuTracer device plane, and Kineto hotspot
# report / table.
set(TestFiles
    "${CMAKE_CURRENT_SOURCE_DIR}/TestHotspotReport.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendFunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendGpuFallback.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendMemory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendMetadata.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendMetal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerBackendOutput.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerChromeTraceHierarchical.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerHeavyFunction.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerGpuTracer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerNativeHotspot.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerThreadpoolTracing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/TestProfilerXPlanePipeline.cpp"
)

add_executable(ProfilerCxxTests ${TestFiles})

target_compile_definitions(
  ProfilerCxxTests PRIVATE PROFILER_HAS_GTEST=1 PROFILER_GOOGLE_TEST USE_GTEST _VARIADIC_MAX=10
)

# Profiler links Core privately; executables may need Core explicitly when linking static archives.
target_link_libraries(ProfilerCxxTests PRIVATE Gtest::gtest_main Profiler::Profiler)

# Thin LTO with GNU bfd/gold has crashed (SIGSEGV) when linking this executable with Kineto and
# CUDA's libnvperf_host. IPO=OFF alone can still leave -flto on the compile line when IPO is enabled
# project-wide, so force -fno-lto for this executable and bundled gtest. (Using -fuse-ld=lld is not
# reliable when the system ld.lld is a different LLVM major than the compiler driving LTO.)
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set_target_properties(ProfilerCxxTests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
  target_compile_options(ProfilerCxxTests PRIVATE -fno-lto)
  target_link_options(ProfilerCxxTests PRIVATE -fno-lto)
  if(TARGET gtest AND TARGET gtest_main)
    set_target_properties(gtest gtest_main PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
    target_compile_options(gtest PRIVATE -fno-lto)
    target_compile_options(gtest_main PRIVATE -fno-lto)
  endif()
endif()

set_target_properties(
  ProfilerCxxTests PROPERTIES FOLDER "Tests" VS_DEBUGGER_WORKING_DIRECTORY
                                             "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

add_test(NAME ProfilerCxxTests COMMAND ProfilerCxxTests
         WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

set_tests_properties(ProfilerCxxTests PROPERTIES TIMEOUT 120 LABELS "Profiler;Unit")

if(PROFILER_ENABLE_COVERAGE AND DEFINED PROFILER_LLVM_PROFILE_FILE)
  set_tests_properties(
    ProfilerCxxTests PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${PROFILER_LLVM_PROFILE_FILE}"
  )
endif()

if(PROFILER_ENABLE_ENZYME)
  target_link_libraries(ProfilerCxxTests PRIVATE Profiler::enzyme)
endif()

if(COMMAND profiler_set_target_output_directories)
  profiler_set_target_output_directories(ProfilerCxxTests)
endif()

message(STATUS "ProfilerCxxTests registered with CTest")
