/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#pragma once

#include <arm_neon.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include "common/intrin.h"
#include "common/normal_cdf.h"
#include "common/vectorization_macros.h"

namespace vectorization
{
namespace detail_neon
{
VECTORIZATION_FORCE_INLINE float64x2_t map1_f64(float64x2_t v, double (*fn)(double))
{
    alignas(16) double b[2];
    vst1q_f64(b, v);
    b[0] = fn(b[0]);
    b[1] = fn(b[1]);
    return vld1q_f64(b);
}
}  // namespace detail_neon
}  // namespace vectorization

template <>
struct simd<double>
{
    using simd_t      = float64x2_t;
    using mask_t      = uint64x2_t;
    using simd_half_t = float64x2_t;
    using simd_int_t  = uint32x2_t;
    using value_t     = double;
    using int_t       = uint32_t;

    static constexpr int size      = 2;
    static constexpr int half_size = 1;

    inline static uint64x2_t const sign_mask = vdupq_n_u64(0x8000000000000000ULL);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        __builtin_prefetch(addr, 0, 3);
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = vld1q_f64(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = vld1q_f64(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { vst1q_f64(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { vst1q_f64(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(const scalar_t alpha, simd_t& ret)
    {
        ret = vdupq_n_f64(static_cast<value_t>(alpha));
    }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(const scalar_t alpha, mask_t& ret)
    {
        ret = vdupq_n_u64(static_cast<uint64_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = vdupq_n_f64(0.); }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vaddq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vsubq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vmulq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vdivq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = vfmaq_f64(z, x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        alignas(16) double bx[2];
        alignas(16) double by[2];
        vst1q_f64(bx, x);
        vst1q_f64(by, y);
        bx[0] = std::pow(bx[0], by[0]);
        bx[1] = std::pow(bx[1], by[1]);
        ret   = vld1q_f64(bx);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vsqrtq_f64(vaddq_f64(vmulq_f64(x, x), vmulq_f64(y, y)));
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vminq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vmaxq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        uint64x2_t xs = vreinterpretq_u64_f64(x);
        uint64x2_t ss = vreinterpretq_u64_f64(sign);
        ret           = vreinterpretq_f64_u64(vorrq_u64(vandq_u64(sign_mask, ss), vbicq_u64(xs, sign_mask)));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = vsqrtq_f64(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = vmulq_f64(x, x); }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::ceil));
    }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::floor));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::exp));
    }

    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::expm1));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::exp2));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp10(const simd_t& x, simd_t& ret)
    {
        alignas(16) double b[2];
        vst1q_f64(b, x);
        b[0] = std::pow(10., b[0]);
        b[1] = std::pow(10., b[1]);
        ret  = vld1q_f64(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::log));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::log1p));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::log2));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log10(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::log10));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::sin));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::cos));
    }

    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::tan));
    }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::asin));
    }

    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::acos));
    }

    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::atan));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::sinh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::cosh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::tanh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::asinh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::acosh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::atanh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::cbrt));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret)
    {
        alignas(16) double b[2];
        vst1q_f64(b, x);
        b[0] = vectorization::normalcdf(b[0]);
        b[1] = vectorization::normalcdf(b[1]);
        ret  = vld1q_f64(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret)
    {
        alignas(16) double b[2];
        vst1q_f64(b, x);
        b[0] = vectorization::inv_normalcdf(b[0]);
        b[1] = vectorization::inv_normalcdf(b[1]);
        ret  = vld1q_f64(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f64(x, static_cast<double (*)(double)>(&std::trunc));
    }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret)
    {
        float64x2_t est = vrsqrteq_f64(x);
        est             = vmulq_f64(vrsqrtsq_f64(vmulq_f64(x, est), est), est);
        est             = vmulq_f64(vrsqrtsq_f64(vmulq_f64(x, est), est), est);
        ret             = est;
    }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret)
    {
        ret = vreinterpretq_f64_u64(vbicq_u64(vreinterpretq_u64_f64(x), sign_mask));
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret)
    {
        ret = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(x), sign_mask));
    }

    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        return vgetq_lane_f64(x, 0) + vgetq_lane_f64(x, 1);
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x)
    {
        return (std::max)(vgetq_lane_f64(x, 0), vgetq_lane_f64(x, 1));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x)
    {
        return (std::min)(vgetq_lane_f64(x, 0), vgetq_lane_f64(x, 1));
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_t& to)
    {
        alignas(16) value_t tmp[2] = {from[0], from[stride]};
        to                         = vld1q_f64(tmp);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, int stride, value_t* to)
    {
        alignas(16) value_t b[2];
        vst1q_f64(b, from);
        to[0]      = b[0];
        to[stride] = b[1];
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, const int* strides, simd_t& to)
    {
        alignas(16) value_t tmp[2] = {from[strides[0]], from[strides[1]]};
        to                         = vld1q_f64(tmp);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, const int* stride, value_t* to)
    {
        alignas(16) value_t b[2];
        vst1q_f64(b, from);
        to[stride[0]] = b[0];
        to[stride[1]] = b[1];
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = vbslq_f64(x, y, z);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vceqq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = veorq_u64(vceqq_f64(x, y), vdupq_n_u64(~0ULL));
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcgtq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcltq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcgeq_f64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcleq_f64(x, y);
    }

    VECTORIZATION_FORCE_INLINE static uint64x2_t u32_mask_to_u64(uint32_t a, uint32_t b)
    {
        uint64_t la = a ? ~0ULL : 0ULL;
        uint64_t lb = b ? ~0ULL : 0ULL;
        uint64x2_t r = vdupq_n_u64(0);
        r              = vsetq_lane_u64(la, r, 0);
        r              = vsetq_lane_u64(lb, r, 1);
        return r;
    }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret)
    {
        ret = u32_mask_to_u64(from[0], from[1]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret)
    {
        ret = u32_mask_to_u64(from[0], from[1]);
    }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to)
    {
        to[0] = vgetq_lane_u64(from, 0) ? 1u : 0u;
        to[1] = vgetq_lane_u64(from, 1) ? 1u : 0u;
    }

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to)
    {
        storeu(from, to);
    }

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret)
    {
        ret = veorq_u64(x, vdupq_n_u64(~0ULL));
    }

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = vandq_u64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = vorrq_u64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = veorq_u64(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE broadcast(
        const double* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = vdupq_n_f64(from[0]);
        a1 = vdupq_n_f64(from[1]);
        a2 = vdupq_n_f64(from[2]);
        a3 = vdupq_n_f64(from[3]);
    }

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t simd[N])  // NOLINT
    {
        constexpr int L = size;
        if constexpr (N == 2 && L == 2)
        {
            float64x2_t t0 = vzip1q_f64(simd[0], simd[1]);
            float64x2_t t1 = vzip2q_f64(simd[0], simd[1]);
            simd[0]        = t0;
            simd[1]        = t1;
        }
        else if constexpr (N == L)
        {
            alignas(16) double buf[N * L];
            for (int i = 0; i < N; ++i)
            {
                vst1q_f64(buf + i * L, simd[i]);
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
                simd[i] = vld1q_f64(out + i * L);
            }
        }
        else if constexpr (N == 8 && L == 2)
        {
            alignas(16) double buf[8][2];
            for (int i = 0; i < 8; ++i)
            {
                vst1q_f64(buf[i], simd[i]);
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
                    simd[4 * r + q] = vld1q_f64(&out[r][2 * q]);
                }
            }
        }
        else if constexpr (N == 4 && L == 2)
        {
            alignas(16) double buf[4][2];
            for (int i = 0; i < 4; ++i)
            {
                vst1q_f64(buf[i], simd[i]);
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
                    simd[2 * r + q] = vld1q_f64(&out[r][2 * q]);
                }
            }
        }
        else
        {
            static_assert(sizeof(simd_t) == 0, "ptranspose: unsupported N for NEON double");
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        vst1q_f64(to, vld1q_f64(from));
    }

    VECTORIZATION_SIMD_RETURN_TYPE ploadquad(const double* from, simd_t& to)
    {
        to = vdupq_n_f64(from[0]);
    }
};
