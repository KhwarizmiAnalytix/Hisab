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

#ifndef __VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__
Do_not_include_expression_evaluator_directly_use_expression_it;
#endif

#include <cstdint>

#include "common/vectorization_type_traits.h"
#include "expressions/expression_interface_loader.h"

namespace vectorization
{
struct expressions_evaluator
{
    //================================================================================================
    template <typename E, typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void run(E const& expr, T& rhs)
    {
        if constexpr (is_matrix_compute<E>::value)
        {
            expr.template evaluate<T>(rhs);
        }
        else
        {
            VECTORIZATION_CHECK(
                expr.size() == rhs.size(),
                "expression has different size {} than destination {}",
                expr.size(),
                rhs.size());

            auto*  data = rhs.begin();
            size_t n    = rhs.size();
            size_t i    = 0;

#if VECTORIZATION_VECTORIZED
            using value_t  = typename vectorization::scalar_type<T, T>::value;
            using raw_dest = vectorization::remove_cvref_t<T>;
            static_assert(
                vectorization::is_base_expression<raw_dest>::value,
                "expressions_evaluator::run (vectorized) requires a tensor destination");

            constexpr auto length = T::length();

            const size_t cap = rhs.align_start();
            for (; i < cap; ++i)
                data[i] = vectorization::expression_loader<E, false, false>::evaluate(expr, i);

            if (i >= n)
                return;

            //aligned loop (destination is aligned; source may not be, use unaligned load)
            const size_t loop_peel = rhs.align_end();
            for (; i < loop_peel; i += length)
            {
                const auto temp0 =
                    vectorization::expression_loader<E, true, false>::evaluate(expr, i);
                packet<value_t>::store(temp0, &data[i]);
            }

            //unaligned loop
            const size_t loop_peel2 = i + ((n - i) / (length)) * (length);
            for (; i < loop_peel2; i += length)
            {
                const auto temp =
                    vectorization::expression_loader<E, true, false>::evaluate(expr, i);
                packet<value_t>::storeu(temp, &data[i]);
            }
#endif

            for (; i < n; ++i)
                data[i] = vectorization::expression_loader<E, false, false>::evaluate(expr, i);
        }
    }

    //================================================================================================
    template <typename E, typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void run(E&& expr, T& rhs)
    {
        // Forwarding overload: avoid duplicating the hot loop implementation.
        run(static_cast<vectorization::remove_cvref_t<E> const&>(expr), rhs);
    }

    //================================================================================================
    template <typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void scatter(
        T const& from, T& to, size_t k, size_t index)
    {
        size_t loop_peel = 0;
        auto*  data      = to.begin() + k;
        size_t n         = from.size();

#if VECTORIZATION_VECTORIZED
        using value_t      = typename vectorization::scalar_type<T, T>::value;
        using array_simd_t = typename packet<value_t>::array_simd_t;

        constexpr auto length = T::length();
        loop_peel             = length * (n / length);

        array_simd_t temp{};
        for (size_t i = 0, size = loop_peel; i < size; i += length)
        {
            packet<value_t>::loadu(&from.data()[i], temp);
            packet<value_t>::scatter(temp, static_cast<int>(index), &data[i * index]);
        }
#endif

        for (auto i = loop_peel; i < n; i++)
            data[i * index] = from.data()[i];
    }

    //================================================================================================
    template <typename S, typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void fill(S value, T& rhs) noexcept
    {
        auto*  data = rhs.begin();
        size_t i    = 0;

#if VECTORIZATION_VECTORIZED
        using value_t      = typename vectorization::scalar_type<T, T>::value;
        using array_simd_t = typename packet<value_t>::array_simd_t;

        constexpr auto length = T::length();

        array_simd_t temp{};
        packet<value_t>::set(static_cast<value_t>(value), temp);

        const size_t cap = rhs.align_start();
        for (; i < cap; ++i)
            data[i] = value;

        const size_t loop_peel = rhs.align_end();
        for (; i < loop_peel; i += length)
            packet<value_t>::store(temp, &data[i]);

        const size_t loop_peel2 = i + ((rhs.size() - i) / length) * length;
        for (; i < loop_peel2; i += length)
            packet<value_t>::storeu(temp, &data[i]);
#endif
        for (; i < rhs.size(); ++i)
            data[i] = value;
    }
};
}  // namespace vectorization
//================================================================================================

namespace vectorization
{
//================================================================================================
// accumulate: returns value_t (not hardcoded double) to avoid implicit widening
// for float expressions.
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE auto accumulate(EXPR&& expression) noexcept
{
    using E       = vectorization::remove_cvref_t<EXPR>;
    using value_t = typename vectorization::scalar_type<E, E>::value;

    value_t sum = 0;
    size_t  i   = 0;
    size_t  n   = expression.size();

#if VECTORIZATION_VECTORIZED
    constexpr auto length = E::length();

    if constexpr (vectorization::is_base_expression<E>::value)
    {
        const size_t cap = expression.align_start();
        for (; i < cap; ++i)
            sum += vectorization::expression_loader<E, false, false>::evaluate(
                static_cast<E const&>(expression), i);

        if (i < n)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t sum_packet;
            sum_packet = simd<value_t>::set(static_cast<value_t>(0.));

            const size_t loop_peel = expression.align_end();
            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, true>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::accumulate(temp, sum_packet);
            }

            const size_t loop_peel2 = i + ((n - i) / length) * length;
            for (; i < loop_peel2; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::accumulate(temp, sum_packet);
            }

            sum += simd<value_t>::accumulate(sum_packet);
        }
    }
    else
    {
        const size_t loop_peel = length * (n / length);
        if (loop_peel > 0)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t sum_packet;
            sum_packet = simd<value_t>::set(static_cast<value_t>(0.));

            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::accumulate(temp, sum_packet);
            }
            sum += simd<value_t>::accumulate(sum_packet);
        }
    }
#endif
    for (; i < n; ++i)
        sum += vectorization::expression_loader<E, false, false>::evaluate(
            static_cast<E const&>(expression), i);

    return sum;
}

//================================================================================================
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE auto hmin(EXPR&& expression) noexcept
{
    using E       = vectorization::remove_cvref_t<EXPR>;
    using value_t = typename vectorization::scalar_type<E, E>::value;

    value_t ret = std::numeric_limits<value_t>::max();
    size_t  i   = 0;
    size_t  n   = expression.size();

#if VECTORIZATION_VECTORIZED
    constexpr auto length = E::length();

    if constexpr (vectorization::is_base_expression<E>::value)
    {
        const size_t cap = expression.align_start();
        for (; i < cap; ++i)
            ret = std::fmin(
                ret,
                vectorization::expression_loader<E, false, false>::evaluate(
                    static_cast<E const&>(expression), i));

        if (i < n)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t min_packet;
            min_packet = simd<value_t>::set(std::numeric_limits<value_t>::max());

            const size_t loop_peel = expression.align_end();
            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, true>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmin(temp, min_packet);
            }

            const size_t loop_peel2 = i + ((n - i) / length) * length;
            for (; i < loop_peel2; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmin(temp, min_packet);
            }

            ret = std::fmin(ret, simd<value_t>::hmin(min_packet));
        }
    }
    else
    {
        const size_t loop_peel = length * (n / length);
        if (loop_peel > 0)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t min_packet;
            min_packet = simd<value_t>::set(std::numeric_limits<value_t>::max());

            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmin(temp, min_packet);
            }
            ret = std::fmin(ret, simd<value_t>::hmin(min_packet));
        }
    }
#endif
    for (; i < n; ++i)
        ret = std::fmin(
            ret,
            vectorization::expression_loader<E, false, false>::evaluate(
                static_cast<E const&>(expression), i));

    return ret;
}

//================================================================================================
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE auto hmax(EXPR&& expression) noexcept
{
    using E       = vectorization::remove_cvref_t<EXPR>;
    using value_t = typename vectorization::scalar_type<E, E>::value;

    // Use -max(), not min(): for floats std::numeric_limits<float>::min() is the
    // smallest *positive* value (~1.2e-38), not the most negative one.
    value_t ret = -std::numeric_limits<value_t>::max();
    size_t  i   = 0;
    size_t  n   = expression.size();

#if VECTORIZATION_VECTORIZED
    constexpr auto length = E::length();

    if constexpr (vectorization::is_base_expression<E>::value)
    {
        const size_t cap = expression.align_start();
        for (; i < cap; ++i)
            ret = std::fmax(
                ret,
                vectorization::expression_loader<E, false, false>::evaluate(
                    static_cast<E const&>(expression), i));

        if (i < n)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t max_packet;
            max_packet = simd<value_t>::set(-std::numeric_limits<value_t>::max());

            const size_t loop_peel = expression.align_end();
            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, true>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmax(temp, max_packet);
            }

            const size_t loop_peel2 = i + ((n - i) / length) * length;
            for (; i < loop_peel2; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmax(temp, max_packet);
            }

            ret = std::fmax(ret, simd<value_t>::hmax(max_packet));
        }
    }
    else
    {
        const size_t loop_peel = length * (n / length);
        if (loop_peel > 0)
        {
            using simd_t = typename simd<value_t>::simd_t;
            simd_t max_packet;
            max_packet = simd<value_t>::set(-std::numeric_limits<value_t>::max());

            for (; i < loop_peel; i += length)
            {
                const auto& temp = vectorization::expression_loader<E, true, false>::evaluate(
                    static_cast<E const&>(expression), i);
                packet<value_t>::hmax(temp, max_packet);
            }
            ret = std::fmax(ret, simd<value_t>::hmax(max_packet));
        }
    }
#endif
    for (; i < n; ++i)
        ret = std::fmax(
            ret,
            vectorization::expression_loader<E, false, false>::evaluate(
                static_cast<E const&>(expression), i));

    return ret;
}
}  // namespace vectorization
