/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive function-coverage tests against LibTorch (42 comparisons).
 * Every free function on tensor_t that has a direct LibTorch equivalent is
 * tested element-wise.
 *
 * Input domains used:
 *   xa / ta    general [-3, 3]
 *   xb / tb    independent general [-3, 3]
 *   xpos/tpos  positive [0.5, 4]    — log, sqrt, cbrt, invsqrt, pow base, /
 *   xunit/tunit |x| < 1             — asin, acos, atanh
 *   xge1/tge1  x >= 1               — acosh
 *   xsm/tsm    [-0.5, 0.5]          — tan, sinh, cosh
 */

#include "VectorizationTest.h"

#if VECTORIZATION_HAS_LIBTORCH

#include "TestTensorLibTorchHelpers.h"

namespace
{
using namespace libtorch_test;

template <typename T>
void test_libtorch_all_functions()
{
    using tensor_t = vectorization::tensor<T>;

    constexpr std::size_t n         = 512 + 7;
    constexpr double      tol       = std::is_same_v<T, float> ? 1e-5 : 1e-13;
    constexpr double      tol_loose = std::is_same_v<T, float> ? 1e-4 : 1e-11;
    constexpr double      tol_pow   = std::is_same_v<T, float> ? 2e-3 : 1e-11;

    auto a_gen  = rand_vec<T>(n, T(-3), T(3), 30);
    auto a_gen2 = rand_vec<T>(n, T(-3), T(3), 31);
    auto a_pos  = rand_vec<T>(n, T(0.5), T(4), 32);
    auto a_unit = rand_vec<T>(n, T(-0.9), T(0.9), 33);
    auto a_ge1  = rand_vec<T>(n, T(1.0), T(4), 34);
    auto a_sm   = rand_vec<T>(n, T(-0.5), T(0.5), 35);

    tensor_t xa(a_gen.data(), n);
    tensor_t xb(a_gen2.data(), n);
    tensor_t xpos(a_pos.data(), n);
    tensor_t xunit(a_unit.data(), n);
    tensor_t xge1(a_ge1.data(), n);
    tensor_t xsm(a_sm.data(), n);

    auto ta    = to_torch(a_gen.data(), n);
    auto tb    = to_torch(a_gen2.data(), n);
    auto tpos  = to_torch(a_pos.data(), n);
    auto tunit = to_torch(a_unit.data(), n);
    auto tge1  = to_torch(a_ge1.data(), n);
    auto tsm   = to_torch(a_sm.data(), n);

    // ── abs / neg / rounding ─────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::fabs(xa);
        EXPECT_LT(max_diff(r, torch::abs(ta)), tol) << "fabs";
    }
    {
        tensor_t r(n);
        r = -xa;
        EXPECT_LT(max_diff(r, -ta), tol) << "neg";
    }
    {
        tensor_t r(n);
        r = ::floor(xa);
        EXPECT_LT(max_diff(r, torch::floor(ta)), tol) << "floor";
    }
    {
        tensor_t r(n);
        r = ::ceil(xa);
        EXPECT_LT(max_diff(r, torch::ceil(ta)), tol) << "ceil";
    }
    {
        tensor_t r(n);
        r = ::trunc(xa);
        EXPECT_LT(max_diff(r, torch::trunc(ta)), tol) << "trunc";
    }

    // ── power / root ─────────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::sqrt(xpos);
        EXPECT_LT(max_diff(r, torch::sqrt(tpos)), tol) << "sqrt";
    }
    {
        tensor_t r(n);
        r = ::sqr(xa);
        EXPECT_LT(max_diff(r, ta * ta), tol) << "sqr";
    }
    {
        tensor_t r(n);
        r = ::invsqrt(xpos);
        EXPECT_LT(max_diff(r, T(1) / torch::sqrt(tpos)), tol_loose) << "invsqrt";
    }
    {
        tensor_t r(n);
        r = ::cbrt(xpos);
        EXPECT_LT(max_diff(r, torch::pow(tpos, T(1) / T(3))), tol_loose) << "cbrt";
    }

    // ── exponential ──────────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::exp(xa);
        EXPECT_LT(max_diff(r, torch::exp(ta)), tol_loose) << "exp";
    }
    {
        tensor_t r(n);
        r = ::expm1(xa);
        EXPECT_LT(max_diff(r, torch::expm1(ta)), tol_loose) << "expm1";
    }
    {
        tensor_t r(n);
        r = ::exp2(xa);
        EXPECT_LT(max_diff(r, torch::exp2(ta)), tol_loose) << "exp2";
    }

    // ── logarithm ────────────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::log(xpos);
        EXPECT_LT(max_diff(r, torch::log(tpos)), tol_loose) << "log";
    }
    {
        tensor_t r(n);
        r = ::log1p(xpos);
        EXPECT_LT(max_diff(r, torch::log1p(tpos)), tol_loose) << "log1p";
    }
    {
        tensor_t r(n);
        r = ::log2(xpos);
        EXPECT_LT(max_diff(r, torch::log2(tpos)), tol_loose) << "log2";
    }
    {
        tensor_t r(n);
        r = ::log10(xpos);
        EXPECT_LT(max_diff(r, torch::log10(tpos)), tol_loose) << "log10";
    }

    // ── trigonometry ─────────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::sin(xa);
        EXPECT_LT(max_diff(r, torch::sin(ta)), tol_loose) << "sin";
    }
    {
        tensor_t r(n);
        r = ::cos(xa);
        EXPECT_LT(max_diff(r, torch::cos(ta)), tol_loose) << "cos";
    }
    {
        tensor_t r(n);
        r = ::tan(xsm);
        EXPECT_LT(max_diff(r, torch::tan(tsm)), tol_loose) << "tan";
    }
    {
        tensor_t r(n);
        r = ::asin(xunit);
        EXPECT_LT(max_diff(r, torch::asin(tunit)), tol_loose) << "asin";
    }
    {
        tensor_t r(n);
        r = ::acos(xunit);
        EXPECT_LT(max_diff(r, torch::acos(tunit)), tol_loose) << "acos";
    }
    {
        tensor_t r(n);
        r = ::atan(xa);
        EXPECT_LT(max_diff(r, torch::atan(ta)), tol_loose) << "atan";
    }

    // ── hyperbolic ───────────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::sinh(xsm);
        EXPECT_LT(max_diff(r, torch::sinh(tsm)), tol_loose) << "sinh";
    }
    {
        tensor_t r(n);
        r = ::cosh(xsm);
        EXPECT_LT(max_diff(r, torch::cosh(tsm)), tol_loose) << "cosh";
    }
    {
        tensor_t r(n);
        r = ::tanh(xa);
        EXPECT_LT(max_diff(r, torch::tanh(ta)), tol_loose) << "tanh";
    }
    {
        tensor_t r(n);
        r = ::asinh(xa);
        EXPECT_LT(max_diff(r, torch::asinh(ta)), tol_loose) << "asinh";
    }
    {
        tensor_t r(n);
        r = ::acosh(xge1);
        EXPECT_LT(max_diff(r, torch::acosh(tge1)), tol_loose) << "acosh";
    }
    {
        tensor_t r(n);
        r = ::atanh(xunit);
        EXPECT_LT(max_diff(r, torch::atanh(tunit)), tol_loose) << "atanh";
    }

    // ── binary arithmetic ────────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = xa + xb;
        EXPECT_LT(max_diff(r, ta + tb), tol) << "+";
    }
    {
        tensor_t r(n);
        r = xa - xb;
        EXPECT_LT(max_diff(r, ta - tb), tol) << "-";
    }
    {
        tensor_t r(n);
        r = xa * xb;
        EXPECT_LT(max_diff(r, ta * tb), tol) << "*";
    }
    {
        tensor_t r(n);
        r = xa / xpos;
        EXPECT_LT(max_diff(r, ta / tpos), tol) << "/";
    }

    // ── binary math functions ────────────────────────────────────────────────
    {
        tensor_t r(n);
        r = ::pow(xpos, xb);
        EXPECT_LT(max_diff(r, torch::pow(tpos, tb)), tol_pow) << "pow";
    }
    {
        tensor_t r(n);
        r = ::hypot(xa, xb);
        EXPECT_LT(max_diff(r, torch::hypot(ta, tb)), tol) << "hypot";
    }
    {
        tensor_t r(n);
        r = ::min(xa, xb);
        EXPECT_LT(max_diff(r, torch::minimum(ta, tb)), tol) << "min";
    }
    {
        tensor_t r(n);
        r = ::max(xa, xb);
        EXPECT_LT(max_diff(r, torch::maximum(ta, tb)), tol) << "max";
    }
    {
        tensor_t r(n);
        r = ::copysign(xpos, xb);
        EXPECT_LT(max_diff(r, torch::copysign(tpos, tb)), tol) << "copysign";
    }

    // ── compound assignment (each uses an independent copy of a_gen) ─────────
    {
        auto     t = a_gen;
        tensor_t r(t.data(), n);
        r += xb;
        EXPECT_LT(max_diff(r, ta + tb), tol) << "+=";
    }
    {
        auto     t = a_gen;
        tensor_t r(t.data(), n);
        r -= xb;
        EXPECT_LT(max_diff(r, ta - tb), tol) << "-=";
    }
    {
        auto     t = a_gen;
        tensor_t r(t.data(), n);
        r *= xb;
        EXPECT_LT(max_diff(r, ta * tb), tol) << "*=";
    }
    {
        auto     t = a_gen;
        tensor_t r(t.data(), n);
        r /= xpos;
        EXPECT_LT(max_diff(r, ta / tpos), tol) << "/=";
    }

    // ── ternary: fma(a, b, c) = a*b + c ─────────────────────────────────────
    {
        tensor_t r(n);
        r = ::fma(xa, xb, xpos);
        // torch::addcmul(c, a, b) computes c + 1*a*b
        EXPECT_LT(max_diff(r, torch::addcmul(tpos, ta, tb)), tol_loose) << "fma";
    }
}

}  // namespace

VECTORIZATIONTEST(LibTorch, AllFunctionsFloat)
{
    test_libtorch_all_functions<float>();
    END_TEST();
}

VECTORIZATIONTEST(LibTorch, AllFunctionsDouble)
{
    test_libtorch_all_functions<double>();
    END_TEST();
}

#else

VECTORIZATIONTEST(LibTorch, AllFunctionsFloat)
{ END_TEST(); }
VECTORIZATIONTEST(LibTorch, AllFunctionsDouble)
{ END_TEST(); }

#endif  // VECTORIZATION_HAS_LIBTORCH
