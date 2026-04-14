# Quarisma Bazel User Guide

**Comprehensive guide to building Quarisma with Bazel**

This document consolidates all Bazel-related documentation for the Quarisma project, providing a complete reference for building, testing, and configuring Quarisma using the Bazel build system.

---

## Table of Contents

1. [Overview](#overview)
2. [Getting Started](#getting-started)
3. [Build Flags and Configuration](#build-flags-and-configuration)
4. [Sanitizers](#sanitizers)
5. [Code Coverage](#code-coverage)
6. [Third-Party Dependencies](#third-party-dependencies)
7. [Bazel vs CMake Comparison](#bazel-vs-cmake-comparison)
8. [Build Structure and Architecture](#build-structure-and-architecture)
9. [Advanced Usage](#advanced-usage)
10. [Troubleshooting](#troubleshooting)

---

## Overview

### What is Bazel?

Bazel is a fast, scalable, multi-language build system developed by Google. Quarisma supports both CMake and Bazel build systems, providing flexibility for different development workflows and CI/CD environments.

### Why Use Bazel for Quarisma?

**Advantages of Bazel:**
1. **Incremental builds** - Only rebuilds what changed
2. **Hermetic builds** - More reproducible across environments
3. **Remote caching** - Share build artifacts across team
4. **Parallel execution** - Better parallelization than Make
5. **Cross-platform** - Unified build system for all platforms
6. **Scalable** - Handles large codebases efficiently

**When to Use Bazel:**
- Building large, multi-language projects
- Need reproducible builds
- Want remote caching
- Building from multiple repositories
- CI/CD pipelines requiring hermetic builds
- Working with monorepo structure

**When to Use CMake:**
- Integration with CMake-based projects
- IDE support (CLion, Visual Studio)
- Existing CMake workflows
- Package management with vcpkg/Conan

> **Note**: Both build systems are fully supported and maintained. Bazel defaults match CMake defaults (LOGURU for logging, native profiler backend). Both produce equivalent binaries.

---

## Getting Started

### Prerequisites

- **Bazel 6.0+** or **Bazelisk** (recommended for automatic version management)
- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **Python 3.9+** (for build scripts)
- Platform-specific requirements (see below)

### Installation

#### macOS
```bash
# Install Bazelisk (recommended)
brew install bazelisk

# Or install Bazel directly
brew install bazel
```

#### Linux
```bash
# Install Bazelisk (recommended - automatically manages Bazel versions)
npm install -g @bazel/bazelisk

# Or install Bazel directly
sudo apt-get install bazel
```

#### Windows
```bash
# Install Bazelisk via npm
npm install -g @bazel/bazelisk

# Or use Chocolatey
choco install bazelisk

# Or download from: https://github.com/bazelbuild/bazelisk/releases
```

### Quick Start

#### Basic Build Commands

**Using setup_bazel.py (Recommended):**
```bash
cd Scripts

# Debug build
python3 setup_bazel.py config.build.debug

# Release build
python3 setup_bazel.py config.build.release

# Release build with tests
python3 setup_bazel.py config.build.test.release

# C++20 release build with TBB
python3 setup_bazel.py config.build.release.cxx20.tbb
```

**Using raw Bazel:**
```bash
# Build all targets
bazel build //...

# Build specific library
bazel build //Library/Core:Core

# Build with specific configuration
bazel build --config=release //...

# Run tests
bazel test --config=release //...
```

### Your First Build

1. **Install Bazel/Bazelisk**
   ```bash
   brew install bazelisk  # macOS
   # or
   npm install -g @bazel/bazelisk  # Linux/Windows
   ```

2. **Run a basic build**
   ```bash
   bazel build //...
   ```

3. **Create a personal configuration** (optional)
   ```bash
   cat > .bazelrc.user << 'RCEOF'
   build --config=release
   build --config=avx2
   build --config=mimalloc
   build --config=magic_enum
   RCEOF
   ```

4. **Build with your preferences**
   ```bash
   bazel build //...
   ```

---

## Build Flags and Configuration

### Build Types

Control optimization level and debug information:

```bash
# Debug build (default, includes debug symbols)
python Scripts/setup_bazel.py build.debug
bazel build --config=debug //...

# Release build (optimized, no debug symbols)
python Scripts/setup_bazel.py build.release
bazel build --config=release //...

# Release with debug info (optimized + debug symbols)
python Scripts/setup_bazel.py build.relwithdebinfo
bazel build --config=relwithdebinfo //...
```

### C++ Standard Selection

Choose the C++ standard version:

```bash
# C++17 (default)
python Scripts/setup_bazel.py build.cxx17
bazel build --config=cxx17 //...

# C++20
python Scripts/setup_bazel.py build.release.cxx20
bazel build --config=cxx20 //...

# C++23
python Scripts/setup_bazel.py build.release.cxx23
bazel build --config=cxx23 //...
```

### Vectorization Options

Enable SIMD vectorization for performance:

```bash
# SSE vectorization
python Scripts/setup_bazel.py build.release.sse
bazel build --config=sse //...

# AVX vectorization
python Scripts/setup_bazel.py build.release.avx
bazel build --config=avx //...

# AVX2 vectorization (recommended)
python Scripts/setup_bazel.py build.release.avx2
bazel build --config=avx2 //...

# AVX-512 vectorization
python Scripts/setup_bazel.py build.release.avx512
bazel build --config=avx512 //...

# No vectorization
bazel build //...
```

### Link-Time Optimization (LTO)

Enable LTO for better optimization:

```bash
# Enable LTO
python Scripts/setup_bazel.py build.release.lto.avx2
bazel build --config=lto --config=release //...
```

### Feature Flags

Enable optional features and libraries:

```bash
# Enable mimalloc allocator
python Scripts/setup_bazel.py build.release.mimalloc
bazel build --config=mimalloc //...

# Enable magic_enum
python Scripts/setup_bazel.py build.release.magic_enum
bazel build --config=magic_enum //...

# Enable Kineto profiling
bazel build --config=kineto //...

# Enable TBB (Intel Threading Building Blocks)
python Scripts/setup_bazel.py build.release.tbb
bazel build --config=tbb //...

# Enable OpenMP
bazel build --config=openmp //...

# Combine multiple features
python Scripts/setup_bazel.py build.release.tbb.mimalloc.magic_enum
bazel build --config=release --config=tbb --config=mimalloc --config=magic_enum //...
```

### Logging Backend Selection

Choose one of the following logging backends:

```bash
# Loguru (default) - Full-featured logging with scopes and callbacks
python Scripts/setup_bazel.py build.release.logging_loguru
bazel build --config=logging_loguru //...

# Google glog - Production-grade logging with minimal overhead
python Scripts/setup_bazel.py build.release.logging_glog
bazel build --config=logging_glog //...

# Native fmt-based logging - No dependencies
python Scripts/setup_bazel.py build.release.logging_native
bazel build --config=logging_native //...
```

### Profiler Backend Selection

Choose profiler backend:

```bash
# Native profiler (default)
python Scripts/setup_bazel.py build.release.profiler_native
bazel build --config=profiler_native //...

# Kineto profiler
python Scripts/setup_bazel.py build.release.kineto
bazel build --config=kineto //...

# ITT profiler
python Scripts/setup_bazel.py build.release.profiler_itt
bazel build --config=profiler_itt //...
```

### GPU Support

#### CUDA Support

```bash
# Enable CUDA
python Scripts/setup_bazel.py build.release.cuda
bazel build --config=cuda //...

# CUDA with specific allocation strategy
python Scripts/setup_bazel.py build.release.cuda.gpu_alloc_async
bazel build --config=cuda --config=gpu_alloc_async //...
```

#### HIP Support (AMD ROCm)

```bash
# Enable HIP
python Scripts/setup_bazel.py build.release.hip
bazel build --config=hip //...

# HIP with specific allocation strategy
python Scripts/setup_bazel.py build.release.hip.gpu_alloc_pool_async
bazel build --config=hip --config=gpu_alloc_pool_async //...
```

#### GPU Allocation Strategies

```bash
# Synchronous allocation (default for CPU)
bazel build --config=gpu_alloc_sync //...

# Asynchronous allocation
bazel build --config=gpu_alloc_async //...

# Pool-based asynchronous allocation (default for GPU)
bazel build --config=gpu_alloc_pool_async //...
```

### Algorithm Options

```bash
# Enable LU pivoting
bazel build --config=lu_pivoting //...

# Enable Sobol 1111 dimensions
bazel build --config=sobol_1111 //...
```

### Platform-Specific Builds

```bash
# macOS
bazel build --config=macos //...

# Linux
bazel build --config=linux //...

# Windows (MSVC)
bazel build --config=windows //...
```

### Compiler Selection

```bash
# Clang (default)
python Scripts/setup_bazel.py build.release

# GCC
python Scripts/setup_bazel.py build.release.gcc

# MSVC (Windows only)
python Scripts/setup_bazel.py build.release.msvc
```

### Build Tool Selection

```bash
# Ninja (default)
python Scripts/setup_bazel.py build.release.ninja

# Xcode (macOS only)
python Scripts/setup_bazel.py build.release.xcode

# Visual Studio (Windows only)
python Scripts/setup_bazel.py build.release.vs22
```

### Shared vs Static Libraries

By default, Quarisma builds static libraries. To build shared libraries:

```bash
bazel build --define=quarisma_build_shared_libs=true //...
```

---

## Sanitizers

Sanitizers help detect memory and threading issues during development and testing. They are available on Unix-like systems and require the Clang compiler.

### Supported Sanitizers

| Sanitizer | Description | Platform Support |
|-----------|-------------|------------------|
| **AddressSanitizer (ASan)** | Detects buffer overflows, use-after-free, and other memory errors | Linux, macOS, Windows |
| **ThreadSanitizer (TSan)** | Detects data races and thread synchronization issues | Linux, macOS |
| **UndefinedBehaviorSanitizer (UBSan)** | Detects undefined behavior in C++ code | Linux, macOS |
| **MemorySanitizer (MSan)** | Detects reads of uninitialized memory | Linux only |
| **LeakSanitizer (LSan)** | Detects memory leaks | Linux only |

### Using Sanitizers with Bazel

```bash
# AddressSanitizer (memory errors)
python Scripts/setup_bazel.py build.debug.sanitizer_asan
bazel build --config=asan //...

# ThreadSanitizer (data races)
python Scripts/setup_bazel.py build.debug.sanitizer_tsan
bazel build --config=tsan //...

# UndefinedBehaviorSanitizer (undefined behavior)
python Scripts/setup_bazel.py build.debug.sanitizer_ubsan
bazel build --config=ubsan //...

# MemorySanitizer (uninitialized memory)
bazel build --config=msan //...

# Multiple sanitizers
python Scripts/setup_bazel.py build.debug.sanitizer_asan.sanitizer_ubsan
```

### Requirements

- **Clang compiler only** - Sanitizers are only supported with Clang
- **Debug mode** - Sanitizers automatically force debug builds
- **No optimizations** - All optimizations are disabled for accurate instrumentation

### Performance Impact

- **AddressSanitizer**: ~2x slowdown, ~3x memory usage
- **ThreadSanitizer**: ~5-15x slowdown, ~5-10x memory usage
- **MemorySanitizer**: ~3x slowdown, significant memory usage
- **UndefinedBehaviorSanitizer**: ~20% slowdown, minimal memory overhead
- **LeakSanitizer**: Minimal runtime overhead, memory usage at exit

### Best Practices

1. **Start with AddressSanitizer** - Most common and effective for memory errors
2. **Use UndefinedBehaviorSanitizer** - Catches subtle C++ undefined behavior issues
3. **ThreadSanitizer for Concurrency** - Essential for multi-threaded code
4. **MemorySanitizer for Initialization** - Detects use of uninitialized memory
5. **Regular Testing** - Run sanitizer builds regularly during development

### Limitations

- **Single Sanitizer** - Only one sanitizer can be enabled at a time
- **Clang Dependency** - Requires Clang compiler installation
- **Platform Restrictions** - Some sanitizers are not available on all platforms

---

## Code Coverage

Code coverage analysis helps identify untested code paths and measure test effectiveness.

### Enabling Coverage with Bazel

**Note**: Coverage support in Bazel is currently limited. For comprehensive coverage analysis, use the CMake build system with the coverage tools described below.

### Coverage with CMake (Recommended)

For detailed coverage analysis, use CMake:

```bash
# Clang workflow
cd Scripts
python setup.py config.build.test.ninja.clang.debug.coverage

# GCC workflow
python setup.py config.build.test.ninja.gcc.debug.coverage

# MSVC workflow (Windows)
python setup.py config.build.test.vs22.debug.coverage
```

### Coverage Tools by Compiler

| Compiler | Tool | Output Formats |
|----------|------|----------------|
| **Clang** | llvm-profdata, llvm-cov | HTML, JSON, LCOV |
| **GCC** | gcov, lcov | HTML, JSON, LCOV |
| **MSVC** | OpenCppCoverage | HTML, Cobertura XML |

### Coverage Output

Coverage reports are generated in `<build_dir>/coverage_report/`:

- `html/index.html` - Interactive dashboard with per-file drill-downs
- `coverage_summary.json` - Cobertura-compatible JSON summary
- `coverage.txt` - Plain-text summary
- `coverage.info` - LCOV trace (GCC only)

### Excluding Files from Coverage

Use exclusion patterns to focus coverage on relevant code:

```bash
# Exclude test files and benchmarks
python Tools/coverage/run_coverage.py --build=build --exclude-patterns="Test,Benchmark"

# Exclude generated code
python Tools/coverage/run_coverage.py --build=build --exclude-patterns="*Generated*,*Serialization*"
```

**Default exclusions (always applied):**
- `*ThirdParty*` - Third-party libraries
- `*Testing*` - Test infrastructure
- `/usr/*` - System libraries

### CI/CD Integration

```bash
# Generate coverage in CI
python setup.py config.build.test.ninja.clang.debug.coverage

# Upload coverage reports as artifacts
# Parse coverage_report/coverage.json for automated gating
```

---

## Third-Party Dependencies

Quarisma uses a conditional compilation pattern where each library is controlled by `QUARISMA_ENABLE_XXX` options in CMake. In Bazel, dependencies are managed through the `WORKSPACE.bazel` file and controlled via `--config` flags.

### Dependency Categories

#### Mandatory Core Libraries (Always Included)

| Library | Description | Bazel Target |
|---------|-------------|--------------|
| **fmt** | Modern C++ formatting | `@fmt//:fmt` |
| **cpuinfo** | CPU feature detection | `@cpuinfo//:cpuinfo` |

#### Optional Libraries (Enabled by Default)

| Library | Bazel Config | Description | Target |
|---------|--------------|-------------|--------|
| **magic_enum** | `--config=magic_enum` | Enum reflection | `@magic_enum//:magic_enum` |
| **loguru** | `--config=logging_loguru` | Lightweight logging | `@loguru//:loguru` |

#### Optional Libraries (Disabled by Default)

| Library | Bazel Config | Description | Target |
|---------|--------------|-------------|--------|
| **mimalloc** | `--config=mimalloc` | High-performance allocator | `@mimalloc//:mimalloc` |
| **Google Test** | `--config=gtest` | Testing framework | `@com_google_googletest//:gtest` |
| **Benchmark** | `--config=benchmark` | Microbenchmarking | `@com_google_benchmark//:benchmark` |
| **TBB** | `--config=tbb` | Threading Building Blocks | `@tbb//:tbb` |
| **Kineto** | `--config=kineto` | Profiling library | `@kineto//:kineto` |

### Dependency Management in Bazel

#### WORKSPACE.bazel

External dependencies are declared in `WORKSPACE.bazel`:

```python
# Example: fmt library
http_archive(
    name = "fmt",
    build_file = "//third_party:fmt.BUILD",
    urls = ["https://github.com/fmtlib/fmt/archive/10.1.1.tar.gz"],
    strip_prefix = "fmt-10.1.1",
)
```

#### BUILD Files

Dependencies are linked in `BUILD.bazel` files:

```python
cc_library(
    name = "Core",
    srcs = [...],
    deps = [
        "@fmt//:fmt",
        "@cpuinfo//:cpuinfo",
    ] + select({
        "//bazel:enable_mimalloc": ["@mimalloc//:mimalloc"],
        "//conditions:default": [],
    }),
)
```

### Enabling/Disabling Dependencies

```bash
# Enable high-performance allocator
bazel build --config=mimalloc //...

# Enable multiple features
bazel build --config=release --config=tbb --config=mimalloc --config=magic_enum //...

# Disable optional features (use default build without configs)
bazel build //...
```

### Third-Party Build Configuration

Third-party targets are configured to:
- Suppress warnings (using `-w` or `/w`)
- Use the same C++ standard as the main project
- Avoid altering the main project's compiler/linker settings
- Provide consistent target aliases

---

## Bazel vs CMake Comparison

Both build systems are fully supported and offer different advantages. This section provides side-by-side comparisons for common operations.

### Feature Comparison

| Feature | CMake | Bazel | Notes |
|---------|-------|-------|-------|
| **Incremental Builds** | Good | Excellent | Bazel's caching is more aggressive |
| **Hermetic Builds** | Manual | Automatic | Bazel ensures reproducibility |
| **IDE Integration** | Excellent | Good | CMake has broader IDE support |
| **Learning Curve** | Moderate | Steep | CMake is more familiar to most developers |
| **Build Speed** | Fast | Very Fast | Bazel excels at large codebases |
| **Remote Caching** | Manual | Built-in | Bazel supports remote caching natively |
| **Default Backends** | LOGURU/KINETO | LOGURU/NATIVE | Both use LOGURU for logging |

### Common Build Commands

| Task | CMake | Bazel |
|------|-------|-------|
| **Configure** | `cmake -B build` | N/A (automatic) |
| **Build all** | `cmake --build build` | `bazel build //...` |
| **Build library** | `cmake --build build --target Core` | `bazel build //Library/Core:Core` |
| **Run tests** | `ctest --test-dir build` | `bazel test //...` |
| **Clean** | `rm -rf build` | `bazel clean` |
| **Release build** | `cmake -B build -DCMAKE_BUILD_TYPE=Release` | `bazel build --config=release //...` |

### Build Type Configuration

| Build Type | CMake | Bazel |
|------------|-------|-------|
| **Debug** | `-DCMAKE_BUILD_TYPE=Debug` | `--config=debug` |
| **Release** | `-DCMAKE_BUILD_TYPE=Release` | `--config=release` |
| **RelWithDebInfo** | `-DCMAKE_BUILD_TYPE=RelWithDebInfo` | `--config=relwithdebinfo` |

### Feature Flags Mapping

| Feature | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **LTO** | `-DQUARISMA_ENABLE_LTO=ON` | `--config=lto` |
| **AVX2** | `-DQUARISMA_VECTORIZATION_TYPE=avx2` | `--config=avx2` |
| **AVX512** | `-DQUARISMA_VECTORIZATION_TYPE=avx512` | `--config=avx512` |
| **SSE** | `-DQUARISMA_VECTORIZATION_TYPE=sse` | `--config=sse` |
| **mimalloc** | `-DMEMORY_ENABLE_MIMALLOC=ON` | `--config=mimalloc` |
| **magic_enum** | `-DQUARISMA_ENABLE_MAGICENUM=ON` | `--config=magic_enum` |
| **Kineto** | `-DQUARISMA_ENABLE_KINETO=ON` | `--config=kineto` |
| **TBB** | `-DQUARISMA_ENABLE_TBB=ON` | `--config=tbb` |
| **OpenMP** | `-DQUARISMA_ENABLE_OPENMP=ON` | `--config=openmp` |
| **CUDA** | `-DMEMOY_ENABLE_CUDA=ON` | `--config=cuda` |
| **HIP** | `-DQUARISMA_ENABLE_HIP=ON` | `--config=hip` |
| **Google Test** | `-DQUARISMA_ENABLE_GTEST=ON` | `--config=gtest` |
| **Benchmark** | `-DQUARISMA_ENABLE_BENCHMARK=ON` | `--config=benchmark` |
| **LU Pivoting** | `-DQUARISMA_LU_PIVOTING=ON` | `--config=lu_pivoting` |
| **Sobol 1111** | `-DQUARISMA_SOBOL_1111=ON` | `--config=sobol_1111` |

### Logging Backend Mapping

| Backend | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **Loguru** | `-DLOGGING_BACKEND=LOGURU` | `--config=logging_loguru` |
| **glog** | `-DLOGGING_BACKEND=GLOG` | `--config=logging_glog` |
| **Native** | `-DLOGGING_BACKEND=NATIVE` | `--config=logging_native` |

### Profiler Backend Mapping

| Backend | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **Native** | `-DQUARISMA_PROFILER_BACKEND=NATIVE` | `--config=profiler_native` |
| **Kineto** | `-DQUARISMA_PROFILER_BACKEND=KINETO` | `--config=kineto` |
| **ITT** | `-DQUARISMA_PROFILER_BACKEND=ITT` | `--config=profiler_itt` |

### Sanitizer Mapping

| Sanitizer | CMake Option | Bazel Equivalent |
|-----------|--------------|------------------|
| **AddressSanitizer** | `-DQUARISMA_ENABLE_SANITIZER=ON -DQUARISMA_SANITIZER_TYPE=address` | `--config=asan` |
| **ThreadSanitizer** | `-DQUARISMA_ENABLE_SANITIZER=ON -DQUARISMA_SANITIZER_TYPE=thread` | `--config=tsan` |
| **UBSanitizer** | `-DQUARISMA_ENABLE_SANITIZER=ON -DQUARISMA_SANITIZER_TYPE=undefined` | `--config=ubsan` |
| **MemorySanitizer** | `-DQUARISMA_ENABLE_SANITIZER=ON -DQUARISMA_SANITIZER_TYPE=memory` | `--config=msan` |

### GPU Allocation Strategy Mapping

| Strategy | CMake Option | Bazel Equivalent |
|----------|--------------|------------------|
| **Sync** | `-DQUARISMA_GPU_ALLOC=SYNC` | `--config=gpu_alloc_sync` |
| **Async** | `-DQUARISMA_GPU_ALLOC=ASYNC` | `--config=gpu_alloc_async` |
| **Pool Async** | `-DQUARISMA_GPU_ALLOC=POOL_ASYNC` | `--config=gpu_alloc_pool_async` |

### Example Build Scenarios

#### CMake to Bazel Equivalent

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
               -DQUARISMA_VECTORIZATION_TYPE=avx2 \
               -DMEMORY_ENABLE_MIMALLOC=ON \
               -DQUARISMA_ENABLE_MAGICENUM=ON
cmake --build build
```

**Bazel:**
```bash
bazel build --config=release \
            --config=avx2 \
            --config=mimalloc \
            --config=magic_enum \
            //...
```

#### Production Build

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
               -DQUARISMA_ENABLE_LTO=ON \
               -DQUARISMA_VECTORIZATION_TYPE=avx2
cmake --build build
```

**Bazel:**
```bash
bazel build --config=release --config=lto --config=avx2 //...
```

#### Development Build with Sanitizers

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DQUARISMA_ENABLE_SANITIZER=ON \
               -DQUARISMA_SANITIZER_TYPE=address
cmake --build build
```

**Bazel:**
```bash
bazel build --config=debug --config=asan //...
```

#### GPU-Accelerated Build

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
               -DMEMOY_ENABLE_CUDA=ON \
               -DQUARISMA_VECTORIZATION_TYPE=avx2
cmake --build build
```

**Bazel:**
```bash
bazel build --config=release --config=cuda --config=avx2 //...
```

---

## Build Structure and Architecture

Understanding the Bazel build structure helps you navigate and modify the build configuration.

### File Structure

```
Quarisma/
├── WORKSPACE.bazel              # Workspace definition and external dependencies
├── BUILD.bazel                  # Root build file
├── .bazelrc                     # Build configuration flags
├── .bazelversion                # Specifies Bazel version (7.0.0)
│
├── bazel/                       # Bazel helper files
│   ├── BUILD.bazel             # Config settings
│   └── quarisma.bzl              # Helper functions (copts, defines, linkopts)
│
├── third_party/                 # Third-party BUILD files
│   ├── fmt.BUILD               # fmt library
│   ├── cpuinfo.BUILD           # cpuinfo library
│   ├── magic_enum.BUILD        # magic_enum library
│   └── mimalloc.BUILD          # mimalloc library
│
├── Library/
│   └── Core/
│       └── BUILD.bazel         # Core library build
│
└── ThirdParty/                  # (Existing CMake third-party deps)
```

### Key Concepts

#### 1. WORKSPACE.bazel

- Defines the workspace name
- Declares external dependencies (third-party libraries)
- Equivalent to CMake's `find_package()` and `add_subdirectory(ThirdParty)`

**Example:**
```python
workspace(name = "quarisma")

http_archive(
    name = "fmt",
    build_file = "//third_party:fmt.BUILD",
    urls = ["https://github.com/fmtlib/fmt/archive/10.1.1.tar.gz"],
    strip_prefix = "fmt-10.1.1",
)
```

#### 2. BUILD.bazel

- Defines build targets (libraries, executables, tests)
- Equivalent to CMake's `add_library()`, `add_executable()`, `add_test()`

**Example:**
```python
cc_library(
    name = "Core",
    srcs = glob(["src/**/*.cpp"]),
    hdrs = glob(["include/**/*.h"]),
    deps = [
        "@fmt//:fmt",
        "@cpuinfo//:cpuinfo",
    ],
    visibility = ["//visibility:public"],
)
```

#### 3. .bazelrc

- Sets default build flags
- Defines named configurations (`--config=release`, etc.)
- Equivalent to CMake cache variables and build types

**Example:**
```
# Release configuration
build:release --compilation_mode=opt
build:release --strip=always

# AVX2 vectorization
build:avx2 --copt=-mavx2
build:avx2 --copt=-mfma

# mimalloc allocator
build:mimalloc --define=quarisma_enable_mimalloc=true
```

#### 4. bazel/quarisma.bzl

- Provides reusable functions for compiler flags and defines
- Equivalent to CMake functions in `Cmake/tools/*.cmake`

**Example:**
```python
def quarisma_copts():
    return ["-Wall", "-Wextra", "-Wpedantic"]

def quarisma_defines():
    return select({
        "//bazel:enable_cuda": ["MEMOY_ENABLE_CUDA", "QUARISMA_HAS_CUDA=1"],
        "//conditions:default": ["QUARISMA_HAS_CUDA=0"],
    })
```

#### 5. third_party/*.BUILD

- Build rules for external dependencies
- Created when the external library doesn't provide a BUILD file
- Equivalent to wrapper CMakeLists.txt for third-party libraries

### Configuration System

#### Config Settings

Bazel uses `config_setting` + defines:

```python
config_setting(
    name = "enable_mimalloc",
    define_values = {"quarisma_enable_mimalloc": "true"},
)
```

Used in build with:
```bash
bazel build --config=mimalloc //...
# or
bazel build --define=quarisma_enable_mimalloc=true //...
```

#### Conditional Compilation

**Bazel:**
```python
cc_library(
    name = "Core",
    srcs = ["core.cpp"] + select({
        "//bazel:enable_cuda": glob(["gpu/*.cpp"]),
        "//conditions:default": [],
    }),
)
```

**CMake Equivalent:**
```cmake
if(MEMOY_ENABLE_CUDA)
    target_sources(Core PRIVATE gpu/*.cpp)
endif()
```

### Platform-Specific Configuration

**Bazel:**
```python
cc_library(
    name = "Core",
    linkopts = select({
        "@platforms//os:linux": ["-lpthread", "-ldl", "-lrt"],
        "@platforms//os:macos": ["-framework Security"],
        "@platforms//os:windows": ["-DEFAULTLIB:bcrypt.lib"],
        "//conditions:default": [],
    }),
)
```

**CMake Equivalent:**
```cmake
if(UNIX AND NOT APPLE)
    target_link_libraries(Core PRIVATE pthread dl rt)
elseif(APPLE)
    target_link_libraries(Core PRIVATE "-framework Security")
elseif(WIN32)
    target_link_libraries(Core PRIVATE bcrypt)
endif()
```

---

## Advanced Usage

### Testing

```bash
# Build and run all tests
bazel test //...

# Run specific test
bazel test //Library/Core/Testing/Cxx:core_tests

# Run tests with Google Test
bazel test --config=gtest //...

# Run benchmarks
bazel test --config=benchmark //...

# Run tests with output
bazel test --test_output=all //...
```

### Query Build Graph

```bash
# Show all targets
bazel query //...

# Show dependencies of Core library
bazel query 'deps(//Library/Core:Core)'

# Show reverse dependencies
bazel query 'rdeps(//..., //Library/Core:Core)'

# List all test targets
bazel query 'kind(cc_test, //...)'
```

### Build Analysis

```bash
# Profile build performance
bazel build --profile=profile.json //...

# Analyze profile
bazel analyze-profile profile.json
```

### Remote Caching

Configure remote caching for faster builds:

```bash
# In .bazelrc.user
build --remote_cache=grpc://your-cache-server:9092

# Or use local disk cache
bazel build --disk_cache=/tmp/bazel-cache //...
```

### Custom Bazel Flags

```bash
# Increase verbosity
bazel build --verbose_failures //...

# Use specific number of parallel jobs
bazel build -j 16 //...

# Show build timing
bazel build --profile=/tmp/profile.txt //...

# Disable Bazel server (useful for CI/CD)
bazel build --noserver //...
```

### Cleaning

```bash
# Clean build artifacts
bazel clean

# Clean everything including external dependencies
bazel clean --expunge
```

### Build Output

Build artifacts are located in:
- `bazel-bin/` - Compiled binaries and libraries
- `bazel-out/` - Build outputs
- `bazel-testlogs/` - Test logs

### Personal Configuration

Create a `.bazelrc.user` file in the project root to customize your build settings:

```bash
# .bazelrc.user example
build --config=release
build --config=avx2
build --config=mimalloc
build --config=magic_enum
build --config=logging_native
```

Then simply run:
```bash
bazel build //...
```

### Bazel Version Management

#### Automatic Version Management (Recommended)

Use **Bazelisk** for automatic Bazel version management. Bazelisk reads the `.bazelversion` file and automatically downloads the correct Bazel version.

```bash
# Check current Bazel version
bazelisk version

# Bazelisk automatically uses version from .bazelversion
bazelisk build //...
```

#### Manual Version Management

```bash
# Check installed Bazel version
bazel version

# Upgrade Bazel
# macOS
brew upgrade bazel

# Linux (via npm)
npm install -g @bazel/bazelisk@latest

# Windows
# Download latest from: https://github.com/bazelbuild/bazel/releases
```

#### .bazelversion File

The `.bazelversion` file specifies the Bazel version to use:

```
# Current version in .bazelversion
7.0.0
```

To update:

```bash
# Edit .bazelversion
echo "7.1.0" > .bazelversion

# Bazelisk will automatically download and use this version
bazelisk build //...
```

---

## Troubleshooting

### Common Issues

#### Bazel Not Found

```
Error: Neither bazel nor bazelisk found in PATH
```

**Solution:** Install Bazelisk

```bash
# macOS
brew install bazelisk

# Linux
npm install -g @bazel/bazelisk

# Windows
# Download from: https://github.com/bazelbuild/bazelisk/releases
```

#### Build Fails with Configuration Error

```bash
# Clean Bazel cache
bazel clean --expunge

# Rebuild
python Scripts/setup_bazel.py build.test.release
```

#### Missing Dependencies

If a third-party dependency is missing, check:
1. `WORKSPACE.bazel` - Ensure the dependency is declared
2. `third_party/*.BUILD` - Ensure the BUILD file exists
3. Network connectivity - Bazel downloads dependencies on first build

#### Configuration Conflicts

Some configurations are mutually exclusive:
- Only one logging backend can be active at a time
- TBB and STDThread are mutually exclusive (TBB takes precedence)
- CUDA and HIP cannot both be enabled

#### Platform-Specific Issues

**macOS:**
- Ensure Xcode Command Line Tools are installed: `xcode-select --install`

**Linux:**
- Install required development packages: `sudo apt-get install build-essential`

**Windows:**
- Ensure Visual Studio 2017 or later is installed
- Run builds from "Developer Command Prompt for VS"

#### CUDA Not Found

```bash
# Ensure CUDA is installed and in PATH
# Then rebuild with CUDA config
python Scripts/setup_bazel.py build.release.cuda
```

#### Slow Builds

```bash
# Use release build with optimizations
python Scripts/setup_bazel.py build.release.avx2.lto

# Use parallel build
bazel build -j 8 //...

# Use remote or disk caching
bazel build --disk_cache=/tmp/bazel-cache //...
```

#### Out of Memory During Build

```bash
# Reduce parallel jobs
bazel build -j 2 //...

# Or use Bazel's memory limit
bazel build --memory_limit_mb=4096 //...
```

### Performance Optimization

#### Build Optimization

```bash
# Use release build with LTO and vectorization
python Scripts/setup_bazel.py build.release.lto.avx2

# Use remote caching (if available)
bazel build --remote_cache=grpc://cache-server:9092 //...

# Use local disk cache
bazel build --disk_cache=/tmp/bazel-cache //...
```

#### Test Optimization

```bash
# Run tests in parallel
bazel test -j 8 //...

# Run only changed tests
bazel test --test_filter='*Changed*' //...

# Skip expensive tests
bazel test --test_tag_filters=-expensive //...
```

---

## Common Build Configurations

### Development Build (Fast Iteration)

```bash
python Scripts/setup_bazel.py build.test.debug
```

### Release Build (Optimized)

```bash
python Scripts/setup_bazel.py build.test.release.avx2.lto
```

### Release with Debug Info

```bash
python Scripts/setup_bazel.py build.test.relwithdebinfo.avx2
```

### GPU-Accelerated Build (CUDA)

```bash
python Scripts/setup_bazel.py build.test.release.cuda.avx2
```

### GPU-Accelerated Build (HIP)

```bash
python Scripts/setup_bazel.py build.test.release.hip.avx2
```

### Full-Featured Build

```bash
python Scripts/setup_bazel.py build.test.release.cuda.tbb.mimalloc.magic_enum.avx2.lto
```

### Debug with Sanitizers

```bash
python Scripts/setup_bazel.py build.test.debug.sanitizer_asan
```

### Xcode Build (macOS)

```bash
python Scripts/setup_bazel.py build.test.release.xcode
```

### Visual Studio Build (Windows)

```bash
python Scripts/setup_bazel.py build.test.release.vs22
```

---

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Bazel Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: bazelbuild/setup-bazel@v2
      - name: Build
        run: python Scripts/setup_bazel.py build.test.release
      - name: Run Tests
        run: bazel test //...
```

### Local CI Testing

```bash
# Run full test suite
python Scripts/setup_bazel.py build.test.release

# Run specific test
bazel test //Library/Core/Testing/Cxx:CoreCxxTests

# Run with verbose output
bazel test --test_output=all //Library/Core/Testing/Cxx:CoreCxxTests
```

---

## Contributing

When adding new source files:
1. Update the appropriate `BUILD.bazel` file
2. Add any new dependencies to `WORKSPACE.bazel`
3. Create BUILD files for new third-party dependencies in `third_party/`
4. Update this documentation if adding new configuration options

---

## Support and Resources

### Quarisma Documentation

- [Quarisma README](../README.md) - Project overview and quick start
- [Build Configuration](readme/build/build-configuration.md) - CMake build configuration
- [Third-Party Dependencies](readme/third-party-dependencies.md) - Dependency management
- [Sanitizer Guide](readme/sanitizer.md) - Runtime instrumentation
- [Code Coverage](readme/code-coverage.md) - Coverage analysis

### Bazel Resources

- [Bazel Documentation](https://bazel.build/docs)
- [Bazelisk Documentation](https://github.com/bazelbuild/bazelisk)
- [CMake to Bazel Migration Guide](https://bazel.build/migrate/cmake)
- [Bazel C++ Tutorial](https://bazel.build/tutorials/cpp)

### Getting Help

For issues with the Bazel build:
- Check existing issues on GitHub
- Refer to CMake build as reference implementation
- Consult Bazel documentation: https://bazel.build/

---

## Files Created by Bazel Setup

- `WORKSPACE.bazel` - Dependency definitions
- `BUILD.bazel` - Root build file
- `.bazelrc` - Build configurations
- `.bazelversion` - Bazel version (7.0.0)
- `bazel/BUILD.bazel` - Config settings
- `bazel/quarisma.bzl` - Helper functions
- `Library/*/BUILD.bazel` - Library build files
- `third_party/*.BUILD` - Third-party dependencies

---

## Summary

This guide has covered:

1. **Overview** - Introduction to Bazel and its benefits for Quarisma
2. **Getting Started** - Installation and quick start guide
3. **Build Flags** - Comprehensive list of all build configurations
4. **Sanitizers** - Memory and threading issue detection
5. **Code Coverage** - Coverage analysis (use CMake for full support)
6. **Third-Party Dependencies** - Dependency management in Bazel
7. **Bazel vs CMake** - Side-by-side comparison of both build systems
8. **Build Structure** - Understanding Bazel's architecture
9. **Advanced Usage** - Testing, querying, profiling, and optimization
10. **Troubleshooting** - Common issues and solutions

Both CMake and Bazel build systems are fully supported and produce equivalent binaries. Choose the build system that best fits your workflow and requirements.

---

**Last Updated:** 2025-11-23
**Bazel Version:** 7.0.0
**Quarisma Version:** 1.0.0

