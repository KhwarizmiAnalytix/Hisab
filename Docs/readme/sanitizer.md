# XSigma Sanitizer System

## Overview

The XSigma project includes a simplified sanitizer system for memory debugging and analysis. This system provides runtime instrumentation to detect various types of bugs and memory issues during development and testing.

## Supported Sanitizers

The following sanitizers are available:

- **AddressSanitizer (ASan)** - Detects buffer overflows, use-after-free, and other memory errors
- **UndefinedBehaviorSanitizer (UBSan)** - Detects undefined behavior in C++ code
- **ThreadSanitizer (TSan)** - Detects data races and thread synchronization issues
- **MemorySanitizer (MSan)** - Detects reads of uninitialized memory
- **LeakSanitizer (LSan)** - Detects memory leaks

## Requirements

### Compiler Support
- **Clang only** - Sanitizers are only supported with the Clang compiler
- GCC and MSVC are not supported for sanitizer builds
- The build system will fail with a clear error message if a non-Clang compiler is used

### Build Configuration
- **Debug mode required** - Sanitizers automatically force `CMAKE_BUILD_TYPE=Debug`
- **No optimizations** - All optimizations are disabled (`-O0`) for accurate instrumentation
- **Debug symbols** - Debug information is enabled (`-g`) for better error reporting

## Platform Support

| Sanitizer | Linux | macOS | Windows |
|-----------|-------|-------|---------|
| AddressSanitizer | ✅ | ✅ | ✅ |
| UndefinedBehaviorSanitizer | ✅ | ✅ | ❌ |
| ThreadSanitizer | ✅ | ✅ | ❌ |
| MemorySanitizer | ✅ | ❌ | ❌ |
| LeakSanitizer | ✅ | ❌** | ❌ |

**LeakSanitizer is not supported on Apple Silicon (ARM64) macOS systems.

## Usage

### CMake Configuration

Enable sanitizers using CMake options:

```bash
# Enable AddressSanitizer
cmake -B build -S . -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=address

# Enable UndefinedBehaviorSanitizer
cmake -B build -S . -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=undefined

# Enable ThreadSanitizer
cmake -B build -S . -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=thread

# Enable MemorySanitizer (Linux only)
cmake -B build -S . -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=memory

# Enable LeakSanitizer
cmake -B build -S . -DPROJECT_ENABLE_SANITIZER=ON -DPROJECT_SANITIZER_TYPE=leak
```

### Setup Script Usage

The `setup.py` script provides convenient sanitizer configuration:

```bash
# Using dot notation (recommended)
python setup.py config.build.test.ninja.clang --sanitizer.address
python setup.py config.build.test.ninja.clang --sanitizer.undefined
python setup.py config.build.test.ninja.clang --sanitizer.thread
python setup.py config.build.test.ninja.clang --sanitizer.memory
python setup.py config.build.test.ninja.clang --sanitizer.leak

# Using explicit flags
python setup.py config.build.test.ninja.clang --enable-sanitizer --sanitizer-type=address
python setup.py config.build.test.ninja.clang --sanitizer-type=undefined
```

## Architecture

### Target Scope
- **Main Library Only** - Sanitizers instrument only the main XSigma library code
- **Third-party Exclusion** - Third-party dependencies are excluded to prevent Windows linker mismatches
- **Suppression Files** - Runtime suppressions handle known false positives in third-party code

### Build System Integration
- **CMake Module** - `Cmake/tools/sanitize.cmake` handles all sanitizer configuration
- **Automatic Validation** - Compiler and platform compatibility is checked automatically
- **Error Handling** - Clear error messages for unsupported configurations
- **Suppression Files** - Automatic configuration of suppression files for each sanitizer type
- **Windows Compatibility** - Special handling for Windows linker compatibility issues

### Compiler Flags
The sanitizer system applies minimal, essential flags:

```cmake
# Base flags for all sanitizers
-fsanitize=${SANITIZER_TYPE}
-O0                          # Disable all optimizations
-g                           # Enable debug symbols
-fno-omit-frame-pointer      # Preserve frame pointers

# Sanitizer-specific flags
# AddressSanitizer
-fno-optimize-sibling-calls

# MemorySanitizer
-fsanitize-memory-track-origins=2
```

## CI/CD Integration

The sanitizer system is integrated into GitHub Actions CI/CD:

- **Cross-platform Testing** - All sanitizers are tested on Linux, macOS, and Windows
- **Clang Only** - CI uses Clang compiler exclusively for sanitizer builds
- **Matrix Strategy** - Each sanitizer type is tested separately
- **Platform Exclusions** - Unsupported sanitizer/platform combinations are excluded

## Windows Compatibility

The sanitizer system includes special handling for Windows to resolve linker compatibility issues:

### Windows-Specific Fixes
- **Runtime Library Consistency** - Forces `MultiThreadedDLL` runtime library to match sanitizer runtime
- **Iterator Debug Level** - Sets `_ITERATOR_DEBUG_LEVEL=0` to prevent CRT mismatches
- **Build Type Override** - Uses `RelWithDebInfo` instead of `Debug` for better compatibility
- **Global Sanitizer Flags** - Applies minimal sanitizer flags globally to prevent linker mismatches

### Known Limitations
- **UndefinedBehaviorSanitizer** - May fail with undefined symbol errors (`__coe_win::ContinueOnError`)
- **Recommended Alternative** - Use AddressSanitizer for Windows development, which is fully supported

### Implementation Details
The Windows compatibility fixes are automatically applied when building on Windows:
```cmake
if(WIN32)
    # Force consistent _ITERATOR_DEBUG_LEVEL across all targets
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)

    # Force consistent runtime library settings
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    # Apply minimal sanitizer flags globally
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${PROJECT_SANITIZER_TYPE}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${PROJECT_SANITIZER_TYPE}")
endif()
```

## Suppression Files

The sanitizer system includes suppression files to handle known false positives and third-party library issues:

- **`Scripts/suppressions/asan_suppressions.txt`** - AddressSanitizer suppressions
- **`Scripts/suppressions/ubsan_suppressions.txt`** - UndefinedBehaviorSanitizer suppressions
- **`Scripts/suppressions/tsan_suppressions.txt`** - ThreadSanitizer suppressions
- **`Scripts/suppressions/msan_suppressions.txt`** - MemorySanitizer suppressions
- **`Scripts/suppressions/lsan_suppressions.txt`** - LeakSanitizer suppressions

These files are automatically configured via environment variables (`ASAN_OPTIONS`, `UBSAN_OPTIONS`, etc.) when sanitizers are enabled.

### Customizing Suppressions

To add custom suppressions:

1. Edit the appropriate suppression file in the `Scripts/suppressions/` directory
2. Add patterns to match problematic functions, files, or libraries
3. Rebuild and test to verify the suppression works

Example suppression formats:
```
# AddressSanitizer
leak:MyClass::problematic_function
heap-buffer-overflow:ThirdPartyLib::*

# UndefinedBehaviorSanitizer
alignment:*problematic_template*
signed-integer-overflow:*/path/to/file.cpp

# ThreadSanitizer
race:*logging_function*
mutex:*thread_pool*
```

## Best Practices

### Development Workflow
1. **Regular Testing** - Run sanitizer builds regularly during development
2. **Sanitizer Selection** - Choose appropriate sanitizers for the type of issues you're investigating
3. **Performance Impact** - Expect significant performance overhead with sanitizers enabled
4. **Memory Usage** - Sanitizers increase memory usage substantially
5. **Suppression Management** - Review and update suppression files as needed

### Debugging Tips
1. **Start with AddressSanitizer** - Most common and effective for memory errors
2. **Use UndefinedBehaviorSanitizer** - Catches subtle C++ undefined behavior issues
3. **ThreadSanitizer for Concurrency** - Essential for multi-threaded code
4. **MemorySanitizer for Initialization** - Detects use of uninitialized memory

### Limitations
- **Single Sanitizer** - Only one sanitizer can be enabled at a time
- **Performance Overhead** - 2-10x slowdown depending on the sanitizer
- **Memory Overhead** - Significant memory usage increase
- **Clang Dependency** - Requires Clang compiler installation

## Troubleshooting

### Common Issues

**Compiler Not Found**
```
Error: Sanitizers are only supported with Clang compiler.
```
Solution: Install Clang and ensure it's in your PATH.

**Platform Not Supported**
```
Error: MemorySanitizer is not supported on Windows
Error: ThreadSanitizer is not supported on Windows with Clang
```
Solution: Use a different sanitizer or switch to a supported platform.

**Windows UBSan Linker Issues**
```
Warning: UndefinedBehaviorSanitizer may have linker issues on Windows with Clang
lld-link: error: undefined symbol: bool __cdecl __coe_win::ContinueOnError(void)
```
Solution: Use AddressSanitizer instead, which is fully supported on Windows.

**Windows TSan Compiler Error**
```
clang: error: unsupported option '-fsanitize=thread' for target 'x86_64-pc-windows-msvc'
```
Solution: ThreadSanitizer is not supported on Windows. Use AddressSanitizer instead.

**Build Failures**
- Ensure you're using a Debug build configuration
- Check that third-party dependencies are properly excluded
- Verify Clang installation and version compatibility

### Performance Considerations
- **AddressSanitizer**: ~2x slowdown, ~3x memory usage
- **ThreadSanitizer**: ~5-15x slowdown, ~5-10x memory usage
- **MemorySanitizer**: ~3x slowdown, significant memory usage
- **UndefinedBehaviorSanitizer**: ~20% slowdown, minimal memory overhead
- **LeakSanitizer**: Minimal runtime overhead, memory usage at exit

## Migration from Previous System

The new sanitizer system is significantly simplified compared to the previous implementation:

### Removed Features
- Complex suppression files
- GCC and MSVC support
- Multiple sanitizer combinations
- Extensive configuration options
- Third-party sanitizer application

### Simplified Approach
- Clang-only support
- Single sanitizer per build
- Minimal configuration
- Clear error messages
- Automatic platform validation

This simplification improves reliability, reduces maintenance overhead, and provides a more consistent development experience across all supported platforms.
