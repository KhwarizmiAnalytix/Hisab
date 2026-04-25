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

#include "avx512/svml.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

template <>
struct simd<double>
{
    using simd_t      = __m512d;
    using simd_half_t = __m256d;
    using simd_int_t  = __m256i;
    using value_t     = double;
    using mask_t      = __mmask8;
    using int_t       = uint32_t;

    static constexpr int size      = 8;
    static constexpr int half_size = 4;

    inline static simd_t const     sign_mask         = _mm512_set1_pd(-0.0);
    inline static simd_int_t const stride_multiplier = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        _mm_prefetch(PREFETCH_PTR_TYPE(addr), _MM_HINT_T0);
    }

    //======================================================================================
    // load, store, set functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = _mm512_load_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = _mm512_loadu_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { _mm512_store_pd(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { _mm512_storeu_pd(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(scalar_t alpha, simd_t& ret)
    {
        ret = _mm512_set1_pd(static_cast<value_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = _mm512_setzero_pd(); }
    //======================================================================================
    // +, -, *, /, min, max, hypo functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_add_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_sub_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_mul_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_div_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __FMA__
        ret = _mm512_fmadd_pd(x, y, z);
#else
        ret = _mm512_add_pd(_mm512_mul_pd(x, y), z);
#endif  // __FMA__
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_pow_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_hypot_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_min_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_max_pd(x, y);
    }

    //======================================================================================
    // one arg function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = _mm512_sqrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = _mm512_mul_pd(x, x); }
    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret) { ret = _mm512_ceil_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret) { ret = _mm512_floor_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret) { ret = _mm512_exp_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret) { ret = _mm512_expm1_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret) { ret = _mm512_exp2_pd(x); }
    // VECTORIZATION_SIMD_RETURN_TYPE exp10(const simd_t& x, simd_t& ret) { ret = _mm512_exp10_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret) { ret = _mm512_log_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret) { ret = _mm512_log1p_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret) { ret = _mm512_log2_pd(x); }
    // VECTORIZATION_SIMD_RETURN_TYPE log10(const simd_t& x, simd_t& ret) { ret = _mm512_log10_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret) { ret = _mm512_sin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret) { ret = _mm512_cos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret) { ret = _mm512_tan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret) { ret = _mm512_asin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret) { ret = _mm512_acos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret) { ret = _mm512_atan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret) { ret = _mm512_sinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret) { ret = _mm512_cosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret) { ret = _mm512_tanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret) { ret = _mm512_asinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret) { ret = _mm512_acosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret) { ret = _mm512_atanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret) { ret = _mm512_cbrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret) { ret = _mm512_cdfnorm_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret) { ret = _mm512_cdfnorminv_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret) { ret = _mm512_trunc_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret) { ret = _mm512_invsqrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret) { ret = _mm512_abs_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret) { ret = _mm512_xor_pd(x, sign_mask); }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        const auto& sign_z = _mm512_and_pd(sign_mask, sign);
        const auto& fabs_x = _mm512_andnot_pd(sign_mask, x);
        ret                = _mm512_xor_pd(sign_z, fabs_x);
    }

    //======================================================================================
    // horizantal functions
    //======================================================================================
    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        return _mm512_reduce_add_pd(x);
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x) { return _mm512_reduce_max_pd(x); }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x) { return _mm512_reduce_min_pd(x); }

    //======================================================================================
    // gather/scatter function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE
    gather(value_t const* from, int stride, simd_t& to)
    {
        const auto stride_vector = _mm256_set1_epi32(stride);
        const auto indices       = _mm256_mullo_epi32(stride_vector, stride_multiplier);
        to                       = _mm512_i32gather_pd(indices, from, 8);
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    scatter(const simd_t& from, int stride, value_t* to)
    {
        const auto stride_vector = _mm256_set1_epi32(stride);
        const auto indices       = _mm256_mullo_epi32(stride_vector, stride_multiplier);
        _mm512_i32scatter_pd(to, indices, from, 8);
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    gather(value_t const* from, const int* strides, simd_t& to)
    {
        auto indices = _mm256_loadu_si256(reinterpret_cast<const simd_int_t*>(strides));
        to           = _mm512_i32gather_pd(indices, from, 8);
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    scatter(const simd_t& from, const int* strides, value_t* to)
    {
        auto indices = _mm256_loadu_si256(reinterpret_cast<const simd_int_t*>(strides));
        _mm512_i32scatter_pd(to, indices, from, 8);
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = _mm512_mask_mov_pd(z, x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_EQ_OQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_NEQ_UQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_GT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_LT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_GE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_pd_mask(x, y, _CMP_LE_OS);
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = 0x00;
        if (from[0] != 0)
            ret |= 0x01;
        if (from[1] != 0)
            ret |= 0x02;
        if (from[2] != 0)
            ret |= 0x04;
        if (from[3] != 0)
            ret |= 0x08;
        if (from[4] != 0)
            ret |= 0x10;
        if (from[5] != 0)
            ret |= 0x20;
        if (from[6] != 0)
            ret |= 0x40;
        if (from[7] != 0)
            ret |= 0x80;
    };

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = 0x00;
        if (from[0] != 0)
            ret |= 0x01;
        if (from[1] != 0)
            ret |= 0x02;
        if (from[2] != 0)
            ret |= 0x04;
        if (from[3] != 0)
            ret |= 0x08;
        if (from[4] != 0)
            ret |= 0x10;
        if (from[5] != 0)
            ret |= 0x20;
        if (from[6] != 0)
            ret |= 0x40;
        if (from[7] != 0)
            ret |= 0x80;
    };

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        to[0] = (from & 0x01);
        to[1] = (from & 0x02);
        to[2] = (from & 0x04);
        to[3] = (from & 0x08);
        to[4] = (from & 0x10);
        to[5] = (from & 0x20);
        to[6] = (from & 0x40);
        to[7] = (from & 0x80);
    };

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        to[0] = (from & 0x01);
        to[1] = (from & 0x02);
        to[2] = (from & 0x04);
        to[3] = (from & 0x08);
        to[4] = (from & 0x10);
        to[5] = (from & 0x20);
        to[6] = (from & 0x40);
        to[7] = (from & 0x80);
    };

    VECTORIZATION_SIMD_RETURN_TYPE set(int_t const from, mask_t& ret) { ret = (from != 0 ? 0xFF : 0X00); };

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret) { ret = _knot_mask8(x); }

    VECTORIZATION_SIMD_RETURN_TYPE
    and_mask(const mask_t& x, const mask_t& y, mask_t& ret) { ret = _kand_mask8(x, y); }

    VECTORIZATION_SIMD_RETURN_TYPE
    or_mask(const mask_t& x, const mask_t& y, mask_t& ret) { ret = _kor_mask8(x, y); }

    VECTORIZATION_SIMD_RETURN_TYPE
    xor_mask(const mask_t& x, const mask_t& y, mask_t& ret) { ret = _kxor_mask8(x, y); }

    //-----------------------------------------------------------------------------
    VECTORIZATION_SIMD_RETURN_TYPE
    broadcast(const value_t* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = _mm512_set1_pd(from[0]);
        a1 = _mm512_set1_pd(from[1]);
        a2 = _mm512_set1_pd(from[2]);
        a3 = _mm512_set1_pd(from[3]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        _mm512_store_pd(to, _mm512_loadu_pd(from));
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t predux_downto4(const simd_t& a)
    {
        auto lane0 = _mm512_extractf64x4_pd(a, 0);
        auto lane1 = _mm512_extractf64x4_pd(a, 1);
        return _mm256_add_pd(lane0, lane1);
    }

    // Loads 2 doubles from memory a returns the simd
    // {a0, a0  a0, a0, a1, a1, a1, a1}
    VECTORIZATION_FORCE_INLINE static void ploadquad(const double* from, simd_t& to)
    {
        to = _mm512_set_pd(from[1], from[1], from[1], from[1], from[0], from[0], from[0], from[0]);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t loadu_half(const value_t* from)
    {
        return _mm256_loadu_pd(from);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t set(const value_t* from)
    {
        return _mm256_set1_pd(*from);
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
        ret = _mm256_add_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
#ifdef __FMA__
        ret = _mm256_fmadd_pd(x, y, ret);
#else
        ret = _mm256_add_pd(_mm_mul_pd(x, y), ret);
#endif
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_half_t& ret)
    {
        ret = _mm256_set_pd(from[3 * stride], from[2 * stride], from[stride], from[0]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_half_t& from, int stride, value_t* to)
    {
        auto low   = _mm256_extractf128_pd(from, 0);
        to[0]      = _mm_cvtsd_f64(low);
        to[stride] = _mm_cvtsd_f64(_mm_shuffle_pd(low, low, 1));

        auto high      = _mm256_extractf128_pd(from, 1);
        to[stride * 2] = _mm_cvtsd_f64(high);
        to[stride * 3] = _mm_cvtsd_f64(_mm_shuffle_pd(high, high, 1));
    }

#define PACK_OUTPUT_SQ_D(OUTPUT, INPUT, INDEX, STRIDE)                  \
    OUTPUT[INDEX] = _mm512_insertf64x4(OUTPUT[INDEX], INPUT[INDEX], 0); \
    OUTPUT[INDEX] = _mm512_insertf64x4(OUTPUT[INDEX], INPUT[INDEX + STRIDE], 1);

#define PACK_OUTPUT_D(OUTPUT, INPUT, INDEX, STRIDE)                           \
    OUTPUT[INDEX] = _mm512_insertf64x4(OUTPUT[INDEX], INPUT[(2 * INDEX)], 0); \
    OUTPUT[INDEX] = _mm512_insertf64x4(OUTPUT[INDEX], INPUT[(2 * INDEX) + STRIDE], 1);

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t x[N])
    {
        if constexpr (N == 8)
        {
            auto T0 = _mm512_unpacklo_pd(x[0], x[1]);
            auto T1 = _mm512_unpackhi_pd(x[0], x[1]);
            auto T2 = _mm512_unpacklo_pd(x[2], x[3]);
            auto T3 = _mm512_unpackhi_pd(x[2], x[3]);
            auto T4 = _mm512_unpacklo_pd(x[4], x[5]);
            auto T5 = _mm512_unpackhi_pd(x[4], x[5]);
            auto T6 = _mm512_unpacklo_pd(x[6], x[7]);
            auto T7 = _mm512_unpackhi_pd(x[6], x[7]);

            __m256d tmp[16];  // NOLINT

            tmp[0] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 0), _mm512_extractf64x4_pd(T2, 0), 0x20);
            tmp[1] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 0), _mm512_extractf64x4_pd(T3, 0), 0x20);
            tmp[2] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 0), _mm512_extractf64x4_pd(T2, 0), 0x31);
            tmp[3] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 0), _mm512_extractf64x4_pd(T3, 0), 0x31);

            tmp[4] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 1), _mm512_extractf64x4_pd(T2, 1), 0x20);
            tmp[5] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 1), _mm512_extractf64x4_pd(T3, 1), 0x20);
            tmp[6] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 1), _mm512_extractf64x4_pd(T2, 1), 0x31);
            tmp[7] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 1), _mm512_extractf64x4_pd(T3, 1), 0x31);

            tmp[8] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T4, 0), _mm512_extractf64x4_pd(T6, 0), 0x20);
            tmp[9] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T5, 0), _mm512_extractf64x4_pd(T7, 0), 0x20);
            tmp[10] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T4, 0), _mm512_extractf64x4_pd(T6, 0), 0x31);
            tmp[11] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T5, 0), _mm512_extractf64x4_pd(T7, 0), 0x31);

            tmp[12] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T4, 1), _mm512_extractf64x4_pd(T6, 1), 0x20);
            tmp[13] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T5, 1), _mm512_extractf64x4_pd(T7, 1), 0x20);
            tmp[14] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T4, 1), _mm512_extractf64x4_pd(T6, 1), 0x31);
            tmp[15] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T5, 1), _mm512_extractf64x4_pd(T7, 1), 0x31);

            PACK_OUTPUT_SQ_D(x, tmp, 0, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 1, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 2, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 3, 8);

            PACK_OUTPUT_SQ_D(x, tmp, 4, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 5, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 6, 8);
            PACK_OUTPUT_SQ_D(x, tmp, 7, 8);
        }
        else
        {
            auto T0 = _mm512_shuffle_pd(x[0], x[1], 0);
            auto T1 = _mm512_shuffle_pd(x[0], x[1], 0xff);
            auto T2 = _mm512_shuffle_pd(x[2], x[3], 0);
            auto T3 = _mm512_shuffle_pd(x[2], x[3], 0xff);

            __m256d tmp[8];  // NOLINT

            tmp[0] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 0), _mm512_extractf64x4_pd(T2, 0), 0x20);
            tmp[1] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 0), _mm512_extractf64x4_pd(T3, 0), 0x20);
            tmp[2] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 0), _mm512_extractf64x4_pd(T2, 0), 0x31);
            tmp[3] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 0), _mm512_extractf64x4_pd(T3, 0), 0x31);

            tmp[4] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 1), _mm512_extractf64x4_pd(T2, 1), 0x20);
            tmp[5] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 1), _mm512_extractf64x4_pd(T3, 1), 0x20);
            tmp[6] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T0, 1), _mm512_extractf64x4_pd(T2, 1), 0x31);
            tmp[7] = _mm256_permute2f128_pd(
                _mm512_extractf64x4_pd(T1, 1), _mm512_extractf64x4_pd(T3, 1), 0x31);

            PACK_OUTPUT_D(x, tmp, 0, 1);
            PACK_OUTPUT_D(x, tmp, 1, 1);
            PACK_OUTPUT_D(x, tmp, 2, 1);
            PACK_OUTPUT_D(x, tmp, 3, 1);
        }
    }
};

