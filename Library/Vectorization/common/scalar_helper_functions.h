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

#include <cmath>        // for fma, sqrt
#include <type_traits>  // for enable_if_t

#include "common/vectorization_macros.h"
#include "common/normal_cdf.h"  // for normal_distribu...

namespace std
{
template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto signcopy(T a, T b)
{
#ifdef copysign
    return copysign(a, b);
#else
    return std::copysign(a, b);
#endif
}

template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T fma(T a, T b, T c)
{
    return std::fma(a, b, c);
}

template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T neg(T a)
{
    return -a;
};

template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T invsqrt(T a)
{
    return 1. / std::sqrt(a);
};

template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T sqr(T a)
{
    return a * a;
};

template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T cdf(T a)
{
    return vectorization::normalcdf(a);
};
template <typename T, std::enable_if_t<std::is_fundamental<T>::value, int> = 0>
VECTORIZATION_FORCE_INLINE T inv_cdf(T a)
{
    return static_cast<T>(vectorization::inv_normalcdf(a));
};

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto add(T1 a, T2 b)
{
    return a + b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto sub(T1 a, T2 b)
{
    return a - b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto mul(T1 a, T2 b)
{
    return a * b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto div(T1 a, T2 b)
{
    return a / b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto eq(T1 a, T2 b)
{
    return a == b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto neq(T1 a, T2 b)
{
    return a != b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto gt(T1 a, T2 b)
{
    return a > b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto ge(T1 a, T2 b)
{
    return a >= b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto lt(T1 a, T2 b)
{
    return a < b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto le(T1 a, T2 b)
{
    return a <= b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto cmple(T1 a, T2 b)
{
    return a <= b;
}

template <typename T1, std::enable_if_t<std::is_fundamental<T1>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto lnot(T1 a)
{
    return !a;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto lxor(T1 a, T2 b)
{
    return a ^ b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto land(T1 a, T2 b)
{
    return a && b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto lor(T1 a, T2 b)
{
    return a || b;
}

template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_fundamental<T1>::value && std::is_fundamental<T2>::value, int> = 0>
VECTORIZATION_FORCE_INLINE auto if_else(bool a, T1 b, T2 c)
{
    return a ? b : c;
}
}  // namespace std
