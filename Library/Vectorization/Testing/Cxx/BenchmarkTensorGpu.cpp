/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * CPU vs GPU tensor throughput benchmarks.
 *
 * Compares vectorization::tensor<T> expression evaluation on the CPU SIMD
 * backend against the CUDA GPU backend (run_gpu / fill_gpu), across problem
 * sizes ranging from latency-bound (1K elements) to throughput-bound (4M
 * elements).
 *
 * Naming / methodology:
 *   CPU_<Op><T>            — host SIMD path (expressions_evaluator::run on CPU)
 *   GPU_<Op><T>             — device-resident: operands already on the GPU,
 *                            times kernel launch + execution only
 *                            (cudaDeviceSynchronize() inside the timed loop).
 *   GPU_<Op>_Transfer<T>    — same op, but re-uploads inputs / downloads the
 *                            result every iteration: the realistic cost when
 *                            the tensors do not already live on the device.
 *   GPU_TensorAllocFree<T>  — isolates the cost of constructing/destroying a
 *                            device tensor. Every temporary produced inside a
 *                            GPU expression chain pays this because the GPU
 *                            allocator path (Memory/allocator.h) calls
 *                            cudaMalloc/cudaFree directly instead of routing
 *                            through the caching allocator that
 *                            Memory/gpu/cuda_caching_allocator.h provides.
 *
 * GPU benchmarks use wall-clock time (not MeasureProcessCPUTime): most of the
 * "time" is the device executing, not the host CPU, so process-CPU time would
 * understate the cost and make CPU/GPU numbers non-comparable.
 *
 * This file is compiled as a CUDA translation unit (CMake sets LANGUAGE CUDA
 * on it, mirroring TestTensorGpu.cpp) so run_gpu/fill_gpu are instantiated.
 * It degrades to a benchmark stub (no registered benchmarks) when
 * VECTORIZATION_HAS_CUDA is off, or when the CUDA compiler is Clang (see the
 * exclusion note in Testing/Cxx/CMakeLists.txt).
 *
 * Target: benchmark_tensorgpu
 */

#include <benchmark/benchmark.h>

#if VECTORIZATION_HAS_CUDA

#include <cuda_runtime.h>

#include <cstddef>
#include <random>
#include <vector>

#include "terminals/tensor.h"

namespace
{
using namespace vectorization;

template <typename T>
void fill_uniform(std::vector<T>& v, T lo, T hi, unsigned seed)
{
    std::mt19937                      gen(seed);
    std::uniform_real_distribution<T> dist(lo, hi);
    for (auto& x : v)
        x = dist(gen);
}

bool has_cuda_device()
{
    int ndev = 0;
    return cudaGetDeviceCount(&ndev) == cudaSuccess && ndev > 0;
}

}  // namespace

// Explicit sizes: 1K (latency-bound), 64K, 1M, 4M (throughput-bound).
#define BENCH_SIZES ->Arg(1 << 10)->Arg(1 << 16)->Arg(1 << 20)->Arg(1 << 22)->Unit(benchmark::kMicrosecond)

// ---------------------------------------------------------------------------
// Fill: a = scalar
// ---------------------------------------------------------------------------
template <typename T>
static void CPU_Fill(benchmark::State& state)
{
    const size_t n = static_cast<size_t>(state.range(0));
    tensor<T>    a(n);
    for (auto _ : state)
        a = static_cast<T>(3.14159);
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(CPU_Fill, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(CPU_Fill, double) BENCH_SIZES;

template <typename T>
static void GPU_Fill(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t n = static_cast<size_t>(state.range(0));
    tensor<T>    a(n, device_enum::CUDA);
    for (auto _ : state)
    {
        a = static_cast<T>(3.14159);
        cudaDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Fill, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Fill, double) BENCH_SIZES;

// ---------------------------------------------------------------------------
// Binary add: c = a + b  (device-resident; excludes transfer)
// ---------------------------------------------------------------------------
template <typename T>
static void CPU_Add(benchmark::State& state)
{
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-2), static_cast<T>(2), 1u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 2u);

    tensor<T> a(n), b(n), c(n);
    a.copy_from_host(ha);
    b.copy_from_host(hb);

    for (auto _ : state)
        benchmark::DoNotOptimize(c = a + b);
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(CPU_Add, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(CPU_Add, double) BENCH_SIZES;

template <typename T>
static void GPU_Add(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-2), static_cast<T>(2), 1u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 2u);

    tensor<T> a(n, device_enum::CUDA), b(n, device_enum::CUDA), c(n, device_enum::CUDA);
    a.copy_from_host(ha);
    b.copy_from_host(hb);

    for (auto _ : state)
    {
        c = a + b;
        cudaDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Add, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Add, double) BENCH_SIZES;

template <typename T>
static void GPU_Add_Transfer(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-2), static_cast<T>(2), 1u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 2u);

    tensor<T> a(n, device_enum::CUDA), b(n, device_enum::CUDA), c(n, device_enum::CUDA);

    for (auto _ : state)
    {
        a.copy_from_host(ha);
        b.copy_from_host(hb);
        c            = a + b;
        auto result  = c.to_host_vector();
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Add_Transfer, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Add_Transfer, double) BENCH_SIZES;

// ---------------------------------------------------------------------------
// Compound expression: exp(a) + sqrt(b) -- fused into a single kernel/loop
// ---------------------------------------------------------------------------
template <typename T>
static void CPU_Compound(benchmark::State& state)
{
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 4u);

    tensor<T> a(n), b(n), c(n);
    a.copy_from_host(ha);
    b.copy_from_host(hb);

    for (auto _ : state)
        benchmark::DoNotOptimize(c = ::exp(a) + ::sqrt(b));
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(CPU_Compound, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(CPU_Compound, double) BENCH_SIZES;

template <typename T>
static void GPU_Compound(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 4u);

    tensor<T> a(n, device_enum::CUDA), b(n, device_enum::CUDA), c(n, device_enum::CUDA);
    a.copy_from_host(ha);
    b.copy_from_host(hb);

    for (auto _ : state)
    {
        c = ::exp(a) + ::sqrt(b);
        cudaDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Compound, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Compound, double) BENCH_SIZES;

// ---------------------------------------------------------------------------
// Allocation overhead: construct + destroy a device tensor every iteration.
// ---------------------------------------------------------------------------
template <typename T>
static void GPU_TensorAllocFree(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _ : state)
    {
        tensor<T> a(n, device_enum::CUDA);
        benchmark::DoNotOptimize(a.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_TensorAllocFree, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_TensorAllocFree, double) BENCH_SIZES;

#undef BENCH_SIZES

#endif  // VECTORIZATION_HAS_CUDA

BENCHMARK_MAIN();
