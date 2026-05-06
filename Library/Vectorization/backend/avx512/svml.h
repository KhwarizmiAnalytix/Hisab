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
#define svml_ps_ha(op) __svml_##op##f16_ha
#define svml_pd_ha(op) __svml_##op##8_ha
#define svml_ps_mask(op) __svml_##op##f16_mask
#define svml_pd_mask(op) __svml_##op##8_mask

extern "C"
{
    __m512  __svml_expf16(__m512);
    __m512d __svml_exp8(__m512d);
    __m512  __svml_expm1f16(__m512);
    __m512d __svml_expm18(__m512d);
    __m512  __svml_exp2f16(__m512);
    __m512d __svml_exp28(__m512d);
    __m512  __svml_exp10f16(__m512);
    __m512d __svml_exp108(__m512d);
    __m512  __svml_logf16(__m512);
    __m512d __svml_log8(__m512d);
    __m512  __svml_log1pf16(__m512);
    __m512d __svml_log1p8(__m512d);
    __m512  __svml_log2f16(__m512);
    __m512d __svml_log28(__m512d);
    __m512  __svml_log10f16(__m512);
    __m512d __svml_log108(__m512d);
    __m512  __svml_sinf16(__m512);
    __m512d __svml_sin8(__m512d);
    __m512  __svml_cosf16(__m512);
    __m512d __svml_cos8(__m512d);
    __m512  __svml_tanf16(__m512);
    __m512d __svml_tan8(__m512d);
    __m512  __svml_asinf16(__m512);
    __m512d __svml_asin8(__m512d);
    __m512  __svml_acosf16(__m512);
    __m512d __svml_acos8(__m512d);
    __m512  __svml_atanf16(__m512);
    __m512d __svml_atan8(__m512d);
    __m512  __svml_sinhf16(__m512);
    __m512d __svml_sinh8(__m512d);
    __m512  __svml_coshf16(__m512);
    __m512d __svml_cosh8(__m512d);
    __m512  __svml_tanhf16(__m512);
    __m512d __svml_tanh8(__m512d);
    __m512  __svml_asinhf16(__m512);
    __m512d __svml_asinh8(__m512d);
    __m512  __svml_acoshf16(__m512);
    __m512d __svml_acosh8(__m512d);
    __m512  __svml_atanhf16(__m512);
    __m512d __svml_atanh8(__m512d);
    __m512  __svml_cbrtf16(__m512);
    __m512d __svml_cbrt8(__m512d);
    __m512  __svml_cdfnormf16(__m512);
    __m512d __svml_cdfnorm8(__m512d);
    __m512  __svml_cdfnorminvf16(__m512);
    __m512d __svml_cdfnorminv8(__m512d);
    __m512  __svml_truncf16(__m512);
    __m512d __svml_trunc8(__m512d);
    __m512  __svml_invsqrtf16(__m512);
    __m512d __svml_invsqrt8(__m512d);
    __m512  __svml_powf16(__m512, __m512);
    __m512d __svml_pow8(__m512d, __m512d);
    __m512  __svml_powf16_ha(__m512, __m512);
    __m512d __svml_pow8_ha(__m512d, __m512d);
    __m512  __svml_hypotf16(__m512, __m512);
    __m512d __svml_hypot8(__m512d, __m512d);
    __m512  __svml_hypotf16_ha(__m512, __m512);
    __m512d __svml_hypot8_ha(__m512d, __m512d);
#if defined(__VECTORIZATION_COMPILER_MSVC__) && _MSC_VER <= 1920
    __m512  __svml_ceilf16(__m512);
    __m512d __svml_ceil8(__m512d);
    __m512  __svml_floorf16(__m512);
    __m512d __svml_floor8(__m512d);
#endif
}

#if defined(_MSC_VER) && !defined(__clang__)

#define SVML_FUNCTION_ONE_ARG(op)                                                           \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(__m512 x)   \
    { return reinterpret_cast<__m512(VECTORIZATION_VECTORCALL*)(__m512)>(svml_ps(op))(x); } \
                                                                                            \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(__m512d x) \
    { return reinterpret_cast<__m512d(VECTORIZATION_VECTORCALL*)(__m512d)>(svml_pd(op))(x); }

#define SVML_FUNCTION_TWO_ARGS(op)                                                               \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(                 \
        __m512 x, __m512 y)                                                                      \
    {                                                                                            \
        return reinterpret_cast<__m512(VECTORIZATION_VECTORCALL*)(__m512, __m512)>(svml_ps(op))( \
            x, y);                                                                               \
    }                                                                                            \
                                                                                                 \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(                \
        __m512d x, __m512d y)                                                                    \
    {                                                                                            \
        return reinterpret_cast<__m512d(VECTORIZATION_VECTORCALL*)(__m512d, __m512d)>(           \
            svml_pd(op))(x, y);                                                                  \
    }

#else  // !(_MSC_VER && !__clang__)

#define SVML_FUNCTION_ONE_ARG(op)                                                           \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(__m512 x)   \
    { return svml_ps(op)(x); }                                                              \
                                                                                            \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(__m512d x) \
    { return svml_pd(op)(x); }

#define SVML_FUNCTION_TWO_ARGS(op)                                                               \
    VECTORIZATION_FORCE_INLINE __m512 VECTORIZATION_VECTORCALL _mm512_##op##_ps(                 \
        __m512 x, __m512 y)                                                                      \
    { return svml_ps(op)(x, y); }                                                               \
                                                                                                 \
    VECTORIZATION_FORCE_INLINE __m512d VECTORIZATION_VECTORCALL _mm512_##op##_pd(                \
        __m512d x, __m512d y)                                                                    \
    { return svml_pd(op)(x, y); }

#endif  // defined(_MSC_VER) && !defined(__clang__)

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

#undef SVML_FUNCTION_TWO_ARGS_HA
#undef SVML_FUNCTION_TWO_ARGS
#undef SVML_FUNCTION_ONE_ARG
#undef svml_ps
#undef svml_pd
#undef svml_ps_ha
#undef svml_pd_ha
#undef svml_ps_mask
#undef svml_pd_mask

#endif
