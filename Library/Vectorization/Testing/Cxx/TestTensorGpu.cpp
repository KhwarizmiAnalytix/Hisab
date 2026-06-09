/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Integration tests for tensor expression evaluation on the GPU backend.
 *
 * Tensors are allocated with device_enum::CUDA so that
 * expressions_evaluator::run dispatches to run_gpu (a grid of one thread per
 * element).  Results are copied back to the host via tensor::to_host_vector()
 * and compared against std:: math.
 *
 * Tests are skipped automatically when no CUDA device is present at runtime.
 *
 * This file is compiled as a CUDA translation unit (CMake sets LANGUAGE CUDA
 * on it) so that GPU expression-template kernels are instantiated correctly,
 * while remaining a plain .cpp source that needs no __CUDACC__ guards.
 */

#include "VectorizationTest.h"

#if VECTORIZATION_HAS_CUDA

#include <cuda_runtime.h>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "terminals/tensor.h"

namespace
{

using namespace vectorization;

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
        EXPECT_LE(err, allowed)
            << " at i=" << i << "  got=" << got[i] << "  ref=" << ref[i];
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
        hb[i]    = static_cast<T>(t + 0.5);          // [0.5, 1.5)
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

    tensor<T> ga(N, device_enum::CUDA);
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

    tensor<T> ga(N, device_enum::CUDA), gb(N, device_enum::CUDA);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);

    // Add
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ga + gb;  // → run_gpu(add_expr, gc.data(), N)
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Subtract
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ga - gb;
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] - hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Element-wise multiply (operator* is element-wise for 1-D tensors)
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ga * gb;
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] * hb[i];
        expect_near_rel(result, ref, tol);
    }

    // Scalar + tensor
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ga + static_cast<T>(1);
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = ha[i] + static_cast<T>(1);
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

    tensor<T> ga(N, device_enum::CUDA);
    ga.copy_from_host(ha);

    // exp
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ::exp(ga);
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::exp(ha[i]);
        expect_near_rel(result, ref, tol);
    }

    // sqrt
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ::sqrt(ga);
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::sqrt(ha[i]);
        expect_near_rel(result, ref, tol);
    }

    // log
    {
        tensor<T>      gc(N, device_enum::CUDA);
        gc = ::log(ga);
        auto result = gc.to_host_vector();
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
        tensor<T> ga_neg(N, device_enum::CUDA);
        ga_neg.copy_from_host(ha_neg);

        tensor<T> gc(N, device_enum::CUDA);
        gc = ::fabs(ga_neg);
        auto result = gc.to_host_vector();
        for (size_t i = 0; i < N; ++i)
            EXPECT_NEAR(result[i], ha[i], static_cast<T>(tol)) << " at i=" << i;
    }
}

// Compound expression: the full expression tree is fused into a single kernel.
template <typename T>
void test_compound()
{
    constexpr size_t N   = 1024;
    // exp amplifies float errors; use a looser tolerance
    constexpr double tol = std::is_same_v<T, float> ? 5e-4 : 1e-10;

    std::vector<T> ha(N), hb(N);
    for (size_t i = 0; i < N; ++i)
    {
        double t = static_cast<double>(i) / static_cast<double>(N);
        ha[i]    = static_cast<T>(t * 2.0 - 1.0);  // [-1, 1)
        hb[i]    = static_cast<T>(t + 0.5);          // [0.5, 1.5)
    }

    tensor<T> ga(N, device_enum::CUDA), gb(N, device_enum::CUDA);
    ga.copy_from_host(ha);
    gb.copy_from_host(hb);

    // exp(a) + sqrt(b): fused into one run_gpu call
    {
        tensor<T> gc(N, device_enum::CUDA);
        gc = ::exp(ga) + ::sqrt(gb);
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = std::exp(ha[i]) + std::sqrt(hb[i]);
        expect_near_rel(result, ref, tol);
    }

    // (a + b) * scalar: mixed tensor and scalar operands
    {
        constexpr T    kAlpha = static_cast<T>(1.5);
        tensor<T>      gc(N, device_enum::CUDA);
        gc = (ga + gb) * kAlpha;
        auto result = gc.to_host_vector();
        std::vector<T> ref(N);
        for (size_t i = 0; i < N; ++i)
            ref[i] = (ha[i] + hb[i]) * kAlpha;
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
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_fill<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, FillDouble)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_fill<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Binary ops
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, BinaryOpsFloat)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_binary_ops<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, BinaryOpsDouble)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_binary_ops<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Unary math
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, UnaryMathFloat)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_unary_math<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, UnaryMathDouble)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_unary_math<double>();
    END_TEST();
}

// --------------------------------------------------------------------------
// Compound (fused) expressions
// --------------------------------------------------------------------------
VECTORIZATIONTEST(TensorGpu, CompoundFloat)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_compound<float>();
    END_TEST();
}
VECTORIZATIONTEST(TensorGpu, CompoundDouble)
{
    int ndev = 0;
    cudaGetDeviceCount(&ndev);
    if (ndev == 0) GTEST_SKIP() << "No CUDA device";
    test_compound<double>();
    END_TEST();
}

#else  // !VECTORIZATION_HAS_CUDA

VECTORIZATIONTEST(TensorGpu, FillFloat)               { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, FillDouble)              { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, BinaryOpsFloat)          { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, BinaryOpsDouble)         { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, UnaryMathFloat)          { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, UnaryMathDouble)         { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, CompoundFloat)           { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }
VECTORIZATIONTEST(TensorGpu, CompoundDouble)          { GTEST_SKIP() << "Test disabled: VECTORIZATION_HAS_CUDA is OFF"; END_TEST(); }

#endif  // VECTORIZATION_HAS_CUDA
