/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#pragma once

#include <arm_neon.h>

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
VECTORIZATION_FORCE_INLINE float32x4_t map1_f32(float32x4_t v, float (*fn)(float))
{
    alignas(16) float b[4];
    vst1q_f32(b, v);
    b[0] = fn(b[0]);
    b[1] = fn(b[1]);
    b[2] = fn(b[2]);
    b[3] = fn(b[3]);
    return vld1q_f32(b);
}
}  // namespace detail_neon
}  // namespace vectorization

template <>
struct simd<float>
{
    using simd_t      = float32x4_t;
    using mask_t      = uint32x4_t;
    using simd_half_t = float32x2_t;
    using simd_int_t  = uint32x4_t;
    using value_t     = float;
    using int_t       = uint32_t;

    static constexpr int size      = 4;
    static constexpr int half_size = 2;

    inline static uint32x4_t const sign_mask = vdupq_n_u32(0x80000000u);

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(const value_t* addr)
    {
        __builtin_prefetch(addr, 0, 3);
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(const value_t* addr, simd_t& ret) { ret = vld1q_f32(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(const value_t* addr, simd_t& ret) { ret = vld1q_f32(addr); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const simd_t& from, value_t* to) { vst1q_f32(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const simd_t& from, value_t* to) { vst1q_f32(to, from); }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(scalar_t alpha, simd_t& ret)
    {
        ret = vdupq_n_f32(static_cast<value_t>(alpha));
    }

    template <
        typename scalar_t,
        typename std::enable_if<std::is_fundamental<scalar_t>::value, bool>::type = true>
    VECTORIZATION_SIMD_RETURN_TYPE set(scalar_t alpha, mask_t& ret)
    {
        ret = vdupq_n_u32(static_cast<uint32_t>(alpha));
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(simd_t& ret) { ret = vdupq_n_f32(0.f); }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vaddq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vsubq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vmulq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vdivq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        alignas(16) float bx[4];
        alignas(16) float by[4];
        vst1q_f32(bx, x);
        vst1q_f32(by, y);
        bx[0] = std::pow(bx[0], by[0]);
        bx[1] = std::pow(bx[1], by[1]);
        bx[2] = std::pow(bx[2], by[2]);
        bx[3] = std::pow(bx[3], by[3]);
        ret   = vld1q_f32(bx);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vsqrtq_f32(vaddq_f32(vmulq_f32(x, x), vmulq_f32(y, y)));
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vminq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(const simd_t& x, const simd_t& y, simd_t& ret)
    {
        ret = vmaxq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = vfmaq_f32(z, x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t x, simd_t sign, simd_t& ret)
    {
        uint32x4_t xs = vreinterpretq_u32_f32(x);
        uint32x4_t ss = vreinterpretq_u32_f32(sign);
        ret           = vreinterpretq_f32_u32(vorrq_u32(vandq_u32(sign_mask, ss), vbicq_u32(xs, sign_mask)));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqrt(const simd_t& x, simd_t& ret) { ret = vsqrtq_f32(x); }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(const simd_t& x, simd_t& ret) { ret = vmulq_f32(x, x); }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::ceil));
    }

    VECTORIZATION_SIMD_RETURN_TYPE floor(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::floor));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::exp));
    }

    VECTORIZATION_SIMD_RETURN_TYPE expm1(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::expm1));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp2(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::exp2));
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp10(const simd_t& x, simd_t& ret)
    {
        alignas(16) float b[4];
        vst1q_f32(b, x);
        b[0] = std::pow(10.f, b[0]);
        b[1] = std::pow(10.f, b[1]);
        b[2] = std::pow(10.f, b[2]);
        b[3] = std::pow(10.f, b[3]);
        ret  = vld1q_f32(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::log));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log1p(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::log1p));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log2(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::log2));
    }

    VECTORIZATION_SIMD_RETURN_TYPE log10(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::log10));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sin(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::sin));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cos(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::cos));
    }

    VECTORIZATION_SIMD_RETURN_TYPE tan(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::tan));
    }

    VECTORIZATION_SIMD_RETURN_TYPE asin(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::asin));
    }

    VECTORIZATION_SIMD_RETURN_TYPE acos(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::acos));
    }

    VECTORIZATION_SIMD_RETURN_TYPE atan(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::atan));
    }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::sinh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cosh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::cosh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE tanh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::tanh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::asinh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE acosh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::acosh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE atanh(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::atanh));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::cbrt));
    }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(const simd_t& x, simd_t& ret)
    {
        alignas(16) float b[4];
        vst1q_f32(b, x);
        b[0] = static_cast<float>(vectorization::normalcdf(static_cast<double>(b[0])));
        b[1] = static_cast<float>(vectorization::normalcdf(static_cast<double>(b[1])));
        b[2] = static_cast<float>(vectorization::normalcdf(static_cast<double>(b[2])));
        b[3] = static_cast<float>(vectorization::normalcdf(static_cast<double>(b[3])));
        ret  = vld1q_f32(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(const simd_t& x, simd_t& ret)
    {
        alignas(16) float b[4];
        vst1q_f32(b, x);
        b[0] = static_cast<float>(vectorization::inv_normalcdf(static_cast<double>(b[0])));
        b[1] = static_cast<float>(vectorization::inv_normalcdf(static_cast<double>(b[1])));
        b[2] = static_cast<float>(vectorization::inv_normalcdf(static_cast<double>(b[2])));
        b[3] = static_cast<float>(vectorization::inv_normalcdf(static_cast<double>(b[3])));
        ret  = vld1q_f32(b);
    }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(const simd_t& x, simd_t& ret)
    {
        ret = vectorization::detail_neon::map1_f32(x, static_cast<float (*)(float)>(&std::trunc));
    }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(const simd_t& x, simd_t& ret)
    {
        float32x4_t est = vrsqrteq_f32(x);
        est             = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, est), est), est);
        ret             = est;
    }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(const simd_t& x, simd_t& ret)
    {
        ret = vreinterpretq_f32_u32(vbicq_u32(vreinterpretq_u32_f32(x), sign_mask));
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(const simd_t& x, simd_t& ret)
    {
        ret = vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(x), sign_mask));
    }

    VECTORIZATION_FORCE_INLINE static double accumulate(const simd_t& x)
    {
        float32x2_t t = vadd_f32(vget_high_f32(x), vget_low_f32(x));
        t             = vpadd_f32(t, t);
        return static_cast<double>(vget_lane_f32(t, 0));
    }

    VECTORIZATION_FORCE_INLINE static value_t hmax(const simd_t& x)
    {
        float32x2_t t = vmax_f32(vget_high_f32(x), vget_low_f32(x));
        t             = vpmax_f32(t, t);
        return vget_lane_f32(t, 0);
    }

    VECTORIZATION_FORCE_INLINE static value_t hmin(const simd_t& x)
    {
        float32x2_t t = vmin_f32(vget_high_f32(x), vget_low_f32(x));
        t             = vpmin_f32(t, t);
        return vget_lane_f32(t, 0);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int stride, simd_t& to)
    {
        alignas(16) value_t tmp[4] = {
            from[0],
            from[1 * stride],
            from[2 * stride],
            from[3 * stride],
        };
        to = vld1q_f32(tmp);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, int stride, value_t* to)
    {
        alignas(16) float b[4];
        vst1q_f32(b, from);
        to[0]          = b[0];
        to[stride]     = b[1];
        to[stride * 2] = b[2];
        to[stride * 3] = b[3];
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, const int* strides, simd_t& to)
    {
        alignas(16) value_t tmp[4] = {
            from[strides[0]],
            from[strides[1]],
            from[strides[2]],
            from[strides[3]],
        };
        to = vld1q_f32(tmp);
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_t& from, const int* stride, value_t* to)
    {
        alignas(16) float b[4];
        vst1q_f32(b, from);
        to[stride[0]] = b[0];
        to[stride[1]] = b[1];
        to[stride[2]] = b[2];
        to[stride[3]] = b[3];
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(const mask_t& x, const simd_t& y, const simd_t& z, simd_t& ret)
    {
        ret = vbslq_f32(x, y, z);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vceqq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vmvnq_u32(vceqq_f32(x, y));
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcgtq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcltq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcgeq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(const simd_t& x, const simd_t& y, mask_t& ret)
    {
        ret = vcleq_f32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(int_t const* from, mask_t& ret) { ret = vld1q_u32(from); }

    VECTORIZATION_SIMD_RETURN_TYPE load(int_t const* from, mask_t& ret) { ret = vld1q_u32(from); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(const mask_t& from, int_t* to) { vst1q_u32(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE store(const mask_t& from, int_t* to) { vst1q_u32(to, from); }

    VECTORIZATION_SIMD_RETURN_TYPE not_mask(const mask_t& x, mask_t& ret)
    {
        ret = vmvnq_u32(x);
    }

    VECTORIZATION_SIMD_RETURN_TYPE and_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = vandq_u32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE or_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = vorrq_u32(x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE xor_mask(const mask_t& x, const mask_t& y, mask_t& ret)
    {
        ret = veorq_u32(x, y);
    }

    VECTORIZATION_FORCE_INLINE
    static void broadcast(const value_t* from, simd_t& a0, simd_t& a1, simd_t& a2, simd_t& a3)
    {
        a0 = vdupq_n_f32(from[0]);
        a1 = vdupq_n_f32(from[1]);
        a2 = vdupq_n_f32(from[2]);
        a3 = vdupq_n_f32(from[3]);
    }

    template <int N>
    VECTORIZATION_SIMD_RETURN_TYPE ptranspose(simd_t simd[N])  // NOLINT
    {
        constexpr int L = size;
        if constexpr (N == 4 && L == 4)
        {
            float32x4x2_t t01 = vtrnq_f32(simd[0], simd[1]);
            float32x4x2_t t23 = vtrnq_f32(simd[2], simd[3]);
            simd[0]           = vcombine_f32(vget_low_f32(t01.val[0]), vget_low_f32(t23.val[0]));
            simd[1]           = vcombine_f32(vget_low_f32(t01.val[1]), vget_low_f32(t23.val[1]));
            simd[2]           = vcombine_f32(vget_high_f32(t01.val[0]), vget_high_f32(t23.val[0]));
            simd[3]           = vcombine_f32(vget_high_f32(t01.val[1]), vget_high_f32(t23.val[1]));
        }
        else if constexpr (N == L)
        {
            alignas(16) float buf[N * L];
            for (int i = 0; i < N; ++i)
            {
                vst1q_f32(buf + i * L, simd[i]);
            }
            alignas(16) float out[N * L];
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < L; ++j)
                {
                    out[i * L + j] = buf[j * L + i];
                }
            }
            for (int i = 0; i < N; ++i)
            {
                simd[i] = vld1q_f32(out + i * L);
            }
        }
        else if constexpr (N == 8 && L == 4)
        {
            alignas(16) float buf[8][4];
            for (int i = 0; i < 8; ++i)
            {
                vst1q_f32(buf[i], simd[i]);
            }
            alignas(16) float out[4][8];
            for (int i = 0; i < 8; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    out[j][i] = buf[i][j];
                }
            }
            for (int r = 0; r < 4; ++r)
            {
                for (int q = 0; q < 2; ++q)
                {
                    simd[2 * r + q] = vld1q_f32(&out[r][4 * q]);
                }
            }
        }
        else if constexpr (N == 16 && L == 4)
        {
            alignas(16) float buf[16][4];
            for (int i = 0; i < 16; ++i)
            {
                vst1q_f32(buf[i], simd[i]);
            }
            alignas(16) float out[4][16];
            for (int i = 0; i < 16; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    out[j][i] = buf[i][j];
                }
            }
            for (int r = 0; r < 4; ++r)
            {
                for (int q = 0; q < 4; ++q)
                {
                    simd[4 * r + q] = vld1q_f32(&out[r][4 * q]);
                }
            }
        }
        else
        {
            static_assert(sizeof(simd_t) == 0, "ptranspose: unsupported N for NEON float");
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE copy(const value_t* from, value_t* to)
    {
        vst1q_f32(to, vld1q_f32(from));
    }

    VECTORIZATION_SIMD_RETURN_TYPE ploadquad(const value_t* from, simd_t& y)
    {
        float32x2_t lo = vld1_dup_f32(from);
        float32x2_t hi = vld1_dup_f32(from + 1);
        y              = vcombine_f32(lo, hi);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(const value_t* from, int stride, simd_half_t& ret)
    {
        ret = vset_lane_f32(from[1 * stride], ret, 1);
        ret = vset_lane_f32(from[0 * stride], ret, 0);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t loadu_half(const value_t* from)
    {
        return vld1_f32(from);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t set(const value_t* from) { return vld1_dup_f32(from); }

    VECTORIZATION_SIMD_RETURN_TYPE fma(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
        ret = vfma_f32(ret, x, y);
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(const simd_half_t& x, const simd_half_t& y, simd_half_t& ret)
    {
        ret = vadd_f32(x, y);
    }

    VECTORIZATION_FORCE_INLINE static simd_half_t predux_downto4(const simd_t& x)
    {
        return vadd_f32(vget_high_f32(x), vget_low_f32(x));
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(const simd_half_t& from, int stride, value_t* to)
    {
        to[0]      = vget_lane_f32(from, 0);
        to[stride] = vget_lane_f32(from, 1);
    }
};
