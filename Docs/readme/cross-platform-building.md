# Cross-Platform Building

XSigma provides full cross-platform compatibility across Windows, Linux, and macOS. This guide covers platform-specific build instructions and considerations.

## Table of Contents

- [Platform Overview](#platform-overview)
- [Windows (Visual Studio)](#windows-visual-studio)
- [Linux (GCC/Clang)](#linux-gccclang)
- [macOS (Clang)](#macos-clang)
- [Platform-Specific Considerations](#platform-specific-considerations)
- [Cross-Compilation](#cross-compilation)
- [Best Practices](#best-practices)

## Platform Overview

XSigma supports the following platforms:

| Platform | Compilers | Architectures | Status |
|----------|-----------|---------------|--------|
| **Windows** | MSVC 2019+, Clang | x64, ARM64 | ✅ Fully Supported |
| **Linux** | GCC 9+, Clang 10+ | x64, ARM64, ARM | ✅ Fully Supported |
| **macOS** | Apple Clang, Clang | x64, ARM64 (Apple Silicon) | ✅ Fully Supported |

## Windows (Visual Studio)

### Requirements

- Visual Studio 2019 16.9+ or Visual Studio 2022
- CMake 3.16 or later
- Windows 10 or later

### Basic Build

```bash
# Configure with Visual Studio generator
cmake -B build -S . -G "Visual Studio 17 2022"

# Build Release configuration
cmake --build build --config Release

# Build Debug configuration
cmake --build build --config Debug
```

### Optimized Release Build

```bash
# Release build with optimizations
cmake -B build -S . \
    -G "Visual Studio 17 2022" \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON \
    -DVECTORIZATION_CPU_BACKEND=avx2

cmake --build build --config Release
```

### Using Ninja Generator (Faster Builds)

```bash
# Configure with Ninja generator
cmake -B build -S . \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON

# Build with parallel compilation
cmake --build build -j
```

### Platform-Specific Options

```bash
# Specify architecture
cmake -B build -S . -A x64  # 64-bit
cmake -B build -S . -A ARM64  # ARM64

# Specify toolset
cmake -B build -S . -T v142  # VS 2019
cmake -B build -S . -T v143  # VS 2022
cmake -B build -S . -T ClangCL  # Clang for Windows
```

### Windows-Specific Notes

- **Runtime Library**: XSigma uses `/MD` (Multi-threaded DLL) by default
- **Debug Information**: `/Zi` is enabled for better debugging
- **Optimization**: `/O2` for Release, `/Od` for Debug
- **Warnings**: `/W4` warning level enabled
- **Parallel Compilation**: `/MP` flag automatically enabled

## Linux (GCC/Clang)

### Requirements

- GCC 9+ or Clang 10+
- CMake 3.16 or later
- Make or Ninja build system

### Basic Build

```bash
# Configure with default generator (Make)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build build -j$(nproc)
```

### Using GCC

```bash
# Explicitly use GCC
export CC=gcc
export CXX=g++

cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON \
    -DVECTORIZATION_CPU_BACKEND=avx2

cmake --build build -j$(nproc)
```

### Using Clang

```bash
# Explicitly use Clang
export CC=clang
export CXX=clang++

cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON \
    -DVECTORIZATION_CPU_BACKEND=avx2

cmake --build build -j$(nproc)
```

### Using Ninja (Faster Builds)

```bash
# Install Ninja
sudo apt-get install ninja-build  # Ubuntu/Debian
sudo dnf install ninja-build      # Fedora/RHEL

# Configure with Ninja
cmake -B build -S . \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON

cmake --build build -j$(nproc)
```

### Linux-Specific Options

```bash
# Use gold linker (faster linking)
cmake -B build -S . -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=gold"

# Use lld linker (even faster)
cmake -B build -S . -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld"

# Native optimization
cmake -B build -S . -DCMAKE_CXX_FLAGS="-march=native"
```

### Linux-Specific Notes

- **Position Independent Code**: `-fPIC` enabled for shared libraries
- **Optimization**: `-O3` for Release, `-O0` for Debug
- **Debug Information**: `-g` enabled for debugging
- **Warnings**: `-Wall -Wextra` enabled
- **Link-Time Optimization**: Supported with GCC 9+ and Clang 10+

## macOS (Clang)

### Requirements

- Xcode 12+ or Command Line Tools
- CMake 3.16 or later
- macOS 10.15 (Catalina) or later

### Basic Build

```bash
# Configure with default generator (Make)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Optimized Build

```bash
# Release build with optimizations
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON

cmake --build build -j$(sysctl -n hw.ncpu)
```

### Apple Silicon (M1/M2) Specific

```bash
# Build for Apple Silicon (ARM64)
cmake -B build -S . \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_BUILD_TYPE=Release

# Build for Intel (x86_64)
cmake -B build -S . \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DCMAKE_BUILD_TYPE=Release

# Universal Binary (both architectures)
cmake -B build -S . \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_BUILD_TYPE=Release
```

### Using Ninja (Faster Builds)

```bash
# Install Ninja
brew install ninja

# Configure with Ninja
cmake -B build -S . \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON

cmake --build build -j$(sysctl -n hw.ncpu)
```

### macOS-Specific Options

```bash
# Specify minimum macOS version
cmake -B build -S . -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

# Use specific SDK
cmake -B build -S . -DCMAKE_OSX_SYSROOT=/path/to/SDK
```

### macOS-Specific Notes

- **Apple Silicon**: ARM64 architecture uses NEON instead of x86 SIMD
- **Universal Binaries**: Can build for both Intel and Apple Silicon
- **Optimization**: `-O3` for Release, `-O0` for Debug
- **Debug Information**: `-g` enabled for debugging
- **Warnings**: `-Wall -Wextra` enabled
- **Link-Time Optimization**: Fully supported

## Platform-Specific Considerations

### Compiler Differences

| Feature | MSVC | GCC | Clang |
|---------|------|-----|-------|
| **C++17** | ✅ | ✅ | ✅ |
| **C++20** | ✅ | ✅ | ✅ |
| **C++23** | ⚠️ Partial | ⚠️ Partial | ⚠️ Partial |
| **LTO** | ✅ | ✅ | ✅ |
| **AVX2** | ✅ | ✅ | ✅ (x86 only) |
| **AVX-512** | ✅ | ✅ | ✅ (x86 only) |

### Path Separators

XSigma automatically handles path separators across platforms:
- Windows: `\` (backslash)
- Linux/macOS: `/` (forward slash)

Use CMake's path handling functions for cross-platform compatibility.

### Line Endings

- Windows: CRLF (`\r\n`)
- Linux/macOS: LF (`\n`)

Configure your editor to use consistent line endings (LF recommended).

## Cross-Compilation

### Linux to Windows (MinGW)

```bash
# Install MinGW cross-compiler
sudo apt-get install mingw-w64

# Configure for Windows target
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

### Building for ARM on x86

```bash
# Install ARM cross-compiler
sudo apt-get install gcc-arm-linux-gnueabihf

# Configure for ARM target
cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

## Best Practices

### Development Workflow

**Windows:**
```bash
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Debug
```

**Linux/macOS:**
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)  # Linux
cmake --build build -j$(sysctl -n hw.ncpu)  # macOS
```

### Production Builds

**All Platforms:**
```bash
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DPROJECT_ENABLE_LTO=ON \
    -DVECTORIZATION_CPU_BACKEND=avx2

# Platform-specific parallel build
cmake --build build -j  # Auto-detect
```

### CI/CD Builds

Use platform-specific runners and configurations:

```yaml
# GitHub Actions example
jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - run: cmake -B build -S . -G "Visual Studio 17 2022"
      - run: cmake --build build --config Release

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - run: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
      - run: cmake --build build -j

  build-macos:
    runs-on: macos-latest
    steps:
      - run: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
      - run: cmake --build build -j
```

## Related Documentation

- [Build Configuration](build/build-configuration.md) - Build system configuration
- [Vectorization](vectorization.md) - CPU SIMD optimization
- [Third-Party Dependencies](third-party-dependencies.md) - Dependency management
