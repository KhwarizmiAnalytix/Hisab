# High-Performance Computing Guide

Quarisma provides comprehensive support for high-performance computing through CPU vectorization (SIMD), GPU acceleration (CUDA/HIP), and multithreading capabilities.

## Table of Contents

- [Vectorization (SIMD)](#vectorization-simd)
- [GPU Acceleration](#gpu-acceleration)
- [Multithreading](#multithreading)
- [Combining HPC Features](#combining-hpc-features)
- [Performance Tuning](#performance-tuning)

## Vectorization (SIMD)

CPU SIMD (Single Instruction, Multiple Data) instruction sets enable parallel processing on modern processors.

### Supported Instruction Sets

| Instruction Set | Target Processors | Performance | Compatibility |
|-----------------|-------------------|-------------|----------------|
| **SSE/SSE2** | Legacy (pre-2005) | Baseline | Oldest systems |
| **AVX** | Sandy Bridge+ (2011) | 2-4x | Most systems |
| **AVX2** | Haswell+ (2013) | 4-8x | Modern CPUs (default) |
| **AVX-512** | Skylake+ (2016) | 8-16x | High-end systems |

### Building with Vectorization

```bash
cd Scripts

# Build with AVX2 (recommended for modern CPUs)
python setup.py config.build.ninja.clang.release.avx2

# Build with AVX-512 (for latest high-end systems)
python setup.py config.build.ninja.clang.release.avx512

# Build with SSE (for legacy compatibility)
python setup.py config.build.ninja.clang.release.sse

# Build with AVX
python setup.py config.build.ninja.clang.release.avx
```

### Performance Benefits

- **2-8x speedup** for vectorizable operations
- **Automatic vectorization** by compiler
- **Portable** across compatible processors
- **No code changes** required for basic usage

### Vectorization with Other Features

```bash
cd Scripts

# Vectorization with LTO optimization (LTO enabled by default)
python setup.py config.build.ninja.clang.release.avx2

# Vectorization with compiler caching
python setup.py config.build.ninja.clang.release.avx2.ccache

# Vectorization with testing
python setup.py config.build.test.ninja.clang.release.avx2

# Vectorization with code coverage
python setup.py config.build.test.ninja.clang.debug.avx2.coverage
```

## GPU Acceleration

Quarisma supports NVIDIA CUDA and AMD HIP for GPU-accelerated computing.

### CUDA Support

NVIDIA GPU acceleration for massive parallel computing.

#### Requirements

- **NVIDIA GPU** with compute capability 3.5 or higher
- **CUDA Toolkit** 11.0 or later
- **cuDNN** (optional, for deep learning operations)
- **Supported Platforms**: Linux, Windows, macOS (with external GPU)

#### GPU Architectures Supported

| Architecture | GPU Examples | Compute Capability |
|--------------|--------------|-------------------|
| **Fermi** | GTX 400/500 | 2.0 |
| **Kepler** | GTX 600/700 | 3.0-3.5 |
| **Maxwell** | GTX 750/950 | 5.0-5.3 |
| **Pascal** | GTX 1000 series | 6.0-6.2 |
| **Volta** | Tesla V100 | 7.0 |
| **Turing** | RTX 2000 series | 7.5 |
| **Ampere** | RTX 3000 series, A100 | 8.0-8.6 |
| **Ada** | RTX 4000 series | 8.9-9.0 |
| **Hopper** | H100 | 9.0 |

#### Building with CUDA

```bash
cd Scripts

# Basic CUDA support
python setup.py config.build.ninja.clang.cuda

# CUDA with optimizations
python setup.py config.build.ninja.clang.release.cuda

# CUDA with testing
python setup.py config.build.test.ninja.clang.debug.cuda

# CUDA with code coverage
python setup.py config.build.test.ninja.clang.debug.cuda.coverage

# CUDA with sanitizers (use dot notation: sanitizer.address)
python setup.py config.build.test.ninja.clang.debug.cuda --sanitizer.address

# CUDA with compiler caching
python setup.py config.build.ninja.clang.release.cuda.ccache

# CUDA with LTO (LTO enabled by default)
python setup.py config.build.ninja.clang.release.cuda

# CUDA with vectorization
python setup.py config.build.ninja.clang.release.cuda.avx2

# CUDA with TBB multithreading
python setup.py config.build.ninja.clang.release.cuda.tbb
```

#### CUDA Configuration

**CMake Flag**: `MEMOY_ENABLE_CUDA` (default: OFF)

**GPU Architecture Selection**:
```bash
# Auto-detect GPU architecture (recommended)
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_CUDA_ARCH_OPTIONS=native

# Specific architecture
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_CUDA_ARCH_OPTIONS=ampere

# Multiple architectures
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_CUDA_ARCH_OPTIONS=all
```

**Memory Allocation Strategy**:
```bash
# Synchronous allocation (default)
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_GPU_ALLOC=SYNC

# Asynchronous allocation
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_GPU_ALLOC=ASYNC

# Pool-based asynchronous allocation
cmake -B build -S . -DMEMOY_ENABLE_CUDA=ON -DQUARISMA_GPU_ALLOC=POOL_ASYNC
```

#### Performance Metrics

- **10-100x speedup** for GPU-accelerated operations
- **Bandwidth**: 100-900 GB/s depending on GPU
- **Throughput**: Thousands of parallel threads

### HIP Support (AMD GPUs)

AMD GPU acceleration through HIP (Heterogeneous-compute Interface for Portability).

#### Requirements

- **AMD GPU** with RDNA or CDNA architecture
- **ROCm** 4.0 or later
- **CMake** 3.21 or later
- **Supported Platforms**: Linux (primary), Windows (experimental)

#### Building with HIP

```bash
cd Scripts

# Basic HIP support
python setup.py config.build.ninja.clang.hip

# HIP with optimizations
python setup.py config.build.ninja.clang.release.hip

# HIP with testing
python setup.py config.build.test.ninja.clang.debug.hip
```

**Note**: CUDA and HIP are mutually exclusive. Enable only one GPU backend.

## Multithreading

Quarisma provides flexible multithreading options for parallel computing.

### Intel Threading Building Blocks (TBB)

High-level parallel programming framework with task-based parallelism.

#### Features

- **Task-based parallelism**: Automatic load balancing
- **Scalability**: Efficient scaling across multiple cores
- **Thread-safe containers**: Concurrent data structures
- **Performance**: Near-linear scaling with core count

#### Building with TBB

```bash
cd Scripts

# Basic TBB support
python setup.py config.build.ninja.clang.tbb

# TBB with optimizations
python setup.py config.build.ninja.clang.release.tbb

# TBB with testing
python setup.py config.build.test.ninja.clang.debug.tbb

# TBB with CUDA
python setup.py config.build.ninja.clang.release.cuda.tbb

# TBB with vectorization
python setup.py config.build.ninja.clang.release.avx2.tbb
```

#### Use Cases

- Complex parallel algorithms
- Dynamic workload distribution
- Nested parallelism
- Heterogeneous computing

### Native C++ Threading

Standard C++17/20 threading with `std::thread`, `std::async`, and `std::future`.

#### Features

- **No external dependencies**: Uses standard library only
- **Lightweight**: Minimal overhead
- **Portable**: Works on all platforms
- **Flexible**: Full control over thread management

#### Building with Native Threading

```bash
cd Scripts

# Default build (uses native C++ threading)
python setup.py config.build.ninja.clang.release

# With testing
python setup.py config.build.test.ninja.clang.debug

# With CUDA
python setup.py config.build.ninja.clang.release.cuda
```

#### Use Cases

- Simple parallel tasks
- I/O-bound operations
- Minimal overhead requirements
- Cross-platform compatibility

### Comparison: TBB vs Native C++

| Feature | TBB | Native C++ |
|---------|-----|-----------|
| **Ease of Use** | High (task-based) | Medium (manual) |
| **Performance** | Excellent (optimized) | Good (standard) |
| **Scalability** | Excellent | Good |
| **Dependencies** | External library | None |
| **Learning Curve** | Moderate | Low |
| **Best For** | Complex algorithms | Simple tasks |

## Combining HPC Features

Quarisma allows combining multiple HPC features for maximum performance.

### Recommended Combinations

#### Maximum Performance (CPU)

```bash
cd Scripts
python setup.py config.build.ninja.clang.release.avx2.ccache
```

Features: AVX2 vectorization + LTO (enabled by default) + compiler caching

#### Maximum Performance (GPU + CPU)

```bash
cd Scripts
python setup.py config.build.ninja.clang.release.cuda.avx2.ccache
```

Features: CUDA + AVX2 + LTO (enabled by default) + compiler caching

#### Development with GPU

```bash
cd Scripts
python setup.py config.build.test.ninja.clang.debug.cuda.coverage
```

Features: CUDA + testing + code coverage

#### Production Build

```bash
cd Scripts
python setup.py config.build.ninja.clang.release.cuda.avx2.tbb
```

Features: CUDA + AVX2 + LTO (enabled by default) + TBB multithreading

## Performance Tuning

### CPU Optimization Tips

1. **Choose appropriate vectorization**: AVX2 for most systems, AVX-512 for high-end
2. **Enable LTO**: 10-30% improvement in release builds
3. **Use compiler caching**: 50-80% faster incremental builds
4. **Select faster linker**: 5-15% improvement in link time

### GPU Optimization Tips

1. **Match GPU architecture**: Use native detection or specify exact architecture
2. **Choose allocation strategy**: SYNC for simplicity, ASYNC for performance
3. **Combine with CPU optimization**: Use vectorization + CUDA together
4. **Profile your code**: Identify GPU-accelerated bottlenecks

### Multithreading Optimization Tips

1. **Use TBB for complex algorithms**: Better load balancing
2. **Use native threading for simple tasks**: Lower overhead
3. **Combine with CUDA**: Offload compute to GPU, use threads for I/O
4. **Monitor thread scaling**: Ensure near-linear scaling with core count

### Build Time Optimization

```bash
cd Scripts

# Fastest incremental builds
python setup.py config.build.ninja.clang.ccache

# Fastest clean builds
python setup.py config.build.ninja.clang.release  # (LTO enabled by default in release builds)

# Balanced approach
python setup.py config.build.ninja.clang.release.ccache  # (LTO enabled by default in release builds)
```

## Troubleshooting

### CUDA Issues

**Problem**: CUDA not found
- Install CUDA Toolkit 11.0+
- Set `CUDA_PATH` environment variable
- Verify GPU driver is installed

**Problem**: GPU architecture mismatch
- Use `native` option for auto-detection
- Check GPU compute capability: `nvidia-smi`

### Vectorization Issues

**Problem**: Vectorization not working
- Verify CPU supports instruction set
- Check compiler optimization flags
- Use profiler to confirm vectorization

### Multithreading Issues

**Problem**: TBB not found
- Install TBB development package
- Set `TBB_ROOT` environment variable
- Use system package manager

## Related Documentation

- [Setup Guide](setup.md) - Build configuration
- [Build Configuration](build/build-configuration.md) - CMake options
- [Compiler Caching](cache.md) - Build speed optimization
