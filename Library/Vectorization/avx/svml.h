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

#pragma once


#if defined(VECTORIZATION_HAS_SVML) && (defined(VECTORIZATION_HAS_AVX2) || defined(VECTORIZATION_HAS_AVX))

#define svml_ps(op) __svml_##op##f8
#define svml_pd(op) __svml_##op##4
#define svml_ps_mask(op) __svml_##op##f8_mask
#define svml_pd_mask(op) __svml_##op##4_mask

#define SVML_FUNCTION_ONE_ARG(op)                                                      \
    extern "C" __m256 svml_ps(op)(__m256);                                             \
                                                                                       \
    VECTORIZATION_FORCE_INLINE __m256 VECTORIZATION_VECTORCALL _mm256_##op##_ps(__m256 x)            \
    {                                                                                  \
        return reinterpret_cast<__m256(VECTORIZATION_VECTORCALL*)(__m256)>(svml_ps(op))(x);   \
    }                                                                                  \
                                                                                       \
    extern "C" __m256d svml_pd(op)(__m256d);                                           \
                                                                                       \
    VECTORIZATION_FORCE_INLINE __m256d VECTORIZATION_VECTORCALL _mm256_##op##_pd(__m256d x)          \
    {                                                                                  \
        return reinterpret_cast<__m256d(VECTORIZATION_VECTORCALL*)(__m256d)>(svml_pd(op))(x); \
    }

#define SVML_FUNCTION_TWO_ARGS(op)                                                                 \
    extern "C" __m256 svml_ps(op)(__m256, __m256);                                                 \
                                                                                                   \
    VECTORIZATION_FORCE_INLINE __m256 VECTORIZATION_VECTORCALL _mm256_##op##_ps(__m256 x, __m256 y)              \
    {                                                                                              \
        return reinterpret_cast<__m256(VECTORIZATION_VECTORCALL*)(__m256, __m256)>(svml_ps(op))(x, y);    \
    }                                                                                              \
                                                                                                   \
    extern "C" __m256d svml_pd(op)(__m256d, __m256d);                                              \
                                                                                                   \
    VECTORIZATION_FORCE_INLINE __m256d VECTORIZATION_VECTORCALL _mm256_##op##_pd(__m256d x, __m256d y)           \
    {                                                                                              \
        return reinterpret_cast<__m256d(VECTORIZATION_VECTORCALL*)(__m256d, __m256d)>(svml_pd(op))(x, y); \
    }

SVML_FUNCTION_ONE_ARG(exp)
SVML_FUNCTION_ONE_ARG(expm1)
SVML_FUNCTION_ONE_ARG(exp2)
// SVML_FUNCTION_ONE_ARG(exp10)
SVML_FUNCTION_ONE_ARG(log)
SVML_FUNCTION_ONE_ARG(log1p)
SVML_FUNCTION_ONE_ARG(log2)
// SVML_FUNCTION_ONE_ARG(log10)
SVML_FUNCTION_ONE_ARG(sin)
SVML_FUNCTION_ONE_ARG(cos)
SVML_FUNCTION_ONE_ARG(tan)
SVML_FUNCTION_ONE_ARG(asin)
SVML_FUNCTION_ONE_ARG(acos)
SVML_FUNCTION_ONE_ARG(atan)
SVML_FUNCTION_ONE_ARG(sinh)
SVML_FUNCTION_ONE_ARG(cosh)
SVML_FUNCTION_ONE_ARG(tanh)
SVML_FUNCTION_ONE_ARG(asinh)
SVML_FUNCTION_ONE_ARG(acosh)
SVML_FUNCTION_ONE_ARG(atanh)
SVML_FUNCTION_ONE_ARG(cbrt)
SVML_FUNCTION_ONE_ARG(cdfnorm)
SVML_FUNCTION_ONE_ARG(cdfnorminv)
SVML_FUNCTION_ONE_ARG(trunc)
SVML_FUNCTION_ONE_ARG(invsqrt)
SVML_FUNCTION_TWO_ARGS(pow)
SVML_FUNCTION_TWO_ARGS(hypot)

#undef SVML_FUNCTION_TWO_ARGS
#undef SVML_FUNCTION_ONE_ARG
#undef svml_ps
#undef svml_pd
#undef svml_ps_mask
#undef svml_pd_mask