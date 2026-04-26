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

#ifdef VECTORIZATION_HAS_SVML
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-calling-convention"
#endif

#include "backend/avx/svml.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

template <>
struct simd<float>
{
    using simd_t      = __m256;
    using mask_t      = __m256;
    using simd_half_t = __m128;
    using simd_int_t  = __m256i;
    using value_t     = float;
    using int_t       = uint32_t;

    static constexpr int size      = 8;
    static constexpr int half_size = 4;

    inline static simd_t const sign_mask = _mm256_set1_ps(-0.0F);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        _mm_prefetch(PREFETCH_PTR_TYPE(addr), _MM_HINT_T0);
    }

    //======================================================================================
    // load, store, set functions
    //======================================================================================

    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = _mm256_load_ps(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = _mm256_loadu_ps(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { _mm256_store_ps(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { _mm256_storeu_ps(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(scalar_t alpha, simd_t& ret)
    {
        ret = _mm256_set1_ps(static_cast<value_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = _mm256_setzero_ps(); }
    //======================================================================================
    // +, -, *, / functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_add_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_sub_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_mul_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_div_ps(x, y);
    }

#ifdef VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_pow_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_hypot_ps(x, y);
    }
#endif  // VECTORIZATION_HAS_SVML

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_min_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm256_max_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __FMA__
        ret = _mm256_fmadd_ps(x, y, z);
#else
        ret = _mm256_add_ps(_mm256_mul_ps(x, y), z);
#endif  // __FMA__
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        ret = _mm256_or_ps(_mm256_and_ps(sign_mask, sign), _mm256_andnot_ps(sign_mask, x));
    }

    //======================================================================================
    // one arg functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = _mm256_sqrt_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = _mm256_mul_ps(x, x); }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret) { ret = _mm256_ceil_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret) { ret = _mm256_floor_ps(x); }

#ifdef VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret) { ret = _mm256_exp_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret) { ret = _mm256_expm1_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret) { ret = _mm256_exp2_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret) { ret = _mm256_log_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret) { ret = _mm256_log1p_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret) { ret = _mm256_log2_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret) { ret = _mm256_sin_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret) { ret = _mm256_cos_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret) { ret = _mm256_tan_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret) { ret = _mm256_asin_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret) { ret = _mm256_acos_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret) { ret = _mm256_atan_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret) { ret = _mm256_sinh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret) { ret = _mm256_cosh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret) { ret = _mm256_tanh_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret) { ret = _mm256_asinh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret) { ret = _mm256_acosh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret) { ret = _mm256_atanh_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret) { ret = _mm256_cbrt_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret) { ret = _mm256_cdfnorm_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret) { ret = _mm256_cdfnorminv_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret) { ret = _mm256_trunc_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret) { ret = _mm256_invsqrt_ps(x); }
#endif  // VECTORIZATION_HAS_SVML
    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret)
    {
        ret = _mm256_andnot_ps(sign_mask, x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret) { ret = _mm256_xor_ps(x, sign_mask); }

    //======================================================================================
    // horizantal functions
    //======================================================================================
    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        auto t = _mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x));
        t      = _mm_add_ps(t, _mm_movehl_ps(t, t));
        t      = _mm_add_ss(t, _mm_shuffle_ps(t, t, 0x55));

        return static_cast<double>(_mm_cvtss_f32(t));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x)
    {
        auto t = _mm256_max_ps(x, _mm256_permute2f128_ps(x, x, 1));
        t      = _mm256_max_ps(t, _mm256_shuffle_ps(t, t, _MM_SHUFFLE(1, 0, 3, 2)));
        return _mm_cvtss_f32(_mm256_castps256_ps128(_mm256_max_ps(t, _mm256_shuffle_ps(t, t, 1))));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x)
    {
        auto t = _mm256_min_ps(x, _mm256_permute2f128_ps(x, x, 1));
        t      = _mm256_min_ps(t, _mm256_shuffle_ps(t, t, _MM_SHUFFLE(1, 0, 3, 2)));
        return _mm_cvtss_f32(
            _mm256_castps256_ps128((_mm256_min_ps(t, _mm256_shuffle_ps(t, t, 1)))));
    }

    //======================================================================================
    // gather/scatter function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_t& to)
    {
        to = _mm256_set_ps(
            from[7 * stride],
            from[6 * stride],
            from[5 * stride],
            from[4 * stride],
            from[3 * stride],
            from[2 * stride],
            from[stride],
            from[0]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, int stride, value_t* to)
    {
        auto low       = _mm256_extractf128_ps(from, 0);
        to[0]          = _mm_cvtss_f32(low);
        to[stride]     = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 1));
        to[stride * 2] = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 2));
        to[stride * 3] = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 3));

        auto high      = _mm256_extractf128_ps(from, 1);
        to[stride * 4] = _mm_cvtss_f32(high);
        to[stride * 5] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 1));
        to[stride * 6] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 2));
        to[stride * 7] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 3));
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, const int* strides, simd_t& to)
    {
#ifdef __AVX2__
        auto indices = _mm256_loadu_si256(reinterpret_cast<const simd_int_t*>(strides));
        to           = _mm256_i32gather_ps(from, indices, 4);
#else
        to = _mm256_set_ps(
            from[strides[7]],
            from[strides[6]],
            from[strides[5]],
            from[strides[4]],
            from[strides[3]],
            from[strides[2]],
            from[strides[1]],
            from[strides[0]]);
#endif  // __AVX2__
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, const int* stride, value_t* to)
    {
        auto low      = _mm256_extractf128_ps(from, 0);
        to[stride[0]] = _mm_cvtss_f32(low);
        to[stride[1]] = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 1));
        to[stride[2]] = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 2));
        to[stride[3]] = _mm_cvtss_f32(_mm_shuffle_ps(low, low, 3));

        auto high     = _mm256_extractf128_ps(from, 1);
        to[stride[4]] = _mm_cvtss_f32(high);
        to[stride[5]] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 1));
        to[stride[6]] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 2));
        to[stride[7]] = _mm_cvtss_f32(_mm_shuffle_ps(high, high, 3));
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = _mm256_blendv_ps(z, y, x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_EQ_OQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_NEQ_UQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_GT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_LT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_GE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm256_cmp_ps(x, y, _CMP_LE_OS);
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = _mm256_castsi256_ps(_mm256_loadu_si256(reinterpret_cast<const simd_int_t*>(from)));
    };

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = _mm256_castsi256_ps(_mm256_load_si256(reinterpret_cast<const simd_int_t*>(from)));
    };

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        _mm256_storeu_si256(reinterpret_cast<simd_int_t*>(to), _mm256_castps_si256(from));
    };

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        _mm256_store_si256(reinterpret_cast<simd_int_t*>(to), _mm256_castps_si256(from));
    };

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret)
    {
        ret = _mm256_andnot_ps(x, _mm256_cmp_ps(x, x, _CMP_EQ_OQ));
    };

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_and_ps(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_or_ps(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm256_xor_ps(x, y);
    };

    //-----------------------------------------------------------------------------
    VECTORIZATION_FORCE_INLINE
    static void broadcast(const value_t* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = _mm256_broadcast_ss(from);
        a1 = _mm256_broadcast_ss(from + 1);
        a2 = _mm256_broadcast_ss(from + 2);
        a3 = _mm256_broadcast_ss(from + 3);
    }

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t simd[N])  // NOLINT
    {
        if constexpr (N == 8)
        {
            auto T0 = _mm256_unpacklo_ps(simd[0], simd[1]);
            auto T1 = _mm256_unpackhi_ps(simd[0], simd[1]);
            auto T2 = _mm256_unpacklo_ps(simd[2], simd[3]);
            auto T3 = _mm256_unpackhi_ps(simd[2], simd[3]);
            auto T4 = _mm256_unpacklo_ps(simd[4], simd[5]);
            auto T5 = _mm256_unpackhi_ps(simd[4], simd[5]);
            auto T6 = _mm256_unpacklo_ps(simd[6], simd[7]);
            auto T7 = _mm256_unpackhi_ps(simd[6], simd[7]);

            auto S0 = _mm256_shuffle_ps(T0, T2, _MM_SHUFFLE(1, 0, 1, 0));
            auto S1 = _mm256_shuffle_ps(T0, T2, _MM_SHUFFLE(3, 2, 3, 2));
            auto S2 = _mm256_shuffle_ps(T1, T3, _MM_SHUFFLE(1, 0, 1, 0));
            auto S3 = _mm256_shuffle_ps(T1, T3, _MM_SHUFFLE(3, 2, 3, 2));
            auto S4 = _mm256_shuffle_ps(T4, T6, _MM_SHUFFLE(1, 0, 1, 0));
            auto S5 = _mm256_shuffle_ps(T4, T6, _MM_SHUFFLE(3, 2, 3, 2));
            auto S6 = _mm256_shuffle_ps(T5, T7, _MM_SHUFFLE(1, 0, 1, 0));
            auto S7 = _mm256_shuffle_ps(T5, T7, _MM_SHUFFLE(3, 2, 3, 2));

            simd[0] = _mm256_permute2f128_ps(S0, S4, 0x20);
            simd[1] = _mm256_permute2f128_ps(S1, S5, 0x20);
            simd[2] = _mm256_permute2f128_ps(S2, S6, 0x20);
            simd[3] = _mm256_permute2f128_ps(S3, S7, 0x20);
            simd[4] = _mm256_permute2f128_ps(S0, S4, 0x31);
            simd[5] = _mm256_permute2f128_ps(S1, S5, 0x31);
            simd[6] = _mm256_permute2f128_ps(S2, S6, 0x31);
            simd[7] = _mm256_permute2f128_ps(S3, S7, 0x31);
        }
        else
        {
            auto T0 = _mm256_unpacklo_ps(simd[0], simd[1]);
            auto T1 = _mm256_unpackhi_ps(simd[0], simd[1]);
            auto T2 = _mm256_unpacklo_ps(simd[2], simd[3]);
            auto T3 = _mm256_unpackhi_ps(simd[2], simd[3]);

            auto S0 = _mm256_shuffle_ps(T0, T2, _MM_SHUFFLE(1, 0, 1, 0));
            auto S1 = _mm256_shuffle_ps(T0, T2, _MM_SHUFFLE(3, 2, 3, 2));
            auto S2 = _mm256_shuffle_ps(T1, T3, _MM_SHUFFLE(1, 0, 1, 0));
            auto S3 = _mm256_shuffle_ps(T1, T3, _MM_SHUFFLE(3, 2, 3, 2));

            simd[0] = _mm256_permute2f128_ps(S0, S1, 0x20);
            simd[1] = _mm256_permute2f128_ps(S2, S3, 0x20);
            simd[2] = _mm256_permute2f128_ps(S0, S1, 0x31);
            simd[3] = _mm256_permute2f128_ps(S2, S3, 0x31);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        _mm256_store_ps(to, _mm256_loadu_ps(from));
    }

    VECTORIZATION_SIMD_RETURN_TYPE ploadquad(const value_t* from, simd_t& y)
    {
        auto t = _mm256_castps128_ps256(_mm_broadcast_ss(from));
        y      = _mm256_insertf128_ps(t, _mm_broadcast_ss(from + 1), 1);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(const value_t* from, int stride, simd_half_t& ret)
    {
        ret = _mm_set_ps(from[3 * stride], from[2 * stride], from[1 * stride], from[0 * stride]);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t loadu_half(const value_t* from)
    {
        return _mm_loadu_ps(from);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t set(const value_t* from) { return _mm_set1_ps(*from); }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
#ifdef __FMA__
        ret = _mm_fmadd_ps(x, y, ret);
#else
        ret = _mm_add_ps(_mm_mul_ps(x, y), ret);
#endif  // __FMA__
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
        ret = _mm_add_ps(x, y);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t predux_downto4(const simd_t& x)
    {
        return _mm_add_ps(_mm256_castps256_ps128(x), _mm256_extractf128_ps(x, 1));
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_half_t& from, int stride, value_t* to)
    {
        to[0]          = _mm_cvtss_f32(from);
        to[stride]     = _mm_cvtss_f32(_mm_shuffle_ps(from, from, 1));
        to[stride * 2] = _mm_cvtss_f32(_mm_shuffle_ps(from, from, 2));
        to[stride * 3] = _mm_cvtss_f32(_mm_shuffle_ps(from, from, 3));
    }
};