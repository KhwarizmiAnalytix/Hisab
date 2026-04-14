# Build Configuration

This guide covers Quarisma's build configuration options, including build types, C++ standard selection, and optimization settings.

## Table of Contents

- [Build Types](#build-types)
- [C++ Standard Selection](#c-standard-selection)
- [Optimization Options](#optimization-options)
- [Link-Time Optimization (LTO)](#link-time-optimization-lto)
- [Advanced Configuration](#advanced-configuration)
- [CMake Optimization Modules](#cmake-optimization-modules)

## Build Types

Quarisma supports four standard CMake build types, each optimized for different use cases:

| Build Type | Description | Optimization | Use Case |
|------------|-------------|--------------|----------|
| `Debug` | Development build | No optimization, debug info | Active development, debugging |
| `Release` | Production build | Full optimization | Production deployments |
| `RelWithDebInfo` | Release with debug info | Optimized + debug symbols | Performance profiling, production debugging |
| `MinSizeRel` | Size-optimized build | Optimize for size | Embedded systems, size-constrained environments |

### Usage

```bash
# Debug build (default)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Release build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Release with debug info
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Minimal size build
cmake -B build -S . -DCMAKE_BUILD_TYPE=MinSizeRel
```

## C++ Standard Selection

Quarisma supports C++17, C++20, and C++23 standards. The default is C++17 for maximum compatibility.

```bash
# C++17 (default)
cmake -B build -S . -DQUARISMA_CXX_STANDARD=17

# C++20
cmake -B build -S . -DQUARISMA_CXX_STANDARD=20

# C++23
cmake -B build -S . -DQUARISMA_CXX_STANDARD=23
```

### Recommendations

- **C++17**: Maximum compatibility, stable features
- **C++20**: Modern features like concepts, ranges, coroutines
- **C++23**: Latest features, may require recent compiler versions

## Optimization Options

### Link-Time Optimization (LTO)

Enable Link-Time Optimization for maximum performance in release builds:

```bash
# Enable LTO (default is ON)
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON

# Disable LTO
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=OFF
```

**Benefits:**
- Improved runtime performance (5-15% typical)
- Better inlining across translation units
- Dead code elimination
- Reduced binary size in some cases

**Considerations:**
- Increased build time (10-30% longer)
- Higher memory usage during linking
- Requires recent compiler versions

**Compiler-Specific Implementation:**

| Compiler | Flags | Tools |
|----------|-------|-------|
| GCC | `-flto` | `gcc-ar`, `gcc-ranlib` |
| Clang | `-flto` | `llvm-ar`, `llvm-ranlib` (optional) |
| Apple Clang | `-flto` | System tools |
| MSVC | `/GL` (compile), `/LTCG` (link) | Built-in |

### Troubleshooting LTO

If LTO fails:

1. **Disable LTO**:
   ```bash
   cmake -B build -S . -DQUARISMA_ENABLE_LTO=OFF
   ```

2. **Check compiler version** - ensure recent GCC/Clang/MSVC:
   - GCC 7.0+
   - Clang 5.0+
   - Apple Clang 9.0+
   - MSVC 19.14+ (Visual Studio 2017 15.7+)

3. **Increase available memory** - LTO can be memory-intensive:
   ```bash
   # On Linux/macOS, increase available memory or use swap
   # On Windows, ensure sufficient RAM is available
   ```

4. **Check for LTO support**:
   ```bash
   # The CMake configuration will verify LTO support
   # Check the CMake output for "LTO support verified"
   ```

5. **Use with specific build types**:
   ```bash
   # LTO works best with Release builds
   cmake -B build -S . \
       -DCMAKE_BUILD_TYPE=Release \
       -DQUARISMA_ENABLE_LTO=ON
   ```

## Advanced Configuration

### Custom Compiler Flags

Override optimization flags for specific requirements:

```bash
# Custom optimization flags
cmake -B build -S . -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=native -DNDEBUG"

# Add additional flags
cmake -B build -S . -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
```

### Testing Configuration

Enable testing with Google Test:

```bash
cmake -B build -S . \
    -DBUILD_TESTING=ON \
    -DQUARISMA_ENABLE_GTEST=ON

# Build and run tests
cmake --build build
ctest --test-dir build
```

## CMake Optimization Modules

Quarisma includes optimized CMake modules that improve configuration speed and runtime performance. These are included automatically by the top-level CMakeLists.txt and require no manual setup.

### Available Modules

| Module | Purpose | Location |
|--------|---------|----------|
| `build_type.cmake` | Optimized build-type flags, LTO handling, MSVC runtime selection | `Cmake/flags/build_type.cmake` |
| `checks.cmake` | Fast, cached platform/compiler validation and C++17 feature checks | `Cmake/flags/checks.cmake` |
| `platform.cmake` | Platform-specific optimizations, vectorization flags, parallel builds | `Cmake/flags/platform.cmake` |

### Features

- **Automatic optimization**: Compiler-specific flags applied automatically
- **Cached checks**: Platform and compiler checks cached for faster reconfiguration
- **Safe flag application**: Flags tested before application to avoid build failures
- **Parallel build support**: Automatic detection and configuration of parallel compilation

## Best Practices

### Development Workflow

```bash
# Development build with testing
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON \
    -DQUARISMA_ENABLE_GTEST=ON
```

### Production Builds

```bash
# Optimized production build
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON
```

### CI/CD Builds

```bash
# Fast CI build with external libraries
cmake -B build -S . \
    -DQUARISMA_ENABLE_EXTERNAL=ON \
    -DBUILD_TESTING=ON
```

## Related Documentation

- [Vectorization Support](../vectorization.md) - CPU SIMD optimization
- [Sanitizers](../sanitizer.md) - Memory debugging and analysis
- [Code Coverage](../code-coverage.md) - Test coverage analysis
- [Cross-Platform Building](../cross-platform-building.md) - Platform-specific instructions
