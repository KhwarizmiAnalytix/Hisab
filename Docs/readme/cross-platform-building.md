# Cross-Platform Building

XSigma supports CMake builds on Windows, Linux, and macOS. The most portable
entry point is `Scripts/setup.py`, which defaults to Ninja and Clang when no
toolchain selection is supplied.

## Common commands

```bash
# Default Debug build and tests.
python Scripts/setup.py config.build.test.ninja.clang.debug

# GCC on Linux or another environment with a real GNU toolchain.
python Scripts/setup.py config.build.test.ninja.gcc.release

# Visual Studio 2022 on Windows.
python Scripts/setup.py config.build.test.vs22.debug

# Xcode on macOS.
python Scripts/setup.py config.build.test.xcode.release
```

Run `python Scripts/setup.py --help` for the toolchain selectors supported by
the checked-out helper, including specific Visual Studio versions.

## Direct CMake

### Windows

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022"
cmake --build build-vs --config Release --parallel
ctest --test-dir build-vs -C Release --output-on-failure
```

### Linux and macOS

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Use `CC` and `CXX`, or CMake's `CMAKE_C_COMPILER` and
`CMAKE_CXX_COMPILER`, when selecting a non-default compiler. Configure a new
build directory after changing generator or compiler.

## Architecture and SIMD

Select a SIMD backend explicitly when distributing binaries across machines:

```bash
cmake -S . -B build-avx2 -G Ninja -DVECTORIZATION_CPU_BACKEND=avx2
cmake -S . -B build-neon -G Ninja -DVECTORIZATION_CPU_BACKEND=neon
```

The CMake default is host-aware: recognised x86 hosts default to AVX2,
AArch64 hosts to NEON, and other architectures to `no`. `USE_NATIVE_ARCH=ON`
can improve local performance on Clang/GCC but makes portability less likely.

## GPU backends

```bash
# CUDA
cmake -S . -B build-cuda -G Ninja \
  -DMEMORY_GPU_BACKEND=cuda \
  -DVECTORIZATION_GPU_BACKEND=cuda

# Metal, Apple only
cmake -S . -B build-metal -G Ninja \
  -DMEMORY_GPU_BACKEND=metal \
  -DVECTORIZATION_GPU_BACKEND=metal
```

CUDA, HIP, and Metal are mutually exclusive in one binary. Metal requires an
Apple platform. The project's HIP CMake path is Unix-only; use CUDA on Windows
instead. The setup helper forwards matching Memory and Vectorization selectors
when passed a GPU token.

## LTO and diagnostics

Release and RelWithDebInfo CMake configurations default to each module's
`*_LTO_MODE=auto`; Debug defaults to `off`. To override it directly, use the
owning module's LTO cache variable, for example:

```bash
cmake -S . -B build-core -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCORE_LTO_MODE=thin
```

Do not use `PROJECT_ENABLE_LTO`. Sanitizers and coverage are also module-scoped;
the helper is the preferred way to apply them consistently.

## Bazel

The Bazel build supports the same host platforms at varying feature parity.
Use [the Bazel guide](../BAZEL_USER_GUIDE.md) for its toolchain configurations
and GPU limitations.
