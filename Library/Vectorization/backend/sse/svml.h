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

#if VECTORIZATION_HAS_SVML && VECTORIZATION_HAS_SSE

#define svml_ps(op) __svml_##op##f4
#define svml_pd(op) __svml_##op##2
#define svml_ps_ha(op) __svml_##op##f4_ha
#define svml_pd_ha(op) __svml_##op##2_ha
#define svml_ps_mask(op) __svml_##op##f4_mask
#define svml_pd_mask(op) __svml_##op##2_mask

extern "C"
{
    __m128  __svml_expf4(__m128);
    __m128d __svml_exp2(__m128d);
    __m128  __svml_expm1f4(__m128);
    __m128d __svml_expm12(__m128d);
    __m128  __svml_exp2f4(__m128);
    __m128d __svml_exp22(__m128d);
    __m128  __svml_exp10f4(__m128);
    __m128d __svml_exp102(__m128d);
    __m128  __svml_logf4(__m128);
    __m128d __svml_log2(__m128d);
    __m128  __svml_log1pf4(__m128);
    __m128d __svml_log1p2(__m128d);
    __m128  __svml_log2f4(__m128);
    __m128d __svml_log22(__m128d);
    __m128  __svml_log10f4(__m128);
    __m128d __svml_log102(__m128d);
    __m128  __svml_sinf4(__m128);
    __m128d __svml_sin2(__m128d);
    __m128  __svml_cosf4(__m128);
    __m128d __svml_cos2(__m128d);
    __m128  __svml_tanf4(__m128);
    __m128d __svml_tan2(__m128d);
    __m128  __svml_asinf4(__m128);
    __m128d __svml_asin2(__m128d);
    __m128  __svml_acosf4(__m128);
    __m128d __svml_acos2(__m128d);
    __m128  __svml_atanf4(__m128);
    __m128d __svml_atan2(__m128d);
    __m128  __svml_sinhf4(__m128);
    __m128d __svml_sinh2(__m128d);
    __m128  __svml_coshf4(__m128);
    __m128d __svml_cosh2(__m128d);
    __m128  __svml_tanhf4(__m128);
    __m128d __svml_tanh2(__m128d);
    __m128  __svml_asinhf4(__m128);
    __m128d __svml_asinh2(__m128d);
    __m128  __svml_acoshf4(__m128);
    __m128d __svml_acosh2(__m128d);
    __m128  __svml_atanhf4(__m128);
    __m128d __svml_atanh2(__m128d);
    __m128  __svml_cbrtf4(__m128);
    __m128d __svml_cbrt2(__m128d);
    __m128  __svml_cdfnormf4(__m128);
    __m128d __svml_cdfnorm2(__m128d);
    __m128  __svml_cdfnorminvf4(__m128);
    __m128d __svml_cdfnorminv2(__m128d);
    __m128  __svml_truncf4(__m128);
    __m128d __svml_trunc2(__m128d);
    __m128  __svml_invsqrtf4(__m128);
    __m128d __svml_invsqrt2(__m128d);
    __m128  __svml_powf4(__m128, __m128);
    __m128d __svml_pow2(__m128d, __m128d);
    __m128  __svml_powf4_ha(__m128, __m128);
    __m128d __svml_pow2_ha(__m128d, __m128d);
    __m128  __svml_hypotf4(__m128, __m128);
    __m128d __svml_hypot2(__m128d, __m128d);
    __m128  __svml_hypotf4_ha(__m128, __m128);
    __m128d __svml_hypot2_ha(__m128d, __m128d);
#if defined(__VECTORIZATION_COMPILER_MSVC__) && _MSC_VER <= 1920
    __m128  __svml_ceilf4(__m128);
    __m128d __svml_ceil2(__m128d);
    __m128  __svml_floorf4(__m128);
    __m128d __svml_floor2(__m128d);
#endif
}

#if defined(_MSC_VER)

#define SVML_FUNCTION_ONE_ARG(op)                                                           \
    VECTORIZATION_FORCE_INLINE __m128 VECTORIZATION_VECTORCALL _mm_##op##_ps(__m128 x)      \
    { return reinterpret_cast<__m128(VECTORIZATION_VECTORCALL*)(__m128)>(svml_ps(op))(x); } \
                                                                                            \
    VECTORIZATION_FORCE_INLINE __m128d VECTORIZATION_VECTORCALL _mm_##op##_pd(__m128d x)    \
    { return reinterpret_cast<__m128d(VECTORIZATION_VECTORCALL*)(__m128d)>(svml_pd(op))(x); }

#define SVML_FUNCTION_TWO_ARGS(op)                                                               \
    VECTORIZATION_FORCE_INLINE __m128 VECTORIZATION_VECTORCALL _mm_##op##_ps(__m128 x, __m128 y) \
    {                                                                                            \
        return reinterpret_cast<__m128(VECTORIZATION_VECTORCALL*)(__m128, __m128)>(svml_ps(op))( \
            x, y);                                                                               \
    }                                                                                            \
                                                                                                 \
    VECTORIZATION_FORCE_INLINE __m128d VECTORIZATION_VECTORCALL _mm_##op##_pd(                   \
        __m128d x, __m128d y)                                                                    \
    {                                                                                            \
        return reinterpret_cast<__m128d(VECTORIZATION_VECTORCALL*)(__m128d, __m128d)>(           \
            svml_pd(op))(x, y);                                                                  \
    }

#else  // !_MSC_VER

#define SVML_FUNCTION_ONE_ARG(op)                                                        \
    VECTORIZATION_FORCE_INLINE __m128 VECTORIZATION_VECTORCALL _mm_##op##_ps(__m128 x)   \
    { return svml_ps(op)(x); }                                                           \
                                                                                         \
    VECTORIZATION_FORCE_INLINE __m128d VECTORIZATION_VECTORCALL _mm_##op##_pd(__m128d x) \
    { return svml_pd(op)(x); }

#define SVML_FUNCTION_TWO_ARGS(op)                                                               \
    VECTORIZATION_FORCE_INLINE __m128 VECTORIZATION_VECTORCALL _mm_##op##_ps(__m128 x, __m128 y) \
    { return svml_ps(op)(x, y); }                                                                \
                                                                                                 \
    VECTORIZATION_FORCE_INLINE __m128d VECTORIZATION_VECTORCALL _mm_##op##_pd(                   \
        __m128d x, __m128d y)                                                                    \
    { return svml_pd(op)(x, y); }

#endif  // defined(_MSC_VER)

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

#endif  // VECTORIZATION_HAS_SVML && VECTORIZATION_HAS_SSE
