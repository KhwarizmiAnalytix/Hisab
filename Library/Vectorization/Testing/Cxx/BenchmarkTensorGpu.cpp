/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * CPU vs GPU tensor throughput benchmarks.
 *
 * Compares vectorization::tensor<T> expression evaluation on the CPU SIMD
 * backend against the GPU backend (run_gpu / fill_gpu for CUDA/HIP,
 * run_metal / fill_metal for Metal), across problem sizes ranging from
 * latency-bound (1K elements) to throughput-bound (4M elements). Metal is
 * float-only (MSL has no double) — GPU_*<double> benchmarks skip themselves
 * under Metal (see kMetalOnlyBackend).
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
 *                            device tensor (XSigma metal/cuda caching
 *                            allocator path via Memory/allocator.h).
 *   LibTorch_MPS_TensorAllocFree<T> — same alloc/free loop on torch::kMPS
 *                            (Metal builds with LibTorch only), for a direct
 *                            comparison against PyTorch's MPS caching allocator.
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
 * are instantiated; Metal needs no such CMake language (ordinary host C++,
 * device-count queries routed through the metal_device_probe.mm shim, same
 * as TestTensorGpu.cpp). It degrades to a benchmark stub (no registered
 * benchmarks) when none of VECTORIZATION_HAS_CUDA/_HIP/_METAL is on, or when
 * the CUDA compiler is Clang (see the exclusion note in Testing/Cxx/CMakeLists.txt).
 *
 * Target: benchmark_tensorgpu
 */

#include <benchmark/benchmark.h>

#if VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP || VECTORIZATION_HAS_METAL

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
#elif VECTORIZATION_HAS_METAL
// metal_dispatch.h is a plain C++ header (no Objective-C types cross its boundary —
// see its own header comment), so this file can call into it directly without
// becoming Objective-C++ itself, unlike TestTensorGpu.cpp's device-count query (which
// needed a separate .mm shim because that file only wants device_enum, not the rest of
// the Metal backend surface).
#include "backend/gpu/metal/metal_dispatch.h"
using gpu_error_t                 = int;
constexpr gpu_error_t kGpuSuccess = 0;
#define gpuGetDeviceCount(pn) \
    (*(pn) = vectorization::metal_backend::device_available() ? 1 : 0, kGpuSuccess)
// Every metal_backend::dispatch()/dispatch_fill() call already blocks on
// waitUntilCompleted internally (see metal_dispatch.mm) — no separate device-sync API.
#define gpuDeviceSynchronize() ((void)0)
#endif

#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

#include "terminals/tensor.h"

#if VECTORIZATION_HAS_METAL && VECTORIZATION_HAS_LIBTORCH
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <torch/mps.h>
#include <torch/torch.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

namespace
{
using namespace vectorization;

#if VECTORIZATION_HAS_CUDA
constexpr device_enum kActiveGpuDevice   = device_enum::CUDA;
constexpr bool         kMetalOnlyBackend = false;
#elif VECTORIZATION_HAS_HIP
constexpr device_enum kActiveGpuDevice   = device_enum::HIP;
constexpr bool         kMetalOnlyBackend = false;
#elif VECTORIZATION_HAS_METAL
constexpr device_enum kActiveGpuDevice   = device_enum::METAL;
constexpr bool         kMetalOnlyBackend = true;
#endif

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

// Metal is float-only (MSL has no double); tensor<double> on device_enum::METAL throws
// at allocation time (Library/Memory/allocator.h). Every GPU_*<double> benchmark checks
// this before constructing a device tensor so it skips cleanly instead of throwing.
template <typename T>
bool skip_if_unsupported(benchmark::State& state)
{
    if (kMetalOnlyBackend && std::is_same_v<T, double>)
    {
        state.SkipWithError("Metal backend is float-only (no fp64 on Apple GPUs)");
        return true;
    }
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
        return true;
    }
    return false;
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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t n = static_cast<size_t>(state.range(0));
    tensor<T>    a(n, kActiveGpuDevice);
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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-2), static_cast<T>(2), 1u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 2u);

    tensor<T> a(n, kActiveGpuDevice), b(n, kActiveGpuDevice), c(n, kActiveGpuDevice);
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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-2), static_cast<T>(2), 1u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 2u);

    tensor<T> a(n, kActiveGpuDevice), b(n, kActiveGpuDevice), c(n, kActiveGpuDevice);

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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> ha(n), hb(n);
    fill_uniform(ha, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 4u);

    tensor<T> a(n, kActiveGpuDevice), b(n, kActiveGpuDevice), c(n, kActiveGpuDevice);
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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t   n = static_cast<size_t>(state.range(0));
    std::vector<T> hz0(n), hz1(n), hz2(n), hz3(n);
    fill_uniform(hz0, static_cast<T>(-1), static_cast<T>(1), 1u);
    fill_uniform(hz1, static_cast<T>(-1), static_cast<T>(1), 2u);
    fill_uniform(hz2, static_cast<T>(-1), static_cast<T>(1), 3u);
    fill_uniform(hz3, static_cast<T>(-1), static_cast<T>(1), 4u);

    tensor<T> x(n, kActiveGpuDevice), z0(n, kActiveGpuDevice), z1(n, kActiveGpuDevice),
        z2(n, kActiveGpuDevice), z3(n, kActiveGpuDevice);
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
        gpuDeviceSynchronize();
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
    if (skip_if_unsupported<T>(state))
        return;
    const size_t n = static_cast<size_t>(state.range(0));
    for (auto _ : state)
    {
        tensor<T> a(n, kActiveGpuDevice);
        benchmark::DoNotOptimize(a.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(n));
}
BENCHMARK_TEMPLATE(GPU_TensorAllocFree, float) BENCH_SIZES;
BENCHMARK_TEMPLATE(GPU_TensorAllocFree, double) BENCH_SIZES;

#if VECTORIZATION_HAS_METAL && VECTORIZATION_HAS_LIBTORCH
// ---------------------------------------------------------------------------
// LibTorch MPS allocation overhead — same construct/destroy loop as
// GPU_TensorAllocFree, for a head-to-head against PyTorch's MPS caching path.
// ---------------------------------------------------------------------------
template <typename T>
static void LibTorch_MPS_TensorAllocFree(benchmark::State& state)
{
    if constexpr (std::is_same_v<T, double>)
    {
        state.SkipWithError("Metal/MPS comparison is float-only (no fp64 on Apple GPUs)");
        return;
    }
    if (!torch::mps::is_available())
    {
        state.SkipWithError("LibTorch MPS device not available");
        return;
    }

    const int64_t n = state.range(0);
    auto          opts =
        torch::TensorOptions().dtype(torch::kFloat32).device(torch::kMPS).requires_grad(false);

    // Warm the MPS caching allocator once so the timed loop measures reuse,
    // matching XSigma's warm caching-allocator path after the first iteration.
    {
        auto warm = torch::empty({n}, opts);
        benchmark::DoNotOptimize(warm.data_ptr());
    }
    torch::mps::synchronize();

    for (auto _ : state)
    {
        auto t = torch::empty({n}, opts);
        benchmark::DoNotOptimize(t.data_ptr());
    }
    torch::mps::synchronize();
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_TEMPLATE(LibTorch_MPS_TensorAllocFree, float) BENCH_SIZES;
#endif  // VECTORIZATION_HAS_METAL && VECTORIZATION_HAS_LIBTORCH

#undef BENCH_SIZES

// ---------------------------------------------------------------------------
// Reduction: sum_i (A[i] + B[i] * sin(X[i])) over a fixed, small N=512 — a
// latency-bound size chosen specifically to make the GPU's fixed per-launch
// overhead (command buffer encode + commit + wait) visible against the CPU's
// direct SIMD loop, rather than being amortized away like the throughput-bound
// BENCH_SIZES cases above.
//
// CPU: vectorization::accumulate(a + b * sin(x)) — a single host loop, no
// device dispatch at all.
// GPU: three chained elementwise kernels (mul, sin's unary, add — matching
// the same expression tree run_metal/run_gpu would lower) followed by one
// single-threadgroup reduction kernel (reduce_sum_float — see kernels.metal;
// Metal only, CUDA/HIP have no reduction path at all, see the file header).
// ---------------------------------------------------------------------------
constexpr size_t kSumN = 512;

template <typename T>
static void CPU_SumAddMulSin(benchmark::State& state)
{
    std::vector<T> ha(kSumN), hb(kSumN), hx(kSumN);
    fill_uniform(ha, static_cast<T>(-1), static_cast<T>(1), 5u);
    fill_uniform(hb, static_cast<T>(0.5), static_cast<T>(1.5), 6u);
    fill_uniform(hx, static_cast<T>(-3.14159), static_cast<T>(3.14159), 7u);

    tensor<T> a(kSumN), b(kSumN), x(kSumN);
    a.copy_from_host(ha);
    b.copy_from_host(hb);
    x.copy_from_host(hx);

    for (auto _ : state)
        benchmark::DoNotOptimize(vectorization::accumulate(a + b * ::sin(x)));
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSumN));
}
BENCHMARK_TEMPLATE(CPU_SumAddMulSin, float)->Unit(benchmark::kMicrosecond);
BENCHMARK_TEMPLATE(CPU_SumAddMulSin, double)->Unit(benchmark::kMicrosecond);

#if VECTORIZATION_HAS_METAL
// Metal-only: no reduction kernel exists for CUDA/HIP (see file header). Not templated
// on T since Metal is float-only — a <double> variant would have nothing to instantiate.
static void GPU_SumAddMulSin_Metal(benchmark::State& state)
{
    if (!has_gpu_device())
    {
        state.SkipWithError("No GPU device");
        return;
    }
    std::vector<float> ha(kSumN), hb(kSumN), hx(kSumN);
    fill_uniform(ha, -1.0f, 1.0f, 5u);
    fill_uniform(hb, 0.5f, 1.5f, 6u);
    fill_uniform(hx, -3.14159f, 3.14159f, 7u);

    tensor<float> a(kSumN, device_enum::METAL), b(kSumN, device_enum::METAL),
        x(kSumN, device_enum::METAL), c(kSumN, device_enum::METAL);
    a.copy_from_host(ha);
    b.copy_from_host(hb);
    x.copy_from_host(hx);

    for (auto _ : state)
    {
        // Same expression tree as the CPU path — run_metal lowers this into
        // sin -> mul -> add kernel dispatches (see expressions_evaluator_metal.h).
        c         = a + b * ::sin(x);
        float sum = vectorization::metal_backend::reduce_sum(c.data(), kSumN);
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kSumN));
}
BENCHMARK(GPU_SumAddMulSin_Metal)->Unit(benchmark::kMicrosecond);
#endif  // VECTORIZATION_HAS_METAL

#endif  // VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP || VECTORIZATION_HAS_METAL

BENCHMARK_MAIN();
