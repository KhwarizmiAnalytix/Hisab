# =============================================================================
# Quarisma
# Profiling Examples CMakeLists.txt
# =============================================================================
# This CMakeLists.txt configures and builds the Quarisma Profiling example programs that
# demonstrate the usage of profiling APIs.
#
# Examples included: - example_profiling_basic: Comprehensive profiling example with Quarisma,
# Kineto, and ITT
#
# Build these examples with: python setup.py config.build.examples.ninja.clang python setup.py
# config.build.examples.vs22
#
# Or manually: cmake -DQUARISMA_ENABLE_EXAMPLES=ON .. cmake --build .
# =============================================================================
cmake_minimum_required(VERSION 3.16)

# =============================================================================
# Example Source
# Files
# =============================================================================
set(example_sources example_profiling_basic.cpp)

# =============================================================================
# Build Examples
# =============================================================================
foreach(example_source ${example_sources})
  # Extract example name from filename (remove .cpp extension)
  string(REPLACE ".cpp" "" example_name ${example_source})

  # Create executable
  add_executable(${example_name} ${example_source})

  # Set C++ standard
  set_target_properties(
    ${example_name} PROPERTIES CXX_STANDARD ${QUARISMA_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
                               CXX_EXTENSIONS OFF
  )

  # Profiling example uses APIs from Profiler; link Core as well for static builds (PRIVATE Core on
  # Profiler).
  target_link_libraries(${example_name} PRIVATE Profiler::Profiler)

  # Set output directory
  if(COMMAND quarisma_set_target_output_directories)
    quarisma_set_target_output_directories(${example_name})
  else()
    set_target_properties(
      ${example_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
  endif()

  # Add to Examples folder in IDE
  set_target_properties(${example_name} PROPERTIES FOLDER "Examples/Profiling")

  # Apply compiler flags if available
  if(MSVC)
    target_compile_options(${example_name} PRIVATE /WX)
  endif()

  message(STATUS "✓ Example configured: ${example_name}")
endforeach()

message(STATUS "✓ Profiling Examples configured successfully")
