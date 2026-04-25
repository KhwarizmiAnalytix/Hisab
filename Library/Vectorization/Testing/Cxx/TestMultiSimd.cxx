/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of Quarisma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "common/configure.h"
#include "common/constants.h"
#include "common/macros.h"
#include "common/packet.h"
#include "memory/allocator.h"
#include "terminals/vector.h"
#include "xsigmaTest.h"
#include "xsigmaTestingHelper.h"

#define DEBUG_SIMD_TEST

#define MACRO_TEST_SIMD_FUNC(op, op_inv)                                \
    struct func_##op                                                    \
    {                                                                   \
        template <typename T>                                           \
        VECTORIZATION_FORCE_INLINE static void run(T const& a, T const&, T& c) \
        {                                                               \
            if constexpr (quarisma::is_fundamental<T>::value)             \
                c = std::op_inv(std::op(a)) / a;                        \
            else                                                        \
            {                                                           \
                c = op_inv(op(a)) / a;                                  \
            }                                                           \
        };                                                              \
    }

#define MACRO_TEST_SIMD_FUNC2(op)                                         \
    struct func_##op                                                      \
    {                                                                     \
        template <typename T>                                             \
        VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c) \
        {                                                                 \
            if constexpr (quarisma::is_fundamental<T>::value)               \
                c = std::op(a, b);                                        \
            else                                                          \
                c = op(a, b);                                             \
        };                                                                \
    }

namespace quarisma
{
MACRO_TEST_SIMD_FUNC(sqr, sqrt);
MACRO_TEST_SIMD_FUNC(invsqrt, invsqrt);
MACRO_TEST_SIMD_FUNC(exp, log);
MACRO_TEST_SIMD_FUNC(exp2, log2);
MACRO_TEST_SIMD_FUNC(expm1, log1p);
MACRO_TEST_SIMD_FUNC(sin, asin);
MACRO_TEST_SIMD_FUNC(cos, acos);
MACRO_TEST_SIMD_FUNC(atan, tan);
MACRO_TEST_SIMD_FUNC(sinh, asinh);
MACRO_TEST_SIMD_FUNC(cosh, acosh);
MACRO_TEST_SIMD_FUNC(tanh, atanh);
MACRO_TEST_SIMD_FUNC(cdf, inv_cdf);
MACRO_TEST_SIMD_FUNC(ceil, floor);
MACRO_TEST_SIMD_FUNC(trunc, neg);
MACRO_TEST_SIMD_FUNC(fabs, fabs);
MACRO_TEST_SIMD_FUNC(cbrt, sqr);

MACRO_TEST_SIMD_FUNC2(min);
MACRO_TEST_SIMD_FUNC2(max);
MACRO_TEST_SIMD_FUNC2(pow);
MACRO_TEST_SIMD_FUNC2(hypot);
MACRO_TEST_SIMD_FUNC2(copysign);

struct func_sum
{
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& x, T const& y, T& z)
    {
        if constexpr (quarisma::is_fundamental<T>::value)
        {
            z = x * y - x + std::fma(x, y, x);
        }
        else
        {
            z = x * y - x + fma(x, y, x);
        }
    };
};

struct func_mixed_formula
{
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c)
    {
        if constexpr (quarisma::is_fundamental<T>::value)
        {
            auto t = -fabs(a - b) + a + b;
            c      = floor(log1p(fabs(t)) + 1.) + ceil(a) + sin(t) + trunc(a) +
                (fabs(t) < std::numeric_limits<double>::epsilon() ? 1. - 0.5 * t : expm1(t) / t);
        }
        else
        {
            auto t = -fabs(a - b) + a + b;
            c      = floor(log1p(fabs(t)) + 1.) + ceil(a) + sin(t) + trunc(a) +
                if_else(
                    fabs(t) < std::numeric_limits<double>::epsilon(), 1. - 0.5 * t, expm1(t) / t);
        }
    };
};

struct func_mixed_if_else
{
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c)
    {
        double dcf = 0.25;
        if constexpr (std::is_fundamental<T>::value)
        {
            auto x = -dcf * std::fabs(-a + b);
            c      = ((std::fabs(x) < std::numeric_limits<T>::epsilon()) ? 0. : x / x);
        }
        else
        {
            auto x = -dcf * fabs(-a + b);
            c      = if_else(fabs(x) < std::numeric_limits<double>::epsilon(), 0., x / x);
        }
    };
};
struct func_fma
{
    template <typename T1, typename T2, typename T3, typename T4>
    VECTORIZATION_FORCE_INLINE static void run(T1 const& a, T2 const& b, T3 const& c, T4& d)
    {
        if constexpr (
            quarisma::is_fundamental<T1>::value && quarisma::is_fundamental<T2>::value &&
            quarisma::is_fundamental<T3>::value)
            d = std::fma(a, b, c);
        else
            d = fma(a, b, c);
    };
};
}  // namespace quarisma

template <typename>
struct packetTolerance
{
};

template <>
struct packetTolerance<float>
{
    static constexpr float tolerance = (float)7.e-4;
};

template <>
struct packetTolerance<double>
{
    static constexpr double tolerance = 5.e-12;
};

template <typename T, typename F>
void TestFunction(
    size_t n, T const* x, T const* y, T* z_v, T* z_l, T tolerance = packetTolerance<T>::tolerance)
{
    quarisma::vector<T> a(x, n);
    quarisma::vector<T> b(y, n);
    quarisma::vector<T> c_v(z_v, n);

    F::run(a, b, c_v);

    for (size_t i = 0; i < n; ++i)
        F::run(x[i], y[i], z_l[i]);

    quarisma::vector<T> c_l(z_l, n);
    quarisma::vector<T> diff(n);

    diff = fabs(c_v - c_l);

    auto max_error = quarisma::hmax(fabs(if_else(
        fabs(c_v) < std::numeric_limits<T>::epsilon() &&
            fabs(c_l) < std::numeric_limits<T>::epsilon(),
        0.,
        c_v / c_l - 1.)));

#ifdef DEBUG_SIMD_TEST
    VECTORIZATION_LOGF(INFO, "[%s]:   max error(%.2e)", MACRO_CORE_TYPE_ID_NAME(F), float(max_error));
#endif  // DEBUG_SIMD_TEST

    EXPECT_LE(max_error, tolerance);
}

template <typename T>
void TestFMA(
    size_t n, T const* x, T const* y, T* z_v, T* z_l, T tolerance = packetTolerance<T>::tolerance)
{
    quarisma::vector<T> a(x, n);
    quarisma::vector<T> c_v(z_v, n);
    {
        quarisma::vector<T> b(y, n);
        quarisma::vector<T> e(y, n);
        quarisma::func_fma::run(a, b, e, c_v);

        for (size_t i = 0; i < n; ++i)
            quarisma::func_fma::run(x[i], y[i], y[i], z_l[i]);
    }
    /*{
        T b = 0.5;
        T e = 1.5;
        quarisma::func_fma::run(a, b, e, c_v);

        for (size_t i = 0; i < n; ++i)
            quarisma::func_fma::run(x[i], b, e, z_l[i]);
    }*/

    quarisma::vector<T> c_l(z_l, n);
    quarisma::vector<T> diff(n);

    diff = fabs(c_v - c_l);

    auto max_error = quarisma::hmax(fabs(if_else(
        fabs(c_v) < std::numeric_limits<T>::epsilon() &&
            fabs(c_l) < std::numeric_limits<T>::epsilon(),
        0.,
        c_v / c_l - 1.)));

#ifdef DEBUG_SIMD_TEST
    VECTORIZATION_LOGF(
        INFO,
        "[%s]:   max error(%.2e)",
        MACRO_CORE_TYPE_ID_NAME(quarisma::func_fma),
        float(max_error));
#endif  // DEBUG_SIMD_TEST

    EXPECT_LE(max_error, tolerance);
}

template <typename T>
void TestHorizontalFunction(size_t n, T const* x, T const* y)
{
    quarisma::vector<T> a(x, n);
    quarisma::vector<T> b(y, n);

    auto ret_v_min = quarisma::hmin(a);
    auto ret_v_max = quarisma::hmax(b);
    auto ret_v_sum = quarisma::accumulate(a);

    auto   ret_l_min = x[0];
    auto   ret_l_max = y[0];
    double ret_l_sum = 0.;

    for (size_t i = 0; i < n; ++i)
    {
        ret_l_min = std::min(ret_l_min, x[i]);
        ret_l_max = std::max(ret_l_max, y[i]);
        ret_l_sum += x[i];
    }

#ifdef DEBUG_SIMD_TEST
    VECTORIZATION_LOGF(
        INFO,
        "hmin       [%s]:   max error(%.2e)",
        MACRO_CORE_TYPE_ID_NAME(T),
        float(ret_v_min - ret_l_min));
    VECTORIZATION_LOGF(
        INFO,
        "hmax       [%s]:   max error(%.2e)",
        MACRO_CORE_TYPE_ID_NAME(T),
        float(ret_v_max - ret_l_max));
    VECTORIZATION_LOGF(
        INFO,
        "accumulate [%s]:   max error(%.2e)",
        MACRO_CORE_TYPE_ID_NAME(T),
        float(ret_v_sum - ret_l_sum));
#endif

    EXPECT_EQ(std::fabs(ret_v_min - ret_l_min), 0.);
    EXPECT_EQ(std::fabs(ret_v_max - ret_l_max), 0.);
    EXPECT_LT(std::fabs(ret_v_sum - ret_l_sum), packetTolerance<T>::tolerance);
}

namespace
{
template <typename T>
void test_packet_functions()
{
    /*constexpr uint32_t N_alignment = quarisma::packet<T>::alignment();
    EXPECT_EQ(N_alignment, 64);*/

    const size_t n = (2 << 8) + 3;

    using allocator_t = typename quarisma::allocator<T>;
    using value_t     = T;

    auto* x   = allocator_t::allocate(n);
    auto* y   = allocator_t::allocate(n);
    auto* z_v = allocator_t::allocate(n);
    auto* z_l = allocator_t::allocate(n);

    auto dt = 10. / n;
    for (size_t i = 0; i < n; ++i)
    {
        x[i] = static_cast<value_t>(-5. + static_cast<value_t>(i) * dt);
        y[i] = static_cast<value_t>(4. - 0.8 * static_cast<value_t>(i) * dt);
    }

    quarisma::vector<T> a(x, n);
    quarisma::vector<T> b(y, n);
    quarisma::vector<T> c(z_v, n);

    auto alpha = static_cast<value_t>(0.5);

    c = min(alpha, a);
    c = min(a, alpha);

    c = fma(alpha, a, b);
    c = fma(alpha, alpha, a);
    c = fma(alpha, a, alpha);
    c = fma(a, alpha, alpha);

    c = if_else(a > alpha, alpha, a);
    c = if_else(a > alpha, a, alpha);

    auto t = a - b;

    EXPECT_EQ(t.length(), quarisma::vector<T>::length());
    EXPECT_EQ(quarisma::accumulate(if_else(a > alpha, alpha, a)), quarisma::accumulate(min(alpha, a)));
    EXPECT_LE(quarisma::accumulate(t), n);

    TestFunction<value_t, quarisma::func_sum>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_sqr>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_invsqrt>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_exp>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_exp2>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_expm1>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_sin>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_cos>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_atan>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_sinh>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_cosh>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_tanh>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_cdf>(n, x, y, z_v, z_l, (value_t)5.e-4);
    TestFunction<value_t, quarisma::func_ceil>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_trunc>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_fabs>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_cbrt>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_min>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_max>(n, x, y, z_v, z_l);
    //TestFunction<value_t, quarisma::func_pow>(n, x, y, z_v, z_l);

    TestFunction<value_t, quarisma::func_copysign>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_hypot>(n, x, y, z_v, z_l);

    TestFMA<value_t>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_mixed_if_else>(n, x, y, z_v, z_l);
    TestFunction<value_t, quarisma::func_mixed_formula>(n, x, y, z_v, z_l);

    TestHorizontalFunction(n, x, y);

    allocator_t::free(z_l);
    allocator_t::free(z_v);
    allocator_t::free(y);
    allocator_t::free(x);
}
}  // namespace

VECTORIZATIONTEST(Math, MultiSimd)
{
#ifdef VECTORIZATION_VECTORIZED

    test_packet_functions<float>();
    test_packet_functions<double>();

#endif  // VECTORIZATION_VECTORIZED
    END_TEST();
}
