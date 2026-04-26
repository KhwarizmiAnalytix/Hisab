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


#if VECTORIZATION_HAS_SVML
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-calling-convention"
#endif

#include "backend/avx/svml.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#if !VECTORIZATION_HAS_SVML
#  include <cmath>

#  include "common/normal_cdf.h"
#endif

template <>
struct simd<double>
{
    using simd_t      = __m256d;
    using mask_t      = __m256d;
    using simd_half_t = __m128d;
    using simd_int_t  = __m128i;
    using value_t     = double;
    using int_t       = uint32_t;

    static constexpr int size      = 4;
    static constexpr int half_size = 2;

    inline static simd_t const sign_mask = _mm256_set1_pd(-0.0);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        _mm_prefetch(PREFETCH_PTR_TYPE(addr), _MM_HINT_T0);
    }

    //======================================================================================
    // load, store, set functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = _mm256_load_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = _mm256_loadu_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { _mm256_store_pd(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { _mm256_storeu_pd(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(const scalar_t alpha, simd_t& ret)
    {
        ret = _mm256_set1_pd(static_cast<value_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = _mm256_setzero_pd(); }

    //======================================================================================
    // +, -, *, /, min, max, hypo functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_add_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_sub_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_mul_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_div_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __FMA__
        ret = _mm256_fmadd_pd(x, y, z);
#else
        ret = _mm256_add_pd(_mm256_mul_pd(x, y), z);
#endif  // __FMA__
    }

#if VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_pow_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_hypot_pd(x, y);
    }
#else
    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        alignas(32) value_t bx[size];
        alignas(32) value_t by[size];
        storeu(x, bx);
        storeu(y, by);
        for (int i = 0; i < size; ++i)
            bx[i] = std::pow(bx[i], by[i]);
        loadu(bx, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        alignas(32) value_t bx[size];
        alignas(32) value_t by[size];
        storeu(x, bx);
        storeu(y, by);
        for (int i = 0; i < size; ++i)
            bx[i] = std::hypot(bx[i], by[i]);
        loadu(bx, ret);
    }
#endif  // VECTORIZATION_HAS_SVML

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_min_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_max_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        ret = _mm256_or_pd(_mm256_and_pd(sign_mask, sign), _mm256_andnot_pd(sign_mask, x));
    }

    //======================================================================================
    // one arg function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = _mm256_sqrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = _mm256_mul_pd(x, x); }
    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret) { ret = _mm256_ceil_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret) { ret = _mm256_floor_pd(x); }

#if VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret) { ret = _mm256_exp_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret) { ret = _mm256_expm1_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret) { ret = _mm256_exp2_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret) { ret = _mm256_log_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret) { ret = _mm256_log1p_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret) { ret = _mm256_log2_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret) { ret = _mm256_sin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret) { ret = _mm256_cos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret) { ret = _mm256_tan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret) { ret = _mm256_asin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret) { ret = _mm256_acos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret) { ret = _mm256_atan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret) { ret = _mm256_sinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret) { ret = _mm256_cosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret) { ret = _mm256_tanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret) { ret = _mm256_asinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret) { ret = _mm256_acosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret) { ret = _mm256_atanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret) { ret = _mm256_cbrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret) { ret = _mm256_cdfnorm_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret) { ret = _mm256_cdfnorminv_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret) { ret = _mm256_trunc_pd(x); }
#else
    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::exp(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::expm1(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::exp2(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::log(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::log1p(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::log2(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::sin(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::cos(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::tan(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::asin(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::acos(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::atan(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::sinh(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::cosh(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::tanh(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::asinh(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::acosh(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::atanh(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::cbrt(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = vectorization::normalcdf(b[i]);
        loadu(b, ret);
    }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = vectorization::inv_normalcdf(b[i]);
        loadu(b, ret);
    }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret)
    {
        alignas(32) value_t b[size];
        storeu(x, b);
        for (int i = 0; i < size; ++i)
            b[i] = std::trunc(b[i]);
        loadu(b, ret);
    }
#endif  // VECTORIZATION_HAS_SVML

#if VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret) { ret = _mm256_invsqrt_pd(x); }
#else
    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret)
    {
        simd_t s;
        sqrt(x, s);
        ret = _mm256_div_pd(_mm256_set1_pd(1.0), s);
    }
#endif

    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret)
    {
        ret = _mm256_andnot_pd(sign_mask, x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret) { ret = _mm256_xor_pd(x, sign_mask); }

    //======================================================================================
    // horizantal functions
    //======================================================================================
    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        const auto& t1 = _mm_add_pd(_mm256_extractf128_pd(x, 1), _mm256_castpd256_pd128(x));
        const auto& t2 = _mm_unpackhi_pd(t1, t1);

        return _mm_cvtsd_f64(_mm_add_sd(t1, t2));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x)
    {
        const auto& t = _mm256_max_pd(x, _mm256_permute2f128_pd(x, x, 1));
        return _mm256_cvtsd_f64(_mm256_max_pd(t, _mm256_permute_pd(t, 5)));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x)
    {
        const auto& tmp = _mm256_min_pd(x, _mm256_permute2f128_pd(x, x, 1));
        return _mm256_cvtsd_f64(_mm256_min_pd(tmp, _mm256_permute_pd(tmp, 5)));
    }

    //======================================================================================
    // gather/scatter function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_t& to)
    {
        to = _mm256_set_pd(from[3 * stride], from[2 * stride], from[1 * stride], from[0 * stride]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, int stride, value_t* to)
    {
        auto low   = _mm256_extractf128_pd(from, 0);
        to[0]      = _mm_cvtsd_f64(low);
        to[stride] = _mm_cvtsd_f64(_mm_shuffle_pd(low, low, 1));

        auto high      = _mm256_extractf128_pd(from, 1);
        to[stride * 2] = _mm_cvtsd_f64(high);
        to[stride * 3] = _mm_cvtsd_f64(_mm_shuffle_pd(high, high, 1));
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, const int* strides, simd_t& to)
    {
#ifdef __AVX2__
        auto indices = _mm_loadu_si128(reinterpret_cast<const __m128i*>(strides));
        to           = _mm256_i32gather_pd(from, indices, 8);
#else
        to = _mm256_set_pd(from[strides[3]], from[strides[2]], from[strides[1]], from[strides[0]]);
#endif  // __AVX2__
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, const int* stride, value_t* to)
    {
        auto low      = _mm256_extractf128_pd(from, 0);
        to[stride[0]] = _mm_cvtsd_f64(low);
        to[stride[1]] = _mm_cvtsd_f64(_mm_shuffle_pd(low, low, 1));

        auto high     = _mm256_extractf128_pd(from, 1);
        to[stride[2]] = _mm_cvtsd_f64(high);
        to[stride[3]] = _mm_cvtsd_f64(_mm_shuffle_pd(high, high, 1));
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = _mm256_blendv_pd(z, y, x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_EQ_OQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_NEQ_UQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_GT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_LT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_GE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_pd(x, y, _CMP_LE_OS);
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = _mm256_cvtepi32_pd(_mm_loadu_si128(reinterpret_cast<const simd_int_t*>(from)));
    };

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = _mm256_cvtepi32_pd(_mm_load_si128(reinterpret_cast<const simd_int_t*>(from)));
    };

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        _mm_storeu_si128(reinterpret_cast<simd_int_t*>(to), _mm256_cvtpd_epi32(from));
    };

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        _mm_store_si128(reinterpret_cast<simd_int_t*>(to), _mm256_cvtpd_epi32(from));
    };

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret)
    {
        ret = _mm256_andnot_pd(x, _mm256_cmp_pd(x, x, _CMP_EQ_OQ));
    };

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_and_pd(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_or_pd(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_xor_pd(x, y);
    };

    //-----------------------------------------------------------------------------
    VECTORIZATION_SIMD_RETURN_TYPE broadcast(
        const double* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = _mm256_broadcast_sd(from);
        a1 = _mm256_broadcast_sd(from + 1);
        a2 = _mm256_broadcast_sd(from + 2);
        a3 = _mm256_broadcast_sd(from + 3);
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        _mm256_store_pd(to, _mm256_loadu_pd(from));
    }

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t x[N])  // NOLINT
    {
        auto T0 = _mm256_shuffle_pd(x[0], x[1], 15);
        auto T1 = _mm256_shuffle_pd(x[0], x[1], 0);
        auto T2 = _mm256_shuffle_pd(x[2], x[3], 15);
        auto T3 = _mm256_shuffle_pd(x[2], x[3], 0);

        x[1] = _mm256_permute2f128_pd(T0, T2, 32);
        x[3] = _mm256_permute2f128_pd(T0, T2, 49);
        x[0] = _mm256_permute2f128_pd(T1, T3, 32);
        x[2] = _mm256_permute2f128_pd(T1, T3, 49);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ploadquad(const double* from, simd_t& to)
    {
        to = _mm256_broadcast_sd(from);
    }
};
