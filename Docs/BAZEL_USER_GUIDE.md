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
8. [Known Gaps and CMake/Bazel Alignment Plan](#known-gaps-and-cmakebazel-alignment-plan)
9. [Build Structure and Architecture](#build-structure-and-architecture)
10. [Advanced Usage](#advanced-usage)
11. [Troubleshooting](#troubleshooting)

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
bazel build --define=build_shared_libs=true //...
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

```bash
cd Scripts
python setup_bazel.py config.build.test.coverage.native
```

`setup_bazel.py` produces a real, working HTML coverage report (verified:
64.9% line / 71.3% function coverage on a real run), but not by trusting
Bazel's own combined report — `bazel coverage`'s own C++ coverage report
merging is broken (see "Known Bazel coverage limitation" below), so
`setup_bazel.py` builds the LCOV trace itself:

1. Widens `--instrumentation_filter` to cover the requested `//Library`
   tree — Bazel's own default (derived from the package of the target(s) on
   the command line) is normally too narrow, which by itself makes a
   single-test-target `bazel coverage` report "no coverage found."
2. Runs `bazel coverage`, which genuinely executes the instrumented tests
   and writes real `.profraw` profile data — this part of Bazel's pipeline
   works correctly.
3. Harvests those `.profraw` files directly from the output base's sandbox
   stash (`<output_base>/sandbox/sandbox_stash/TestRunner/**/`, filtered to
   this run by mtime) rather than trusting Bazel's own report.
4. Resolves each `cc_test` target's compiled binary under `bazel-bin` (via
   `bazel query`) and runs `llvm-profdata merge` + `llvm-cov export -object
   <binary> -format=lcov` directly — bypassing Bazel's broken merger
   entirely — excluding `/external/` (non-vendored deps like googletest)
   and `/ThirdParty/` the same way CMake's coverage tooling does.
5. Rewrites `SF:` source paths from the transient per-action sandbox
   (`.../execroot/_main/<path>`, gone by the time `genhtml` runs) to the
   real, persistent repo path (via `bazel info workspace`).
6. Converts to HTML with `genhtml --ignore-errors inconsistent,corrupt,
   unsupported` — llvm-cov's LCOV export and genhtml's newer strict
   consistency checker (lcov 2.x) don't always agree on function-vs-line
   hit counts for lambdas/closures (e.g. inside googletest internals); this
   is a known translation quirk between the two tools, not real corruption.

Requires `llvm-profdata`/`llvm-cov` and `genhtml` (part of `lcov`) on
`PATH` (`brew install lcov` on macOS, `apt install lcov` on Debian/Ubuntu).
If the coverage run reused cached test results (nothing changed since the
last coverage build), no fresh `.profraw` is produced and `setup_bazel.py`
falls back to Bazel's own (empty) report with a warning — rerun after a
real source change, or `bazel clean` first, to force fresh execution.

**Clang only.** Unlike the CMake path below (which dispatches by compiler:
Clang → llvm-profdata/llvm-cov, GCC → gcov/lcov, MSVC → OpenCppCoverage),
Bazel coverage HTML generation is currently only implemented for Clang.
`--config=gcc` builds at all (a missing `build:gcc` block in `.bazelrc` was
fixed to add `--repo_env=CC=gcc`/`CXX=g++`), but on macOS `/usr/bin/gcc` is
an Apple Clang shim, not real GNU GCC, and forcing the "GCC" compiler
identity hits a pre-existing, unrelated incompatibility in vendored
`ThirdParty/cpuinfo` (ARM microarchitecture enum values not visible under
that branch) that can't be fixed without editing vendored code. `gcc` +
`coverage` together print a clear "not yet supported" warning and fall back
to Bazel's own (empty) report rather than silently producing a wrong one.
MSVC-via-Bazel isn't set up in this repo at all yet.

#### Known Bazel coverage limitation (worked around above, not fixed upstream)

`bazel coverage`'s own combined-report generation is broken: its bundled
`collect_cc_coverage.sh` never populates the `runtime_objects_list.txt`
manifest entry that `llvm-cov export` needs (`-object <test-binary>`), so
its own export step silently reports "No filenames specified!" and produces
a trace with every file at zero hits — this is the upstream "Bazel C++ code
coverage support is poor and limited" gap that script's own header comment
references (tracking issue #1118), not something fixable via Bazel flags.
The instrumentation and profiling runtime are not the problem, only Bazel's
own C++ coverage report-merging plumbing is — which is exactly why
`setup_bazel.py` drives `llvm-profdata`/`llvm-cov` itself instead of relying
on it.

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

Quarisma uses a conditional compilation pattern where each library is controlled by its own per-library `<LIB>_ENABLE_XXX` options in CMake (e.g. `PARALLEL_ENABLE_SANITIZER`, `MEMORY_ENABLE_TBB`, `CORE_ENABLE_MAGICENUM`) — there is no single global `QUARISMA_ENABLE_XXX` prefix. In Bazel, dependencies are managed through the `WORKSPACE.bazel` / `MODULE.bazel` files and controlled via `--config` / `--define` flags using the same lowercase per-library convention (`parallel_enable_sanitizer`, `memory_enable_tbb`, `core_enable_magic_enum`).

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

CMake options are per-library prefixed (`PARALLEL_*`, `MEMORY_*`, `CORE_*`, `VECTORIZATION_*`, `LOGGING_*`, `PROFILER_*`) — there is no single global `QUARISMA_*` option. Bazel's `--define` keys mirror those names lowercased. Verified directly against the current `Library/*/CMakeLists.txt` and `bazel/*.bzl`/`.bazelrc`:

| Feature | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **LTO** | `-D<LIB>_LTO_MODE=thin\|full\|ipo\|auto` (per-library; e.g. `-DPARALLEL_LTO_MODE=thin`) | `--config=lto` |
| **AVX2** | `-DVECTORIZATION_CPU_BACKEND=avx2` | `--config=avx2` |
| **AVX512** | `-DVECTORIZATION_CPU_BACKEND=avx512` | `--config=avx512` |
| **SSE** | `-DVECTORIZATION_CPU_BACKEND=sse` | `--config=sse` |
| **NEON** | `-DVECTORIZATION_CPU_BACKEND=neon` | `--config=neon` |
| **mimalloc** | `-DMEMORY_ENABLE_MIMALLOC=ON` (default ON) | `--config=mimalloc` (default on) |
| **magic_enum** | `-DCORE_ENABLE_MAGICENUM=ON` (default ON) | `--config=magic_enum` |
| **Kineto** | `-DPROFILER_BACKEND=KINETO` (default) | `--config=kineto` |
| **TBB (Parallel backend)** | `-DPARALLEL_BACKEND=tbb` | `--config=tbb` (also sets `memory_enable_tbb=true`) |
| **OpenMP (Parallel backend)** | `-DPARALLEL_BACKEND=openmp` | `--config=openmp` |
| **CUDA** | `-DMEMORY_GPU_BACKEND=cuda` (+ `VECTORIZATION_GPU_BACKEND=cuda`) | `--config=cuda` |
| **HIP** | `-DMEMORY_GPU_BACKEND=hip` (+ `VECTORIZATION_GPU_BACKEND=hip`) | `--config=hip` |
| **Google Test** | `-D<LIB>_ENABLE_GTEST=ON` (default ON; per-library) | `--config=gtest` |
| **Benchmark** | `-D<LIB>_ENABLE_BENCHMARK=ON` (default ON; per-library) | `--config=benchmark` |
| **LU Pivoting** | `-DCORE_LU_PIVOTING=ON` | `--config=lu_pivoting` |
| **Sobol 1111** | `-DCORE_SOBOL_1111=ON` | `--config=sobol_1111` |
| **SLEEF** | `-DVECTORIZATION_ENABLE_SLEEF=ON` | `--config=sleef` (⚠️ see [Known Gaps](#known-gaps-and-cmakebazel-alignment-plan) — not yet hermetic) |

### Logging Backend Mapping

| Backend | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **Loguru** (default) | `-DLOGGING_BACKEND=LOGURU` | `--config=logging_loguru` |
| **glog** | `-DLOGGING_BACKEND=GLOG` | `--config=logging_glog` |
| **spdlog** | `-DLOGGING_BACKEND=SPDLOG` | `--config=logging_spdlog` |
| **Native** | `-DLOGGING_BACKEND=NATIVE` | `--config=logging_native` |

### Profiler Backend Mapping

The native traceme/xplane profiler pipeline (`PROFILER_HAS_NATIVE`) is always compiled, independent
of this choice. `PROFILER_BACKEND` only selects the *instrumentation* backend layered on top of it:

| Backend | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **Kineto** (default) | `-DPROFILER_BACKEND=KINETO` | `--config=kineto` |
| **ITT** | `-DPROFILER_BACKEND=ITT` | `--config=itt` |

### Sanitizer Mapping

CMake's sanitizer options are also per-library (e.g. `PARALLEL_ENABLE_SANITIZER` / `PARALLEL_SANITIZER_TYPE`, `MEMORY_ENABLE_SANITIZER` / `MEMORY_SANITIZER_TYPE`); Bazel applies sanitizer flags globally via `--config`.

| Sanitizer | CMake Option (example: Parallel) | Bazel Equivalent |
|-----------|--------------|------------------|
| **AddressSanitizer** | `-DPARALLEL_ENABLE_SANITIZER=ON -DPARALLEL_SANITIZER_TYPE=address` | `--config=asan` |
| **ThreadSanitizer** | `-DPARALLEL_ENABLE_SANITIZER=ON -DPARALLEL_SANITIZER_TYPE=thread` | `--config=tsan` |
| **UBSanitizer** | `-DPARALLEL_ENABLE_SANITIZER=ON -DPARALLEL_SANITIZER_TYPE=undefined` | `--config=ubsan` |
| **MemorySanitizer** | `-DPARALLEL_ENABLE_SANITIZER=ON -DPARALLEL_SANITIZER_TYPE=memory` | `--config=msan` |
| **LeakSanitizer** | `-DPARALLEL_ENABLE_SANITIZER=ON -DPARALLEL_SANITIZER_TYPE=leak` | `--config=lsan` |

### GPU Backend Mapping

There is no "GPU allocation strategy" (sync/async/pool_async) option in either build system today — that table previously here described flags that don't exist in the codebase and has been removed. The actual GPU control point is the GPU **backend** selector, shared by Memory and Vectorization:

| Backend | CMake Option | Bazel Equivalent |
|---------|--------------|------------------|
| **None** (default) | `-DMEMORY_GPU_BACKEND=none -DVECTORIZATION_GPU_BACKEND=none` | (default; no `--config`) |
| **CUDA** | `-DMEMORY_GPU_BACKEND=cuda -DVECTORIZATION_GPU_BACKEND=cuda` | `--config=cuda` |
| **HIP** | `-DMEMORY_GPU_BACKEND=hip -DVECTORIZATION_GPU_BACKEND=hip` | `--config=hip` |
| **Metal** | `-DMEMORY_GPU_BACKEND=metal -DVECTORIZATION_GPU_BACKEND=metal` | not exposed as a `--config` yet |

### Example Build Scenarios

#### CMake to Bazel Equivalent

**CMake:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
               -DVECTORIZATION_CPU_BACKEND=avx2 \
               -DMEMORY_ENABLE_MIMALLOC=ON \
               -DCORE_ENABLE_MAGICENUM=ON
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
               -DPARALLEL_LTO_MODE=thin \
               -DVECTORIZATION_CPU_BACKEND=avx2
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
               -DPARALLEL_ENABLE_SANITIZER=ON \
               -DPARALLEL_SANITIZER_TYPE=address
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
               -DMEMORY_GPU_BACKEND=cuda \
               -DVECTORIZATION_GPU_BACKEND=cuda \
               -DVECTORIZATION_CPU_BACKEND=avx2
cmake --build build
```

**Bazel:**
```bash
bazel build --config=release --config=cuda --config=avx2 //...
```

### Dependency Vendoring Strategy

CMake and Bazel take different approaches to third-party dependencies, and this is a real (not just cosmetic) source of drift risk:

| | CMake | Bazel |
|---|---|---|
| **Source of truth** | Every dependency is a pinned git submodule under `ThirdParty/`, built from source in-tree via `add_subdirectory`/`FetchContent`. | Mixed: some deps are the *same* vendored `ThirdParty/` submodules wired in via `local_repository`/`new_local_repository` (fmt, glog, loguru, spdlog, kineto, svml, **googletest**); others are fetched independently over the network via `http_archive` (magic_enum, mimalloc, benchmark, tbb) or resolved from the Bazel Central Registry via `bazel_dep` in `MODULE.bazel` (rules_cc, abseil, gflags, re2, and — separately — a `googletest` bzlmod module that, until this session, wasn't even the one actually used by the project's own targets). |
| **Version drift risk** | None — one pinned commit per submodule, shared by every build. | Real. A network-fetched `http_archive` pin can silently diverge from the vendored submodule's pinned commit for the *same* library (this is exactly what happened with googletest: Bazel fetched v1.14.0 over the network while `ThirdParty/googletest` was pinned to a different commit that CMake actually built — see the Known Gaps section below for the fix applied). |
| **Offline builds** | Fully offline-capable (nothing is fetched). | Partially — vendored-submodule deps work offline; `http_archive`/`bazel_dep`-resolved deps require network access (or a pre-warmed repository cache) on first fetch. |

---

## Known Gaps and CMake/Bazel Alignment Plan

The Bazel setup is functionally younger than the CMake one and had accumulated real drift — some caught only by reading the Starlark, some only by actually running `bazel build`/`bazel test` against every backend. This section is a living record: what's been fixed, what's still open, and the plan for closing the rest.

### Fixed — session 1 (Parallel/Vectorization spot fixes)

| # | Bug | Symptom | Fix |
|---|-----|---------|-----|
| 1 | `Library/Parallel/BUILD.bazel` nested a `selects.with_or()` inside another `selects.with_or()`'s default branch (`srcs`/`hdrs` of `parallel_lib`). | `bazel build //Library/Parallel/...` failed at analysis time for **every** backend, not just TBB/OpenMP — `expected value of type 'list(label)' ... but got select(...)`. Bazel does not allow a `select()` as a branch value of another `select()`. | Flattened to one `selects.with_or()` level with three parallel OR-groups (tbb / openmp / default). |
| 2 | `Library/Parallel/tools/parallel_tools.h` defined the OpenMP threadprivate global without `static` — an ODR violation waiting to happen. | `bazel test --config=openmp //Library/Parallel/...` failed the *link* step with a real `duplicate symbol` error. | Added `static`. (A separate, lower-probability cross-TU scoping concern in the same mechanism was flagged during code review — see "Still open" below.) |
| 3 | Root `BUILD.bazel` hardcoded `cc_import` targets pointing at `build_ninja/ThirdParty/...` — CMake's output directory, not anything Bazel produces. | `bazel build --config=sleef //Library/Vectorization/...` failed with a confusing `missing input file` error unless a prior CMake build happened to exist. | Replaced with `bazel/sleef_configure.bzl` (fail-fast repository rule). **Still not hermetic** — see "Still open". |
| 4 | `WORKSPACE.bazel` fetched googletest via `http_archive` while `MODULE.bazel` separately declared a *different* pinned version via `bazel_dep` — neither matched the `ThirdParty/googletest` submodule commit CMake actually builds. | Silent correctness/reproducibility gap, not a build failure. | `com_google_googletest` now points at the vendored submodule via `local_repository`. |
| 5 | `.bazelrc` comment mislabeled `--experimental_repository_cache_hardlinks` as a fetch-timeout flag. | Cosmetic. | Corrected the comment. |

### Fixed — session 2 (full per-library CMake/Bazel audit + fixes)

A complete audit compared every `Library/*/CMakeLists.txt` against its `BUILD.bazel`/`bazel/*.bzl` counterpart for all 6 libraries (Core, Logging, Memory, Parallel, Profiler, Vectorization). ~25 mismatches were found and fixed, each verified with a real `bazel build`/`bazel test` run on macOS arm64 (not just static reading):

**Source-level bug (not a Bazel/CMake gap):** `Library/Memory/allocator.h` called `gpu::caching_allocator_for_device(...)` unguarded — the `gpu` namespace only exists when a GPU backend is compiled in, so any build with GPU backend `none` (the default in **both** build systems) failed to compile. Confirmed independently with a standalone `clang++ -fsyntax-only` repro. Fixed by guarding the call sites with `#if MEMORY_HAS_CUDA || MEMORY_HAS_HIP || MEMORY_HAS_METAL`.

**Bazel-only fixes, one per affected define/glob/dep:**
- Core: `CORE_SSE/AVX/AVX2/AVX512` and `CORE_HAS_EXCEPTION_PTR` were never defined by Bazel at all (reused the existing `vectorization_type_*` config_settings; `EXCEPTION_PTR` fixed to `1`, a documented simplification of CMake's compiler probe).
- Logging: `logger.cpp` was dropped from the NATIVE backend build (CMake compiles it for every backend); `LOGGING_BUILDING_DLL` was a public `defines` entry instead of `local_defines` (Windows dllexport/dllimport bug); `-ldbghelp`/`dbghelp.lib` added on Windows; test target's hardcoded `@loguru` dep replaced with the same backend-`select()` the library uses.
- Parallel: `PARALLEL_BUILDING_DLL` moved to `local_defines`; redundant `-lpthread` removed; per-backend test-file excludes added; the single hardcoded benchmark target replaced with a glob-driven per-file loop.
- Profiler: ITT backend was missing `bespoke/kineto/kineto_shim.{h,cpp}` (needed unconditionally by `bespoke/common/collection.*`); stray `PROFILER_HAS_CUDA/HIP` test-only defines removed (CMake never defines them anywhere); dead `includes=["kineto"]` path removed.
- Vectorization: `benchmark_simdunary` pointed at a nonexistent file (fixed to 4 correctly-named per-file targets, matching CMake's `benchmark_<suffix>` naming); the `vectorization_type=no` hdrs glob excluded all of `backend/**` instead of just the ISA-specific subdirs, dropping the shared `backend/simd.h` dispatcher.
- Shared: `quarisma_copts()` was missing `/WX` (MSVC) and `-include cstdlib` (non-MSVC) that every library's CMakeLists.txt applies (Memory opts out of the latter, matching its own Clang-specific CMake exception).

**New toolchain wiring (structural gaps — CMake features with previously no Bazel path at all), all verified with real builds/tests on this machine:**
- **OpenMP** (`bazel/openmp_configure.bzl`, new): host compiler probe (plain `-fopenmp`, or Homebrew `libomp` on macOS), wired into `parallel_copts()`/`parallel_linkopts()`. Verified: `bazel test --define parallel_backend=openmp //Library/Parallel/...` links and runs against a real `libomp.dylib`.
- **ITT** (`ThirdParty/ittapi.BUILD`, new): the vendored `ThirdParty/ittapi` submodule had no Bazel build file; added one and wired `@ittapi//:ittnotify` into Profiler's `enable_itt` deps. Verified: `TestITTWrapper` (17 tests) passes.
- **Metal — Memory** (`bazel/memory.bzl`, `Library/Memory/BUILD.bazel`): `.mm` sources compiled via the genrule-rename-to-`.cc` + `-x objective-c++` workaround (`cc_library` has no native `.mm` support without `rules_apple`), isolated into a small dedicated `memory_metal_objcxx` library so the ObjC++ compile mode doesn't leak onto plain `.cpp` sources in the same target. `-framework Metal -framework Foundation` linked. Verified: 88 Memory tests pass, including real `MetalBufferAllocator`/`MetalCachingAllocator` tests against actual Metal hardware.
- **Metal — Vectorization** (own separate `VECTORIZATION_GPU_BACKEND`, independent of Memory's `MEMORY_GPU_BACKEND`): same `.mm`-rename technique for `backend/gpu/metal/metal_dispatch.mm` (isolated in `vectorization_metal_objcxx`), plus a `genrule` that embeds `kernels.metal` into `metal_kernels_source.h` the same way CMake's `configure_file()` does (byte-for-byte `@METAL_KERNEL_SOURCE@` substitution). **Caught a real bug in the process**: the test-only Objective-C++ shim `Testing/Cxx/metal_device_probe.mm` (providing `xsigma_metal_device_count()` for `TestTensorGpu.cpp`) was never wired into the Bazel test target at all — because this project's macOS linkopts use `-undefined dynamic_lookup`, the missing symbol didn't fail at link time, it resolved to a null stub that **crashed with SIGSEGV at runtime** the first time `TensorGpu.FillFloat` called it. Fixed by wiring the shim in (isolated in `vectorization_test_metal_objcxx`). Verified: `TensorGpu.{FillFloat,BinaryOpsFloat,UnaryMathFloat,CompoundFloat}` genuinely pass against real Metal hardware; the `*Double` variants correctly self-skip (Metal has no fp64).
- **Accelerate** (`bazel/vectorization.bzl`): `VECTORIZATION_HAS_ACCELERATE` (not `_ENABLE_`; CMake's `compile_definition()` helper does an ENABLE→HAS name substitution) plus `-framework Accelerate` on macOS. Verified: framework genuinely linked (`otool -L`), 42 tests pass.
- **Packet size** (`bazel/BUILD.bazel`, `bazel/vectorization.bzl`): a `bazel_skylib` `string_flag` (`--//bazel:vectorization_packet_size=N`, 1-16, default 1) replacing the previously-nonexistent Bazel equivalent of `VECTORIZATION_PACKET_SIZE`. Verified via `bazel aquery` that the define lands with the requested value, and a full test run at `packet_size=4`.
- **Vectorization's own CUDA/HIP backend** (separate from Memory's): the library target itself needs no device-language compilation (confirmed by checking CMake's own `Testing/Cxx/CMakeLists.txt` — device-tagging via `set_source_files_properties(... LANGUAGE CUDA/HIP)` is confined entirely to test/benchmark files, never the main library), so `VECTORIZATION_HAS_CUDA/HIP/METAL` defines plus linking against the same `@local_config_cuda`/`@local_config_hip` repos Memory uses was enough. `TestTensorGpu.cpp` (which *does* need nvcc/hipcc to instantiate GPU expression-template kernels) is excluded from the Bazel test glob under CUDA/HIP, matching CMake's own conditional inclusion.
- **HIP runtime linking — Memory** (`bazel/hip_configure.bzl`, new, modeled on the pre-existing `cuda_configure.bzl`): `MEMORY_HAS_HIP`/`VECTORIZATION_HAS_HIP` were being defined but nothing was ever linked. **Not verifiable here** (no ROCm toolkit on this machine) beyond confirming the fail-fast path gives a clear, actionable error instead of a confusing one.
- **NUMA** (`Library/Memory/BUILD.bazel`): `-lnuma` linkopt added under `enable_numa`, matching CMake's plain `find_package(Numa)`. **Not verifiable here** (Linux-only; confirmed the expected failure mode is `numa.h: file not found`, not a Bazel/Starlark error).
- **MKL — Vectorization** (`bazel/vectorization.bzl`): `VECTORIZATION_HAS_MKL` defined correctly (gated on x86_64, matching CMake's own processor guard — confirmed staying `0` on this arm64 host even with the flag explicitly passed). No `@mkl` external repo wired (matches Memory's own pre-existing, already-commented-out `@mkl` dependency — there's no real MKL install anywhere to verify a hermetic Bazel dep against).

**A previously-undiscovered, unrelated Bazel bug found during verification (not in the original per-library audit):** `bazel/vectorization.bzl`'s SIMD-tier `select()` had `//bazel:cpu_aarch64` as a bare peer key alongside the six explicit `vectorization_type_*` keys. On any aarch64 host (this machine), passing an explicit `--define vectorization_type=X` for any tier except `neon` (which only avoided the bug by the coincidence of producing an identical output) hit an "Illegal ambiguous match" analysis error — i.e., `--config=sse/avx/avx2/avx512/sve` and `--define vectorization_type=no` were all broken on ARM. Root cause: Bazel's `select()` ambiguity rule only auto-resolves multiple matches when one is a strict superset ("more specialized") of the others, or all matches are identical values; a bare `constraint_values`-only setting and a bare `define_values`-only setting are on different axes and never satisfy either condition. Fixed by adding `vectorization_type_X_aarch64` combined config_settings (`constraint_values` + `define_values` together) that *are* supersets of `cpu_aarch64`, and pairing each bare tier key with its `_aarch64` counterpart via `selects.with_or()`. Verified: all 7 tiers plus the unset default now build cleanly on this arm64 host.

**Code-review findings addressed:** an independent multi-angle review of the full diff (4 parallel passes: reuse/simplification, altitude/conventions, removed-behavior tracing, line-by-line diff scan) flagged the `-x objective-c++ -fobjc-arc` copts being applied target-wide (not per-file, unlike CMake's `set_source_files_properties`) as the most severe finding — a real behavior divergence, not just style, since any future plain `.cpp` file using `id`/`BOOL`/`Class`/`SEL` as an ordinary identifier would silently fail to compile only under Bazel. Fixed for both Memory and Vectorization by isolating the renamed `.mm`→`.cc` files into small dedicated `cc_library` targets with the ObjC++ copts scoped to just them. Two other review findings (a claimed ambiguity in `vectorization_svml_deps()`/`vectorization_svml_hdrs_extra()`, and general reuse/duplication across the new `*_configure.bzl` files) were checked and are respectively a false positive (Bazel's same-value escape hatch already covers it — verified empirically) and a real but low-severity style item (not fixed this session; noted below).

### Still open

1. **`--config=sleef` is not hermetic.** SLEEF's own CMake build does architecture-specific codegen; reimplementing that natively in Starlark is a substantial undertaking. The recommended path is `rules_foreign_cc`'s `cmake()` rule, pointed at the vendored `ThirdParty/sleef` submodule with the same cache flags CMake's `Library/Vectorization/CMakeLists.txt` already passes.
2. **CUDA/HIP device-language (`.cu`/`.hip`) compilation has no Bazel support anywhere in this project.** This project's only existing CUDA precedent (`cuda_configure.bzl`) is host-side runtime linking (`cudart`/`cupti` prebuilt libs); nothing invokes `nvcc`/`hipcc`. This blocks, and is deliberately left unaddressed for: `Library/Core/Testing/Cxx/CudaEnzymeADTest.cu` (Core's Enzyme+CUDA test), `Library/Vectorization/Testing/Cxx/TestTensorGpu.cpp` and `BenchmarkTensorGpu.cpp` under `--define vectorization_enable_cuda/hip=true` (both excluded from Bazel's glob under those configs), and `Testing/Cxx/Test*.cu` files generally. Closing this needs `rules_cuda` (or a bespoke nvcc/hipcc invocation rule) — a `rules_foreign_cc`-scale addition, and *completely* unverifiable in this development environment (no CUDA/ROCm toolkit here at all, not even the fail-fast path, since toolchain resolution itself needs the toolkit present).
3. **Memory's `memkind` support is a pre-existing CMake-side gap, not Bazel drift.** `MEMORY_ENABLE_MEMKIND` defines a macro but no `find_package`/`target_link_libraries` call for memkind exists anywhere in CMake either (unlike `numa.cmake`/`hip.cmake`). Nothing to align Bazel to; worth its own ticket on the CMake side.
4. **Logging's Clang-on-Windows `-O1` SPDLOG ICE workaround** (`CMakeLists.txt:306-316`) has no Bazel equivalent — no Windows machine available to verify, no existing compiler-id `config_setting` to key off, and the failure mode if hit is a loud compiler crash at build time, not a silent correctness bug.
5. **A latent ODR-adjacent scoping issue in `Library/Parallel/tools/parallel_tools.h`'s OpenMP threadprivate mechanism**, flagged during code review: the "already initialized" flag is a single namespace-scope variable shared by *every* `Functor` type dispatched from the same TU, unlike the TBB backend (a member of a per-functor struct) or the Native backend (a `thread_local` local, one copy per template instantiation). A shared, non-test-local functor type driven through OpenMP `parallel_for` from two different `.cpp` files could see toolchain-dependent COMDAT-selection behavior. Pre-existing (not introduced by the Bazel-alignment work), low-probability, not fixed this session.
6. **Minor duplication across the new `bazel/*_configure.bzl` repository rules** (`hip_configure.bzl`, `openmp_configure.bzl`), flagged during code review: `_is_windows()` and the "fail with an actionable message" `genrule`+`cc_library` template are now each copy-pasted 3-4 times across `svml_configure.bzl`/`cuda_configure.bzl`/`sleef_configure.bzl`/`hip_configure.bzl`/`openmp_configure.bzl`. A shared `bazel/repo_rule_utils.bzl` would remove the duplication; not done this session (style/maintenance, not correctness).
7. **This document had drifted significantly** before the first pass (stale `QUARISMA_*`-prefixed CMake option names, a fictional "GPU Allocation Strategy" table, a wrong `.bazelversion` reference, a `File Structure` tree describing a nonexistent `third_party/` directory) — the comparison tables and file tree were corrected then, but sections outside the comparison (Getting Started, Sanitizers, Coverage, etc.) were never audited and may have similar drift.
8. **The bzlmod migration is incomplete beyond googletest.** `MODULE.bazel` declares `bazel_dep`s for a handful of libraries alongside `WORKSPACE.bazel`'s legacy declarations for everything vendored under `ThirdParty/`. This hybrid (`build --enable_workspace`) is Bazel's documented transitional pattern, not a bug, but Bazel 9 removes WORKSPACE support entirely, so it's worth a deliberate decision on how far the migration should go.

### Alignment plan

**Phase 1 (done) + Phase 1.5 (done, this session):** the full per-library audit and fix pass described above — every CMake↔Bazel mismatch found across Core/Logging/Memory/Parallel/Profiler/Vectorization, all 7 SIMD tiers on ARM, and 6 new toolchain integrations (OpenMP, ITT, Metal ×2, Accelerate, packet size), each verified with a real build/test run on this machine where possible.

**Phase 2 — Hermetic SLEEF (scoped, not started):** introduce `rules_foreign_cc`, wrap `ThirdParty/sleef`'s CMakeLists.txt with a `cmake()` rule mirroring the CMake cache flags already in `Library/Vectorization/CMakeLists.txt`, remove `bazel/sleef_configure.bzl`'s CMake-build-directory probing once the hermetic path is verified on Linux/macOS/Windows.

**Phase 2.5 — CUDA/HIP device-language compilation (scoped, not started):** the single largest remaining gap. Needs `rules_cuda` or equivalent, a real CUDA/ROCm toolkit to develop and verify against (unavailable in this environment), and covers `CudaEnzymeADTest.cu`, `TestTensorGpu.cpp`/`BenchmarkTensorGpu.cpp` under CUDA/HIP, and `Test*.cu` generally.

**Phase 3 — Full documentation audit:** extend the correction pass done in Phase 1 (comparison tables, file structure) to the rest of this guide (Getting Started, Sanitizers, Coverage, Third-Party Dependencies sections) and to `Docs/BAZEL_FILES_SUMMARY.txt`, which has never been reviewed.

**Phase 4 — Decide the bzlmod migration's end state:** either complete it (migrate every `ThirdParty/`-vendored dependency + the remaining `http_archive` fetches to `bazel_dep`/`local_path_override` and drop `WORKSPACE.bazel` + `--enable_workspace` once Bazel 9 removes WORKSPACE support entirely) or explicitly document the hybrid as the intended long-term state.

**Ongoing — CI parity:** none of the bugs fixed across either session were CI-gated; all were found by manually invoking `bazel build`/`bazel test` per backend/config. Adding a CI matrix that runs `bazel test //...` across `--config=tbb`/`--config=openmp`/`--define vectorization_type=`{every tier}/default (mirroring whatever CMake matrix already exists) would have caught most of these classes of regression automatically.

---

## Build Structure and Architecture

Understanding the Bazel build structure helps you navigate and modify the build configuration.

### File Structure

```
Quarisma/
├── WORKSPACE.bazel              # Legacy-WORKSPACE deps (vendored ThirdParty/ + a few http_archive)
├── MODULE.bazel                 # Bzlmod deps (rules_cc, googletest, abseil, gflags, re2, ...)
├── MODULE.bazel.lock            # Bzlmod resolution lockfile
├── BUILD.bazel                  # Root build file
├── .bazelrc                     # Build configuration flags
├── .bazelignore                 # Paths excluded from Bazel's package scan
├── .bazelversion                # Pins the Bazel version (8.4.2)
│
├── bazel/                       # Bazel helper files
│   ├── BUILD.bazel              # config_setting definitions (mirror CMake --define keys)
│   ├── quarisma.bzl             # Shared helper functions (copts, defines, linkopts)
│   ├── core.bzl / memory.bzl / parallel.bzl / logging.bzl / profiler.bzl / vectorization.bzl
│   │                             # Per-library copts/defines, mirroring each Library/*/CMakeLists.txt
│   ├── vectorization_settings.bzl  # SIMD-tier × OS config_setting generation
│   ├── cuda_configure.bzl       # Repository rule: resolves CUDA install path (mirrors FindCUDA)
│   ├── svml_configure.bzl       # Repository rule: probes native SVML support per SIMD tier
│   └── sleef_configure.bzl      # Repository rule: locates CMake-built SLEEF artifacts (see Known Gaps)
│
├── ThirdParty/                  # Vendored git submodules — build files for those Bazel uses
│   ├── fmt.BUILD, glog is used directly (ships its own BUILD.bazel), loguru.BUILD, spdlog.BUILD,
│   │   kineto.BUILD, svml.BUILD, tbb.BUILD, mimalloc.BUILD, magic_enum.BUILD  # hand-written BUILD files
│   └── googletest/BUILD.bazel   # ships its own official Bazel build (used via local_repository)
│
└── Library/
    ├── Core/BUILD.bazel, Logging/BUILD.bazel, Memory/BUILD.bazel,
    │   Parallel/BUILD.bazel, Profiler/BUILD.bazel, Vectorization/BUILD.bazel
    └── */Testing/**/BUILD.bazel  # Test + benchmark targets per library
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
build:mimalloc --define=memory_enable_mimalloc=true
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
    define_values = {"memory_enable_mimalloc": "true"},
)
```

Used in build with:
```bash
bazel build --config=mimalloc //...
# or
bazel build --define=memory_enable_mimalloc=true //...
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
