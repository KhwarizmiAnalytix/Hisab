include_guard(GLOBAL)

# if(NOT PROJECT_ENABLE_COVERAGE AND NOT LOGGING_ENABLE_SANITIZER AND NOT MEMORY_ENABLE_SANITIZER
# AND NOT CORE_ENABLE_SANITIZER AND NOT PARALLEL_ENABLE_SANITIZER AND NOT PROFILER_ENABLE_SANITIZER)
# message("--avx compiler flags: ${VECTORIZATION_COMPILER_FLAGS}") set(CMAKE_C_FLAGS
# "${CMAKE_CXX_FLAGS} ${VECTORIZATION_COMPILER_FLAGS}") set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}
# ${VECTORIZATION_COMPILER_FLAGS}") endif() make sure Crun is linked in with the native compiler, it
# is not used by default for shared libraries and is required for things like java to work.
if(CMAKE_SYSTEM MATCHES "SunOS.*")
  if(NOT CMAKE_COMPILER_IS_GNUCXX)
    find_library(PROJECT_SUNCC_CRUN_LIBRARY Crun /opt/SUNWspro/lib)
    if(PROJECT_SUNCC_CRUN_LIBRARY)
      link_libraries(${PROJECT_SUNCC_CRUN_LIBRARY})
    endif()
    find_library(PROJECT_SUNCC_CSTD_LIBRARY Cstd /opt/SUNWspro/lib)
    if(PROJECT_SUNCC_CSTD_LIBRARY)
      link_libraries(${PROJECT_SUNCC_CSTD_LIBRARY})
    endif()
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Enable exceptions because QUARISMA and third party code rely on C++ exceptions. Allow C++ to
  # catch exceptions. Emscripten disables it by default due to high overhead. Generate helper
  # functions to get stack traces for uncaught exceptions
  string(APPEND CMAKE_CXX_FLAGS " -fwasm-exceptions")
  string(APPEND CMAKE_C_FLAGS " -fwasm-exceptions")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fwasm-exceptions -sEXCEPTION_STACK_TRACES=1")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fwasm-exceptions -sEXCEPTION_STACK_TRACES=1")
  string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fwasm-exceptions -sEXCEPTION_STACK_TRACES=1")
  # Consumers linking to QUARISMA also need to add the exception flag.
  if(TARGET QUARISMAplatform)
    target_link_options(QUARISMAplatform INTERFACE "-fwasm-exceptions" "-sEXCEPTION_STACK_TRACES=1")
  endif()
  if(PROJECT_WEBASSEMBLY_THREADS)
    # Remove after https://github.com/WebAssembly/design/issues/1271 is closed Set Wno flag globally
    # because even though the flag is added in QUARISMACompilerWarningFlags.cmake, wrapping tools do
    # not link with `QUARISMAplatform`
    string(APPEND CMAKE_CXX_FLAGS " -pthread -Wno-pthreads-mem-growth")
    string(APPEND CMAKE_C_FLAGS " -pthread -Wno-pthreads-mem-growth")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -pthread")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -pthread")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -pthread")
    # Consumers linking to QUARISMA also need to add the pthread flag.
    if(TARGET QUARISMAplatform)
      target_compile_options(QUARISMAplatform INTERFACE "-pthread" "-Wno-pthreads-mem-growth")
      target_link_options(QUARISMAplatform INTERFACE "-pthread")
    endif()
  endif()
  if(PROJECT_WEBASSEMBLY_64_BIT)
    string(APPEND CMAKE_CXX_FLAGS " -sMEMORY64=1")
    string(APPEND CMAKE_C_FLAGS " -sMEMORY64=1")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -sMEMORY64=1")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -sMEMORY64=1")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -sMEMORY64=1")
    # Consumers linking to QUARISMA also need to add the memory64 flag.
    if(TARGET QUARISMAplatform)
      target_compile_options(QUARISMAplatform INTERFACE "-sMEMORY64=1")
      target_link_options(QUARISMAplatform INTERFACE "-sMEMORY64=1")
    endif()
  endif()
endif()

# A GCC compiler.
if(CMAKE_COMPILER_IS_GNUCXX)
  if(PROJECT_HAS_X)
    unset(WIN32)
  endif()
  if(WIN32)
    # The platform is gcc on cygwin.
    string(APPEND CMAKE_CXX_FLAGS " -mwin32")
    string(APPEND CMAKE_C_FLAGS " -mwin32")
    link_libraries(-lgdi32)
  endif()
  if(MINGW)
    string(APPEND CMAKE_CXX_FLAGS " -mthreads")
    string(APPEND CMAKE_C_FLAGS " -mthreads")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -mthreads")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -mthreads")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -mthreads")
  endif()
  if(CMAKE_SYSTEM MATCHES "SunOS.*")
    # Disable warnings that occur in X11 headers.
    if(DART_ROOT AND BUILD_TESTING)
      string(APPEND CMAKE_CXX_FLAGS " -Wno-unknown-pragmas")
      string(APPEND CMAKE_C_FLAGS " -Wno-unknown-pragmas")
    endif()
  endif()
else()
  if(CMAKE_ANSI_CFLAGS)
    string(APPEND CMAKE_C_FLAGS " ${CMAKE_ANSI_CFLAGS}")
  endif()
  if(CMAKE_SYSTEM MATCHES "OSF1-V.*")
    string(APPEND CMAKE_CXX_FLAGS " -timplicit_local -no_implicit_include")
  endif()
  if(CMAKE_SYSTEM MATCHES "AIX.*")
    # allow t-ypeid and d-ynamic_cast usage (normally off by default on xlC)
    string(APPEND CMAKE_CXX_FLAGS " -qrtti=all")
    # silence duplicate symbol warnings on AIX
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -bhalt:5")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -bhalt:5")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " -bhalt:5")
  endif()
  if(CMAKE_SYSTEM MATCHES "HP-UX.*")
    string(APPEND CMAKE_C_FLAGS " +W2111 +W2236 +W4276")
    string(APPEND CMAKE_CXX_FLAGS " +W2111 +W2236 +W4276")
  endif()
endif()

# figure out whether the compiler might be the Intel compiler
set(_MAY_BE_INTEL_COMPILER FALSE)
if(UNIX)
  if(CMAKE_CXX_COMPILER_ID)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  else()
    if(NOT CMAKE_COMPILER_IS_GNUCXX)
      set(_MAY_BE_INTEL_COMPILER TRUE)
    endif()
  endif()
endif()

# if so, test whether -i_dynamic is needed
if(_MAY_BE_INTEL_COMPILER)
  include(${CMAKE_CURRENT_LIST_DIR}/TestNO_ICC_IDYNAMIC_NEEDED.cmake)
  testno_icc_idynamic_needed(NO_ICC_IDYNAMIC_NEEDED ${CMAKE_CURRENT_LIST_DIR})
  if(NO_ICC_IDYNAMIC_NEEDED)
    # no flag needed
  else()
    string(APPEND CMAKE_CXX_FLAGS " -i_dynamic")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  # --diag_suppress=236 is for constant value asserts used for error handling This can be restricted
  # to the implementation and doesn't need to propagate
  string(APPEND CMAKE_CXX_FLAGS " --diag_suppress=236")

  # --diag_suppress=381 is for redundant semi-colons used in macros This needs to propagate to
  # anything that includes QUARISMA headers
  string(APPEND CMAKE_CXX_FLAGS " --diag_suppress=381")
endif()

if(MSVC)
  # Use the highest warning level for visual c++ compiler. set(CMAKE_CXX_WARNING_LEVEL 4)
  # if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]") string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS
  # "${CMAKE_CXX_FLAGS}") else() set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4") endif() Enable C++20
  # support: /Zc:__cplusplus
  string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
  # Treat warnings as errors set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX") Disable C4244: conversion
  # warnings
  string(APPEND CMAKE_CXX_FLAGS " /wd4244 /wd4267 /wd4715 /wd4018")
  string(APPEND CMAKE_C_FLAGS " /wd4244 /wd4267 /wd4715 /wd4018")

  # Disable deprecation warnings for standard C and STL functions in VS2015+ and later
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_SCL_SECURE_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS)

  # Enable /MP flag for Visual Studio
  set(CMAKE_CXX_MP_FLAG ON CACHE BOOL "Build with /MP flag enabled")
  set(PROCESSOR_COUNT "$ENV{NUMBER_OF_PROCESSORS}")
  set(CMAKE_CXX_MP_NUM_PROCESSORS ${PROCESSOR_COUNT}
      CACHE STRING "The maximum number of processes for the /MP flag"
  )
  if(CMAKE_CXX_MP_FLAG)
    string(APPEND CMAKE_CXX_FLAGS " /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
    string(APPEND CMAKE_C_FLAGS " /MP${CMAKE_CXX_MP_NUM_PROCESSORS}")
  endif()

  # Enable /bigobj for MSVC to allow larger symbol tables
  string(APPEND CMAKE_CXX_FLAGS " /bigobj")
  string(APPEND CMAKE_C_FLAGS " /bigobj")

  # Enable faster PDB generation
  string(APPEND CMAKE_CXX_FLAGS " /Zi")
  string(APPEND CMAKE_C_FLAGS " /Zi")

  # Use /utf-8 so that MSVC uses utf-8 in source files and object files
  string(APPEND CMAKE_CXX_FLAGS " /utf-8")
  string(APPEND CMAKE_C_FLAGS " /utf-8")

  # use /EHsc for exception handling
  string(APPEND CMAKE_CXX_FLAGS " /EHsc")
  string(APPEND CMAKE_C_FLAGS " /EHsc")
endif()

if(APPLE)
  message(STATUS "Applying macOS LLVM linker options")

  if(DEFINED PROJECT_LLVM_INSTALL_PREFIX AND PROJECT_LLVM_INSTALL_PREFIX)
    set(_quarisma_llvm_prefix "${PROJECT_LLVM_INSTALL_PREFIX}")
  elseif(EXISTS "/opt/homebrew/opt/llvm/lib")
    set(_quarisma_llvm_prefix "/opt/homebrew/opt/llvm")
  elseif(EXISTS "/usr/local/opt/llvm/lib")
    set(_quarisma_llvm_prefix "/usr/local/opt/llvm")
  else()
    set(_quarisma_llvm_prefix "/opt/homebrew/opt/llvm")
  endif()

  set(LLVM_LINK_FLAGS
      -L${_quarisma_llvm_prefix}/lib/c++ -L${_quarisma_llvm_prefix}/lib
      -Wl,-rpath,${_quarisma_llvm_prefix}/lib/c++ -Wl,-rpath,${_quarisma_llvm_prefix}/lib
  )

  # Add each flag only if not already included
  foreach(flag IN LISTS LLVM_LINK_FLAGS)
    string(FIND "${CMAKE_EXE_LINKER_FLAGS}" "${flag}" flag_found)
    if(flag_found EQUAL -1)
      add_link_options(${flag})
    endif()
  endforeach()
endif()
