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

#include <cmath>
#include <cstddef>
#include <limits>

namespace quarisma
{
namespace constants
{
inline constexpr size_t MAX_BIT              = 32;
inline constexpr double PI                   = 3.14159265358979323846264338327950288;
inline constexpr double SQRT_2               = 1.41421356237309504880168872420969807;
inline constexpr double SQRT_PI              = 1.77245385090551602729816748334114518;
inline constexpr double INVERSE_SQRT_PI      = 1. / SQRT_PI;
inline constexpr double INVERSE_SQRT_SQRT_PI = 0.75112554446494248285870300477623;
inline constexpr size_t POW_2_32             = 4294967296;
inline constexpr double INVERSE_SQRT_2       = 1. / SQRT_2;
inline constexpr double SQRT_2PI             = SQRT_PI * SQRT_2;
inline constexpr double INVERSE_SQRT_2PI     = 1. / (SQRT_2PI);
inline constexpr double SQRT_HALF_PI         = SQRT_PI / SQRT_2;
inline constexpr double ONE_THIRD            = 1. / 3.;
inline constexpr double PI_SQUARE            = PI * PI;
inline constexpr double SQRT_3               = 1.732050807568877293527446341505872;
inline constexpr double LOG_2                = 0.69314718055994529;
inline constexpr double INVERSE_SQRT_3       = 1. / SQRT_3;
};  // namespace constants

namespace time
{
inline constexpr int MONTHS_PER_YEAR    = 12;
inline constexpr int MAX_DAYS_PER_MONTH = 31;
inline constexpr int DAYS_PER_WEEK      = 7;
inline constexpr int HOURS_PER_DAY      = 24;
inline constexpr int MINUTES_PER_HOUR   = 60;
inline constexpr int SECONDS_PER_MINUTE = 60;
inline constexpr int MILLIS_PER_SECOND  = 1000;
}  // namespace time

//-----------------------------------------------------------------------------
template <typename T>
inline constexpr bool is_almost_zero(T x, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
{
    return (std::fabs(x) < epsilon);
}
}  // namespace quarisma
