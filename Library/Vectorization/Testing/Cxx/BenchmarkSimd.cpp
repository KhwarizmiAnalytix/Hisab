/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * SIMD micro-benchmarks (Google Benchmark). Built as target `benchmark_simd`, same pattern as
 * `benchmark_parallel` in Library/Parallel/Testing/Cxx.
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#include <benchmark/benchmark.h>

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

//#include "common/constants.h"
#include "common/vectorization_macros.h"
#include "expressions/expressions.h"
#include "terminals/vector.h"


namespace
{
double y_max = exp(5.);
double y_min = exp(-5.);
size_t ny    = 1024 * 256;
double dy    = (y_max - y_min) / static_cast<double>(ny);
double T     = 5.;

};  // namespace

//#define BENCHMARK_DONOT_OPTIMIZE(X) benchmark::DoNotOptimize(X)
#define BENCHMARK_DONOT_OPTIMIZE(X) X

#define MACRO_TEST_SIMD_FUNC(op, op_inv)                                   \
    class func_##op                                                        \
    {                                                                      \
    public:                                                                \
        template <typename T>                                              \
        VECTORIZATION_FORCE_INLINE static void run(T const& a, T const&, T& c)    \
        {                                                                  \
            if constexpr (vectorization::is_fundamental<T>::value)                \
                BENCHMARK_DONOT_OPTIMIZE(c = std::op_inv(std::op(a)) / a); \
            else                                                           \
            {                                                              \
                BENCHMARK_DONOT_OPTIMIZE(c = op_inv(op(a)) / a);           \
            }                                                              \
        };                                                                 \
    }

#define MACRO_TEST_SIMD_FUNC2(op)                                         \
    class func_##op                                                       \
    {                                                                     \
    public:                                                               \
        template <typename T>                                             \
        VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c) \
        {                                                                 \
            if constexpr (vectorization::is_fundamental<T>::value)               \
                BENCHMARK_DONOT_OPTIMIZE(c = std::op(a, b));              \
            else                                                          \
                BENCHMARK_DONOT_OPTIMIZE(c = op(a, b));                   \
        };                                                                \
    }

namespace std
{
template <typename T>
inline T accumulate(T a, double c)
{
    return a + c;
}
}  // namespace std

namespace vectorization
{
MACRO_TEST_SIMD_FUNC(sqr, sqrt);
MACRO_TEST_SIMD_FUNC(invsqrt, sqr);
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

class func_sum
{
public:
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& x, T const& y, T& z)
    {
        BENCHMARK_DONOT_OPTIMIZE(z = x * y - x + x * y - x);
    };
};

class func_mixed_formula
{
public:
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c)
    {
        if constexpr (vectorization::is_fundamental<T>::value)
        {
            auto t = -fabs(a - b) + a + b;
            c      = floor(log1p(fabs(t)) + 1) + ceil(a) + sin(t) + trunc(a) +
                (fabs(t) < std::numeric_limits<T>::epsilon() ? 1. - 0.5 * t : expm1(t) / t);
        }
        else
        {
            auto t = -fabs(a - b) + a + b;
            c      = floor(log1p(fabs(t)) + 1) + ceil(a) + sin(t) + trunc(a) +
                if_else(
                    fabs(t) < std::numeric_limits<double>::epsilon(), 1. - 0.5 * t, expm1(t) / t);
        }
    };
};

class func_mixed_if_else
{
public:
    template <typename T>
    VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c)
    {
        double dcf = 0.25;
        if constexpr (std::is_arithmetic<T>::value)
        {
            auto x = -dcf * fabs(-a + b);
            c      = a * dcf *
                ((fabs(x) < std::numeric_limits<double>::epsilon()) /*&& !(x == 0.)*/ ? 1. - 0.5 * x
                                                                                      : expm1(x) /
                                                                                            x);
        }
        else
        {
            c      = 0.;
            auto x = -dcf * fabs(-a + b);
            c += a * dcf *
                 if_else(
                     fabs(x) < std::numeric_limits<double>::epsilon() /* && !(x == 0.)*/,
                     1. - 0.5 * x,
                     expm1(x) / x);
        }
    };
};
}  // namespace vectorization

#define SIMD_BENCHMARK(op)                                                \
    template <typename scalar_t>                                          \
    static void Vectorized_##op(benchmark::State& state)                  \
    {                                                                     \
        const size_t n = (2 << 12) + 3;                                   \
                                                                          \
        vectorization::vector<scalar_t>   a(n);                                  \
        vectorization::vector<scalar_t>   b(n);                                  \
        vectorization::vector<scalar_t>   c(n);                                  \
        std::default_random_engine generator;                             \
                                                                          \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.);   \
                                                                          \
        for (size_t i = 0; i < n; ++i)                                    \
        {                                                                 \
            auto tmp_x = distribution(generator);                         \
            auto tmp_y = distribution(generator);                         \
            a[i]       = tmp_x;                                           \
            b[i]       = tmp_y;                                           \
        }                                                                 \
                                                                          \
        for (auto _ : state)                                              \
            vectorization::func_##op::run(a, b, c);                              \
    }                                                                     \
    template <typename scalar_t>                                          \
    static void Scalar_##op(benchmark::State& state)                      \
    {                                                                     \
        const size_t n = (2 << 12) + 3;                                   \
                                                                          \
        vectorization::vector<scalar_t>   a(n);                                  \
        vectorization::vector<scalar_t>   b(n);                                  \
        vectorization::vector<scalar_t>   c(n);                                  \
        std::default_random_engine generator;                             \
                                                                          \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.);   \
                                                                          \
        for (size_t i = 0; i < n; ++i)                                    \
        {                                                                 \
            auto tmp_x = distribution(generator);                         \
            auto tmp_y = distribution(generator);                         \
            a[i]       = tmp_x;                                           \
            b[i]       = tmp_y;                                           \
        }                                                                 \
                                                                          \
        for (auto _ : state)                                              \
            for (size_t i = 0; i < n; ++i)                                \
                vectorization::func_##op::run(a[i], b[i], c[i]);                 \
    }                                                                     \
    BENCHMARK_TEMPLATE(Vectorized_##op, float)->MeasureProcessCPUTime();  \
    BENCHMARK_TEMPLATE(Scalar_##op, float)->MeasureProcessCPUTime();      \
    BENCHMARK_TEMPLATE(Vectorized_##op, double)->MeasureProcessCPUTime(); \
    BENCHMARK_TEMPLATE(Scalar_##op, double)->MeasureProcessCPUTime();

namespace vectorization_test
{
inline void transpose8x8_intrinsic([[maybe_unused]] vectorization::array<simd<double>::simd_t, 8> reg)
{
#if VECTORIZATION_HAS_AVX512
    simd<double>::simd_t tmp[8];

    tmp[0] = _mm512_unpacklo_pd(reg[0], reg[1]);
    tmp[1] = _mm512_unpackhi_pd(reg[0], reg[1]);
    tmp[2] = _mm512_unpacklo_pd(reg[2], reg[3]);
    tmp[3] = _mm512_unpackhi_pd(reg[2], reg[3]);
    tmp[4] = _mm512_unpacklo_pd(reg[4], reg[5]);
    tmp[5] = _mm512_unpackhi_pd(reg[4], reg[5]);
    tmp[6] = _mm512_unpacklo_pd(reg[6], reg[7]);
    tmp[7] = _mm512_unpackhi_pd(reg[6], reg[7]);

    const int shuffle20 = _MM_SHUFFLE(2, 0, 2, 0);
    const int shuffle31 = _MM_SHUFFLE(3, 1, 3, 1);
    // The result of the next pass is not perfect transpose of 2x2 blocks
    // The next pass will fix the order
    reg[0] = _mm512_shuffle_f64x2(tmp[0], tmp[2], shuffle20);
    reg[1] = _mm512_shuffle_f64x2(tmp[1], tmp[3], shuffle20);
    reg[2] = _mm512_shuffle_f64x2(tmp[0], tmp[2], shuffle31);
    reg[3] = _mm512_shuffle_f64x2(tmp[1], tmp[3], shuffle31);
    reg[4] = _mm512_shuffle_f64x2(tmp[4], tmp[6], shuffle20);
    reg[5] = _mm512_shuffle_f64x2(tmp[5], tmp[7], shuffle20);
    reg[6] = _mm512_shuffle_f64x2(tmp[4], tmp[6], shuffle31);
    reg[7] = _mm512_shuffle_f64x2(tmp[5], tmp[7], shuffle31);

    simd<double>::simd_t s0 = reg[0];
    simd<double>::simd_t s1 = reg[4];

    reg[0] = _mm512_shuffle_f64x2(s0, s1, shuffle20);
    reg[4] = _mm512_shuffle_f64x2(s0, s1, shuffle31);

    s0     = reg[1];
    s1     = reg[5];
    reg[1] = _mm512_shuffle_f64x2(s0, s1, shuffle20);
    reg[5] = _mm512_shuffle_f64x2(s0, s1, shuffle31);

    s0     = reg[2];
    s1     = reg[6];
    reg[2] = _mm512_shuffle_f64x2(s0, s1, shuffle20);
    reg[6] = _mm512_shuffle_f64x2(s0, s1, shuffle31);

    s0     = reg[3];
    s1     = reg[7];
    reg[3] = _mm512_shuffle_f64x2(s0, s1, shuffle20);
    reg[7] = _mm512_shuffle_f64x2(s0, s1, shuffle31);

#endif  // VECTORIZATION_HAS_AVX512
}

inline void transpose16x16_intrinsic([[maybe_unused]] vectorization::array<simd<float>::simd_t, 16> reg)
{
#if VECTORIZATION_HAS_AVX512
    simd<float>::simd_t tmp[16];
// Transpose 8x8 blocks (block size is 2x2) within 16x16 matrix
// Not a true transpose:
// 1 2 3 4 -> 1 5 2 6
// 5 6 7 8 -> 3 6 4 8
// But we fix it in the next pass
#pragma unroll
    for (int i = 0; i < 8; ++i)
    {
        tmp[2 * i]     = _mm512_unpacklo_ps(reg[2 * i], reg[2 * i + 1]);
        tmp[2 * i + 1] = _mm512_unpackhi_ps(reg[2 * i], reg[2 * i + 1]);
    }
    const int shuffle20 = _MM_SHUFFLE(2, 0, 2, 0);
    const int shuffle31 = _MM_SHUFFLE(3, 1, 3, 1);
    const int shuffle10 = _MM_SHUFFLE(1, 0, 1, 0);
    const int shuffle32 = _MM_SHUFFLE(3, 2, 3, 2);
    // Transpose 4x4 blocks (block size is 4x4) within 16x16 matrix
#pragma unroll
    for (int i = 0; i < 4; ++i)
    {
        reg[4 * i + 0] = _mm512_shuffle_ps(tmp[4 * i + 0], tmp[4 * i + 2], shuffle10);
        reg[4 * i + 1] = _mm512_shuffle_ps(tmp[4 * i + 0], tmp[4 * i + 2], shuffle32);
        reg[4 * i + 2] = _mm512_shuffle_ps(tmp[4 * i + 1], tmp[4 * i + 3], shuffle10);
        reg[4 * i + 3] = _mm512_shuffle_ps(tmp[4 * i + 1], tmp[4 * i + 3], shuffle32);
    }

    // Transpose 2x2 blocks (block size is 8x8) within 16x16 matrix
    // Similarly to the first pass the shuffle mess up the transpose but we will
    // fix it in the next pass
#pragma unroll
    for (int i = 0; i < 2; ++i)
    {
#pragma unroll
        for (int j = 0; j < 4; ++j)
        {
            tmp[8 * i + j] = _mm512_shuffle_f32x4(reg[8 * i + j], reg[8 * i + 4 + j], shuffle20);
            tmp[8 * i + 4 + j] =
                _mm512_shuffle_f32x4(reg[8 * i + j], reg[8 * i + 4 + j], shuffle31);
        }
    }

    // Transpose 1 blocks (block size is 16x16) within 16x16 matrix
#pragma unroll
    for (int i = 0; i < 8; ++i)
    {
        reg[i]     = _mm512_shuffle_f32x4(tmp[i], tmp[8 + i], shuffle20);
        reg[8 + i] = _mm512_shuffle_f32x4(tmp[i], tmp[8 + i], shuffle31);
    }
#endif  // VECTORIZATION_HAS_AVX512
}
}  // namespace vectorization_test

template <typename scalar_t>
static void Vectorized_transpose(benchmark::State& state)
{
    vectorization::array<simd<float>::simd_t, 16> reg_s;
    vectorization::array<simd<double>::simd_t, 8> reg_d;

    const auto n = 16 * 16;

    vectorization::vector<scalar_t> r(n);
    for (size_t i = 0; i < n; ++i)
    {
        r[i] = y_min + i * dy;
    }

    if constexpr (std::is_same_v<float, scalar_t>)
    {
        vectorization::packet<float, 16>::load(r.data(), reg_s);
    }
    else
    {
        vectorization::packet<double, 8>::load(r.data(), reg_d);
    }
    const size_t c = (2 << 12) + 3;
    for (auto _ : state)
    {
        for (size_t i = 0; i < c; ++i)
        {
            if constexpr (std::is_same_v<float, scalar_t>)
            {
                simd<float>::ptranspose<16>(reg_s.data());
            }
            else
            {
                simd<double>::ptranspose<8>(reg_d.data());
            }
        }
    }
}

template <typename scalar_t>
static void Scalar_transpose(benchmark::State& state)
{
    vectorization::array<simd<float>::simd_t, 16> reg_s;
    vectorization::array<simd<double>::simd_t, 8> reg_d;

    const auto n = 16 * 16;

    vectorization::vector<scalar_t> r(n);
    for (size_t i = 0; i < n; ++i)
    {
        r[i] = y_min + i * dy;
    }

    if constexpr (std::is_same_v<float, scalar_t>)
    {
        vectorization::packet<float, 16>::load(r.data(), reg_s);
    }
    else
    {
        vectorization::packet<double, 8>::load(r.data(), reg_d);
    }

    const size_t c = (2 << 12) + 3;
    for (auto _ : state)
    {
        for (size_t i = 0; i < c; ++i)
        {
            if constexpr (std::is_same_v<float, scalar_t>)
            {
                vectorization_test::transpose16x16_intrinsic(reg_s);
            }
            else
            {
                vectorization_test::transpose8x8_intrinsic(reg_d);
            }
        }
    }
}

BENCHMARK_TEMPLATE(Vectorized_transpose, float)->MeasureProcessCPUTime();
BENCHMARK_TEMPLATE(Scalar_transpose, float)->MeasureProcessCPUTime();
BENCHMARK_TEMPLATE(Vectorized_transpose, double)->MeasureProcessCPUTime();
BENCHMARK_TEMPLATE(Scalar_transpose, double)->MeasureProcessCPUTime();

SIMD_BENCHMARK(sum);
SIMD_BENCHMARK(mixed_formula);
SIMD_BENCHMARK(mixed_if_else);
SIMD_BENCHMARK(sqr);
SIMD_BENCHMARK(invsqrt);
SIMD_BENCHMARK(exp);
SIMD_BENCHMARK(exp2);
SIMD_BENCHMARK(expm1);
SIMD_BENCHMARK(sin);
SIMD_BENCHMARK(cos);
SIMD_BENCHMARK(atan);
SIMD_BENCHMARK(sinh);
SIMD_BENCHMARK(cosh);
SIMD_BENCHMARK(tanh);
SIMD_BENCHMARK(cdf);
SIMD_BENCHMARK(ceil);
SIMD_BENCHMARK(trunc);
SIMD_BENCHMARK(fabs);
SIMD_BENCHMARK(cbrt);
SIMD_BENCHMARK(copysign);
SIMD_BENCHMARK(min);
SIMD_BENCHMARK(max);
SIMD_BENCHMARK(pow);
SIMD_BENCHMARK(hypot);

#define SIMDHORIZANTALBENCHMARK(op1, op2)                                   \
    template <typename scalar_t>                                            \
    static void Vectorized_h##op1(benchmark::State& state)                  \
    {                                                                       \
        const size_t n = (2 << 12) + 3;                                     \
                                                                            \
        vectorization::vector<scalar_t>   a(n);                                    \
        std::default_random_engine generator;                               \
                                                                            \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.);     \
                                                                            \
        for (size_t i = 0; i < n; ++i)                                      \
        {                                                                   \
            auto tmp_x = distribution(generator);                           \
            a[i]       = tmp_x;                                             \
        }                                                                   \
                                                                            \
        for (auto _ : state)                                                \
            BENCHMARK_DONOT_OPTIMIZE(vectorization::op1(a));                       \
    }                                                                       \
    template <typename scalar_t>                                            \
    static void Scalar_h##op2(benchmark::State& state)                      \
    {                                                                       \
        const size_t               n = (2 << 12) + 3;                       \
        vectorization::vector<scalar_t>   a(n);                                    \
        std::default_random_engine generator;                               \
                                                                            \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.);     \
                                                                            \
        for (size_t i = 0; i < n; ++i)                                      \
        {                                                                   \
            auto tmp_x = distribution(generator);                           \
            a[i]       = tmp_x;                                             \
        }                                                                   \
                                                                            \
        for (auto _ : state)                                                \
        {                                                                   \
            scalar_t c = 0;                                                 \
            for (size_t i = 0; i < n; ++i)                                  \
                BENCHMARK_DONOT_OPTIMIZE(c = std::op2(a[i], c));            \
        }                                                                   \
    }                                                                       \
    BENCHMARK_TEMPLATE(Vectorized_h##op1, float)->MeasureProcessCPUTime();  \
    BENCHMARK_TEMPLATE(Scalar_h##op2, float)->MeasureProcessCPUTime();      \
    BENCHMARK_TEMPLATE(Vectorized_h##op1, double)->MeasureProcessCPUTime(); \
    BENCHMARK_TEMPLATE(Scalar_h##op2, double)->MeasureProcessCPUTime();

SIMDHORIZANTALBENCHMARK(hmin, min);
SIMDHORIZANTALBENCHMARK(hmax, max);
SIMDHORIZANTALBENCHMARK(accumulate, accumulate);

BENCHMARK_MAIN();
