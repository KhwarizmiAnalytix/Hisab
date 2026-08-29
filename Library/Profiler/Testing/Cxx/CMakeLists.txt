# =============================================================================
# Profiler C++ tests
# =============================================================================
xsigma_add_googletest()

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

# LibTorch (PyTorch C++ profiler) for TestProfilerHeavyFunction. Driven by
# PROFILER_ENABLE_LIBTORCH (setup.py `torch` token).
set(_profiler_torch_found OFF)
if(PROFILER_ENABLE_LIBTORCH)
  set(_saved_module_path "${CMAKE_MODULE_PATH}")
  set(CMAKE_MODULE_PATH "")
  set(MKL_LINK "sdl")
  find_package(Torch QUIET)
  unset(MKL_LINK)
  set(CMAKE_MODULE_PATH "${_saved_module_path}")
  unset(_saved_module_path)
  if(Torch_FOUND)
    set(_profiler_torch_found ON)
    message(STATUS "LibTorch found: ${TORCH_INSTALL_PREFIX} (${Torch_VERSION})")
    if(TARGET caffe2::mkl)
      get_target_property(_mkl_link_dirs caffe2::mkl INTERFACE_LINK_DIRECTORIES)
    else()
      set(_mkl_link_dirs "")
    endif()
    foreach(_mkl_dir IN LISTS _mkl_link_dirs)
      file(GLOB _mkl_ver_libs "${_mkl_dir}/libmkl_*.so.[0-9]*")
      foreach(_vlib IN LISTS _mkl_ver_libs)
        string(REGEX REPLACE "\\.so\\.[0-9].*$" ".so" _unversioned "${_vlib}")
        if(NOT EXISTS "${_unversioned}")
          get_filename_component(_vlib_base "${_vlib}" NAME)
          get_filename_component(_unver_base "${_unversioned}" NAME)
          execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${_vlib_base}" "${_unversioned}"
            RESULT_VARIABLE _sym_rc
          )
          if(_sym_rc EQUAL 0)
            message(STATUS "MKL: created linker symlink ${_unver_base} -> ${_vlib_base}")
          endif()
        endif()
      endforeach()
    endforeach()
    unset(_mkl_link_dirs)
    unset(_mkl_ver_libs)
  else()
    message(STATUS "LibTorch not found — PyTorch heavy-function profiler skipped "
                   "(add install dir to CMAKE_PREFIX_PATH to enable)"
    )
  endif()
endif()

if(_profiler_torch_found)
  target_link_libraries(ProfilerCxxTests PRIVATE torch)
  target_compile_definitions(ProfilerCxxTests PRIVATE PROFILER_HAS_LIBTORCH=1)
  if(WIN32)
    file(GLOB _torch_dlls "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(
      TARGET ProfilerCxxTests
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_torch_dlls} $<TARGET_FILE_DIR:ProfilerCxxTests>
      VERBATIM
    )
  endif()
else()
  target_compile_definitions(ProfilerCxxTests PRIVATE PROFILER_HAS_LIBTORCH=0)
endif()
unset(_profiler_torch_found)

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
