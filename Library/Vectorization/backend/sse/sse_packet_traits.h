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

#ifndef __sse_packet_traits_h__
#define __sse_packet_traits_h__

#define VECTORIZATION_SWIZZLE(v, p, q, r, s) \
    (_mm_castsi128_ps(                \
        _mm_shuffle_epi32(_mm_castps_si128(v), ((s) << 6 | (r) << 4 | (q) << 2 | (p)))))
template <>
struct simd<float>
{
    using type                     = __m128;
    using half_type                = __m128;
    static constexpr int size      = 4;
    static constexpr int half_size = 2;

    template <bool aligned = false>
    VECTORIZATION_FORCE_INLINE static void load(const float* from, float* to)
    {
        if constexpr (aligned)
            _mm_store_ps(to, _mm_load_ps(from));
        else
            _mm_store_ps(to, _mm_loadu_ps(from));
    }

    VECTORIZATION_FORCE_INLINE static void prefetch(const float* addr)
    {
        _mm_prefetch((PREFETCH_PTR_TYPE)(addr), _MM_HINT_T0);
    }

    VECTORIZATION_FORCE_INLINE static void initiate(type& pack) { pack = _mm_set1_ps(0.); }

    VECTORIZATION_FORCE_INLINE static void loadu(const float* addr, type& pack)
    {
        pack = _mm_loadu_ps(addr);
    }

    VECTORIZATION_FORCE_INLINE static void storeu(float* to, const type& from) { _mm_storeu_ps(to, from); }

    VECTORIZATION_FORCE_INLINE static void store(float* to, const type& from) { _mm_store_ps(to, from); }

    VECTORIZATION_FORCE_INLINE static void set(type& pack, float alpha) { pack = _mm_set1_ps(alpha); }

    VECTORIZATION_FORCE_INLINE static void fma(const type& a, const type& b, type& c)
    {
#ifdef __FMA__
        c = _mm_fmadd_ps(a, b, c);
#else
        c = _mm_add_ps(_mm_mul_ps(a, b), c);
#endif  // __FMA__
    }

    VECTORIZATION_FORCE_INLINE static void broadcast(
        const float* from, type& a0, type& a1, type& a2, type& a3)
    {
        a3 = _mm_load_ps(from);
        a0 = VECTORIZATION_SWIZZLE(a3, 0, 0, 0, 0);
        a1 = VECTORIZATION_SWIZZLE(a3, 1, 1, 1, 1);
        a2 = VECTORIZATION_SWIZZLE(a3, 2, 2, 2, 2);
        a3 = VECTORIZATION_SWIZZLE(a3, 3, 3, 3, 3);
    }

    VECTORIZATION_FORCE_INLINE static type add(const type& a, const type& b) { return _mm_add_ps(a, b); }

    /*VECTORIZATION_FORCE_INLINE static half_type padd_half(const half_type& a, const half_type& b) {}

    VECTORIZATION_FORCE_INLINE static half_type predux_downto4(const type& a)
    {
        return _mm_add_ps(a, a);
    }*/

    VECTORIZATION_FORCE_INLINE static void scatter(float* to, const type& from, int stride)
    {
        _mm_scatter_ps(from, to, stride);
    }

    template <int N>
    VECTORIZATION_FORCE_INLINE static void ptranspose(type simd[N])  // NOLINT
    {
        transpose<N>(simd);
    }

    // Loads 2 floats from memory a returns the simd {a0, a0  a0, a0, a1, a1, a1, a1}
    VECTORIZATION_FORCE_INLINE static void ploadquad(const float* from, type& to)
    {
        to = VECTORIZATION_SWIZZLE(_mm_load_ss(from), 0, 0, 0, 0);
    }

    VECTORIZATION_FORCE_INLINE static half_type pgather(const float* from, int stride)
    {
        return _mm_set_ps(from[3 * stride], from[2 * stride], from[stride], from[0]);
    }
    /*
        VECTORIZATION_FORCE_INLINE static half_type loadu_half(const float* from) {}

        VECTORIZATION_FORCE_INLINE static half_type set(const float* from) { return _mm_set1_ps(*from);
       }*/
};
#undef VECTORIZATION_SWIZZLE1
#define VECTORIZATION_SWIZZLE1(v, p, q)         \
    (_mm_castsi128_pd(_mm_shuffle_epi32( \
        _mm_castpd_si128(v), ((q * 2 + 1) << 6 | (q * 2) << 4 | (p * 2 + 1) << 2 | (p * 2)))))

template <>
struct simd<double>
{
    using type                     = __m128d;
    using half_type                = __m128d;
    static constexpr int size      = 2;
    static constexpr int half_size = 1;

    template <bool aligned = false>
    __VECTORIZATION_FUNCTION_ATTRIBUTE__ static void load(const double* from, double* to)
    {
        if constexpr (aligned)
            _mm_store_pd(to, _mm_load_pd(from));
        else
            _mm_store_pd(to, _mm_loadu_pd(from));
    }

    VECTORIZATION_FORCE_INLINE static void prefetch(const double* addr)
    {
        _mm_prefetch((PREFETCH_PTR_TYPE)(addr), _MM_HINT_T0);
    }

    VECTORIZATION_FORCE_INLINE static void initiate(type& pack) { pack = _mm_setzero_pd(); }

    VECTORIZATION_FORCE_INLINE static void loadu(const double* addr, type& pack)
    {
        pack = _mm_loadu_pd(addr);
    }

    VECTORIZATION_FORCE_INLINE static void storeu(double* to, const type& from)
    {
        _mm_storeu_pd(to, from);
    }

    VECTORIZATION_FORCE_INLINE static void store(double* to, const type& from) { _mm_storeu_pd(to, from); }

    VECTORIZATION_FORCE_INLINE static void set(type& pack, double alpha) { pack = _mm_set1_pd(alpha); }

    VECTORIZATION_FORCE_INLINE static void load(const double* addr, type& pack)
    {
        pack = _mm_load_pd(addr);
    }

    VECTORIZATION_FORCE_INLINE static void set(const double* addr, type& pack)
    {
        pack = _mm_set1_pd(*addr);
    }

    VECTORIZATION_FORCE_INLINE static void fma(const type& a, const type& b, type& c)
    {
#ifdef __FMA__
        c = _mm_fmadd_pd(a, b, c);
#else
        c = _mm_add_pd(_mm_mul_pd(a, b), c);
#endif
    }

    VECTORIZATION_FORCE_INLINE static void broadcast(
        const double* from, type& a0, type& a1, type& a2, type& a3)
    {
#ifdef __SSE3__
        a0 = _mm_loaddup_pd(from + 0);
        a1 = _mm_loaddup_pd(from + 1);
        a2 = _mm_loaddup_pd(from + 2);
        a3 = _mm_loaddup_pd(from + 3);
#else
        a1 = _mm_load_pd(from);
        a0 = VECTORIZATION_SWIZZLE1(a1, 0, 0);
        a1 = VECTORIZATION_SWIZZLE1(a1, 1, 1);
        a3 = _mm_load_pd(from + 2);
        a2 = VECTORIZATION_SWIZZLE1(a3, 0, 0);
        a3 = VECTORIZATION_SWIZZLE1(a3, 1, 1);
#endif
    }

    VECTORIZATION_FORCE_INLINE static type add(const type& a, const type& b) { return _mm_add_pd(a, b); }

    VECTORIZATION_FORCE_INLINE static void scatter(double* to, const type& from, int stride)
    {
        _mm_scatter_pd(from, to, stride);
    }

    template <int N>
    VECTORIZATION_FORCE_INLINE static void ptranspose(type simd[N])  // NOLINT
    {
        transpose<N>(simd);
    }

    VECTORIZATION_FORCE_INLINE static void ploadquad(const double* from, type& to)
    {
        to = _mm_set1_pd(*from);
    }

    VECTORIZATION_FORCE_INLINE static type pgather(const double* from, int stride)
    {
        return _mm_set_pd(from[stride], from[0]);
    }
};
#undef VECTORIZATION_SWIZZLE1

#endif
