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
#include "common/packet.h"
#include "memory/allocator.h"
#include "terminals/vector.h"
#include "xsigmaTest.h"
#include "xsigmaTestingHelper.h"

#ifdef VECTORIZATION_VECTORIZED
namespace
{
#define TEST_SIMD_FUNC_1_ARGS(op)                            \
    template <typename value_t>                              \
    void test_##op(value_t tolerance, value_t x = 0.1)       \
    {                                                        \
        constexpr size_t n = simd<value_t>::size;            \
        using simd_t       = typename simd<value_t>::simd_t; \
                                                             \
        std::array<value_t, n> out;                          \
                                                             \
        simd_t a;                                            \
        simd<value_t>::set(x, a);                            \
                                                             \
        simd_t c;                                            \
        simd<value_t>::setzero(c);                           \
                                                             \
        simd<value_t>::op(a, c);                             \
        auto z = std::op(x);                                 \
                                                             \
        simd<value_t>::storeu(c, out.data());                \
        for (auto e : out)                                   \
        {                                                    \
            VECTORIZATION_LOGF(INFO, "vec: %f seq: %f", e, z);      \
            EXPECT_LE(std::fabs(e - z), tolerance);          \
        }                                                    \
    }

#define TEST_SIMD_FUNC_2_ARGS(op)                            \
    template <typename value_t>                              \
    void test_##op(value_t tolerance)                        \
    {                                                        \
        constexpr size_t n = simd<value_t>::size;            \
        using simd_t       = typename simd<value_t>::simd_t; \
                                                             \
        std::array<value_t, n> out;                          \
                                                             \
        value_t x = 0.1;                                     \
                                                             \
        simd_t a;                                            \
        simd<value_t>::set(x, a);                            \
                                                             \
        value_t y = 2.1;                                     \
        simd_t  b;                                           \
        simd<value_t>::set(y, b);                            \
                                                             \
        simd_t c;                                            \
        simd<value_t>::setzero(c);                           \
                                                             \
        simd<value_t>::op(a, b, c);                          \
        auto z = std::op(x, y);                              \
                                                             \
        simd<value_t>::storeu(c, out.data());                \
        for (auto e : out)                                   \
        {                                                    \
            VECTORIZATION_LOGF(INFO, "vec: %f seq: %f", e, z);      \
            EXPECT_LE(std::fabs(e - z), tolerance);          \
        }                                                    \
    }

template <typename value_t>
void test_accumulate(value_t tolerance)
{
    constexpr size_t n = simd<value_t>::size;
    using simd_t       = typename simd<value_t>::simd_t;

    value_t x = 0.1;

    simd_t a;
    simd<value_t>::set(x, a);

    auto e = simd<value_t>::accumulate(a);
    auto z = x * n;

    EXPECT_LE(std::fabs(e - z), tolerance);
}

template <typename value_t>
void test_hmin(value_t tolerance)
{
    using simd_t = typename simd<value_t>::simd_t;

    value_t x = 0.1;

    simd_t a;
    simd<value_t>::set(x, a);

    auto e = simd<value_t>::hmin(a);
    auto z = x;

    EXPECT_LE(std::fabs(e - z), tolerance);
}

template <typename value_t>
void test_hmax(value_t tolerance)
{
    using simd_t = typename simd<value_t>::simd_t;

    value_t x = 0.1;

    simd_t a;
    simd<value_t>::set(x, a);

    auto e = simd<value_t>::hmax(a);
    auto z = x;

    EXPECT_LE(std::fabs(e - z), tolerance);
}

TEST_SIMD_FUNC_1_ARGS(sqrt);
TEST_SIMD_FUNC_1_ARGS(sqr);
TEST_SIMD_FUNC_1_ARGS(ceil);
TEST_SIMD_FUNC_1_ARGS(floor);
TEST_SIMD_FUNC_1_ARGS(exp);
TEST_SIMD_FUNC_1_ARGS(expm1);
TEST_SIMD_FUNC_1_ARGS(exp2);
TEST_SIMD_FUNC_1_ARGS(log);
TEST_SIMD_FUNC_1_ARGS(log1p);
TEST_SIMD_FUNC_1_ARGS(log2);
TEST_SIMD_FUNC_1_ARGS(sin);
TEST_SIMD_FUNC_1_ARGS(cos);
TEST_SIMD_FUNC_1_ARGS(tan);
TEST_SIMD_FUNC_1_ARGS(asin);
TEST_SIMD_FUNC_1_ARGS(acos);
TEST_SIMD_FUNC_1_ARGS(atan);
TEST_SIMD_FUNC_1_ARGS(sinh);
TEST_SIMD_FUNC_1_ARGS(cosh);
TEST_SIMD_FUNC_1_ARGS(tanh);
TEST_SIMD_FUNC_1_ARGS(asinh);
TEST_SIMD_FUNC_1_ARGS(acosh);
TEST_SIMD_FUNC_1_ARGS(atanh);
TEST_SIMD_FUNC_1_ARGS(cbrt);
TEST_SIMD_FUNC_1_ARGS(cdf);
TEST_SIMD_FUNC_1_ARGS(inv_cdf);
TEST_SIMD_FUNC_1_ARGS(trunc);
TEST_SIMD_FUNC_1_ARGS(invsqrt);
TEST_SIMD_FUNC_1_ARGS(fabs);
TEST_SIMD_FUNC_1_ARGS(neg);

TEST_SIMD_FUNC_2_ARGS(add);
TEST_SIMD_FUNC_2_ARGS(sub);
TEST_SIMD_FUNC_2_ARGS(mul);
TEST_SIMD_FUNC_2_ARGS(div);
TEST_SIMD_FUNC_2_ARGS(min);
TEST_SIMD_FUNC_2_ARGS(max);
TEST_SIMD_FUNC_2_ARGS(pow);
TEST_SIMD_FUNC_2_ARGS(hypot);
TEST_SIMD_FUNC_2_ARGS(signcopy);

template <typename>
struct simdTolerance
{
};

template <>
struct simdTolerance<float>
{
    static constexpr float tolerance = 0.0007f;
};

template <>
struct simdTolerance<double>
{
    static constexpr double tolerance = 5.e-12;
};

template <typename value_t>
void test_all_simd_function(value_t tolerance)
{
    test_sqrt(tolerance);
    test_sqr(tolerance);
    test_ceil(tolerance);
    test_floor(tolerance);
    test_exp(tolerance);
    test_expm1(tolerance);
    test_exp2(tolerance);
    test_log(tolerance);
    test_log1p(tolerance);
    test_log2(tolerance);
    test_sin(tolerance);
    test_cos(tolerance);
    test_tan(tolerance);
    test_asin(tolerance);
    test_acos(tolerance);
    test_atan(tolerance);
    test_sinh(tolerance);
    test_cosh(tolerance);
    test_tanh(tolerance);
    test_asinh(tolerance);
    test_acosh<value_t>(tolerance, 1.5);
    test_atanh(tolerance);
    test_cbrt(tolerance);
    test_cdf(tolerance);
    test_inv_cdf(tolerance);
    test_trunc(tolerance);
    test_invsqrt(tolerance);
    test_fabs<value_t>(tolerance, -1.0);
    test_neg(tolerance);

    test_add(tolerance);
    test_sub(tolerance);
    test_mul(tolerance);
    test_div(tolerance);
    test_min(tolerance);
    test_max(tolerance);

    test_pow(tolerance);
    test_hypot(tolerance);
    test_signcopy(tolerance);

    test_accumulate(tolerance);
    test_hmax(tolerance);
    test_hmin(tolerance);

    using simd_t       = typename simd<value_t>::simd_t;
    using mask_t       = typename simd<value_t>::mask_t;
    using xsigma_int_t = typename simd<value_t>::int_t;

    const size_t n = (2 << 8) + 3;

    using allocator_t = typename quarisma::allocator<value_t>;

    auto* x = allocator_t::allocate(n);
    auto* y = allocator_t::allocate(n);

    auto dt = 10. / n;
    for (size_t i = 0; i < n; ++i)
    {
        x[i] = static_cast<value_t>(-5. + static_cast<value_t>(i) * dt);
    }

    auto aligned_start = allocator_t::first_aligned(x, n);
    auto aligned_end   = allocator_t::last_aligned(aligned_start, n, simd<value_t>::size);

    simd_t tmp_load;
    for (size_t i = aligned_start; i < aligned_end; i += simd<value_t>::size)
    {
        simd<value_t>::load(&x[i], tmp_load);
        simd<value_t>::store(tmp_load, &x[i]);
    }

    int offset = n / simd<value_t>::size;

    simd_t ret;
    simd<value_t>::gather(x, offset, ret);
    simd<value_t>::scatter(ret, offset, y);
    for (size_t i = 0; i < simd<value_t>::size; ++i)
        EXPECT_EQ(x[i * offset], y[i * offset]);

    int indces[simd<value_t>::size];

    for (size_t i = 0; i < simd<value_t>::size; ++i)
    {
        indces[i] = static_cast<int>(i * (n / simd<value_t>::size));
    }

    simd<value_t>::gather(x, indces, ret);
    simd<value_t>::scatter(ret, indces, y);

    for (size_t i = 0; i < simd<value_t>::size; ++i)
    {
        size_t k = i * (n / simd<value_t>::size);

        EXPECT_EQ(x[k], y[k]);
    }

    simd_t a;
    simd<value_t>::set(0.1, a);
    simd_t b;
    simd<value_t>::set(0.1, b);

    VECTORIZATION_ALIGN(VECTORIZATION_ALIGNMENT) xsigma_int_t tmp[simd<value_t>::size];

    mask_t m;
    simd<value_t>::set(0, m);
    simd<value_t>::eq(a, b, m);
    simd<value_t>::storeu(m, &tmp[0]);
    simd<value_t>::loadu(&tmp[0], m);

    simd<value_t>::load(&tmp[0], m);
    simd<value_t>::store(m, &tmp[0]);

    simd<value_t>::neq(a, b, m);
    simd<value_t>::storeu(m, &tmp[0]);

    simd<value_t>::gt(a, b, m);
    simd<value_t>::storeu(m, &tmp[0]);

    simd<value_t>::lt(a, b, m);
    simd<value_t>::storeu(m, &tmp[0]);

    simd<value_t>::ge(a, b, m);
    simd<value_t>::storeu(m, &tmp[0]);

    mask_t l;
    simd<value_t>::le(a, b, l);
    simd<value_t>::storeu(l, &tmp[0]);

    mask_t o;
    simd<value_t>::and_mask(m, l, o);
    simd<value_t>::storeu(o, &tmp[0]);

    simd<value_t>::or_mask(m, l, o);
    simd<value_t>::storeu(o, &tmp[0]);

    simd<value_t>::xor_mask(m, l, o);
    simd<value_t>::storeu(o, &tmp[0]);

    simd<value_t>::not_mask(m, o);
    simd<value_t>::storeu(o, &tmp[0]);

    allocator_t::free(y);
    allocator_t::free(x);
}
}  // namespace

VECTORIZATIONTEST(Math, Simd)
{
    test_all_simd_function<float>(simdTolerance<float>::tolerance);
    test_all_simd_function<double>(simdTolerance<double>::tolerance);

    END_TEST();
}
#else
VECTORIZATIONTEST(Math, Simd)
{
    END_TEST();
}

#endif
