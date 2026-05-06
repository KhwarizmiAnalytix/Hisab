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



#include <array>
#include <utility>
#include <cstddef>

#include "common/intrin.h"
#include "common/vectorization_macros.h"
#include "common/scalar_helper_functions.h"

#if VECTORIZATION_HAS_AVX512
#include "backend/avx512/double/simd.h"
#include "backend/avx512/float/simd.h"
#elif VECTORIZATION_HAS_AVX2 || VECTORIZATION_HAS_AVX
#include "backend/avx/double/simd.h"
#include "backend/avx/float/simd.h"
#elif VECTORIZATION_HAS_SSE
#include "backend/sse/double/simd.h"
#include "backend/sse/float/simd.h"
#elif VECTORIZATION_HAS_SVE
#include "backend/sve/double/simd.h"
#include "backend/sve/float/simd.h"
#elif VECTORIZATION_HAS_NEON
#include "backend/neon/double/simd.h"
#include "backend/neon/float/simd.h"
#endif


namespace vectorization
{
template <typename simd_t, uint32_t N>
struct array
{
    VECTORIZATION_ALIGN(VECTORIZATION_ALIGNMENT)
    simd_t data_[N];

    VECTORIZATION_NODISCARD constexpr const auto& operator[](const ptrdiff_t _Off) const noexcept
    {
        return data_[_Off];
    }

    VECTORIZATION_NODISCARD constexpr auto& operator[](const ptrdiff_t _Off) noexcept
    {
        return data_[_Off];
    }

    VECTORIZATION_NODISCARD constexpr const auto& data() const { return data_; }

    VECTORIZATION_NODISCARD constexpr auto& data() { return data_; }
};

template <typename T, uint32_t N = packet_size<T>::value>
struct packet
{
    using value_t = T;

#if VECTORIZATION_VECTORIZED
    using simd_t                     = typename simd<value_t>::simd_t;
    using mask_t                     = typename simd<value_t>::mask_t;
    static constexpr uint32_t Length = N * simd<value_t>::size;
#else
    using simd_t                     = value_t;
    using mask_t                     = bool;
    static constexpr uint32_t Length = N;
#endif  // VECTORIZATION_VECTORIZED

    using array_simd_t = typename vectorization::array<simd_t, N>;
    using array_mask_t = typename vectorization::array<mask_t, N>;

    template <typename F, std::size_t... I>
    VECTORIZATION_SIMD_RETURN_TYPE simd_for_each_lane_impl(F&& f, std::index_sequence<I...>)
    {
        (static_cast<void>(f(std::integral_constant<std::size_t, I>{})), ...);
    }

    template <typename F>
    VECTORIZATION_SIMD_RETURN_TYPE simd_for_each_lane(F&& f)
    {
        simd_for_each_lane_impl(
            std::forward<F>(f), std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    static constexpr uint32_t size() noexcept { return N; }

    static constexpr uint32_t length() noexcept { return Length; }

    // static constexpr uint32_t alignment() noexcept { return VECTORIZATION_ALIGNMENT; }

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(value_t const* from)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::prefetch(from + static_cast<std::ptrdiff_t>(I * simd<value_t>::size)), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(value_t const* from, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::load(
                from + static_cast<std::ptrdiff_t>(I * simd<value_t>::size), data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE loadu(value_t const* from, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::loadu(
                from + static_cast<std::ptrdiff_t>(I * simd<value_t>::size), data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE store(array_simd_t const& data, value_t* to)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::store(
                data.data_[I], to + static_cast<std::ptrdiff_t>(I * simd<value_t>::size)), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(array_simd_t const& data, value_t* to)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::storeu(
                data.data_[I], to + static_cast<std::ptrdiff_t>(I * simd<value_t>::size)), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE set(value_t a, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::set(a, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::setzero(data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int const* offsets, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::gather(
                 from,
                 offsets + static_cast<int>(I) * simd<value_t>::size,
                 data.data_[I]),
             ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int const offset, array_simd_t& data)
    {
        const auto stride = offset * simd<value_t>::size;
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::gather(
                 from + static_cast<std::ptrdiff_t>(static_cast<int>(I) * stride),
                 offset,
                 data.data_[I]),
             ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(array_simd_t const& data, int const* offsets, value_t* to)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::scatter(
                 data.data_[I],
                 offsets + static_cast<int>(I) * simd<value_t>::size,
                 to),
             ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(array_simd_t const& data, int const offset, value_t* to)
    {
        const auto stride = offset * simd<value_t>::size;
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::scatter(
                 data.data_[I],
                 offset,
                 to + static_cast<std::ptrdiff_t>(static_cast<int>(I) * stride)),
             ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::add(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::add(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::add(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sub(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sub(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sub(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::mul(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::mul(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::mul(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::div(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::div(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::div(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE pow(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::pow(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE pow(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::pow(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE pow(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::pow(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE hypot(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::hypot(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE hypot(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::hypot(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE hypot(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::hypot(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::min(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::min(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::min(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::max(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::max(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::max(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(
        array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::signcopy(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::signcopy(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::signcopy(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x.data_[I], y.data_[I], z.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        simd_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x, y.data_[I], z.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x.data_[I], y, z.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, array_simd_t const& y, simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x.data_[I], y.data_[I], z, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        simd_t const& x, array_simd_t const& y, simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x, y.data_[I], z, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& y, simd_t const& x, simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fma(x, y.data_[I], z, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::if_else(x.data_[I], y.data_[I], z.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, simd_t const y, array_simd_t const& z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::if_else(x.data_[I], y, z.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, array_simd_t const& y, simd_t const z, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::if_else(x.data_[I], y.data_[I], z, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::eq(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::eq(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::eq(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::neq(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::neq(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::neq(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::gt(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::gt(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::gt(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::ge(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::ge(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::ge(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::lt(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::lt(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::lt(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::le(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::le(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::le(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::and_mask(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::or_mask(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::xor_mask(x.data_[I], y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::and_mask(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::or_mask(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::xor_mask(x, y.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::and_mask(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::or_mask(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::xor_mask(x.data_[I], y, data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE lnot(array_mask_t const& x, array_mask_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::not_mask(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqrt(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sqrt(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sqr(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::ceil(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE floor(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::floor(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::exp(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE expm1(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::expm1(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp2(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::exp2(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp10(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::exp10(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE log(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::log(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE log1p(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::log1p(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE log2(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::log2(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE log10(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::log10(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sin(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sin(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE cos(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::cos(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE tan(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::tan(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE asin(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::asin(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE acos(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::acos(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE atan(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::atan(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::sinh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE cosh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::cosh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE tanh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::tanh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::asinh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE acosh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::acosh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE atanh(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::atanh(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::cbrt(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::cdf(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::inv_cdf(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::trunc(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::invsqrt(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::fabs(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(array_simd_t const& x, array_simd_t& data)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::neg(x.data_[I], data.data_[I]), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE accumulate(array_simd_t const& y, simd_t& x)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::add(x, y.data_[I], x), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE hmax(array_simd_t const& y, simd_t& x)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::max(x, y.data_[I], x), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }

    VECTORIZATION_SIMD_RETURN_TYPE hmin(array_simd_t const& y, simd_t& x)
    {
        [&]<std::size_t... I>(std::index_sequence<I...>)
        {
            (simd<value_t>::min(x, y.data_[I], x), ...);
        }(std::make_index_sequence<static_cast<std::size_t>(N)>{});
    }
};  // struct packet
}  // namespace vectorization

