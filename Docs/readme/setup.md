# Quarisma Setup Guide

## Overview

The `Scripts/setup.py` script provides a convenient interface for configuring and building Quarisma with various options. This guide explains how to use the script and documents all available CMake flags.

## Basic Usage

### Syntax

```bash
cd Scripts
python setup.py config.build.test.[builder].[compiler].[options]
```

### Components

- **config**: Configure the build
- **build**: Build the project
- **test**: Run tests
- **builder**: `ninja`, `vs22`, `xcode`, `cmake`
- **compiler**: `clang`, `gcc`, `msvc` (compiler-specific)
- **options**: Various feature flags (see below)

### Examples

```bash
# Debug build with Clang
python setup.py config.build.ninja.clang.debug

# Release build with optimizations (LTO enabled by default)
python setup.py config.build.ninja.clang.release.avx2

# With testing enabled
python setup.py config.build.test.ninja.clang.debug

# MSVC with coverage
python setup.py config.build.vs22.debug.coverage
```
## Build Directory Naming Convention

The `setup.py` script automatically generates build directory names based on the flags you provide. This allows you to maintain multiple build configurations simultaneously without conflicts.

**Base Naming Pattern:**
- `build_[builder]` - Base directory name determined by the build system
  - `build_ninja` - Ninja build system
  - `build_vs22` - Visual Studio 2022
  - `build_xcode` - Xcode (macOS)
  - `build_eclipse` - Eclipse IDE

**With Feature Suffixes:**
- `build_[builder]_[suffix]` - Additional suffixes are appended for special configurations
  - `build_ninja_coverage` - Ninja with code coverage enabled
  - `build_ninja_address` - Ninja with AddressSanitizer
  - `build_ninja_avx2` - Ninja with AVX2 vectorization
  - `build_ninja_lto` - Ninja with Link-Time Optimization
  - `build_vs22_debug_coverage` - Visual Studio with debug and coverage

**Multiple Suffixes:**
When multiple feature flags are used, they are concatenated:
- `build_ninja_debug_coverage_avx2` - Ninja with debug, coverage, and AVX2
- `build_vs22_release_lto` - Visual Studio with release and LTO

**Examples of Build Directory Names:**

| Command | Build Directory | Purpose |
|---------|-----------------|---------|
| `config.build.ninja.clang.debug` | `build_ninja` | Standard debug build |
| `config.build.ninja.clang.release` | `build_ninja` | Standard release build |
| `config.build.test.ninja.clang.debug.coverage` | `build_ninja_coverage` | Debug with code coverage |
| `config.build.test.ninja.clang.debug --sanitizer.address` | `build_ninja_address` | Debug with AddressSanitizer |
| `config.build.ninja.clang.release.avx2` | `build_ninja_avx2` | Release with AVX2 vectorization |
| `config.build.vs22.debug` | `build_vs22` | MSVC debug build |
| `config.build.vs22.debug.coverage` | `build_vs22_coverage` | MSVC with code coverage |
| `config.build.test.xcode.release.lto` | `build_xcode_lto` | Xcode release with LTO |

**Benefits of This Approach:**
- ✅ **Multiple Configurations**: Keep debug, release, coverage, and sanitizer builds simultaneously
- ✅ **Easy Cleanup**: Remove specific build configurations by deleting their directories
- ✅ **Clear Organization**: Directory names clearly indicate what configuration they contain
- ✅ **No Conflicts**: Different configurations don't interfere with each other
- ✅ **Reproducibility**: Same command always produces the same directory name

## CMake Flags Reference

### Build Type Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `debug` | `CMAKE_BUILD_TYPE` | Debug build with symbols | - |
| `release` | `CMAKE_BUILD_TYPE` | Release build with optimizations | - |
| `relwithdebinfo` | `CMAKE_BUILD_TYPE` | Release with debug symbols | - |
| `minsizerel` | `CMAKE_BUILD_TYPE` | Minimal size release | - |

### Compiler Flags

| Flag | CMake Variable | Description |
|------|----------------|-------------|
| `clang` | `CMAKE_CXX_COMPILER` | Use Clang compiler |
| `gcc` | `CMAKE_CXX_COMPILER` | Use GCC compiler |
| `msvc` | `CMAKE_CXX_COMPILER` | Use MSVC compiler |

### Optimization Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `lto` | `QUARISMA_ENABLE_LTO` | Link-Time Optimization | ON |
| `avx2` | `QUARISMA_VECTORIZATION_TYPE` | AVX2 vectorization | - |
| `avx` | `QUARISMA_VECTORIZATION_TYPE` | AVX vectorization | - |
| `avx512` | `QUARISMA_VECTORIZATION_TYPE` | AVX-512 vectorization | - |
| `sse` | `QUARISMA_VECTORIZATION_TYPE` | SSE vectorization | - |

### Feature Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `cuda` | `MEMOY_ENABLE_CUDA` | GPU acceleration with CUDA | OFF |
| `tbb` | `QUARISMA_ENABLE_TBB` | Intel Threading Building Blocks | OFF |
| `mkl` | `QUARISMA_ENABLE_MKL` | Intel Math Kernel Library | OFF |
| `numa` | `QUARISMA_ENABLE_NUMA` | NUMA support | OFF |
| `memkind` | `QUARISMA_ENABLE_MEMKIND` | Memory kind support | OFF |
| `external` | `QUARISMA_ENABLE_EXTERNAL` | Use external system libraries | ON |

### Testing Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `test` | `BUILD_TESTING` | Enable testing | ON |
| `gtest` | `QUARISMA_ENABLE_GTEST` | Google Test framework | ON |
| `benchmark` | `QUARISMA_ENABLE_BENCHMARK` | Benchmark library | OFF |

### Analysis Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `coverage` | `QUARISMA_ENABLE_COVERAGE` | Code coverage analysis | OFF |
| `sanitizer` | `QUARISMA_ENABLE_SANITIZER` | Enable sanitizers | OFF |
| `sanitizer.address` | `QUARISMA_SANITIZER_TYPE` | Address Sanitizer | - |
| `sanitizer.thread` | `QUARISMA_SANITIZER_TYPE` | Thread Sanitizer | - |
| `sanitizer.undefined` | `QUARISMA_SANITIZER_TYPE` | Undefined Behavior Sanitizer | - |
| `sanitizer.memory` | `QUARISMA_SANITIZER_TYPE` | Memory Sanitizer | - |
| `valgrind` | `QUARISMA_ENABLE_VALGRIND` | Valgrind support | OFF |
| `clangtidy` | `QUARISMA_ENABLE_CLANGTIDY` | Clang-Tidy analysis | OFF |
| `iwyu` | `QUARISMA_ENABLE_IWYU` | Include-What-You-Use | OFF |
| `cppcheck` | `QUARISMA_ENABLE_CPPCHECK` | Cppcheck analysis | OFF |

### Logging Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `loguru` | `LOGGING_ENABLE_LOGURU` | Loguru logging backend | ON |
| `logging_backend=glog` | `LOGGING_BACKEND` | Use GLOG backend | - |
| `logging_backend=native` | `LOGGING_BACKEND` | Use native backend | - |

### Library Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `magic_enum` | `QUARISMA_ENABLE_MAGICENUM` | Magic enum library | ON |
| `mimalloc` | `MEMORY_ENABLE_MIMALLOC` | mimalloc allocator | ON |

### Caching Flags

| Flag | CMake Variable | Description | Default |
|------|----------------|-------------|---------|
| `ccache` | `QUARISMA_CACHE_BACKEND` | Use ccache compiler cache | - |
| `sccache` | `QUARISMA_CACHE_BACKEND` | Use sccache distributed cache | - |
| `buildcache` | `QUARISMA_CACHE_BACKEND` | Use buildcache | - |
| `none` | `QUARISMA_CACHE_BACKEND` | Disable caching | - |

### C++ Standard Flags

| Flag | CMake Variable | Description |
|------|----------------|-------------|
| `cxxstd=17` | `QUARISMA_CXX_STANDARD` | C++17 standard |
| `cxxstd=20` | `QUARISMA_CXX_STANDARD` | C++20 standard |
| `cxxstd=23` | `QUARISMA_CXX_STANDARD` | C++23 standard |

## Common Build Configurations

### Development Build

```bash
python setup.py config.build.test.ninja.clang.debug
```

### Release Build

```bash
python setup.py config.build.ninja.clang.release.avx2
```

### With Coverage

```bash
python setup.py config.build.ninja.clang.debug.coverage
```

### With Sanitizers

```bash
python setup.py config.build.test.ninja.clang.debug --sanitizer.address
```

### Minimal Build

```bash
python setup.py config.build.ninja.clang.release
```

## Build Directory Naming

The build directory is automatically named based on the configuration:

- `build_ninja` - Ninja builder
- `build_ninja_coverage` - Ninja with coverage
- `build_vs22` - Visual Studio 2022
- `build_xcode` - Xcode

Suffixes are added for special configurations (e.g., `_coverage`, `_sanitizer`).

## Troubleshooting

### Build fails with missing dependencies

Ensure all submodules are initialized:
```bash
git submodule update --init --recursive
```

### Compiler not found

Verify the compiler is installed and in PATH:
```bash
which clang++  # Linux/macOS
where clang++  # Windows
```

### Cache not working

Verify cache tool is installed:
```bash
which ccache
which sccache
```

## Advanced Configuration

For more control, use CMake directly:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DQUARISMA_ENABLE_LTO=ON \
      -DQUARISMA_VECTORIZATION_TYPE=avx2 \
      ..
cmake --build . --parallel
```
