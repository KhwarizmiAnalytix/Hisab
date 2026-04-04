# Quarisma

[![CI Status](https://github.com/QuarismaAnalyitix/Quarisma/actions/workflows/ci.yml/badge.svg)](https://github.com/QuarismaAnalyitix/Quarisma/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/QuarismaAnalyitix/Quarisma/branch/main/graph/badge.svg)](https://codecov.io/gh/QuarismaAnalyitix/Quarisma)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11420/badge)](https://www.bestpractices.dev/projects/11420)
[![License: GPL-3.0 or Commercial](https://img.shields.io/badge/License-GPL--3.0%20or%20Commercial-blue.svg)](LICENSE)


> **Note**: Quarisma is actively working toward OpenSSF Best Practices certification. See [Docs/OpenSSF_Badge_Update_Guide.md](Docs/OpenSSF_Badge_Update_Guide.md) for our compliance roadmap and current status.
## Project Introduction

**Quarisma** is a modern, high-performance quantitative analysis library designed for both CPU and GPU computing. Built with a production-ready C++ foundation and dual build system support (CMake and Bazel), Quarisma provides cross-platform compatibility, advanced optimization capabilities, and flexible dependency management for demanding computational workloads.

### Key Features

- **High-Performance Computing** - Optimized for CPU and GPU acceleration
- **Cross-Platform Compatibility** - Windows, Linux, and macOS support
- **Dual Build System Support** - CMake 3.16+ and Bazel with feature parity and consistent defaults
- **Advanced Optimization** - Link-Time Optimization (LTO), vectorization (SSE/AVX/AVX2/AVX-512), and compiler-specific flags
- **Flexible Logging System** - Three backend options (LOGURU, GLOG, NATIVE) with configurable levels
- **Comprehensive Testing & Analysis** - Sanitizers, code coverage analysis, static analysis tools, and memory profiling
- **Production-Ready** - Thoroughly tested across multiple platforms and compilers

## Table of Contents

- [Project Introduction](#project-introduction)
- [Obtaining Quarisma](#obtaining-quarisma)
- [Build Optimizations](#build-optimizations)
- [Analysis Tools](#analysis-tools)
- [Third-Party Dependencies](#third-party-dependencies)
- [High-Performance Computing](#high-performance-computing)
- [Features](#features)
- [Documentation](#documentation)
- [Best Practices](#best-practices)
- [Coding Standards](#coding-standards)
- [Providing Feedback](#providing-feedback)
- [Contributing](#contributing)
- [License](#license)

## Obtaining Quarisma

### Download and Installation

Quarisma is available through multiple channels:

#### From GitHub (Recommended)

Clone the repository directly from GitHub:

```bash
# Clone the repository
git clone https://github.com/QuarismaAnalyitix/Quarisma.git
cd Quarisma

# Initialize Git submodules for third-party dependencies
git submodule update --init --recursive

# Install Python dependencies
pip install -r requirements.txt
```

### System Requirements

Quarisma requires the following minimum versions:

- **Build System** (choose one or both):
  - **CMake** 3.16 or later
  - **Bazel** 6.0+ or **Bazelisk** (recommended for automatic version management)
- **C++17** compatible compiler:
  - **Windows**: MSVC 2019+ or Clang 10+
  - **Linux**: GCC 9+ or Clang 10+
  - **macOS**: Apple Clang 12+ or Clang 10+
- **Python** 3.9+ (for build scripts and testing)
- **Git** (for cloning and submodule management)

### Quick Start: Building from Source

After cloning and installing dependencies, follow these platform-specific instructions to build Quarisma:

#### Understanding setup.py Flags

The `setup.py` script uses a standardized flag ordering convention to organize build configuration:

- **config** - Runs the CMake configuration phase to generate build files
- **build** - Compiles the source code and builds the project
- **test** - Runs the test suite after building

##### Profiler selection

Use the `--profiler.<backend>` flag (where `<backend>` is `kineto`, `native`, or `itt`) to control `QUARISMA_PROFILER_TYPE` and automatically toggle the matching Kineto/TraceMe/ITT feature set. Example:

```bash
python setup.py config.build.ninja.clang.release --profiler.native
```

If you omit the profiler flag, `KINETO` is selected by default. The helper no longer exposes separate `kineto` or `ittapi` switches – the single `profiler.xxx` option now controls which profiler backend you are compiling for.

#### Unix/Linux (GCC and Clang)

**Using setup.py (Recommended):**
```bash
cd Scripts

# Debug build with Clang
python setup.py config.build.ninja.clang.debug

# Release build with optimizations
python setup.py config.build.ninja.clang.release.avx2

# With testing enabled
python setup.py config.build.test.ninja.clang.debug
```

**Using raw CMake:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build . --parallel $(nproc)
```

#### macOS (Clang)

**Using setup.py (Recommended):**
```bash
cd Scripts

# Debug build with Apple Clang
python setup.py config.build.ninja.clang.debug

# Release build with optimizations
python setup.py config.build.ninja.clang.release
```

**Using raw CMake:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build . --parallel $(sysctl -n hw.ncpu)
```

#### Windows (MSVC and Clang)

**Using setup.py (Recommended):**
```bash
cd Scripts

# Debug build with MSVC
python setup.py config.build.vs22.debug

# Release build with Clang
python setup.py config.build.ninja.clang.release

# With testing enabled
python setup.py config.build.test.vs22.debug
```

**Using raw CMake:**
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%
```

### Building with Bazel

Quarisma supports **both CMake and Bazel** build systems, providing flexibility for different development workflows and CI/CD environments. The Bazel build system offers fast incremental builds, hermetic builds, and excellent caching capabilities.

> **Note**: Both build systems are fully supported and maintained. Bazel defaults match CMake defaults where applicable (for example LOGURU for logging). The **profiler backend defaults to Kineto** in both CMake (`QUARISMA_PROFILER_TYPE=KINETO`) and Bazel (no `native_profiler` / `itt` config). Enable the **native** backend with CMake `-DQUARISMA_PROFILER_TYPE=NATIVE` or `python setup.py ... --profiler.native`, and with Bazel `--config=native_profiler` (or `--define=quarisma_profiler_type=native`).

#### Prerequisites

Install Bazel or Bazelisk (recommended):

**Linux:**
```bash
# Install Bazelisk (recommended - automatically manages Bazel versions)
npm install -g @bazel/bazelisk

# Or install Bazel directly
sudo apt-get install bazel
```

**macOS:**
```bash
# Install Bazelisk (recommended)
brew install bazelisk

# Or install Bazel directly
brew install bazel
```

**Windows:**
```bash
# Install Bazelisk via npm
npm install -g @bazel/bazelisk

# Or download Bazel from https://github.com/bazelbuild/bazel/releases
```

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

# Build with specific configuration
bazel build --config=release //...

# Run tests
bazel test --config=release //...
```

#### Configuration Options

The `setup_bazel.py` script uses a prefix-based naming convention for clear and organized configuration:

##### Logging Backends

- **`logging_loguru`** (default) - Full-featured logging with scopes, callbacks, and advanced formatting
- **`logging_glog`** - Google's production-grade logging library with minimal overhead
- **`logging_native`** - Minimal native implementation with zero external dependencies

```bash
cd Scripts

# Use GLOG backend
python3 setup_bazel.py build.release.logging_glog

# Use native logging (no dependencies)
python3 setup_bazel.py build.release.logging_native
```

##### Profiler Backends

- **`profiler_native`** (default) - Native TraceMe profiler implementation
- **`profiler_kineto`** - Advanced profiling with Kineto (coming soon)
- **`profiler_itt`** - Intel ITT API profiler integration

```bash
cd Scripts

# Use native profiler (default)
python3 setup_bazel.py build.release.profiler_native

# Use ITT profiler
python3 setup_bazel.py build.release.profiler_itt
```

##### Sanitizers

- **`sanitizer_asan`** - Address Sanitizer (memory errors, buffer overflows)
- **`sanitizer_tsan`** - Thread Sanitizer (data race detection)
- **`sanitizer_ubsan`** - Undefined Behavior Sanitizer
- **`sanitizer_msan`** - Memory Sanitizer (uninitialized memory reads)

```bash
cd Scripts

# Address Sanitizer
python3 setup_bazel.py build.debug.sanitizer_asan

# Thread Sanitizer
python3 setup_bazel.py build.debug.sanitizer_tsan

# Multiple sanitizers
python3 setup_bazel.py build.debug.sanitizer_asan.sanitizer_ubsan
```

##### Build Types

- **`debug`** - Debug build with symbols and no optimization
- **`release`** - Optimized release build
- **`relwithdebinfo`** - Release build with debug symbols

```bash
cd Scripts

# Debug build
python3 setup_bazel.py build.debug

# Release build
python3 setup_bazel.py build.release

# Release with debug info
python3 setup_bazel.py build.relwithdebinfo
```

##### C++ Standards

- **`cxx17`** - C++17 standard (default)
- **`cxx20`** - C++20 standard
- **`cxx23`** - C++23 standard

```bash
cd Scripts

# C++20 build
python3 setup_bazel.py build.release.cxx20

# C++23 build
python3 setup_bazel.py build.release.cxx23
```

##### Feature Flags

- **`tbb`** - Intel Threading Building Blocks
- **`mimalloc`** - High-performance memory allocator
- **`magic_enum`** - Enum reflection utilities
- **`openmp`** - OpenMP parallel processing
- **`cuda`** - NVIDIA CUDA support
- **`hip`** - AMD HIP support
- **`lto`** - Link-Time Optimization

```bash
cd Scripts

# Enable TBB
python3 setup_bazel.py build.release.tbb

# Enable mimalloc
python3 setup_bazel.py build.release.mimalloc

# Enable LTO
python3 setup_bazel.py build.release.lto

# Combine multiple features
python3 setup_bazel.py build.release.tbb.mimalloc.lto
```

#### Example Build Scenarios

**Development Build (Debug with Sanitizers):**
```bash
cd Scripts
python3 setup_bazel.py build.test.debug.sanitizer_asan.sanitizer_ubsan
```

**Production Build (Release with Optimizations):**
```bash
cd Scripts
python3 setup_bazel.py build.release.cxx20.tbb.lto
```

**CI/CD Build (Release with Tests):**
```bash
cd Scripts
python3 setup_bazel.py build.test.release.logging_glog
```

**High-Performance Build (All Optimizations):**
```bash
cd Scripts
python3 setup_bazel.py build.release.cxx20.tbb.mimalloc.lto
```

**Custom Backend Configuration:**
```bash
cd Scripts
python3 setup_bazel.py build.release.logging_glog.profiler_native.cxx20
```

#### Running Tests

```bash
cd Scripts

# Run all tests
python3 setup_bazel.py build.test.release

# Run tests with specific configuration
python3 setup_bazel.py build.test.debug.sanitizer_asan

# Run tests with custom backends
python3 setup_bazel.py build.test.release.logging_glog.profiler_native
```

**Using raw Bazel:**
```bash
# Run all tests
bazel test --config=release //...

# Run specific test suite
bazel test --config=release //Library/Core/Testing/Cxx:CoreCxxTests

# Run tests with output
bazel test --config=release --test_output=all //...
```

#### Configuration Summary

The build system displays a comprehensive configuration summary before building:

```
================================================================================
QUARISMA BAZEL BUILD CONFIGURATION SUMMARY
================================================================================

Build Configuration:
  Build Type:        RELEASE
  C++ Standard:      CXX20
  Vectorization:     None

Feature Flags:
  QUARISMA_ENABLE_MIMALLOC         OFF
  QUARISMA_ENABLE_MAGIC_ENUM       OFF
  QUARISMA_ENABLE_TBB              ON
  QUARISMA_ENABLE_OPENMP           OFF
  QUARISMA_ENABLE_CUDA             OFF
  QUARISMA_ENABLE_HIP              OFF
  QUARISMA_ENABLE_LTO              OFF

Logging Backend:
  Backend:           LOGURU

Profiler Backend:
  Backend:           NATIVE

Sanitizers:
  None                           (disabled)

================================================================================
```

#### Bazel vs CMake

Both build systems are fully supported and offer different advantages:

| Feature | CMake | Bazel | Notes |
|---------|-------|-------|-------|
| **Incremental Builds** | Good | Excellent | Bazel's caching is more aggressive |
| **Hermetic Builds** | Manual | Automatic | Bazel ensures reproducibility |
| **IDE Integration** | Excellent | Good | CMake has broader IDE support |
| **Learning Curve** | Moderate | Steep | CMake is more familiar to most developers |
| **Build Speed** | Fast | Very Fast | Bazel excels at large codebases |
| **Default Backends** | LOGURU/KINETO | LOGURU/NATIVE | Both use LOGURU for logging |

**When to use CMake:**
- IDE integration is critical
- Team is familiar with CMake
- Need extensive platform-specific customization

**When to use Bazel:**
- Large codebase with many dependencies
- CI/CD pipelines requiring hermetic builds
- Need extremely fast incremental builds
- Working with monorepo structure

### Setup Script Documentation

For detailed information about the `setup.py` script, available CMake flags, and advanced configuration options, see:

📖 **[Setup Guide](Docs/readme/setup.md)** - Comprehensive guide to setup.py and all QUARISMA CMake flags

## Build Optimizations
### Cache Build Systems

Quarisma supports multiple compiler caching systems to dramatically speed up incremental builds. Each caching solution is optimized for specific use cases:

#### BuildCache (Windows)

**BuildCache** is the recommended caching solution for Windows builds, providing incremental build caching with minimal configuration.

```bash
cd Scripts

# Use BuildCache for Windows
python setup.py config.build.ninja.clang.buildcache
```

**Use Case**: Windows developers working on local machines who want fast incremental builds with persistent caching.

#### ccache (CI/CD Pipelines - Linux/macOS)

**ccache** is a distributed compiler cache optimized for CI/CD pipelines on Linux and macOS, providing excellent performance for continuous integration environments.

```bash
cd Scripts

# Use ccache for faster incremental builds (Linux/macOS CI)
python setup.py config.build.ninja.clang.ccache
```

**Use Case**: CI/CD pipelines on Linux and macOS where multiple builds run on different machines and benefit from shared caching.

#### sccache (Alternative CI/CD Caching)

**sccache** is an alternative distributed cache with cloud storage support, providing flexibility for complex CI/CD environments with remote caching capabilities.

```bash
cd Scripts

# Use sccache for distributed caching with cloud storage
python setup.py config.build.ninja.clang.sccache
```

**Use Case**: CI/CD pipelines requiring cloud-based caching or distributed build environments across multiple machines.

#### Disabling Caching

```bash
cd Scripts

# Disable all caching (default)
python setup.py config.build.ninja.clang.none
```

📖 **[Read more: Compiler Caching Guide](Docs/readme/cache.md)** - Complete caching configuration and performance tuning

### Link-Time Optimization (LTO)

Link-Time Optimization is **disabled by default**. Use the `lto` flag to enable it.

```bash
cd Scripts

# Release build without LTO (default - no flag needed)
python setup.py config.build.ninja.clang.release

# Enable LTO if needed (add 'lto' flag to toggle ON)
python setup.py config.build.ninja.clang.release.lto  # (LTO enabled)
```

**LTO Behavior**:
- **Default**: LTO is OFF
- **Toggle**: Adding `lto` flag toggles it ON (since it defaults to OFF)
- **Benefits**: 10-30% performance improvement in release builds
- **Trade-off**: Increases build time (use for final releases)

### Linker Optimizations

The build system automatically detects and uses the fastest available linker for your platform and compiler combination, providing significant performance improvements during the linking phase.

#### Supported Linkers

| Platform | Compiler | Preferred Linker | Fallback |
|----------|----------|------------------|----------|
| Linux | Clang | mold | lld |
| Linux | GCC | mold | gold |
| macOS | Clang/Apple Clang | lld | system linker |
| Windows | Clang | lld-link | default |
| Windows | MSVC | default | - |

#### Installation

**Linux:**
```bash
# Install faster linkers (mold, lld)
sudo apt-get install lld mold
```

**macOS:**
```bash
# Install lld (optional)
brew install llvm
```

**Windows:**
Linker optimization is handled automatically by the build system. No additional installation required.

## Analysis Tools

### Runtime Analysis

Quarisma includes comprehensive runtime analysis tools for detecting memory errors, data races, and undefined behavior:

#### Sanitizers

Enable memory debugging and analysis with modern sanitizers:

```bash
cd Scripts

# Address Sanitizer (memory errors, buffer overflows)
python setup.py config.build.test.ninja.clang.debug --sanitizer.address

# Thread Sanitizer (data race detection)
python setup.py config.build.test.ninja.clang.debug --sanitizer.thread

# Undefined Behavior Sanitizer
python setup.py config.build.test.ninja.clang.debug --sanitizer.undefined

# Memory Sanitizer (uninitialized memory reads)
python setup.py config.build.test.ninja.clang.debug --sanitizer.memory

# Leak Sanitizer (memory leaks)
python setup.py config.build.test.ninja.clang.debug --sanitizer.leak
```

📖 **[Sanitizer Setup Guide](Docs/readme/sanitizer.md)** - Complete sanitizer configuration and usage

#### Valgrind Memory Profiling

For detailed memory leak detection and profiling:

```bash
cd Scripts

# Enable Valgrind support
python setup.py config.build.test.ninja.clang.debug.valgrind
```

📖 **[Valgrind Setup Guide](Docs/readme/valgrind.md)** - Complete Valgrind configuration and usage

### Static Analysis

#### Clang-Tidy

Perform static code analysis and automatic fixes:

```bash
cd Scripts

# Run clang-tidy checks
python setup.py config.build.ninja.clang.clangtidy

# Run with automatic fixes
python setup.py config.build.ninja.clang.clangtidy.fix
```

#### Cppcheck

Comprehensive static analysis for additional code quality checks:

```bash
cd Scripts

# Run cppcheck analysis
python setup.py config.build.ninja.clang.cppcheck
```

#### Include-What-You-Use (IWYU)

Optimize header dependencies and reduce unnecessary includes:

```bash
cd Scripts

# Run IWYU analysis
python setup.py config.build.ninja.clang.iwyu
```

📖 **[Static Analysis Guide](Docs/readme/static-analysis.md)** - Complete configuration and usage

### Linting System

Quarisma uses a comprehensive linting framework to maintain code quality and consistency across the codebase.

#### Running Linters

```bash
cd Tools/linter

# Run all linters
python -m lintrunner

# Run specific linter
python -m lintrunner --only=clangtidy

# Run with automatic fixes
python -m lintrunner --fix
```

#### Linter Configuration

The linting system is configured through:
- **`.lintrunner.toml`** - Main linter configuration file
- **`Tools/linter/config/quarisma_linter_config.yaml`** - Quarisma-specific paths and settings
- **`Tools/linter/adapters/`** - Individual linter adapter implementations

📖 **[Linter Documentation](Docs/readme/linter.md)** - Complete linter guide with configuration details and adapter documentation

### Code Coverage

Generate comprehensive code coverage reports to measure test effectiveness and identify untested code paths. Quarisma supports multiple compilers (Clang, GCC, MSVC) with unified reporting.

#### Quick Start

**Generate coverage with Clang:**
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.debug.coverage
# View HTML report: ../build_ninja_coverage/coverage_report/html/index.html
```

**Generate coverage with MSVC:**
```bash
cd Scripts
python setup.py config.build.test.vs22.debug.coverage
# View HTML report: ../build_vs22_coverage/coverage_report/html/index.html
```

**Generate coverage with GCC:**
```bash
cd Scripts
python setup.py config.build.test.ninja.gcc.debug.coverage
# View HTML report: ../build_ninja_coverage/coverage_report/html/index.html
```

#### Coverage Tools

- **Clang/GCC**: Uses gcov/lcov for coverage data collection and reporting
- **MSVC**: Uses OpenCppCoverage for native Windows coverage analysis
- **Unified Runner**: `Tools/coverage/run_coverage.py` provides consistent interface across all compilers

#### Report Formats

- **HTML** - Interactive visual reports with source code highlighting
- **JSON** - Machine-readable format for CI/CD integration
- **LCOV** - Standard coverage format compatible with Codecov

📖 **[Code Coverage Guide](Docs/readme/code-coverage.md)** - Comprehensive multi-compiler coverage documentation

## Third-Party Dependencies

Quarisma uses carefully selected third-party libraries to provide robust functionality while maintaining minimal external dependencies. All dependencies are managed through Git submodules or system package managers.

### Core Dependencies (Always Included)

| Library | Purpose | Version |
|---------|---------|---------|
| **fmt** | String formatting and output | Latest |
| **cpuinfo** | CPU feature detection | Latest |

### Optional Dependencies

| Library | Purpose | CMake Flag | Default |
|---------|---------|-----------|---------|
| **Google Test** | Unit testing framework | `QUARISMA_ENABLE_GTEST` | ON |
| **Benchmark** | Performance benchmarking | `QUARISMA_ENABLE_BENCHMARK` | OFF |
| **Loguru** | Advanced logging backend | `QUARISMA_ENABLE_LOGURU` | ON |
| **magic_enum** | Enum reflection utilities | `QUARISMA_ENABLE_MAGICENUM` | ON |
| **mimalloc** | High-performance memory allocator | `QUARISMA_ENABLE_MIMALLOC` | ON |
| **TBB** | Intel Threading Building Blocks | `QUARISMA_ENABLE_TBB` | OFF |
| **MKL** | Intel Math Kernel Library | `QUARISMA_ENABLE_MKL` | OFF |
| **CUDA** | GPU acceleration | `QUARISMA_ENABLE_CUDA` | OFF |

### Enabling/Disabling Dependencies

```bash
cd Scripts

# Enable CUDA support
python setup.py config.build.ninja.clang.cuda

# Enable TBB for parallel computing
python setup.py config.build.ninja.clang.tbb

# Use external system libraries instead of submodules
python setup.py config.build.ninja.clang.external

# Disable optional features for minimal builds
python setup.py config.build.ninja.clang.test.magic_enum
```

### Logging System

Quarisma provides a flexible logging system with multiple backend options to suit different use cases and performance requirements.

#### Logging Backends

- **LOGURU** (default) - Full-featured logging with scopes, callbacks, and advanced formatting
- **GLOG** - Google's production-grade logging library with minimal overhead
- **NATIVE** - Minimal native implementation with zero external dependencies

#### Configuring Logging

```bash
cd Scripts

# Use GLOG backend
python setup.py config.build.test.ninja.clang --logging.GLOG

# Use native logging (no dependencies)
python setup.py config.build.test.ninja.clang --logging.NATIVE
```

📖 **[Logging System Guide](Docs/readme/logging.md)** - Complete logging documentation with configuration examples

📖 **[Third-Party Dependencies Guide](Docs/readme/third-party-dependencies.md)** - Dependency management and integration

## High-Performance Computing

Quarisma provides comprehensive support for high-performance computing through CPU vectorization (SIMD), GPU acceleration (CUDA/HIP), and multithreading(TBB/native).

### Quick Start

```bash
cd Scripts

# CPU vectorization (AVX2 - recommended)
python setup.py config.build.ninja.clang.release.avx2

# GPU acceleration (CUDA)
python setup.py config.build.ninja.clang.release.cuda

# Multithreading (TBB)
python setup.py config.build.ninja.clang.release.tbb

# Combined: CUDA + AVX2 + LTO (LTO enabled by default)
python setup.py config.build.ninja.clang.release.cuda.avx2
```

### Features

- **Vectorization (SIMD)**: SSE, AVX, AVX2, AVX-512 instruction sets (2-8x speedup)
- **GPU Acceleration**: NVIDIA CUDA and AMD HIP support (10-100x speedup)
- **Multithreading**: Intel TBB and native C++17/20 threading
- **Flexible Combinations**: Mix and match features for optimal performance

### Supported Platforms

- **CPU Vectorization**: All platforms (Windows, Linux, macOS)
- **CUDA**: Linux, Windows, macOS (with NVIDIA GPU)
- **HIP**: Linux (primary), Windows (experimental)
- **Multithreading**: All platforms

📖 **[Read more: High-Performance Computing Guide](Docs/readme/high-performance-computing.md)** - Comprehensive guide to SIMD, GPU acceleration, and multithreading

---

## Features

### Build Configuration

Configure your build with multiple options including build types (Debug, Release, RelWithDebInfo, MinSizeRel), C++ standard selection (C++17/20/23), and optimization settings.

**Key capabilities:**
- Multiple build types for different use cases
- Link-Time Optimization (LTO) for maximum performance
- Custom compiler flags and optimization levels

📖 **[Read more: Build Configuration Guide](Docs/readme/build/build-configuration.md)**

---

### Cross-Platform Building

Full cross-platform compatibility across Windows, Linux, and macOS with platform-specific optimizations and build instructions.

**Supported platforms:**
- Windows (MSVC 2019+, Clang)
- Linux (GCC 9+, Clang 10+)
- macOS (Apple Clang, including Apple Silicon)

📖 **[Read more: Cross-Platform Building Guide](Docs/readme/cross-platform-building.md)**

---

## Documentation

### Core Documentation

- **[Coding Standards](Docs/readme/coding-standards.md)** - Comprehensive guide to naming conventions, code formatting, testing, and best practices
- **[Setup Guide](Docs/readme/setup.md)** - Detailed setup.py script and CMake flags reference
- **[Build Configuration](Docs/readme/build/build-configuration.md)** - Build types, C++ standards, optimization options
- **[High-Performance Computing](Docs/readme/high-performance-computing.md)** - SIMD, GPU acceleration, and multithreading
- **[Logging System](Docs/readme/logging.md)** - Flexible logging with three backend options
- **[Third-Party Dependencies](Docs/readme/third-party-dependencies.md)** - Dependency management and integration
- **[Vectorization](Docs/readme/vectorization.md)** - CPU SIMD optimization (SSE, AVX, AVX2, AVX-512)
- **[Sanitizers](Docs/readme/sanitizer.md)** - Memory debugging and analysis tools
- **[Code Coverage](Docs/readme/code-coverage.md)** - Multi-compiler coverage analysis and reporting
- **[Static Analysis](Docs/readme/static-analysis.md)** - IWYU and Cppcheck tools
- **[Compiler Caching](Docs/readme/cache.md)** - Compiler cache types, installation, and configuration
- **[Cross-Platform Building](Docs/readme/cross-platform-building.md)** - Platform-specific build instructions
- **[Linting System](Docs/readme/linter.md)** - Comprehensive linter documentation and configuration
- **[Usage Examples](Docs/readme/usage-examples.md)** - Practical build configuration examples
- **[Profiler](Docs/profiler/README.md)** - Profiler library, backends (native / Kineto / ITT), examples, and Kineto notes

### Additional Documentation

<!-- [CI/CD Pipeline](Docs/ci/CI_CD_PIPELINE.md) - Continuous integration setup (File not found: Docs/ci/ directory does not exist) -->
<!-- [CI Quick Start](Docs/ci/CI_QUICK_START.md) - Getting started with CI (File not found: Docs/ci/ directory does not exist) -->
- **[Valgrind Setup](Docs/readme/valgrind.md)** - Memory debugging with Valgrind

## Running Tests

```bash
# Enable testing during configuration
cd Scripts
python setup.py config.build.test.ninja.clang.debug.gtest
```

## Troubleshooting

### Common Issues

**Missing Third-Party Libraries**

If you see warnings about missing third-party libraries:

1. **Initialize existing submodules** (recommended):
   ```bash
   # Sync and initialize all submodules from the repository
   git submodule sync --recursive
   git submodule update --init --recursive
   ```

   **Note**: Do NOT use `git submodule add` unless you're adding a new dependency. The repository already has submodules configured; this command just initializes them locally.

2. Use external system libraries:
   ```bash
   cd Scripts
   python setup.py config.build.ninja.clang.external
   ```

3. Disable unused features:
   ```bash
   cd Scripts
   python setup.py config.build.ninja.clang.magic_enum
   ```

**Vectorization Issues**

If vectorization fails to compile:

1. Check compiler support
2. Use lower vectorization: add `avx` flag to setup.py command
3. Disable vectorization: omit vectorization flags from setup.py command

**LTO Issues**

If you want to enable Link-Time Optimization:

1. **Enable LTO**: Add `lto` flag to setup.py command (toggles default OFF to ON)
   ```bash
   cd Scripts
   python setup.py config.build.ninja.clang.release.lto  # (LTO enabled)
   ```
2. Check compiler version (GCC 5+, Clang 3.5+, MSVC 2015+)
3. Ensure sufficient available memory (LTO is memory-intensive)

**Build Performance**

For faster builds:

1. Use external libraries: add `external` flag to setup.py command
2. Disable unused features
3. Use parallel compilation: (automatically handled by setup.py)
4. Enable build optimizations (see [Build Optimizations](#build-optimizations) section)

For more detailed troubleshooting, see the specific feature documentation.

## Migration Guide

### From Previous Build System

The new CMake system maintains **complete backward compatibility**:

- ✅ Same CMake options and values
- ✅ Same function names and signatures
- ✅ Same compiler flags and definitions
- ✅ Same target configuration behavior

### Vectorization Configuration

The vectorization system supports CPU SIMD instruction sets:

**CPU Vectorization:**
- SSE, AVX, AVX2, AVX512 instruction sets
- Cross-platform compiler support (MSVC, GCC, Clang)
- Automatic detection and configuration

**Configuration:**
- Set via `QUARISMA_VECTORIZATION_TYPE` option
- Options: `no`, `sse`, `avx`, `avx2`, `avx512`

**No migration required** - all existing configurations continue to work.

## Best Practices

### Development Workflow

**For daily development:**
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.debug
```

**For production releases:**
```bash
cd Scripts
python setup.py config.build.ninja.clang.release.lto  # (LTO enabled for maximum optimization)
```

**For CI/CD pipelines:**
```bash
cd Scripts
python setup.py config.build.test.ninja.clang.external
```

### Performance Optimization

1. **Enable high-performance libraries**: TBB, mimalloc
2. **Use appropriate vectorization**: AVX2 for modern CPUs
3. **Enable LTO**: For maximum optimization in release builds

### Resource-Constrained Environments

1. **Use minimal builds**: Disable optional features
2. **Disable vectorization**: For maximum compatibility
3. **Use external libraries**: Reduce build time and repository size

---

## Coding Standards

Quarisma maintains strict coding standards to ensure code quality, consistency, and maintainability across the entire codebase. All contributions must adhere to these standards.

**Key principles:** No exceptions (use return values), RAII for resource management, smart pointers for ownership, and const correctness. Code must follow `snake_case` naming conventions, use `clang-format` for formatting, achieve 98% test coverage, use STL algorithms instead of raw loops, maintain consistent header/implementation file organization, follow builder pattern conventions, pass all clang-tidy checks, and use `auto` appropriately for clarity.

For a comprehensive guide covering naming conventions, code formatting, include paths, DLL export macros, testing requirements, static analysis, documentation standards, memory management, concurrency, security best practices, STL algorithm usage, file organization, builder patterns, clang-tidy enforcement, and auto keyword guidelines, see:

📖 **[Complete Coding Standards Guide](Docs/readme/coding-standards.md)** - Comprehensive documentation with examples and detailed requirements

---

## Providing Feedback

We value your feedback and encourage you to report bugs, suggest enhancements, and share your experiences with Quarisma.

### Reporting Bugs

If you encounter a bug or unexpected behavior:

1. **Search existing issues** - Check [GitHub Issues](https://github.com/QuarismaAnalyitix/Quarisma/issues) to see if the bug has already been reported
2. **Create a new issue** - If not found, [open a new issue](https://github.com/QuarismaAnalyitix/Quarisma/issues/new) with:
   - **Clear title** - Concise description of the bug
   - **Detailed description** - What you were trying to do and what went wrong
   - **Steps to reproduce** - Exact steps to reproduce the issue
   - **Expected vs. actual behavior** - What you expected vs. what actually happened
   - **Environment details** - OS, compiler, Quarisma version, build configuration
   - **Minimal reproducible example** - Code or commands that demonstrate the issue
   - **Logs or error messages** - Any relevant output or stack traces

3. **Follow the Code of Conduct** - Be respectful and constructive in your report

### Requesting Features and Enhancements

To suggest a new feature or enhancement:

1. **Search existing discussions** - Check [GitHub Discussions](https://github.com/QuarismaAnalyitix/Quarisma/discussions) and [Issues](https://github.com/QuarismaAnalyitix/Quarisma/issues) for similar requests
2. **Create a discussion or issue** - [Start a discussion](https://github.com/QuarismaAnalyitix/Quarisma/discussions/new) or [open an issue](https://github.com/QuarismaAnalyitix/Quarisma/issues/new) with:
   - **Clear title** - Concise description of the feature
   - **Motivation** - Why this feature would be useful
   - **Proposed solution** - How you envision the feature working
   - **Alternative approaches** - Other ways to solve the problem (if applicable)
   - **Use cases** - Real-world scenarios where this feature would help
   - **Additional context** - Any other relevant information

3. **Engage in discussion** - Maintainers and community members may ask clarifying questions

### Security Vulnerabilities

**Do not report security vulnerabilities through public GitHub issues.** Instead:

1. **Use GitHub Security Advisories** - Report privately at https://github.com/QuarismaAnalyitix/Quarisma/security/advisories/new
2. **Email** - Send details to [security@quarisma.co.uk](mailto:security@quarisma.co.uk)

See [SECURITY.md](SECURITY.md) for complete vulnerability reporting guidelines and our response timeline.

### Communication Channels

- **Bug Reports & Feature Requests**: [GitHub Issues](https://github.com/QuarismaAnalyitix/Quarisma/issues)
- **Questions & Discussions**: [GitHub Discussions](https://github.com/QuarismaAnalyitix/Quarisma/discussions)
- **Security Issues**: [GitHub Security Advisories](https://github.com/QuarismaAnalyitix/Quarisma/security/advisories) or [security@quarisma.co.uk](mailto:security@quarisma.co.uk)
- **General Inquiries**: [info@quarisma.co.uk](mailto:info@quarisma.co.uk)
- **Licensing Questions**: [licensing@quarisma.co.uk](mailto:licensing@quarisma.co.uk)
- **Code of Conduct Violations**: [conduct@quarisma.co.uk](mailto:conduct@quarisma.co.uk)

---

## Contributing

We welcome contributions from the community! Quarisma is built by developers like you, and we appreciate your interest in improving the project.

### Quick Start for Contributors

1. **Read the guidelines** - Review [CONTRIBUTING.md](CONTRIBUTING.md) for complete contribution guidelines
2. **Fork the repository** - Create your own fork on GitHub
3. **Create a feature branch** - Branch from `main` with a descriptive name
4. **Make your changes** - Follow our [coding standards](#coding-standards)
5. **Write tests** - Ensure 98% code coverage for new code
6. **Run checks** - Verify formatting, linting, and tests pass locally
7. **Submit a pull request** - Push your branch and create a PR with a clear description

### Contribution Areas

We welcome contributions in many areas:

- **Bug fixes** - Help us fix reported issues
- **Features** - Implement new functionality or enhancements
- **Documentation** - Improve guides, examples, and API documentation
- **Tests** - Add tests for existing code or new features
- **Performance** - Optimize existing code and improve efficiency
- **Build system** - Improve CMake configuration and build process
- **CI/CD** - Enhance testing and automation pipelines

### Key Requirements

- **Code Quality** - Follow [coding standards](#coding-standards) and pass all checks
- **Testing** - Minimum 98% code coverage for new code
- **Documentation** - Update README and docs for new features
- **Licensing** - Contributions are dual-licensed (GPL-3.0-or-later and commercial)
- **Code of Conduct** - Follow our [Code of Conduct](CODE_OF_CONDUCT.md)

### Important Notes

- **Contribution Policy** - Read [CONTRIBUTING.md](CONTRIBUTING.md) to understand our contribution policy and rights
- **Intellectual Property** - Ensure your contribution is original or properly licensed
- **Dual Licensing** - By contributing, you grant QuarismaAnalyitix rights to offer commercial licenses
- **No Guarantee** - Submission does not guarantee acceptance; maintainers have final authority

For complete details, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## License

See [LICENSE](LICENSE) file for details.

---

**Quarisma** - High-performance C++ library with modern CMake build system
