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

#include "common/vectorization_macros.h"
#include "common/packet.h"
#include "common/scalar_helper_functions.h"
#include "common/vectorization_type_traits.h"

namespace vectorization
{
template <typename LHS, bool vectorize>
class expression_loader final
{
public:
    using rmv_lhs = vectorization::remove_cvref_t<LHS>;

    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(rmv_lhs const& expr, size_t index) noexcept
    {
        if constexpr (vectorization::is_base_expression<rmv_lhs>::value)
        {
            if constexpr (vectorize)
            {
                using value_t      = typename scalar_type<rmv_lhs, rmv_lhs>::value;
                using packet_t     = packet<value_t, packet_size<value_t>::value>;
                using array_simd_t = typename packet_t::array_simd_t;

                array_simd_t t{};

                auto const* ptr = &expr.data()[index];

                // Prefetch is a CPU-only hint; it must look ahead, not at the
                // address about to be loaded (that data is already in flight).
#if !VECTORIZATION_ON_GPU_DEVICE
                packet<value_t>::prefetch(ptr + packet_t::length() * 8);
#endif
                packet<value_t>::loadu(ptr, t);

                return t;
            }
            else
            {
                return expr.data()[index];
            }
        }
        else if constexpr (vectorization::is_pure_expression<rmv_lhs>::value)
        {
            return rmv_lhs::template evaluate<vectorize>(expr, index);
        }
        else if constexpr (std::is_fundamental<rmv_lhs>::value)
        {
            return expr;
        }
    }
};
}  // namespace vectorization
