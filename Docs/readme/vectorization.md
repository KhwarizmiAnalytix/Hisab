# Vectorization Support

Quarisma supports multiple CPU SIMD (Single Instruction, Multiple Data) instruction sets for high-performance computing. Vectorization allows the CPU to process multiple data elements simultaneously, significantly improving performance for data-parallel operations.

## Table of Contents

- [Supported Vectorization Types](#supported-vectorization-types)
- [Quick Start](#quick-start)
- [Choosing the Right Vectorization Level](#choosing-the-right-vectorization-level)
- [Platform-Specific Considerations](#platform-specific-considerations)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

## Supported Vectorization Types

Quarisma supports the following CPU SIMD instruction sets:

| Type | Description | GCC/Clang Flags | MSVC Flags | CPU Requirements |
|------|-------------|-----------------|------------|------------------|
| `no` | No vectorization | None | None | Any CPU |
| `sse` | SSE/SSE2 instructions | `-msse -msse2` | `/arch:SSE2` | Intel Pentium 4+ (2001+) |
| `avx` | AVX instructions | `-mavx` | `/arch:AVX` | Intel Sandy Bridge+ (2011+) |
| `avx2` | AVX2 instructions (default) | `-mavx -mavx2` | `/arch:AVX2` | Intel Haswell+ (2013+) |
| `avx512` | AVX-512 instructions | `-mavx -mavx2 -mavx512f` | `/arch:AVX512` | Intel Skylake-X+ (2017+) |
| `neon` | AArch64 NEON (128-bit) | `-march=armv8-a` (typical) | — | Apple Silicon, Linux AArch64 |
| `sve` | AArch64 SVE (fixed 128-bit) | `-march=armv8-a+sve -msve-vector-bits=128` | — | SVE-capable CPUs (see below) |

The `sve` preset uses a **fixed** 128-bit vector length so lane counts match the NEON kernels. Wider `-msve-vector-bits` values are not supported yet.

## Quick Start

### Basic Usage

```bash
# AVX2 vectorization (default - good balance of performance and compatibility)
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=avx2

# SSE for older CPUs
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=sse

# AVX-512 for latest CPUs
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=avx512

# Disable vectorization
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=no
```

### Combined with Other Options

```bash
# High-performance build with AVX2 and LTO
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON \
    -DQUARISMA_VECTORIZATION_TYPE=avx2

cmake --build build --config Release
```

## Choosing the Right Vectorization Level

### Performance vs. Compatibility Trade-off

| Level | Performance | Compatibility | Recommended For |
|-------|-------------|---------------|-----------------|
| `no` | Baseline | Maximum | Cross-platform, embedded systems |
| `sse` | ~2-4x | Very High | Legacy systems, maximum compatibility |
| `avx` | ~4-8x | High | Modern systems (2011+) |
| `avx2` | ~8-16x | Good | Modern systems (2013+) - **Default** |
| `avx512` | ~16-32x | Limited | Latest high-end systems (2017+) |

### Recommendations by Use Case

**Maximum Compatibility:**
```bash
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=sse
```
Use SSE for software that needs to run on a wide range of systems.

**Balanced Performance (Recommended):**
```bash
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=avx2
```
AVX2 is the default and provides excellent performance on most modern CPUs (2013+).

**Maximum Performance:**
```bash
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=avx512
```
Use AVX-512 for high-end servers and workstations with latest CPUs.

**Cross-Platform Distribution:**
```bash
cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=no
```
Disable vectorization for maximum portability across different CPU architectures.

## Platform-Specific Considerations

### Windows (MSVC)

```bash
# AVX2 build on Windows
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_VECTORIZATION_TYPE=avx2

cmake --build build --config Release
```

**Notes:**
- MSVC automatically enables SSE2 on x64 builds
- `/arch:AVX` and `/arch:AVX2` are supported on Visual Studio 2012+
- `/arch:AVX512` requires Visual Studio 2017 15.7+

### Linux (GCC/Clang)

```bash
# AVX2 build on Linux
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_VECTORIZATION_TYPE=avx2

cmake --build build -j$(nproc)
```

**Notes:**
- GCC and Clang have excellent vectorization support
- Consider using `-march=native` for maximum performance on the build machine
- Use specific flags for cross-compilation

### macOS (Clang)

```bash
# Optimized for Apple Silicon or Intel
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_VECTORIZATION_TYPE=avx2

cmake --build build -j$(sysctl -n hw.ncpu)
```

**Notes:**
- Apple Silicon (M1/M2) uses ARM NEON instead of x86 SIMD
- Intel Macs support AVX2 (2013+ models)
- Quarisma automatically detects the architecture

## Troubleshooting

### Compilation Errors

If vectorization fails to compile:

1. **Check CPU compatibility** - ensure your CPU supports the selected instruction set
2. **Use lower vectorization level**:
   ```bash
   cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=avx  # instead of avx2
   ```
3. **Disable vectorization**:
   ```bash
   cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=no
   ```

### Runtime Errors (Illegal Instruction)

If the program crashes with "Illegal instruction" error:

1. **The binary was built with instructions not supported by the CPU**
2. **Solution**: Rebuild with a lower vectorization level:
   ```bash
   cmake -B build -S . -DQUARISMA_VECTORIZATION_TYPE=sse
   ```

### Performance Not Improving

If vectorization doesn't improve performance:

1. **Verify vectorization is enabled**: Check compiler output for SIMD flags
2. **Profile your code**: Use profiling tools to identify bottlenecks
3. **Check data alignment**: Ensure data is properly aligned for SIMD operations
4. **Review algorithms**: Some algorithms benefit more from vectorization than others

## Best Practices

### Development Workflow

```bash
# Development build without vectorization (faster compilation)
cmake -B build_dev -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DQUARISMA_VECTORIZATION_TYPE=no
```

### Production Builds

```bash
# Production build with optimal vectorization
cmake -B build_prod -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DQUARISMA_ENABLE_LTO=ON \
    -DQUARISMA_VECTORIZATION_TYPE=avx2
```

### Testing Multiple Configurations

Test your application with different vectorization levels to ensure correctness:

```bash
# Test with no vectorization
cmake -B build_no_vec -S . -DQUARISMA_VECTORIZATION_TYPE=no
cmake --build build_no_vec
./build_no_vec/tests

# Test with AVX2
cmake -B build_avx2 -S . -DQUARISMA_VECTORIZATION_TYPE=avx2
cmake --build build_avx2
./build_avx2/tests
```

### CPU Feature Detection

Quarisma includes cpuinfo for runtime CPU feature detection. Use it to verify available features:

```cpp
#include <cpuinfo.h>

void check_cpu_features() {
    cpuinfo_initialize();

    if (cpuinfo_has_x86_avx2()) {
        // AVX2 is available
    }

    if (cpuinfo_has_x86_avx512f()) {
        // AVX-512 is available
    }
}
```

## Related Documentation

- [Build Configuration](build/build-configuration.md) - Build system configuration
- [Cross-Platform Building](cross-platform-building.md) - Platform-specific instructions
- [Third-Party Dependencies](third-party-dependencies.md) - cpuinfo library usage
