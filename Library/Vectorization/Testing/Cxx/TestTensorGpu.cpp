/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Integration tests for tensor expression evaluation on the GPU backend
 * (CUDA, HIP, or Metal).
 *
 * Tensors are allocated with kActiveGpuDevice (device_enum::CUDA, ::HIP, or
 * ::METAL depending on which backend is active) so that
 * expressions_evaluator::run dispatches to the GPU path. CUDA and HIP are
 * treated identically by the evaluator dispatch (expressions_evaluator.h);
 * Metal is a separate dispatch condition (expressions_evaluator_metal.h) but
 * shares this same test file since the *tensor<T>*-level behavior it verifies
 * is backend-agnostic. Results are copied back to the host via
 * tensor::to_host_vector() and compared against std:: math.
 *
 * Tests are skipped automatically when no GPU device is present at runtime.
 * Metal is float-only (MSL has no double type on any Apple GPU) — the
 * *Double tests skip themselves under Metal rather than attempting a
 * construction that would throw (see kMetalOnlyBackend below).
 *
 * This file is compiled as a CUDA or HIP translation unit (CMake sets
 * LANGUAGE CUDA/HIP on it) so that GPU expression-template kernels are
 * instantiated correctly, while remaining a plain .cpp source that needs no
 * __CUDACC__/__HIPCC__ guards. Metal needs no such CMake language — it's
 * ordinary host C++, with device-count queries routed through the tiny
 * Objective-C++ shim in metal_device_probe.mm (xsigma_metal_device_count)
 * since this file itself must stay plain .cpp.
 */

#if VECTORIZATION_HAS_CUDA
// nvcc (cicc) forces fmt's FMT_USE_INT128 fallback (fmt::detail::uint128, no
// operator~) even for host-only formatting of long double. #include-d here
// (rather than force-included via -include on the nvcc command line) because
// nvcc reapplies -include when it recompiles its own flattened cudafe1.cpp
// intermediate, which would duplicate all of <fmt> with no header guards
// left and corrupt the parser's namespace state. See cuda_fmt_int128_fix.h.
#include "cuda_fmt_int128_fix.h"
#endif

#include "VectorizationTest.h"

#if VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP || VECTORIZATION_HAS_METAL

#if VECTORIZATION_HAS_CUDA
#include <cuda_runtime.h>
#define gpuGetDeviceCount cudaGetDeviceCount
#elif VECTORIZATION_HAS_HIP
#include <hip/hip_runtime.h>
#define gpuGetDeviceCount hipGetDeviceCount
#elif VECTORIZATION_HAS_METAL
// Implemented in metal_device_probe.mm (Objective-C++) — this file stays plain .cpp.
extern "C" int xsigma_metal_device_count();
#define gpuGetDeviceCount(pn) (*(pn) = xsigma_metal_device_count())
#endif

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "common/scalar_helper_functions.h"
#include "terminals/tensor.h"

namespace
{

using namespace vectorization;

#if VECTORIZATION_HAS_CUDA
constexpr device_enum kActiveGpuDevice  = device_enum::CUDA;
constexpr bool        kMetalOnlyBackend = false;
#elif VECTORIZATION_HAS_HIP
constexpr device_enum kActiveGpuDevice  = device_enum::HIP;
constexpr bool        kMetalOnlyBackend = false;
#elif VECTORIZATION_HAS_METAL
constexpr device_enum kActiveGpuDevice  = device_enum::METAL;
constexpr bool        kMetalOnlyBackend = true;
#endif

// ---- Comparison helper ---------------------------------------------------

// Check element-wise: |got[i] - ref[i]| <= tol * max(1, |ref[i]|) + tol
template <typename T>
void expect_near_rel(const std::vector<T>& got, const std::vector<T>& ref, double tol)
{
    ASSERT_EQ(got.size(), ref.size());
    for (size_t i = 0; i < ref.size(); ++i)
    {
        double err     = std::fabs(static_cast<double>(got[i]) - static_cast<double>(ref[i]));
        double allowed = tol * std::max(1.0, std::fabs(static_cast<double>(ref[i]))) + tol;
        EXPECT_LE(err, allowed) << " at i=" << i << "  got=" << got[i] << "  ref=" << ref[i];
    }
}

// ---- Input generation ---------------------------------------------------

template <typename T>
void make_inputs(std::vector<T>& ha, std::vector<T>& hb, size_t N)
{
    ha.resize(N);
    hb.resize(N);
    for (size_t i = 0; i < N; ++i)
    {
        double t = static_cast<double>(i) / static_cast<double>(N);
        ha[i]    = static_cast<T>(t * 4.0 - 2.0);  // [-2, 2)
        hb[i]    = static_cast<T>(t + 0.5);        // [0.5, 1.5)
    }
}

// ---- Test bodies --------------------------------------------------------

// Fill a GPU tensor with a scalar and verify the values come back correctly.
template <typename T>
void test_fill()
{
    constexpr size_t N    = 1024;
    constexpr T      kVal = static_cast<T>(2.71828);
    constexpr double tol  = std::is_same_v<T, float> ? 5e-6 : 1e-13;

    tensor<T> ga(N, kActiveGpuDevice);
    ga = kVal;  // Dispatches to fill_gpu kernel

    auto result = ga.to_host_vector();
    for (size_t i = 0; i < N; ++i)
        EXPECT_NEAR(result[i], kVal, tol) << " at i=" << i;
}

// Element-wise add, sub, and mul on GPU tensors.
template <typename T>
void test_binary_ops()
{
    constexpr size_t N   = 1024;
    constexpr double tol = std::is_same_v<T, float> ? 5e-5 : 1e-11;

    std::vector<T> ha, hb;
    make_inputs(ha, hb, N);

    tensor<T> ga(N, kActiveGpuDevice), gb(N, kActiveGpuDevice);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);

    // Add
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ga + gb;  // → run_gpu(add_expr, gc.data(), N)
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Subtract
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ga - gb;
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] - hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Element-wise multiply (operator* is element-wise for 1-D tensors)
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ga * gb;
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] * hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Scalar + tensor
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ga + static_cast<T>(1);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + static_cast<T>(1);
        expect_near_rel(result, ref, tol);
    }

    // Destination aliases a leaf (`a = a + b`): must not overwrite `a` before `b` is read.
    {
        tensor<T> acc(N, kActiveGpuDevice);
        acc.copy_from_host(ha);
        acc                   = acc + gb;
        auto           result = acc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + hb[i];
        expect_near_rel(result, ref, tol);
    }
}

// Unary math ops: exp, sqrt, log on GPU tensors.
template <typename T>
void test_unary_math()
{
    constexpr size_t N   = 1024;
    constexpr double tol = std::is_same_v<T, float> ? 5e-5 : 1e-11;

    // Use strictly positive values for sqrt/log
    std::vector<T> ha(N);
    for (size_t i = 0; i < N; ++i)
        ha[i] = static_cast<T>(static_cast<double>(i + 1) / static_cast<double>(N) * 2.0);

    tensor<T> ga(N, kActiveGpuDevice);
    ga.copy_from_host(ha);

    // exp
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::exp(ga);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::exp(ha[i]);
        expect_near_rel(result, ref, tol);
    }

    // sqrt
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::sqrt(ga);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::sqrt(ha[i]);
        expect_near_rel(result, ref, tol);
    }

    // log
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::log(ga);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::log(ha[i]);
        expect_near_rel(result, ref, tol);
    }

    // fabs
    {
        std::vector<T> ha_neg(N);
        for (size_t i = 0; i < N; ++i)
            ha_neg[i] = static_cast<T>(-static_cast<double>(ha[i]));
        tensor<T> ga_neg(N, kActiveGpuDevice);
        ga_neg.copy_from_host(ha_neg);

        tensor<T> gc(N, kActiveGpuDevice);
        gc          = ::fabs(ga_neg);
        auto result = gc.to_host_vector();
        for (size_t i = 0; i < N; ++i)
            EXPECT_NEAR(result[i], ha[i], static_cast<T>(tol)) << " at i=" << i;
    }
}

// Compound expression: the full expression tree is fused into a single kernel.
template <typename T>
void test_compound()
{
    constexpr size_t N = 1024;
    // exp amplifies float errors; use a looser tolerance
    constexpr double tol = std::is_same_v<T, float> ? 5e-4 : 1e-10;

    std::vector<T> ha(N), hb(N);
    for (size_t i = 0; i < N; ++i)
    {
        double t = static_cast<double>(i) / static_cast<double>(N);
        ha[i]    = static_cast<T>(t * 2.0 - 1.0);  // [-1, 1)
        hb[i]    = static_cast<T>(t + 0.5);        // [0.5, 1.5)
    }

    tensor<T> ga(N, kActiveGpuDevice), gb(N, kActiveGpuDevice);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);

    // exp(a) + sqrt(b): fused into one run_gpu call
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::exp(ga) + ::sqrt(gb);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::exp(ha[i]) + std::sqrt(hb[i]);
        expect_near_rel(result, ref, tol);
    }

    // (a + b) * scalar: mixed tensor and scalar operands
    {
        constexpr T kAlpha = static_cast<T>(1.5);
        tensor<T>   gc(N, kActiveGpuDevice);
        gc                    = (ga + gb) * kAlpha;
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = (ha[i] + hb[i]) * kAlpha;
        expect_near_rel(result, ref, tol);
    }

    // y + a + 5*d — one fused kernel (Metal JIT / CUDA tree eval), no per-node temps
    {
        constexpr T kFive = static_cast<T>(5);
        tensor<T>   gd(N, kActiveGpuDevice);
        gd.copy_from_host(hb);
        tensor<T> gx(N, kActiveGpuDevice);
        gx                    = ga + gb + kFive * gd;
        auto           result = gx.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + hb[i] + kFive * hb[i];
        expect_near_rel(result, ref, tol);

        tensor<T> acc(N, kActiveGpuDevice);
        acc.copy_from_host(ha);
        acc        = acc + gb + kFive * gd;
        auto acc_h = acc.to_host_vector();
        expect_near_rel(acc_h, ref, tol);
    }
}

// Ops that used to throw on Metal (min/max/pow/floor/if_else/...) now fuse like CPU/CUDA.
template <typename T>
void test_fused_catalog()
{
    constexpr size_t N   = 1024;
    constexpr double tol = std::is_same_v<T, float> ? 5e-4 : 1e-10;

    std::vector<T> ha(N), hb(N);
    for (size_t i = 0; i < N; ++i)
    {
        double t = static_cast<double>(i) / static_cast<double>(N);
        ha[i]    = static_cast<T>(t * 2.0 - 1.0);
        hb[i]    = static_cast<T>(t + 0.5);
    }

    tensor<T> ga(N, kActiveGpuDevice), gb(N, kActiveGpuDevice);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);

    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = min(ga, gb);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::min(ha[i], hb[i]);
        expect_near_rel(result, ref, tol);
    }
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = max(ga, gb) + ::floor(ga);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::max(ha[i], hb[i]) + std::floor(ha[i]);
        expect_near_rel(result, ref, tol);
    }
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::pow(gb, static_cast<T>(2)) + ::hypot(ga, gb);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::pow(hb[i], static_cast<T>(2)) + std::hypot(ha[i], hb[i]);
        expect_near_rel(result, ref, tol);
    }
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::if_else(ga > static_cast<T>(0), ga, gb);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] > static_cast<T>(0) ? ha[i] : hb[i];
        expect_near_rel(result, ref, tol);
    }
    {
        tensor<T> gc(N, kActiveGpuDevice);
        gc                    = ::cbrt(ga) + ::sqr(gb) * ::invsqrt(gb);
        auto           result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::cbrt(ha[i]) + std::sqr(hb[i]) * std::invsqrt(hb[i]);
        expect_near_rel(result, ref, tol);
    }
    {
        tensor<T> acc(N, kActiveGpuDevice);
        acc.copy_from_host(ha);
        acc += gb;
        auto           result = acc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + hb[i];
        expect_near_rel(result, ref, tol);
    }
}

}  // namespace

// --------------------------------------------------------------------------
// Scalar fill
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, FillFloat)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_fill<float>();
    END_TEST();
}

VECTORIZATIONTEST(TensorGpu, StoresDeviceIndex)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    tensor<float> t(64, kActiveGpuDevice, 0);
    EXPECT_EQ(kActiveGpuDevice, t.device());
    EXPECT_EQ(0, t.device_index());
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CopyClonesIndependentStorage)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";

    constexpr size_t N    = 64;
    constexpr float  kVal = 1.5f;
    tensor<float>    src(N, kActiveGpuDevice);
    src = kVal;
    tensor<float> dst(src);
    EXPECT_EQ(src.device(), dst.device());
    EXPECT_EQ(src.device_index(), dst.device_index());
    EXPECT_NE(src.data(), dst.data());
    expect_near_rel(dst.to_host_vector(), src.to_host_vector(), 5e-6);
    src        = static_cast<float>(9);
    auto src_h = src.to_host_vector();
    auto dst_h = dst.to_host_vector();
    for (size_t i = 0; i < N; ++i)
        EXPECT_NEAR(dst_h[i], kVal, 5e-6f);
    for (size_t i = 0; i < N; ++i)
        EXPECT_NEAR(src_h[i], 9.0f, 5e-6f);
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, ExpressionCtorKeepsDevice)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";

    constexpr size_t   N   = 64;
    constexpr double   tol = 5e-5;
    std::vector<float> ha, hb;
    make_inputs(ha, hb, N);
    tensor<float> ga(N, kActiveGpuDevice), gb(N, kActiveGpuDevice);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);
    tensor<float> gc = ga + gb;
    EXPECT_EQ(kActiveGpuDevice, gc.device());
    EXPECT_EQ(0, gc.device_index());
    std::vector<float> ref(N);
    for (size_t i = 0; i < N; ++i)
        ref[i] = ha[i] + hb[i];
    expect_near_rel(gc.to_host_vector(), ref, tol);
    auto cloned = gc.clone();
    EXPECT_EQ(gc.device(), cloned.device());
    EXPECT_NE(gc.data(), cloned.data());
    expect_near_rel(cloned.to_host_vector(), gc.to_host_vector(), 5e-6);
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, ExpressionLeavesAliasStorage)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";

    constexpr size_t N = 64;
    tensor<float>    ga(N, kActiveGpuDevice), gb(N, kActiveGpuDevice);
    ga     = 1.5f;
    gb     = 2.5f;
    auto e = ga + gb;
    EXPECT_EQ(e.lhs().data(), ga.data());
    EXPECT_EQ(e.rhs().data(), gb.data());
    tensor<float> gc = e;
    EXPECT_EQ(kActiveGpuDevice, gc.device());
    std::vector<float> got = gc.to_host_vector();
    for (size_t i = 0; i < N; ++i)
        EXPECT_NEAR(got[i], 4.0f, 5e-6f);
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, LinspaceAndToCpu)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";

    tensor<float> g(0.0f, 4.0f, 5u, kActiveGpuDevice);
    EXPECT_EQ(g.device(), kActiveGpuDevice);
    tensor<float> h = g.to_cpu();
    EXPECT_EQ(h.device(), device_enum::CPU);
    EXPECT_NE(h.data(), g.data());
    EXPECT_EQ(h.size(), 5u);
    EXPECT_NEAR(h[0], 0.0f, 5e-6f);
    EXPECT_NEAR(h[2], 2.0f, 5e-6f);
    EXPECT_NEAR(h[4], 4.0f, 5e-6f);
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FillDouble)
{
    if (kMetalOnlyBackend)
        GTEST_SKIP() << "Metal backend is float-only (no fp64 on Apple GPUs)";
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_fill<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Binary ops
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, BinaryOpsFloat)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_binary_ops<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, BinaryOpsDouble)
{
    if (kMetalOnlyBackend)
        GTEST_SKIP() << "Metal backend is float-only (no fp64 on Apple GPUs)";
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_binary_ops<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Unary math
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, UnaryMathFloat)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_unary_math<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, UnaryMathDouble)
{
    if (kMetalOnlyBackend)
        GTEST_SKIP() << "Metal backend is float-only (no fp64 on Apple GPUs)";
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_unary_math<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Compound (fused) expressions
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, CompoundFloat)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_compound<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FusedCatalogFloat)
{
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_fused_catalog<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CompoundDouble)
{
    if (kMetalOnlyBackend)
        GTEST_SKIP() << "Metal backend is float-only (no fp64 on Apple GPUs)";
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_compound<double>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FusedCatalogDouble)
{
    if (kMetalOnlyBackend)
        GTEST_SKIP() << "Metal backend is float-only (no fp64 on Apple GPUs)";
    int ndev = 0;
    gpuGetDeviceCount(&ndev);
    if (ndev == 0)
        GTEST_SKIP() << "No GPU device";
    test_fused_catalog<double>();
    END_TEST();
}

#else  // !(VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP || VECTORIZATION_HAS_METAL)

VECTORIZATIONTEST(TensorGpu, FillFloat)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, StoresDeviceIndex)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CopyClonesIndependentStorage)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, ExpressionCtorKeepsDevice)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, ExpressionLeavesAliasStorage)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, LinspaceAndToCpu)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FillDouble)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, BinaryOpsFloat)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, BinaryOpsDouble)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, UnaryMathFloat)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, UnaryMathDouble)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CompoundFloat)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FusedCatalogFloat)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CompoundDouble)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FusedCatalogDouble)
{
    GTEST_SKIP() << "Test disabled: no GPU backend (CUDA/HIP/Metal) is enabled";
    END_TEST();
}

#endif  // VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP || VECTORIZATION_HAS_METAL
