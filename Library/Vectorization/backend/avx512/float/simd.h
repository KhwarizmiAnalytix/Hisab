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

#include "backend/avx512/svml.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

template <>
struct simd<float>
{
    using simd_t      = __m512;
    using simd_half_t = __m256;
    using simd_int_t  = __m512i;
    using value_t     = float;
    using mask_t      = __mmask16;
    using int_t       = uint32_t;

    static constexpr int size      = 16;
    static constexpr int half_size = 8;

    inline static simd_t const     sign_mask = _mm512_set1_ps(-0.0F);
    inline static simd_int_t const stride_multiplier =
        _mm512_set_epi32(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        _mm_prefetch(PREFETCH_PTR_TYPE(addr), _MM_HINT_T0);
    }

    //======================================================================================
    // load, store, set functions
    //======================================================================================

    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = _mm512_load_ps(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = _mm512_loadu_ps(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { _mm512_store_ps(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { _mm512_storeu_ps(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(scalar_t alpha, simd_t& ret)
    {
        ret = _mm512_set1_ps(static_cast<value_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = _mm512_setzero_ps(); }
    //======================================================================================
    // +, -, *, / functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_add_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_sub_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_mul_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_div_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_pow_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_hypot_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_min_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm512_max_ps(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __FMA__
        ret = _mm512_fmadd_ps(x, y, z);
#else
        ret = _mm512_add_ps(_mm512_mul_ps(x, y), z);
#endif  // __FMA__
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        const auto& sign_z = _mm512_and_ps(sign_mask, sign);
        const auto& fabs_x = _mm512_andnot_ps(sign_mask, x);
        ret                = _mm512_xor_ps(sign_z, fabs_x);
    }

    //======================================================================================
    // one arg functions
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = _mm512_sqrt_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = _mm512_mul_ps(x, x); }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret) { ret = _mm512_ceil_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret) { ret = _mm512_floor_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret) { ret = _mm512_exp_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret) { ret = _mm512_expm1_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret) { ret = _mm512_exp2_ps(x); }
    // VECTORIZATION_SIMD_RETURN_TYPE exp10(const simd_t& x, simd_t& ret) { ret = _mm512_exp10_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret) { ret = _mm512_log_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret) { ret = _mm512_log1p_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret) { ret = _mm512_log2_ps(x); }
    // VECTORIZATION_SIMD_RETURN_TYPE log10(const simd_t& x, simd_t& ret) { ret = _mm512_log10_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret) { ret = _mm512_sin_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret) { ret = _mm512_cos_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret) { ret = _mm512_tan_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret) { ret = _mm512_asin_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret) { ret = _mm512_acos_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret) { ret = _mm512_atan_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret) { ret = _mm512_sinh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret) { ret = _mm512_cosh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret) { ret = _mm512_tanh_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret) { ret = _mm512_asinh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret) { ret = _mm512_acosh_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret) { ret = _mm512_atanh_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret) { ret = _mm512_cbrt_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret) { ret = _mm512_cdfnorm_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret) { ret = _mm512_cdfnorminv_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret) { ret = _mm512_trunc_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret) { ret = _mm512_invsqrt_ps(x); }
    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret) { ret = _mm512_abs_ps(x); }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret) { ret = _mm512_xor_ps(x, sign_mask); }

    //======================================================================================
    // horizantal functions
    //======================================================================================
    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        return static_cast<double>(_mm512_reduce_add_ps(x));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x) { return _mm512_reduce_max_ps(x); }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x) { return _mm512_reduce_min_ps(x); }

    //======================================================================================
    // gather/scatter function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE
    gather(const value_t* from, int stride, simd_t& ret)
    {
        // Create an index vector based on the stride
        __m512i index = _mm512_mullo_epi32(_mm512_set1_epi32(stride), stride_multiplier);

        // Use AVX-512 gather instruction
        ret = _mm512_i32gather_ps(index, from, sizeof(float));
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    scatter(const simd_t& from, int stride, value_t* to)
    {
        // Create an index vector based on the stride
        __m512i index = _mm512_mullo_epi32(_mm512_set1_epi32(stride), stride_multiplier);

        // Use AVX-512 scatter instruction
        _mm512_i32scatter_ps(to, index, from, sizeof(float));
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    gather(const value_t* from, const int* strides, simd_t& to)
    {
        // Load strides into a SIMD register
        __m512i index = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(strides));

        // Use AVX-512 gather instruction
        to = _mm512_i32gather_ps(index, from, sizeof(float));
    }

    VECTORIZATION_SIMD_RETURN_TYPE
    scatter(const simd_t& from, const int* strides, value_t* to)
    {
        // Load strides into a SIMD register
        __m512i index = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(strides));

        // Use AVX-512 scatter instruction
        _mm512_i32scatter_ps(to, index, from, sizeof(float));
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = _mm512_mask_mov_ps(z, x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_EQ_OQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_NEQ_UQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_GT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_LT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_GE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm512_cmp_ps_mask(x, y, _CMP_LE_OS);
    }

    //======================================================================================
    // comparaison function
    //======================================================================================
    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = 0x0000;
        if (from[0] != 0)
            ret |= 0x0001;
        if (from[1] != 0)
            ret |= 0x0002;
        if (from[2] != 0)
            ret |= 0x0004;
        if (from[3] != 0)
            ret |= 0x0008;
        if (from[4] != 0)
            ret |= 0x0010;
        if (from[5] != 0)
            ret |= 0x0020;
        if (from[6] != 0)
            ret |= 0x0040;
        if (from[7] != 0)
            ret |= 0x0080;
        if (from[8] != 0)
            ret |= 0x0100;
        if (from[9] != 0)
            ret |= 0x0200;
        if (from[10] != 0)
            ret |= 0x0400;
        if (from[11] != 0)
            ret |= 0x0800;
        if (from[12] != 0)
            ret |= 0x1000;
        if (from[13] != 0)
            ret |= 0x2000;
        if (from[14] != 0)
            ret |= 0x4000;
        if (from[15] != 0)
            ret |= 0x8000;
    };

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = 0x0000;
        if (from[0] != 0)
            ret |= 0x0001;
        if (from[1] != 0)
            ret |= 0x0002;
        if (from[2] != 0)
            ret |= 0x0004;
        if (from[3] != 0)
            ret |= 0x0008;
        if (from[4] != 0)
            ret |= 0x0010;
        if (from[5] != 0)
            ret |= 0x0020;
        if (from[6] != 0)
            ret |= 0x0040;
        if (from[7] != 0)
            ret |= 0x0080;
        if (from[8] != 0)
            ret |= 0x0100;
        if (from[9] != 0)
            ret |= 0x0200;
        if (from[10] != 0)
            ret |= 0x0400;
        if (from[11] != 0)
            ret |= 0x0800;
        if (from[12] != 0)
            ret |= 0x1000;
        if (from[13] != 0)
            ret |= 0x2000;
        if (from[14] != 0)
            ret |= 0x4000;
        if (from[15] != 0)
            ret |= 0x8000;
    };

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        to[0]  = (from & 0x0001);
        to[1]  = (from & 0x0002);
        to[2]  = (from & 0x0004);
        to[3]  = (from & 0x0008);
        to[4]  = (from & 0x0010);
        to[5]  = (from & 0x0020);
        to[6]  = (from & 0x0040);
        to[7]  = (from & 0x0080);
        to[8]  = (from & 0x0100);
        to[9]  = (from & 0x0200);
        to[10] = (from & 0x0400);
        to[11] = (from & 0x0800);
        to[12] = (from & 0x1000);
        to[13] = (from & 0x2000);
        to[14] = (from & 0x4000);
        to[15] = (from & 0x8000);
    };

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        to[0]  = (from & 0x0001);
        to[1]  = (from & 0x0002);
        to[2]  = (from & 0x0004);
        to[3]  = (from & 0x0008);
        to[4]  = (from & 0x0010);
        to[5]  = (from & 0x0020);
        to[6]  = (from & 0x0040);
        to[7]  = (from & 0x0080);
        to[8]  = (from & 0x0100);
        to[9]  = (from & 0x0200);
        to[10] = (from & 0x0400);
        to[11] = (from & 0x0800);
        to[12] = (from & 0x1000);
        to[13] = (from & 0x2000);
        to[14] = (from & 0x4000);
        to[15] = (from & 0x8000);
    };

    VECTORIZATION_SIMD_RETURN_TYPE set(int_t const from, mask_t& ret)
    {
        ret = (from != 0 ? 0xFFFF : 0X0000);
    };

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret) { ret = _mm512_knot(x); };

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm512_kand(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm512_kor(x, y);
    };

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm512_kxor(x, y);
    };

    //-----------------------------------------------------------------------------
    VECTORIZATION_FORCE_INLINE
    static void broadcast(const value_t* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = _mm512_broadcastss_ps(_mm_load_ps1(from));
        a1 = _mm512_broadcastss_ps(_mm_load_ps1(from + 1));
        a2 = _mm512_broadcastss_ps(_mm_load_ps1(from + 2));
        a3 = _mm512_broadcastss_ps(_mm_load_ps1(from + 3));
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        _mm512_store_ps(to, _mm512_loadu_ps(from));
    }

#ifdef __AVX512DQ__
// AVX512F does not define _mm512_extractf32x8_ps to extract _m256 from _m512
#define VECTORIZATION_EXTRACT_8f_FROM_16f(INPUT, OUTPUT)         \
    __m256 OUTPUT##_0 = _mm512_extractf32x8_ps(INPUT, 0); \
    __m256 OUTPUT##_1 = _mm512_extractf32x8_ps(INPUT, 1);
#define VECTORIZATION_INSERT_8f_INTO_16f(OUTPUT, INPUTA, INPUTB) \
    OUTPUT = _mm512_insertf32x8(OUTPUT, INPUTA, 0);       \
    OUTPUT = _mm512_insertf32x8(OUTPUT, INPUTB, 1);

#else
#define VECTORIZATION_EXTRACT_8f_FROM_16f(INPUT, OUTPUT)                 \
    __m256 OUTPUT##_0 = _mm256_insertf128_ps(                     \
        _mm256_castps128_ps256(_mm512_extractf32x4_ps(INPUT, 0)), \
        _mm512_extractf32x4_ps(INPUT, 1),                         \
        1);                                                       \
    __m256 OUTPUT##_1 = _mm256_insertf128_ps(                     \
        _mm256_castps128_ps256(_mm512_extractf32x4_ps(INPUT, 2)), \
        _mm512_extractf32x4_ps(INPUT, 3),                         \
        1);

#define VECTORIZATION_INSERT_8f_INTO_16f(OUTPUT, INPUTA, INPUTB)                     \
    OUTPUT = _mm512_insertf32x4(OUTPUT, _mm256_extractf128_ps(INPUTA, 0), 0); \
    OUTPUT = _mm512_insertf32x4(OUTPUT, _mm256_extractf128_ps(INPUTA, 1), 1); \
    OUTPUT = _mm512_insertf32x4(OUTPUT, _mm256_extractf128_ps(INPUTB, 0), 2); \
    OUTPUT = _mm512_insertf32x4(OUTPUT, _mm256_extractf128_ps(INPUTB, 1), 3);
#endif

#define PACK_OUTPUT(OUTPUT, INPUT, INDEX, STRIDE) \
    VECTORIZATION_INSERT_8f_INTO_16f(OUTPUT[INDEX], INPUT[INDEX], INPUT[INDEX + STRIDE]);

#define PACK_OUTPUT_2(OUTPUT, INPUT, INDEX, STRIDE) \
    VECTORIZATION_INSERT_8f_INTO_16f(OUTPUT[INDEX], INPUT[2 * INDEX], INPUT[2 * INDEX + STRIDE]);

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t x[N])  // NOLINT
    {
        if constexpr (N == 16)
        {
            auto T0  = _mm512_unpacklo_ps(x[0], x[1]);
            auto T1  = _mm512_unpackhi_ps(x[0], x[1]);
            auto T2  = _mm512_unpacklo_ps(x[2], x[3]);
            auto T3  = _mm512_unpackhi_ps(x[2], x[3]);
            auto T4  = _mm512_unpacklo_ps(x[4], x[5]);
            auto T5  = _mm512_unpackhi_ps(x[4], x[5]);
            auto T6  = _mm512_unpacklo_ps(x[6], x[7]);
            auto T7  = _mm512_unpackhi_ps(x[6], x[7]);
            auto T8  = _mm512_unpacklo_ps(x[8], x[9]);
            auto T9  = _mm512_unpackhi_ps(x[8], x[9]);
            auto T10 = _mm512_unpacklo_ps(x[10], x[11]);
            auto T11 = _mm512_unpackhi_ps(x[10], x[11]);
            auto T12 = _mm512_unpacklo_ps(x[12], x[13]);
            auto T13 = _mm512_unpackhi_ps(x[12], x[13]);
            auto T14 = _mm512_unpacklo_ps(x[14], x[15]);
            auto T15 = _mm512_unpackhi_ps(x[14], x[15]);
            auto S0  = _mm512_shuffle_ps(T0, T2, _MM_SHUFFLE(1, 0, 1, 0));
            auto S1  = _mm512_shuffle_ps(T0, T2, _MM_SHUFFLE(3, 2, 3, 2));
            auto S2  = _mm512_shuffle_ps(T1, T3, _MM_SHUFFLE(1, 0, 1, 0));
            auto S3  = _mm512_shuffle_ps(T1, T3, _MM_SHUFFLE(3, 2, 3, 2));
            auto S4  = _mm512_shuffle_ps(T4, T6, _MM_SHUFFLE(1, 0, 1, 0));
            auto S5  = _mm512_shuffle_ps(T4, T6, _MM_SHUFFLE(3, 2, 3, 2));
            auto S6  = _mm512_shuffle_ps(T5, T7, _MM_SHUFFLE(1, 0, 1, 0));
            auto S7  = _mm512_shuffle_ps(T5, T7, _MM_SHUFFLE(3, 2, 3, 2));
            auto S8  = _mm512_shuffle_ps(T8, T10, _MM_SHUFFLE(1, 0, 1, 0));
            auto S9  = _mm512_shuffle_ps(T8, T10, _MM_SHUFFLE(3, 2, 3, 2));
            auto S10 = _mm512_shuffle_ps(T9, T11, _MM_SHUFFLE(1, 0, 1, 0));
            auto S11 = _mm512_shuffle_ps(T9, T11, _MM_SHUFFLE(3, 2, 3, 2));
            auto S12 = _mm512_shuffle_ps(T12, T14, _MM_SHUFFLE(1, 0, 1, 0));
            auto S13 = _mm512_shuffle_ps(T12, T14, _MM_SHUFFLE(3, 2, 3, 2));
            auto S14 = _mm512_shuffle_ps(T13, T15, _MM_SHUFFLE(1, 0, 1, 0));
            auto S15 = _mm512_shuffle_ps(T13, T15, _MM_SHUFFLE(3, 2, 3, 2));

            VECTORIZATION_EXTRACT_8f_FROM_16f(S0, S0);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S1, S1);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S2, S2);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S3, S3);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S4, S4);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S5, S5);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S6, S6);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S7, S7);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S8, S8);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S9, S9);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S10, S10);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S11, S11);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S12, S12);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S13, S13);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S14, S14);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S15, S15);

            __m256 tmp[32];  // NOLINT
            tmp[0] = _mm256_permute2f128_ps(S0_0, S4_0, 0x20);
            tmp[1] = _mm256_permute2f128_ps(S1_0, S5_0, 0x20);
            tmp[2] = _mm256_permute2f128_ps(S2_0, S6_0, 0x20);
            tmp[3] = _mm256_permute2f128_ps(S3_0, S7_0, 0x20);
            tmp[4] = _mm256_permute2f128_ps(S0_0, S4_0, 0x31);
            tmp[5] = _mm256_permute2f128_ps(S1_0, S5_0, 0x31);
            tmp[6] = _mm256_permute2f128_ps(S2_0, S6_0, 0x31);
            tmp[7] = _mm256_permute2f128_ps(S3_0, S7_0, 0x31);

            tmp[8]  = _mm256_permute2f128_ps(S0_1, S4_1, 0x20);
            tmp[9]  = _mm256_permute2f128_ps(S1_1, S5_1, 0x20);
            tmp[10] = _mm256_permute2f128_ps(S2_1, S6_1, 0x20);
            tmp[11] = _mm256_permute2f128_ps(S3_1, S7_1, 0x20);
            tmp[12] = _mm256_permute2f128_ps(S0_1, S4_1, 0x31);
            tmp[13] = _mm256_permute2f128_ps(S1_1, S5_1, 0x31);
            tmp[14] = _mm256_permute2f128_ps(S2_1, S6_1, 0x31);
            tmp[15] = _mm256_permute2f128_ps(S3_1, S7_1, 0x31);

            // Second set of _m256 outputs
            tmp[16] = _mm256_permute2f128_ps(S8_0, S12_0, 0x20);
            tmp[17] = _mm256_permute2f128_ps(S9_0, S13_0, 0x20);
            tmp[18] = _mm256_permute2f128_ps(S10_0, S14_0, 0x20);
            tmp[19] = _mm256_permute2f128_ps(S11_0, S15_0, 0x20);
            tmp[20] = _mm256_permute2f128_ps(S8_0, S12_0, 0x31);
            tmp[21] = _mm256_permute2f128_ps(S9_0, S13_0, 0x31);
            tmp[22] = _mm256_permute2f128_ps(S10_0, S14_0, 0x31);
            tmp[23] = _mm256_permute2f128_ps(S11_0, S15_0, 0x31);

            tmp[24] = _mm256_permute2f128_ps(S8_1, S12_1, 0x20);
            tmp[25] = _mm256_permute2f128_ps(S9_1, S13_1, 0x20);
            tmp[26] = _mm256_permute2f128_ps(S10_1, S14_1, 0x20);
            tmp[27] = _mm256_permute2f128_ps(S11_1, S15_1, 0x20);
            tmp[28] = _mm256_permute2f128_ps(S8_1, S12_1, 0x31);
            tmp[29] = _mm256_permute2f128_ps(S9_1, S13_1, 0x31);
            tmp[30] = _mm256_permute2f128_ps(S10_1, S14_1, 0x31);
            tmp[31] = _mm256_permute2f128_ps(S11_1, S15_1, 0x31);

            // Pack them into the output
            PACK_OUTPUT(x, tmp, 0, 16);
            PACK_OUTPUT(x, tmp, 1, 16);
            PACK_OUTPUT(x, tmp, 2, 16);
            PACK_OUTPUT(x, tmp, 3, 16);

            PACK_OUTPUT(x, tmp, 4, 16);
            PACK_OUTPUT(x, tmp, 5, 16);
            PACK_OUTPUT(x, tmp, 6, 16);
            PACK_OUTPUT(x, tmp, 7, 16);

            PACK_OUTPUT(x, tmp, 8, 16);
            PACK_OUTPUT(x, tmp, 9, 16);
            PACK_OUTPUT(x, tmp, 10, 16);
            PACK_OUTPUT(x, tmp, 11, 16);

            PACK_OUTPUT(x, tmp, 12, 16);
            PACK_OUTPUT(x, tmp, 13, 16);
            PACK_OUTPUT(x, tmp, 14, 16);
            PACK_OUTPUT(x, tmp, 15, 16);
        }
        else
        {
            auto T0 = _mm512_unpacklo_ps(x[0], x[1]);
            auto T1 = _mm512_unpackhi_ps(x[0], x[1]);
            auto T2 = _mm512_unpacklo_ps(x[2], x[3]);
            auto T3 = _mm512_unpackhi_ps(x[2], x[3]);

            auto S0 = _mm512_shuffle_ps(T0, T2, _MM_SHUFFLE(1, 0, 1, 0));
            auto S1 = _mm512_shuffle_ps(T0, T2, _MM_SHUFFLE(3, 2, 3, 2));
            auto S2 = _mm512_shuffle_ps(T1, T3, _MM_SHUFFLE(1, 0, 1, 0));
            auto S3 = _mm512_shuffle_ps(T1, T3, _MM_SHUFFLE(3, 2, 3, 2));

            VECTORIZATION_EXTRACT_8f_FROM_16f(S0, S0);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S1, S1);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S2, S2);
            VECTORIZATION_EXTRACT_8f_FROM_16f(S3, S3);

            __m256 tmp[8];  // NOLINT

            tmp[0] = _mm256_permute2f128_ps(S0_0, S1_0, 0x20);
            tmp[1] = _mm256_permute2f128_ps(S2_0, S3_0, 0x20);
            tmp[2] = _mm256_permute2f128_ps(S0_0, S1_0, 0x31);
            tmp[3] = _mm256_permute2f128_ps(S2_0, S3_0, 0x31);

            tmp[4] = _mm256_permute2f128_ps(S0_1, S1_1, 0x20);
            tmp[5] = _mm256_permute2f128_ps(S2_1, S3_1, 0x20);
            tmp[6] = _mm256_permute2f128_ps(S0_1, S1_1, 0x31);
            tmp[7] = _mm256_permute2f128_ps(S2_1, S3_1, 0x31);

            PACK_OUTPUT_2(x, tmp, 0, 1);
            PACK_OUTPUT_2(x, tmp, 1, 1);
            PACK_OUTPUT_2(x, tmp, 2, 1);
            PACK_OUTPUT_2(x, tmp, 3, 1);
        }
    }
};

