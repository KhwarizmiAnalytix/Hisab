/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Tensor correctness tests against LibTorch (PyTorch C++ frontend).
 *
 * Each test:
 *   1. Creates the same data in both XSigma tensor<T> and torch::Tensor.
 *   2. Applies the same operation.
 *   3. Compares results element-wise within a tolerance.
 *
 * Guard: compiled only when VECTORIZATION_HAS_LIBTORCH=1 (set by CMake when
 *        find_package(Torch) succeeds and VECTORIZATION_ENABLE_LIBTORCH=ON).
 */

#include "VectorizationTest.h"

#if VECTORIZATION_HAS_LIBTORCH

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

// LibTorch — must come before any header that pulls <cassert> on MSVC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <torch/torch.h>
#pragma GCC diagnostic pop

#include "terminals/tensor.h"

namespace
{

// ── helpers ──────────────────────────────────────────────────────────────────

template <typename T>
struct TorchDtype
{
    static constexpr auto value = torch::kFloat32;
};
template <>
struct TorchDtype<double>
{
    static constexpr auto value = torch::kFloat64;
};

template <typename T>
std::vector<T> rand_vec(std::size_t n, T lo, T hi, unsigned seed = 42)
{
    std::mt19937                      gen(seed);
    std::uniform_real_distribution<T> dist(lo, hi);
    std::vector<T>                    v(n);
    for (auto& x : v)
        x = dist(gen);
    return v;
}

// Compare XSigma tensor result vs torch result element-wise.
// Returns max absolute difference.
template <typename T>
double max_diff(const vectorization::tensor<T>& xs, const torch::Tensor& th)
{
    auto     th_cpu = th.cpu().to(TorchDtype<T>::value).contiguous();
    const T* ptr    = reinterpret_cast<const T*>(th_cpu.data_ptr());
    double   d      = 0.0;
    for (std::size_t i = 0; i < xs.size(); ++i)
        d = std::max(d, static_cast<double>(std::fabs(xs.data()[i] - ptr[i])));
    return d;
}

// Build a torch::Tensor (CPU) from a raw pointer + size.
template <typename T>
torch::Tensor to_torch(const T* data, std::size_t n)
{
    return torch::from_blob(
               const_cast<T*>(data),
               {static_cast<int64_t>(n)},
               torch::TensorOptions().dtype(TorchDtype<T>::value).device(torch::kCPU))
        .clone();
}

template <typename T>
torch::Tensor to_torch_2d(const T* data, std::size_t rows, std::size_t cols)
{
    return torch::from_blob(
               const_cast<T*>(data),
               {static_cast<int64_t>(rows), static_cast<int64_t>(cols)},
               torch::TensorOptions().dtype(TorchDtype<T>::value).device(torch::kCPU))
        .clone();
}

// ── per-type test suites ──────────────────────────────────────────────────────

// Core elementwise ops: arithmetic, basic transcendentals
template <typename T>
void test_libtorch_elementwise()
{
    using tensor_t            = vectorization::tensor<T>;
    constexpr std::size_t n   = 512 + 7;  // deliberate non-multiple of SIMD width
    constexpr double      tol = std::is_same_v<T, float> ? 1e-5 : 1e-13;

    auto ax = rand_vec<T>(n, T(-4), T(4), 1);
    auto ay = rand_vec<T>(n, T(-4), T(4), 2);
    auto az = rand_vec<T>(n, T(1), T(4), 3);  // positive, for log/sqrt/div

    tensor_t xa(ax.data(), n);
    tensor_t xb(ay.data(), n);
    tensor_t xc(az.data(), n);

    auto ta = to_torch(ax.data(), n);
    auto tb = to_torch(ay.data(), n);
    auto tc = to_torch(az.data(), n);

    // --- add ---
    {
        tensor_t out(n);
        out = xa + xb;
        EXPECT_LT(max_diff(out, ta + tb), tol) << "add failed";
    }
    // --- sub ---
    {
        tensor_t out(n);
        out = xa - xb;
        EXPECT_LT(max_diff(out, ta - tb), tol) << "sub failed";
    }
    // --- mul ---
    {
        tensor_t out(n);
        out = xa * xb;
        EXPECT_LT(max_diff(out, ta * tb), tol) << "mul failed";
    }
    // --- div (xc > 0, no divide-by-zero) ---
    {
        tensor_t out(n);
        out = xa / xc;
        EXPECT_LT(max_diff(out, ta / tc), tol) << "div failed";
    }
    // --- exp ---
    {
        tensor_t out(n);
        out = ::exp(xa);
        EXPECT_LT(max_diff(out, torch::exp(ta)), tol * 10) << "exp failed";
    }
    // --- log ---
    {
        tensor_t out(n);
        out = ::log(xc);
        EXPECT_LT(max_diff(out, torch::log(tc)), tol * 10) << "log failed";
    }
    // --- log2 ---
    {
        tensor_t out(n);
        out = ::log2(xc);
        EXPECT_LT(max_diff(out, torch::log2(tc)), tol * 10) << "log2 failed";
    }
    // --- log10 ---
    {
        tensor_t out(n);
        out = ::log10(xc);
        EXPECT_LT(max_diff(out, torch::log10(tc)), tol * 10) << "log10 failed";
    }
    // --- sqrt ---
    {
        tensor_t out(n);
        out = ::sqrt(xc);
        EXPECT_LT(max_diff(out, torch::sqrt(tc)), tol) << "sqrt failed";
    }
    // --- sin ---
    {
        tensor_t out(n);
        out = ::sin(xa);
        EXPECT_LT(max_diff(out, torch::sin(ta)), tol * 10) << "sin failed";
    }
    // --- cos ---
    {
        tensor_t out(n);
        out = ::cos(xa);
        EXPECT_LT(max_diff(out, torch::cos(ta)), tol * 10) << "cos failed";
    }
    // --- tanh ---
    {
        tensor_t out(n);
        out = ::tanh(xa);
        EXPECT_LT(max_diff(out, torch::tanh(ta)), tol * 10) << "tanh failed";
    }
    // --- abs ---
    {
        tensor_t out(n);
        out = ::fabs(xa);
        EXPECT_LT(max_diff(out, torch::abs(ta)), tol) << "abs failed";
    }
    // --- neg ---
    {
        tensor_t out(n);
        out = -xa;
        EXPECT_LT(max_diff(out, -ta), tol) << "neg failed";
    }
}

// Binary math functions: pow, element-wise min/max, fused multiply-add
template <typename T>
void test_libtorch_binary_fns()
{
    using tensor_t          = vectorization::tensor<T>;
    constexpr std::size_t n = 512 + 7;
    // pow needs looser tolerance: compound rounding over base^exp
    constexpr double tol     = std::is_same_v<T, float> ? 1e-4 : 1e-12;
    constexpr double tol_pow = std::is_same_v<T, float> ? 2e-3 : 1e-11;

    auto ax = rand_vec<T>(n, T(-3), T(3), 4);
    auto ay = rand_vec<T>(n, T(-3), T(3), 5);
    auto az = rand_vec<T>(n, T(0.5), T(4), 6);  // positive base for pow

    tensor_t xa(ax.data(), n);
    tensor_t xb(ay.data(), n);
    tensor_t xc(az.data(), n);  // positive base

    auto ta = to_torch(ax.data(), n);
    auto tb = to_torch(ay.data(), n);
    auto tc = to_torch(az.data(), n);

    // --- pow(positive_base, exponent) ---
    {
        tensor_t out(n);
        out = ::pow(xc, xb);
        EXPECT_LT(max_diff(out, torch::pow(tc, tb)), tol_pow) << "pow failed";
    }
    // --- element-wise min ---
    {
        tensor_t out(n);
        out = ::min(xa, xb);
        EXPECT_LT(max_diff(out, torch::minimum(ta, tb)), tol) << "element-wise min failed";
    }
    // --- element-wise max ---
    {
        tensor_t out(n);
        out = ::max(xa, xb);
        EXPECT_LT(max_diff(out, torch::maximum(ta, tb)), tol) << "element-wise max failed";
    }
    // --- fused multiply-add: a*b + c ---
    {
        tensor_t out(n);
        out = xa * xb + xc;
        // torch::addcmul computes: self + value * t1 * t2  (value defaults to 1)
        EXPECT_LT(max_diff(out, torch::addcmul(tc, ta, tb)), tol) << "fma (a*b+c) failed";
    }
}

// 2-D tensor ops: verify that elementwise results are shape-independent
template <typename T>
void test_libtorch_2d()
{
    using tensor_t             = vectorization::tensor<T>;
    constexpr std::size_t rows = 32;
    constexpr std::size_t cols = 33;  // non-multiple of common SIMD widths
    constexpr std::size_t n    = rows * cols;
    constexpr double      tol  = std::is_same_v<T, float> ? 1e-5 : 1e-13;

    auto ax = rand_vec<T>(n, T(-2), T(2), 7);
    auto ay = rand_vec<T>(n, T(-2), T(2), 8);
    auto az = rand_vec<T>(n, T(0.5), T(3), 9);  // positive for log/sqrt

    tensor_t xa(ax.data(), rows, cols);
    tensor_t xb(ay.data(), rows, cols);
    tensor_t xc(az.data(), rows, cols);

    auto ta = to_torch_2d(ax.data(), rows, cols);
    auto tb = to_torch_2d(ay.data(), rows, cols);
    auto tc = to_torch_2d(az.data(), rows, cols);

    // --- add 2-D ---
    {
        tensor_t out(rows, cols);
        out = xa + xb;
        EXPECT_LT(max_diff(out, ta + tb), tol) << "2D add failed";
    }
    // --- sub 2-D ---
    {
        tensor_t out(rows, cols);
        out = xa - xb;
        EXPECT_LT(max_diff(out, ta - tb), tol) << "2D sub failed";
    }
    // --- mul 2-D ---
    {
        tensor_t out(rows, cols);
        out = xa * xb;
        EXPECT_LT(max_diff(out, ta * tb), tol) << "2D mul failed";
    }
    // --- exp 2-D ---
    {
        tensor_t out(rows, cols);
        out = ::exp(xc);
        EXPECT_LT(max_diff(out, torch::exp(tc)), tol * 10) << "2D exp failed";
    }
    // --- sqrt 2-D ---
    {
        tensor_t out(rows, cols);
        out = ::sqrt(xc);
        EXPECT_LT(max_diff(out, torch::sqrt(tc)), tol) << "2D sqrt failed";
    }
    // --- tanh 2-D ---
    {
        tensor_t out(rows, cols);
        out = ::tanh(xa);
        EXPECT_LT(max_diff(out, torch::tanh(ta)), tol * 10) << "2D tanh failed";
    }
}

template <typename T>
void test_libtorch_gpu()
{
    if (!torch::cuda::is_available())
    {
        GTEST_SKIP() << "CUDA not available — skipping GPU comparison test";
        return;
    }

    constexpr std::size_t n   = 1024;
    constexpr double      tol = std::is_same_v<T, float> ? 1e-5 : 1e-12;

    auto ax = rand_vec<T>(n, T(-2), T(2), 10);
    auto ay = rand_vec<T>(n, T(-2), T(2), 11);

    // XSigma runs on CPU
    vectorization::tensor<T> xa(ax.data(), n);
    vectorization::tensor<T> xb(ay.data(), n);
    vectorization::tensor<T> out_cpu(n);
    out_cpu = xa + xb;

    // Torch runs on CUDA
    auto ta_gpu  = to_torch(ax.data(), n).to(torch::kCUDA);
    auto tb_gpu  = to_torch(ay.data(), n).to(torch::kCUDA);
    auto out_gpu = (ta_gpu + tb_gpu).cpu();

    EXPECT_LT(max_diff(out_cpu, out_gpu), tol) << "CPU vs GPU add mismatch";

    // exp on GPU
    vectorization::tensor<T> out_exp(n);
    out_exp       = ::exp(xa);
    auto texp_gpu = torch::exp(ta_gpu).cpu();
    EXPECT_LT(max_diff(out_exp, texp_gpu), tol * 10) << "CPU exp vs GPU exp mismatch";
}

}  // namespace

// ── test entry points ─────────────────────────────────────────────────────────

VECTORIZATIONTEST(LibTorch, ElementwiseFloat)
{
    test_libtorch_elementwise<float>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, ElementwiseDouble)
{
    test_libtorch_elementwise<double>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, BinaryFnsFloat)
{
    test_libtorch_binary_fns<float>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, BinaryFnsDouble)
{
    test_libtorch_binary_fns<double>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, TwoDimFloat)
{
    test_libtorch_2d<float>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, TwoDimDouble)
{
    test_libtorch_2d<double>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, GpuVsCpuFloat)
{
    test_libtorch_gpu<float>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, GpuVsCpuDouble)
{
    test_libtorch_gpu<double>();
    END_TEST();
}

#else  // !VECTORIZATION_HAS_LIBTORCH

#include "VectorizationTest.h"

VECTORIZATIONTEST(LibTorch, ElementwiseFloat)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, ElementwiseDouble)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, BinaryFnsFloat)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, BinaryFnsDouble)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, TwoDimFloat)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, TwoDimDouble)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, GpuVsCpuFloat)
{
    END_TEST();
}
VECTORIZATIONTEST(LibTorch, GpuVsCpuDouble)
{
    END_TEST();
}

#endif  // VECTORIZATION_HAS_LIBTORCH
