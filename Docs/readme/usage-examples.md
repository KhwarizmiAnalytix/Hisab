# Usage Examples

This guide provides practical examples of different Quarisma build configurations for various use cases. Each example demonstrates how to configure the build system for specific requirements.

## Table of Contents

- [Minimal Build](#minimal-build)
- [High-Performance Build](#high-performance-build)
- [Development Build](#development-build)
- [External Libraries Build](#external-libraries-build)
- [Testing and Benchmarking Build](#testing-and-benchmarking-build)
- [Production Build](#production-build)
- [Debugging Build](#debugging-build)
- [CI/CD Build](#cicd-build)
- [Enhanced Profiler](#enhanced-profiler)

## Minimal Build

**Goal**: Fastest build time, smallest binary, core functionality only

**Use Case**: Quick testing, embedded systems, resource-constrained environments

```bash
# Disable all optional libraries for fastest build
cmake -B build_minimal -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_MAGICENUM=OFF \
    -DQUARISMA_ENABLE_LOGURU=OFF \
    -DQUARISMA_VECTORIZATION_TYPE=no

cmake --build build_minimal -j
```

**Result**:
- ✅ Fastest build time
- ✅ Smallest binary size
- ✅ Core functionality only
- ✅ No external dependencies (except mandatory: fmt, cpuinfo)

## High-Performance Build

**Goal**: Maximum runtime performance with parallel processing and optimized memory allocation

**Use Case**: Production deployments, performance-critical applications, data processing

```bash
# Enable performance libraries with optimizations
cmake -B build_performance -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON \
    -DQUARISMA_ENABLE_TBB=ON \
    -DQUARISMA_ENABLE_MIMALLOC=ON \
    -DQUARISMA_VECTORIZATION_TYPE=avx2

cmake --build build_performance -j
```

**Result**:
- ✅ Maximum runtime performance
- ✅ Link-Time Optimization enabled
- ✅ Intel TBB for parallel processing
- ✅ mimalloc for optimized memory allocation
- ✅ AVX2 vectorization

**Performance Gains**:
- 5-15% improvement from LTO
- 2-4x speedup from TBB parallelization
- 10-20% improvement from mimalloc
- 8-16x speedup from AVX2 vectorization

## Development Build

**Goal**: Full development environment with testing, benchmarking, and debugging capabilities

**Use Case**: Active development, debugging, testing

```bash
# Enable testing and debugging tools
cmake -B build_dev -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DQUARISMA_BUILD_TESTING=ON \
    -DQUARISMA_ENABLE_GTEST=ON \
    -DQUARISMA_ENABLE_BENCHMARK=ON \
    -DQUARISMA_ENABLE_SANITIZER=ON \
    -DQUARISMA_SANITIZER_TYPE=address

cmake --build build_dev -j

# Run tests
ctest --test-dir build_dev --output-on-failure
```

**Result**:
- ✅ Debug symbols enabled
- ✅ Google Test framework
- ✅ Benchmarking support
- ✅ AddressSanitizer for memory debugging
- ✅ No optimization (faster compilation)

## External Libraries Build

**Goal**: Faster build using pre-installed system libraries

**Use Case**: Development with system-installed dependencies, CI/CD with cached libraries

```bash
# Use system-installed libraries (faster build)
cmake -B build_external -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_EXTERNAL=ON \
    -DQUARISMA_ENABLE_TBB=ON \
    -DQUARISMA_ENABLE_GTEST=ON

cmake --build build_external -j
```

**Prerequisites**:
```bash
# Ubuntu/Debian
sudo apt-get install libfmt-dev libgtest-dev libtbb-dev

# macOS
brew install fmt googletest tbb

# Fedora/RHEL
sudo dnf install fmt-devel gtest-devel tbb-devel
```

**Result**:
- ✅ Faster build (libraries already compiled)
- ✅ Smaller repository size
- ✅ Shared libraries across projects

## Testing and Benchmarking Build

**Goal**: Comprehensive testing and performance benchmarking

**Use Case**: Quality assurance, performance testing, regression testing

```bash
# Enable all testing and benchmarking features
cmake -B build_test -S . \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DQUARISMA_BUILD_TESTING=ON \
    -DQUARISMA_GOOGLE_TEST=ON \
    -DQUARISMA_ENABLE_BENCHMARK=ON \
    -DQUARISMA_ENABLE_COVERAGE=ON

cmake --build build_test -j

# Run tests with coverage
ctest --test-dir build_test --output-on-failure

# Generate coverage report
cmake --build build_test --target coverage-html
```

**Result**:
- ✅ Optimized code with debug symbols
- ✅ Unit testing with Google Test
- ✅ Performance benchmarking
- ✅ Code coverage analysis

## Production Build

**Goal**: Optimized, stable build for production deployment

**Use Case**: Production servers, release distributions, customer deployments

```bash
# Production-ready build with all optimizations
cmake -B build_production -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON \
    -DQUARISMA_ENABLE_MIMALLOC=ON \
    -DQUARISMA_VECTORIZATION_TYPE=avx2 \
    -DQUARISMA_LOGGING_BACKEND=GLOG

cmake --build build_production -j

# Optional: Strip debug symbols for smaller binary
strip build_production/bin/quarisma
```

**Result**:
- ✅ Maximum optimization
- ✅ Production-grade logging (GLOG)
- ✅ High-performance memory allocator
- ✅ SIMD vectorization
- ✅ Smallest possible binary (after stripping)

## Debugging Build

**Goal**: Maximum debugging information with sanitizers and analysis tools

**Use Case**: Debugging crashes, memory issues, undefined behavior

```bash
# Debug build with multiple sanitizers and analysis tools
cmake -B build_debug -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DQUARISMA_ENABLE_SANITIZER=ON \
    -DQUARISMA_SANITIZER_TYPE=address \
    -DQUARISMA_ENABLE_IWYU=ON \
    -DQUARISMA_ENABLE_CPPCHECK=ON \
    -DQUARISMA_BUILD_TESTING=ON

cmake --build build_debug -j

# Run with sanitizer
./build_debug/bin/quarisma

# Check analysis results
less build_debug/iwyu.log
less build_debug/cppcheckoutput.log
```

**Result**:
- ✅ Full debug symbols
- ✅ AddressSanitizer for memory errors
- ✅ Include-What-You-Use analysis
- ✅ Cppcheck static analysis
- ✅ No optimization for accurate debugging

## CI/CD Build

**Goal**: Fast, reproducible builds for continuous integration

**Use Case**: GitHub Actions, GitLab CI, Jenkins, automated testing

```bash
# Fast CI build with external libraries and testing
cmake -B build_ci -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_EXTERNAL=ON \
    -DQUARISMA_BUILD_TESTING=ON \
    -DQUARISMA_ENABLE_GTEST=ON \
    -DQUARISMA_ENABLE_COVERAGE=ON

cmake --build build_ci -j

# Run tests
ctest --test-dir build_ci --output-on-failure

# Generate coverage report
cmake --build build_ci --target coverage-html
```

**CI Configuration Example** (GitHub Actions):
```yaml
name: CI Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libfmt-dev libgtest-dev ninja-build

      - name: Configure
        run: |
          cmake -B build -S . \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DQUARISMA_ENABLE_EXTERNAL=ON \
            -DQUARISMA_BUILD_TESTING=ON

      - name: Build
        run: cmake --build build -j

      - name: Test
        run: ctest --test-dir build --output-on-failure
```

**Result**:
- ✅ Fast build with cached dependencies
- ✅ Automated testing
- ✅ Code coverage reporting
- ✅ Reproducible builds

## Enhanced Profiler

**Goal**: Advanced profiling with nanosecond-precision timing and memory tracking

**Use Case**: Performance analysis, optimization, bottleneck identification

### Overview

The Enhanced Profiler is an experimental feature providing:
- Nanosecond-precision timing
- Memory tracking
- Hierarchical scopes
- Thread safety
- Multiple output formats (console/JSON/CSV/XML)

### Location

- **Source**: `Library/Core/profiler`
- **Tests**: `Library/Core/Testing/Cxx/TestEnhancedProfiler.cpp`

### Usage Example

```cpp
#include "profiler/native/core/profiler.h"

void example_function() {
    // Profile entire function
    QUARISMA_PROFILE_FUNCTION();

    // Profile specific scope
    {
        QUARISMA_PROFILE_SCOPE("DataProcessing");
        // Your code here
    }

    // Generate report
    quarisma::profiler_impl::generate_report("profile_results.json");
}
```

### Building with Profiler

```bash
# Build with profiler enabled
cmake -B build_profile -S . \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DQUARISMA_ENABLE_PROFILER=ON

cmake --build build_profile -j

# Run with profiling
./build_profile/bin/quarisma

# View results
cat profile_results.json
```

**Note**: The Enhanced Profiler is experimental and may change in future releases. See `Library/Core/Testing/Cxx/TestEnhancedProfiler.cpp` for complete examples.

## Comparison Table

| Configuration | Build Time | Binary Size | Performance | Use Case |
|---------------|------------|-------------|-------------|----------|
| **Minimal** | Fastest | Smallest | Baseline | Quick testing, embedded |
| **High-Performance** | Medium | Medium | Maximum | Production, data processing |
| **Development** | Slow | Large | Low | Active development |
| **External** | Fast | Small | Good | Development with system libs |
| **Testing** | Medium | Large | Good | QA, regression testing |
| **Production** | Medium | Small | Maximum | Production deployment |
| **Debugging** | Slow | Largest | Lowest | Debugging, analysis |
| **CI/CD** | Fast | Small | Good | Automated builds |

## Best Practices

### Choosing the Right Configuration

1. **Development**: Use Development Build for daily work
2. **Testing**: Use Testing Build before commits
3. **Debugging**: Use Debugging Build when investigating issues
4. **Production**: Use Production Build for releases
5. **CI/CD**: Use CI/CD Build for automated pipelines

### Multiple Build Directories

Keep separate build directories for different configurations:

```bash
# Development
cmake -B build_dev -S . -DCMAKE_BUILD_TYPE=Debug

# Release
cmake -B build_release -S . -DCMAKE_BUILD_TYPE=Release

# Testing
cmake -B build_test -S . -DQUARISMA_BUILD_TESTING=ON
```

### Switching Configurations

```bash
# Clean and reconfigure
rm -rf build
cmake -B build -S . [new options]
cmake --build build -j
```

## Related Documentation

- [Build Configuration](build/build-configuration.md) - Detailed build options
- [Third-Party Dependencies](third-party-dependencies.md) - Dependency management
- [Sanitizers](sanitizer.md) - Memory debugging tools
- [Code Coverage](code-coverage.md) - Coverage analysis
- [Cross-Platform Building](cross-platform-building.md) - Platform-specific instructions
