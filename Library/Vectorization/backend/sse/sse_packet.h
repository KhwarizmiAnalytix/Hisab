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

namespace vectorization
{
template <size_t SIZE, typename T, typename... Ts>
using all_same =
    std::enable_if_t<std::conjunction_v<std::is_same<T, Ts>...> && sizeof...(Ts) == SIZE>;

template <size_t N>
class packet_mask<float, N>
{
    static constexpr uint32_t SIZE      = N / 4;
    static constexpr uint32_t Alignment = 64;

public:
    using packet_t       = __m128;
    using value_t        = uint32_t;
    using packet_value_t = float;

    static constexpr uint32_t size() noexcept { return SIZE; }
    static constexpr uint32_t length() noexcept { return N; }
    static constexpr uint32_t alignment() noexcept { return Alignment; }

    SIMD_MASK_ALL_MACROS(8, ps, );

    template <typename... Ts, typename = all_same<SIZE, packet_t, Ts...>>
    VECTORIZATION_FORCE_INLINE explicit packet_mask(Ts... args) : data_{args...}
    {
    }

    VECTORIZATION_FORCE_INLINE explicit packet_mask(value_t m) noexcept { assign(m); }

    VECTORIZATION_FORCE_INLINE packet_mask() noexcept = default;

    VECTORIZATION_FORCE_INLINE packet_mask& operator=(packet_mask const& rhs) noexcept /*NOLINT*/
    {
        return assign(rhs);
    }
    VECTORIZATION_FORCE_INLINE explicit packet_mask(value_t const* m) noexcept { loadu(m); }

    VECTORIZATION_FORCE_INLINE packet_mask operator&(packet_mask const& b) const noexcept
    {
        return land(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator&&(packet_mask const& b) const noexcept
    {
        return land(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator&(value_t b) const noexcept { return land(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator&&(value_t b) const noexcept { return land(*this, b); }

    VECTORIZATION_FORCE_INLINE packet_mask operator|(packet_mask const& b) const noexcept
    {
        return lor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator||(packet_mask const& b) const noexcept
    {
        return lor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator|(value_t b) const noexcept { return lor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator||(value_t b) const noexcept { return lor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator^(packet_mask const& b) const noexcept
    {
        return lxor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator^(value_t b) const noexcept { return lxor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator!() const noexcept { return lnot(*this); }

private:
    friend packet<float, N>;

    VECTORIZATION_ALIGN(Alignment) std::array<packet_t, SIZE> data_;
};

template <size_t N>
class packet_mask<double, N>
{
    static constexpr uint32_t SIZE      = N / 2;
    static constexpr uint32_t Alignment = 64;

public:
    using packet_t       = __m128d;
    using value_t        = uint32_t;
    using packet_value_t = double;

    static constexpr uint32_t size() noexcept { return SIZE; }
    static constexpr uint32_t length() noexcept { return N; }
    static constexpr uint32_t alignment() noexcept { return Alignment; }

    SIMD_MASK_ALL_MACROS(8, pd, );

    template <typename... Ts, typename = all_same<SIZE, packet_t, Ts...>>
    VECTORIZATION_FORCE_INLINE explicit packet_mask(Ts... args) : data_{args...}
    {
    }

    VECTORIZATION_FORCE_INLINE explicit packet_mask(value_t m) noexcept { assign(m); }

    VECTORIZATION_FORCE_INLINE explicit packet_mask(value_t const* m) noexcept { loadu(m); }

    VECTORIZATION_FORCE_INLINE packet_mask() noexcept = default;

    VECTORIZATION_FORCE_INLINE packet_mask& operator=(packet_mask const& rhs) noexcept /*NOLINT*/
    {
        return assign(rhs);
    }

    VECTORIZATION_FORCE_INLINE packet_mask operator&(packet_mask const& b) const noexcept
    {
        return land(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator&&(packet_mask const& b) const noexcept
    {
        return land(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator&(value_t b) const noexcept { return land(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator&&(value_t b) const noexcept { return land(*this, b); }

    VECTORIZATION_FORCE_INLINE packet_mask operator|(packet_mask const& b) const noexcept
    {
        return lor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator||(packet_mask const& b) const noexcept
    {
        return lor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator|(value_t b) const noexcept { return lor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator||(value_t b) const noexcept { return lor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator^(packet_mask const& b) const noexcept
    {
        return lxor(*this, b);
    }
    VECTORIZATION_FORCE_INLINE packet_mask operator^(value_t b) const noexcept { return lxor(*this, b); }
    VECTORIZATION_FORCE_INLINE packet_mask operator!() const noexcept { return lnot(*this); }

private:
    friend packet<double, N>;

    VECTORIZATION_ALIGN(Alignment) packet_t data_[SIZE];
};

}  // namespace vectorization

namespace vectorization
{
#ifdef __SSE4_1__
#define IF_ELSE(mask, a, b) _mm_blendv_ps(b, a, mask);
#else
#define IF_ELSE(mask, a, b) _mm_or_ps(_mm_andnot_ps(mask, b), _mm_and_ps(mask, a));
#endif

template <size_t N>
class packet<float, N>
{
public:
    static constexpr uint32_t SIZE      = N / 4;
    static constexpr uint32_t Alignment = 64;

    using packet_t  = __m128;
    using value_t   = float;
    using mask_type = packet_mask<value_t, N>;
    using size_type = uint32_t;

private:
    VECTORIZATION_ALIGN(Alignment) packet_t data_[SIZE];

public:
    static constexpr uint32_t size() noexcept { return SIZE; }

    static constexpr uint32_t length() noexcept { return N; }

    static constexpr uint32_t alignment() noexcept { return Alignment; }

    template <typename... Ts, typename = all_same<SIZE, packet_t, Ts...>>
    VECTORIZATION_FORCE_INLINE explicit packet(Ts... args) : data_{args...}
    {
    }

    VECTORIZATION_FORCE_INLINE packet() : data_{0} {}

    VECTORIZATION_FORCE_INLINE packet(value_t f) noexcept { assign(f); }

    VECTORIZATION_FORCE_INLINE explicit packet(value_t const* p) noexcept { loadu(p); }

    VECTORIZATION_FORCE_INLINE packet& operator=(packet const& rhs) noexcept /*NOLINT*/
    {
        assign(rhs);
        return *this;
    }
    VECTORIZATION_FORCE_INLINE packet& operator=(value_t b) noexcept
    {
        assign(b);
        return *this;
    }

    friend VECTORIZATION_FORCE_INLINE packet operator+(value_t b, packet const& rhs)
    {
        return add(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator-(value_t b, packet const& rhs)
    {
        return sub(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator*(value_t b, packet const& rhs)
    {
        return mul(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator/(value_t b, packet const& rhs)
    {
        return div(b, rhs);
    }
    VECTORIZATION_FORCE_INLINE packet  operator+(packet const& b) const { return add(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator+(value_t b) const { return add(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator+=(packet const& b) { return adda(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator+=(value_t b) { return adda(*this, b); }

    VECTORIZATION_FORCE_INLINE packet  operator-(packet const& b) const { return sub(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator-(value_t b) const { return sub(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator-=(packet const& b) { return suba(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator-=(value_t b) { return suba(*this, b); }

    VECTORIZATION_FORCE_INLINE packet  operator*(packet const& b) const { return mul(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator*(value_t b) const { return mul(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator*=(packet const& b) { return mula(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator*=(value_t b) { return mula(*this, b); }

    VECTORIZATION_FORCE_INLINE packet  operator/(packet const& b) const { return div(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator/(value_t b) const { return div(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator/=(packet const& b) { return diva(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator/=(value_t b) { return diva(*this, b); }

    VECTORIZATION_FORCE_INLINE mask_type operator==(packet const& rhs) const { return cmpeq(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator==(value_t b) const { return cmpeq(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator!=(packet const& rhs) const { return cmpneq(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator!=(value_t b) const { return cmpneq(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator>(packet const& rhs) const { return cmpgt(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator>(value_t b) const { return cmpgt(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator>=(packet const& rhs) const { return cmpge(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator>=(value_t b) const { return cmpge(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator<(packet const& rhs) const { return cmplt(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator<(value_t b) const { return cmplt(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator<=(packet const& rhs) const { return cmple(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator<=(value_t b) const { return cmple(*this, b); }

    MACROS_SIMD_FLOAT128_ALL(8);
};

#undef IF_ELSE

#ifdef __SSE4_1__
#define IF_ELSE(mask, a, b) _mm_blendv_pd(b, a, mask);
#else
#define IF_ELSE(mask, a, b) _mm_or_pd(_mm_andnot_pd(mask, b), _mm_and_pd(mask, a));
#endif

template <size_t N>
class packet<double, N>
{
public:
    static constexpr uint32_t Alignment = 64;
    static constexpr uint32_t SIZE      = N / 2;

    using value_t   = double;
    using packet_t  = __m128d;
    using mask_type = packet_mask<value_t, N>;
    using size_type = uint32_t;

private:
    VECTORIZATION_ALIGN(Alignment) packet_t data_[SIZE];

public:
    static constexpr uint32_t size() noexcept { return SIZE; }
    static constexpr uint32_t length() noexcept { return N; }
    static constexpr uint32_t alignment() noexcept { return Alignment; }

    template <typename... Ts, typename = all_same<SIZE, packet_t, Ts...>>
    VECTORIZATION_FORCE_INLINE explicit packet(Ts... args) : data_{args...}
    {
    }

    VECTORIZATION_FORCE_INLINE packet() : data_{0} {}

    VECTORIZATION_FORCE_INLINE packet(value_t f) noexcept { assign(f); }

    VECTORIZATION_FORCE_INLINE explicit packet(value_t const* p) noexcept { loadu(p); }

    VECTORIZATION_FORCE_INLINE packet& operator=(packet const& rhs) noexcept /*NOLINT*/
    {
        assign(rhs);
        return *this;
    }
    VECTORIZATION_FORCE_INLINE packet& operator=(value_t b) noexcept
    {
        assign(b);
        return *this;
    }

    friend VECTORIZATION_FORCE_INLINE packet operator+(value_t b, packet const& rhs)
    {
        return add(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator-(value_t b, packet const& rhs)
    {
        return sub(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator*(value_t b, packet const& rhs)
    {
        return mul(b, rhs);
    }
    friend VECTORIZATION_FORCE_INLINE packet operator/(value_t b, packet const& rhs)
    {
        return div(b, rhs);
    }
    VECTORIZATION_FORCE_INLINE packet  operator+(packet const& b) const { return add(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator+(value_t b) const { return add(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator+=(packet const& b) { return adda(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator+=(value_t b) { return adda(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator-(packet const& b) const { return sub(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator-(value_t b) const { return sub(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator-=(packet const& b) { return suba(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator-=(value_t b) { return suba(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator*(packet const& b) const { return mul(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator*(value_t b) const { return mul(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator*=(packet const& b) { return mula(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator*=(value_t b) { return mula(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator/(packet const& b) const { return div(*this, b); }
    VECTORIZATION_FORCE_INLINE packet  operator/(value_t b) const { return div(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator/=(packet const& b) { return diva(*this, b); }
    VECTORIZATION_FORCE_INLINE packet& operator/=(value_t b) { return diva(*this, b); }

    VECTORIZATION_FORCE_INLINE mask_type operator==(packet const& rhs) const { return cmpeq(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator==(value_t b) const { return cmpeq(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator!=(packet const& rhs) const { return cmpneq(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator!=(value_t b) const { return cmpneq(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator>(packet const& rhs) const { return cmpgt(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator>(value_t b) const { return cmpgt(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator>=(packet const& rhs) const { return cmpge(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator>=(value_t b) const { return cmpge(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator<(packet const& rhs) const { return cmplt(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator<(value_t b) const { return cmplt(*this, b); }
    VECTORIZATION_FORCE_INLINE mask_type operator<=(packet const& rhs) const { return cmple(*this, rhs); }
    VECTORIZATION_FORCE_INLINE mask_type operator<=(value_t b) const { return cmple(*this, b); }

    MACROS_SIMD_DOUBLE128_ALL(8);
};

#undef IF_ELSE
}  // namespace vectorization

