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

#if VECTORIZATION_HAS_SVML && VECTORIZATION_HAS_AVX512

#define svml_ps(op) __svml_##op##f16
#define svml_pd(op) __svml_##op##8
#define svml_ps_mask(op) __svml_##op##f16_mask
#define svml_pd_mask(op) __svml_##op##8_mask

#define SVML_FUNCTION_ONE_ARG(op)                                                      \
    extern "C" __m512 svml_ps(op)(__m512);                                             \
                                                                                       \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(__m512 x)            \
    {                                                                                  \
        return reinterpret_cast<__m512(VECTORIZATION_VECTORCALL*)(__m512)>(svml_ps(op))(x);   \
    }                                                                                  \
                                                                                       \
    extern "C" __m512d svml_pd(op)(__m512d);                                           \
                                                                                       \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(__m512d x)          \
    {                                                                                  \
        return reinterpret_cast<__m512d(VECTORIZATION_VECTORCALL*)(__m512d)>(svml_pd(op))(x); \
    }

#define SVML_FUNCTION_TWO_ARGS(op)                                                                 \
    extern "C" __m512 svml_ps(op)(__m512, __m512);                                                 \
                                                                                                   \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(__m512 x, __m512 y)              \
    {                                                                                              \
        return reinterpret_cast<__m512(VECTORIZATION_VECTORCALL*)(__m512, __m512)>(svml_ps(op))(x, y);    \
    }                                                                                              \
                                                                                                   \
    extern "C" __m512d svml_pd(op)(__m512d, __m512d);                                              \
                                                                                                   \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(__m512d x, __m512d y)           \
    {                                                                                              \
        return reinterpret_cast<__m512d(VECTORIZATION_VECTORCALL*)(__m512d, __m512d)>(svml_pd(op))(x, y); \
    }

SVML_FUNCTION_ONE_ARG(exp)
SVML_FUNCTION_ONE_ARG(expm1)
SVML_FUNCTION_ONE_ARG(exp2)
SVML_FUNCTION_ONE_ARG(exp10)
SVML_FUNCTION_ONE_ARG(log)
SVML_FUNCTION_ONE_ARG(log1p)
SVML_FUNCTION_ONE_ARG(log2)
SVML_FUNCTION_ONE_ARG(log10)
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

#if defined(__VECTORIZATION_COMPILER_MSVC__) && _MSC_VER <= 1920
SVML_FUNCTION_ONE_ARG(ceil)
SVML_FUNCTION_ONE_ARG(floor)
#endif

#undef SVML_FUNCTION_TWO_ARGS
#undef SVML_FUNCTION_ONE_ARG
#undef svml_ps
#undef svml_pd
#undef svml_ps_mask
#undef svml_pd_mask

#endif
