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

#include <type_traits>

#if VECTORIZATION_HAS_SVML
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-calling-convention"
#endif

#include "backend/sse/svml.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#include "common/vectorization_macros.h"
#include "backend/sse/intrinsics.h"

template <>
struct simd<double>
{
    using simd_t      = __m128d;
    using mask_t      = __m128d;
    using simd_half_t = __m128d;
    using simd_int_t  = __m128i;
    using value_t     = double;
    using int_t       = uint32_t;

    static constexpr int size      = 2;
    static constexpr int half_size = 1;

    inline static simd_t const sign_mask = _mm_set1_pd(-0.0);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        _mm_prefetch(PREFETCH_PTR_TYPE(addr), _MM_HINT_T0);
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = _mm_load_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = _mm_loadu_pd(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { _mm_store_pd(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { _mm_storeu_pd(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(const scalar_t alpha, simd_t& ret)
    {
        ret = _mm_set1_pd(static_cast<value_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = _mm_setzero_pd(); }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_add_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_sub_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_mul_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_div_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __FMA__
        ret = _mm_fmadd_pd(x, y, z);
#else
        ret = _mm_add_pd(_mm_mul_pd(x, y), z);
#endif
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_pow_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_hypot_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_min_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = _mm_max_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        ret = _mm_signcopy_pd(x, sign);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = _mm_sqrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = _mm_mul_pd(x, x); }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret) { ret = _mm_ceil_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret) { ret = _mm_floor_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret) { ret = _mm_exp_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret) { ret = _mm_expm1_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret) { ret = _mm_exp2_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE exp10(const simd_t& x, simd_t& ret) { ret = _mm_exp10_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret) { ret = _mm_log_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret) { ret = _mm_log1p_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret) { ret = _mm_log2_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE log10(const simd_t& x, simd_t& ret) { ret = _mm_log10_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret) { ret = _mm_sin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret) { ret = _mm_cos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret) { ret = _mm_tan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret) { ret = _mm_asin_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret) { ret = _mm_acos_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret) { ret = _mm_atan_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret) { ret = _mm_sinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret) { ret = _mm_cosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret) { ret = _mm_tanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret) { ret = _mm_asinh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret) { ret = _mm_acosh_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret) { ret = _mm_atanh_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret) { ret = _mm_cbrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret) { ret = _mm_cdfnorm_pd(x); }
    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret) { ret = _mm_cdfnorminv_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret) { ret = _mm_trunc_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret) { ret = _mm_invsqrt_pd(x); }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret)
    {
        ret = _mm_andnot_pd(sign_mask, x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret) { ret = _mm_xor_pd(x, sign_mask); }

    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        return _mm_accumulate_pd(x);
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x)
    {
        return _mm_hmax_pd(x);
    }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x)
    {
        return _mm_hmin_pd(x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_t& to)
    {
        to = _mm_gather_pd(from, stride);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, int stride, value_t* to)
    {
        _mm_scatter_pd(from, to, stride);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, const int* strides, simd_t& to)
    {
        to = _mm_gather_pd(from, strides);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, const int* stride, value_t* to)
    {
        _mm_scatter_pd(from, to, stride);
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
#ifdef __SSE4_1__
        ret = _mm_blendv_pd(z, y, x);
#else
        ret = _mm_or_pd(_mm_andnot_pd(x, z), _mm_and_pd(x, y));
#endif
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_EQ_OQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_NEQ_UQ);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_GT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_LT_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_GE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = _mm_cmp_pd(x, y, _CMP_LE_OS);
    }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = _mm_loadu_mask_pd(from);
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = _mm_load_mask_pd(from);
    }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        _mm_storeu_mask_pd(from, to);
    }

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        _mm_store_mask_pd(from, to);
    }

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret)
    {
        ret = _mm_mask_not_pd(x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm_and_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm_or_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = _mm_xor_pd(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE broadcast(
        const double* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
#ifdef __SSE3__
        a0 = _mm_loaddup_pd(from);
        a1 = _mm_loaddup_pd(from + 1);
        a2 = _mm_loaddup_pd(from + 2);
        a3 = _mm_loaddup_pd(from + 3);
#else
        a0 = _mm_set1_pd(from[0]);
        a1 = _mm_set1_pd(from[1]);
        a2 = _mm_set1_pd(from[2]);
        a3 = _mm_set1_pd(from[3]);
#endif
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        _mm_store_pd(to, _mm_loadu_pd(from));
    }

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t simd[N])  // NOLINT
    {
        constexpr int L = size;
        if constexpr (N == 2 && L == 2)
        {
            __m128d t0 = _mm_shuffle_pd(simd[0], simd[1], 0);
            __m128d t1 = _mm_shuffle_pd(simd[0], simd[1], 3);
            simd[0]    = t0;
            simd[1]    = t1;
        }
        else if constexpr (N == L)
        {
            alignas(16) double buf[N * L];
            for (int i = 0; i < N; ++i)
            {
                _mm_store_pd(buf + i * L, simd[i]);
            }
            alignas(16) double out[N * L];
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < L; ++j)
                {
                    out[i * L + j] = buf[j * L + i];
                }
            }
            for (int i = 0; i < N; ++i)
            {
                simd[i] = _mm_load_pd(out + i * L);
            }
        }
        else if constexpr (N == 8 && L == 2)
        {
            alignas(16) double buf[8][2];
            for (int i = 0; i < 8; ++i)
            {
                _mm_store_pd(buf[i], simd[i]);
            }
            alignas(16) double out[2][8];
            for (int i = 0; i < 8; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    out[j][i] = buf[i][j];
                }
            }
            for (int r = 0; r < 2; ++r)
            {
                for (int q = 0; q < 4; ++q)
                {
                    simd[4 * r + q] = _mm_load_pd(&out[r][2 * q]);
                }
            }
        }
        else if constexpr (N == 4 && L == 2)
        {
            alignas(16) double buf[4][2];
            for (int i = 0; i < 4; ++i)
            {
                _mm_store_pd(buf[i], simd[i]);
            }
            alignas(16) double out[2][4];
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    out[j][i] = buf[i][j];
                }
            }
            for (int r = 0; r < 2; ++r)
            {
                for (int q = 0; q < 2; ++q)
                {
                    simd[2 * r + q] = _mm_load_pd(&out[r][2 * q]);
                }
            }
        }
        else
        {
            static_assert(sizeof(simd_t) == 0, "ptranspose: unsupported N for SSE double");
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE ploadquad(const double* from, simd_t& to)
    {
        to = _mm_set1_pd(from[0]);
    }
};
