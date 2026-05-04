/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * SIMD micro-benchmarks: two-argument math functions (min, max, pow, hypot, copysign).
 * Target: benchmark_simdbinaryfn
 */

#include <benchmark/benchmark.h>

#include <random>

#include "common/vectorization_macros.h"
#include "expressions/expressions.h"
#include "terminals/tensor.h"

#define BENCHMARK_DONOT_OPTIMIZE(X) benchmark::DoNotOptimize(X)

#define MACRO_TEST_SIMD_FUNC2(op)                                                \
    class func_##op                                                              \
    {                                                                            \
    public:                                                                      \
        template <typename T>                                                    \
        VECTORIZATION_FORCE_INLINE static void run(T const& a, T const& b, T& c) \
        {                                                                        \
            if constexpr (vectorization::is_fundamental<T>::value)               \
                BENCHMARK_DONOT_OPTIMIZE(c = std::op(a, b));                      \
            else                                                                 \
                BENCHMARK_DONOT_OPTIMIZE(c = op(a, b));                          \
        };                                                                       \
    }

namespace vectorization
{
MACRO_TEST_SIMD_FUNC2(min);
MACRO_TEST_SIMD_FUNC2(max);
MACRO_TEST_SIMD_FUNC2(pow);
MACRO_TEST_SIMD_FUNC2(hypot);
MACRO_TEST_SIMD_FUNC2(copysign);
}  // namespace vectorization

#define SIMD_BENCHMARK(op)                                                \
    template <typename scalar_t>                                          \
    static void Vectorized_##op(benchmark::State& state)                  \
    {                                                                     \
        const size_t n = (2 << 16) + 3;                                   \
                                                                          \
        vectorization::vector<scalar_t> a(n);                             \
        vectorization::vector<scalar_t> b(n);                             \
        vectorization::vector<scalar_t> c(n);                             \
        std::default_random_engine      generator;                        \
                                                                          \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.); \
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
            vectorization::func_##op::run(a, b, c);                       \
    }                                                                     \
    template <typename scalar_t>                                          \
    static void Scalar_##op(benchmark::State& state)                      \
    {                                                                     \
        const size_t n = (2 << 16) + 3;                                   \
                                                                          \
        vectorization::vector<scalar_t> a(n);                             \
        vectorization::vector<scalar_t> b(n);                             \
        vectorization::vector<scalar_t> c(n);                             \
        std::default_random_engine      generator;                        \
                                                                          \
        std::uniform_real_distribution<scalar_t> distribution(-5., 5.); \
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
                vectorization::func_##op::run(a[i], b[i], c[i]);          \
    }                                                                     \
    BENCHMARK_TEMPLATE(Vectorized_##op, float)->MeasureProcessCPUTime();  \
    BENCHMARK_TEMPLATE(Scalar_##op, float)->MeasureProcessCPUTime();      \
    BENCHMARK_TEMPLATE(Vectorized_##op, double)->MeasureProcessCPUTime(); \
    BENCHMARK_TEMPLATE(Scalar_##op, double)->MeasureProcessCPUTime();

SIMD_BENCHMARK(min);
SIMD_BENCHMARK(max);
SIMD_BENCHMARK(pow);
SIMD_BENCHMARK(hypot);
SIMD_BENCHMARK(copysign);

BENCHMARK_MAIN();
