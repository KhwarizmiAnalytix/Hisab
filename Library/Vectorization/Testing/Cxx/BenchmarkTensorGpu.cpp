/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * CPU vs GPU tensor throughput benchmarks.
 *
 * Compares vectorization::tensor<T> expression evaluation on the CPU SIMD
 * backend against the GPU backend (run_gpu / fill_gpu, CUDA or HIP), across
 * problem sizes ranging from latency-bound (1K elements) to throughput-bound
 * (4M elements).
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
 *   CPU/GPU_MonteCarloPath<T> — multi-factor Monte Carlo path update
 *                            X += sigma_0*Z_0 + ... + sigma_3*Z_3, repeated
 *                            over kMcSteps time steps per iteration. X holds
 *                            one running value per path; Z_i are per-factor
 *                            shocks (fixed across steps); sigma_i are scalar
 *                            loadings. GPU variant syncs once after all
 *                            kMcSteps launches, not per step.
 *
 * GPU benchmarks use wall-clock time (not MeasureProcessCPUTime): most of the
 * "time" is the device executing, not the host CPU, so process-CPU time would
 * understate the cost and make CPU/GPU numbers non-comparable.
 *
 * This file is compiled as a CUDA or HIP translation unit (CMake sets
 * LANGUAGE CUDA/HIP on it, mirroring TestTensorGpu.cpp) so run_gpu/fill_gpu
 * are instantiated. It degrades to a benchmark stub (no registered
 * benchmarks) when neither VECTORIZATION_HAS_CUDA nor VECTORIZATION_HAS_HIP
 * is on, or when the CUDA compiler is Clang (see the exclusion note in
 * Testing/Cxx/CMakeLists.txt).
 *
 * Target: benchmark_tensorgpu
 */

#include <benchmark/benchmark.h>

#if VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP

#if VECTORIZATION_HAS_CUDA
#include <cuda_runtime.h>
using gpu_error_t                 = cudaError_t;
constexpr gpu_error_t kGpuSuccess = cudaSuccess;
#define gpuGetDeviceCount cudaGetDeviceCount
#define gpuDeviceSynchronize cudaDeviceSynchronize
#elif VECTORIZATION_HAS_HIP
#include <hip/hip_runtime.h>
using gpu_error_t                 = hipError_t;
constexpr gpu_error_t kGpuSuccess = hipSuccess;
#define gpuGetDeviceCount hipGetDeviceCount
#define gpuDeviceSynchronize hipDeviceSynchronize
#endif

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

bool has_gpu_device()
{
    int ndev = 0;
    return gpuGetDeviceCount(&ndev) == kGpuSuccess && ndev > 0;
}

}  // namespace

// Explicit sizes: 1K (latency-bound), 64K, 1M, 4M (throughput-bound).
#define BENCH_SIZES \
    ->Arg(1 << 10)->Arg(1 << 16)->Arg(1 << 20)->Arg(1 << 22)->Unit(benchmark::kMicrosecond)

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
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
        return;
    }
    const size_t n = static_cast<size_t>(state.range(0));
    tensor<T>    a(n, device_enum::CUDA);
    for (auto _ : state)
    {
        a = static_cast<T>(3.14159);
        gpuDeviceSynchronize();
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
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
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
        gpuDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Add, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Add, double) BENCH_SIZES;

template <typename T>
static void GPU_Add_Transfer(benchmark::State& state)
{
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
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
        c           = a + b;
        auto result = c.to_host_vector();
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
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
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
        gpuDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_Compound, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_Compound, double) BENCH_SIZES;

// ---------------------------------------------------------------------------
// Monte Carlo path update: X += sigma_0*Z_0 + sigma_1*Z_1 + sigma_2*Z_2 + sigma_3*Z_3,
// repeated over kMcSteps time steps (Z_i held fixed across steps — only the
// running sum X changes). Models the per-step diffusion update of a
// multi-factor Monte Carlo path simulation, where X holds one running value
// per path, Z_i are per-factor shocks, and sigma_i are scalar loadings.
// Device-resident: for GPU, all kMcSteps kernel launches are queued on the
// default stream and synced once at the end, matching how a real engine
// would only synchronize after advancing the full path.
// ---------------------------------------------------------------------------
constexpr int kMcSteps = 64;

template <typename T>
static void CPU_MonteCarloPath(benchmark::State& state)
{
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> hz0(n), hz1(n), hz2(n), hz3(n);
    fill_uniform(hz0, static_cast<T>(-1), static_cast<T>(1), 1u);
    fill_uniform(hz1, static_cast<T>(-1), static_cast<T>(1), 2u);
    fill_uniform(hz2, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hz3, static_cast<T>(-1), static_cast<T>(1), 4u);

    tensor<T> x(n), z0(n), z1(n), z2(n), z3(n);
    x = static_cast<T>(0);
    z0.copy_from_host(hz0);
    z1.copy_from_host(hz1);
    z2.copy_from_host(hz2);
    z3.copy_from_host(hz3);

    const T sigma0 = static_cast<T>(0.10);
    const T sigma1 = static_cast<T>(0.15);
    const T sigma2 = static_cast<T>(0.20);
    const T sigma3 = static_cast<T>(0.25);

    for (auto _ : state)
    {
        for (int step = 0; step < kMcSteps; ++step)
            x = x + sigma0 * z0 + sigma1 * z1 + sigma2 * z2 + sigma3 * z3;
        benchmark::DoNotOptimize(x.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n) * kMcSteps);
}
BENCHMARK_TEMPLATE(CPU_MonteCarloPath, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(CPU_MonteCarloPath, double) BENCH_SIZES;

template <typename T>
static void GPU_MonteCarloPath(benchmark::State& state)
{
    if (!has_cuda_device())
    {
        state.SkipWithError("No CUDA device");
        return;
    }
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> hz0(n), hz1(n), hz2(n), hz3(n);
    fill_uniform(hz0, static_cast<T>(-1), static_cast<T>(1), 1u);
    fill_uniform(hz1, static_cast<T>(-1), static_cast<T>(1), 2u);
    fill_uniform(hz2, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hz3, static_cast<T>(-1), static_cast<T>(1), 4u);

    tensor<T> x(n, device_enum::CUDA), z0(n, device_enum::CUDA), z1(n, device_enum::CUDA),
        z2(n, device_enum::CUDA), z3(n, device_enum::CUDA);
    x = static_cast<T>(0);
    z0.copy_from_host(hz0);
    z1.copy_from_host(hz1);
    z2.copy_from_host(hz2);
    z3.copy_from_host(hz3);

    const T sigma0 = static_cast<T>(0.10);
    const T sigma1 = static_cast<T>(0.15);
    const T sigma2 = static_cast<T>(0.20);
    const T sigma3 = static_cast<T>(0.25);

    for (auto _ : state)
    {
        for (int step = 0; step < kMcSteps; ++step)
            x = x + sigma0 * z0 + sigma1 * z1 + sigma2 * z2 + sigma3 * z3;
        cudaDeviceSynchronize();
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n) * kMcSteps);
}
BENCHMARK_TEMPLATE(GPU_MonteCarloPath, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_MonteCarloPath, double) BENCH_SIZES;

// ---------------------------------------------------------------------------
// Allocation overhead: construct + destroy a device tensor every iteration.
// ---------------------------------------------------------------------------
template <typename T>
static void GPU_TensorAllocFree(benchmark::State& state)
{
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
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

#endif  // VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP

BENCHMARK_MAIN();
