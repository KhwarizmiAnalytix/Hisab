/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Unary SIMD benchmarks: vectorized tensor ops (::op) vs scalar loops (std::op per element).
 * Scalar path uses <cmath> where defined, plus std::sqr / std::invsqrt / std::cdf / std::inv_cdf
 * from scalar_helper_functions.h (same reference as expression templates).
 *
 * Target: benchmark_simdunary
 */

#include <benchmark/benchmark.h>

#include <cstddef>
#include <random>

#include "common/vectorization_macros.h"
#include "expressions/expressions.h"
#include "terminals/tensor.h"

template <typename T>
void fill_tensor_uniform(vectorization::vector<T>& a, double lo, double hi, unsigned seed = 5489u)
{
    std::mt19937                     gen(seed);
    std::uniform_real_distribution<double> dist(lo, hi);
    for (std::size_t i = 0; i < a.size(); ++i)
        a[i] = static_cast<T>(dist(gen));
}

// Vectorized: one expression evaluation over the full buffer.
// Scalar: same buffer, element-wise std::op (reference loop).
#define UNARY_BENCH_PAIR(OP, lo, hi)                                                         \
    template <typename T>                                                                    \
    static void Vectorized_##OP(benchmark::State& state)                                     \
    {                                                                                        \
        constexpr std::size_t n = (2u << 16) + 3;                                            \
        vectorization::vector<T> a(n);                                                       \
        vectorization::vector<T> out(n);                                                     \
        fill_tensor_uniform(a, (lo), (hi));                                                  \
        for (auto _ : state)                                                                 \
            benchmark::DoNotOptimize(out = ::OP(a));                                         \
    }                                                                                        \
    template <typename T>                                                                    \
    static void Scalar_loop_##OP(benchmark::State& state)                                     \
    {                                                                                        \
        constexpr std::size_t n = (2u << 16) + 3;                                            \
        vectorization::vector<T> a(n);                                                       \
        vectorization::vector<T> out(n);                                                     \
        fill_tensor_uniform(a, (lo), (hi));                                                  \
        for (auto _ : state)                                                                 \
        {                                                                                    \
            for (std::size_t i = 0; i < n; ++i)                                              \
                benchmark::DoNotOptimize(out[i] = std::OP(a[i]));                            \
        }                                                                                    \
    }                                                                                        \
    BENCHMARK_TEMPLATE(Vectorized_##OP, float)->MeasureProcessCPUTime();                     \
    BENCHMARK_TEMPLATE(Scalar_loop_##OP, float)->MeasureProcessCPUTime();                     \
    BENCHMARK_TEMPLATE(Vectorized_##OP, double)->MeasureProcessCPUTime();                    \
    BENCHMARK_TEMPLATE(Scalar_loop_##OP, double)->MeasureProcessCPUTime();

// --- Unary math (domains chosen so both paths are well-defined) ---
UNARY_BENCH_PAIR(sqrt, 0.25, 4.0)
UNARY_BENCH_PAIR(sqr, -4.0, 4.0)
UNARY_BENCH_PAIR(invsqrt, 0.25, 4.0)
UNARY_BENCH_PAIR(exp, -4.0, 4.0)
UNARY_BENCH_PAIR(exp2, -4.0, 4.0)
UNARY_BENCH_PAIR(expm1, -0.75, 0.75)
UNARY_BENCH_PAIR(log, 0.05, 4.0)
UNARY_BENCH_PAIR(log1p, -0.75, 3.0)
UNARY_BENCH_PAIR(log2, 0.05, 4.0)
UNARY_BENCH_PAIR(sin, -3.0, 3.0)
UNARY_BENCH_PAIR(cos, -3.0, 3.0)
UNARY_BENCH_PAIR(tan, -1.2, 1.2)
UNARY_BENCH_PAIR(asin, -0.99, 0.99)
UNARY_BENCH_PAIR(acos, -0.99, 0.99)
UNARY_BENCH_PAIR(atan, -3.0, 3.0)
UNARY_BENCH_PAIR(sinh, -3.0, 3.0)
UNARY_BENCH_PAIR(cosh, -3.0, 3.0)
UNARY_BENCH_PAIR(tanh, -3.0, 3.0)
UNARY_BENCH_PAIR(asinh, -3.0, 3.0)
UNARY_BENCH_PAIR(acosh, 1.01, 4.0)
UNARY_BENCH_PAIR(atanh, -0.9, 0.9)
UNARY_BENCH_PAIR(cbrt, -8.0, 8.0)
UNARY_BENCH_PAIR(cdf, -5.0, 5.0)
UNARY_BENCH_PAIR(inv_cdf, 0.02, 0.98)
UNARY_BENCH_PAIR(ceil, -5.0, 5.0)
UNARY_BENCH_PAIR(floor, -5.0, 5.0)
UNARY_BENCH_PAIR(trunc, -5.0, 5.0)
UNARY_BENCH_PAIR(fabs, -5.0, 5.0)
UNARY_BENCH_PAIR(neg, -5.0, 5.0)

#undef UNARY_BENCH_PAIR

BENCHMARK_MAIN();
