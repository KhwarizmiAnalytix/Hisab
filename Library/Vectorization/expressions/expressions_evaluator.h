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

#include "expressions/expression_interface_loader.h"

namespace quarisma
{
struct expressions_evaluator
{
    //================================================================================================
    template <typename E, typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void run(E const& expr, T& rhs)
    {
        if constexpr (is_matrix_operation<E>::value)
        {
            expr.template evaluate<T>(rhs);
        }
        else
        {
            VECTORIZATION_CHECK(
                expr.size() == rhs.size(),
                "expression has diferrent size ",
                expr.size(),
                " than destination ",
                rhs.size());

            size_t loop_peel = 0;
            auto*  data      = rhs.begin();
            size_t n         = rhs.size();

#if defined(VECTORIZATION_VECTORIZED)
            using value_t = typename quarisma::scalar_type<T, T>::value;

            constexpr auto length = T::length();
            loop_peel             = length * (n / length);
            for (size_t i = 0, size = loop_peel; i < size; i += length)
            {
                const auto temp = quarisma::expression_loader<E, true>::evaluate(expr, i);
                packet<value_t>::storeu(temp, &data[i]);
            }
#endif

            for (auto i = loop_peel; i < n; i++)
                data[i] = quarisma::expression_loader<E, false>::evaluate(expr, i);
        }
    }

    //================================================================================================
    template <typename E, typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void run(E&& expr, T& rhs)
    {
        if constexpr (is_matrix_operation<E>::value)
        {
            expr.template evaluate<T>(rhs);
        }
        else
        {
            VECTORIZATION_CHECK(
                expr.size() == rhs.size(),
                "expression has diferrent size ",
                expr.size(),
                " than destination ",
                rhs.size());

            size_t loop_peel = 0;
            auto*  data      = rhs.begin();
            size_t n         = rhs.size();

#if defined(VECTORIZATION_VECTORIZED)
            using value_t = typename quarisma::scalar_type<T, T>::value;

            constexpr auto length = T::length();
            loop_peel             = length * (n / length);
            for (size_t i = 0, size = loop_peel; i < size; i += length)
            {
                const auto temp = quarisma::expression_loader<E, true>::evaluate(expr, i);
                packet<value_t>::storeu(temp, &data[i]);
            }
#endif

            for (auto i = loop_peel; i < n; i++)
                data[i] = quarisma::expression_loader<E, false>::evaluate(expr, i);
        }
    }

    //================================================================================================
    template <typename T>
    VECTORIZATION_FUNCTION_ATTRIBUTE static void scatter(T const& from, T& to, size_t k, size_t index)
    {
        size_t loop_peel = 0;
        auto*  data      = to.begin() + k;
        size_t n         = from.size();

#if defined(VECTORIZATION_VECTORIZED)
        using value_t      = typename quarisma::scalar_type<T, T>::value;
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
        size_t loop_peel = 0;
        auto*  data      = rhs.begin();

#if defined(VECTORIZATION_VECTORIZED)
        using value_t      = typename quarisma::scalar_type<T, T>::value;
        using array_simd_t = typename packet<value_t>::array_simd_t;

        constexpr auto length = T::length();
        loop_peel             = length * (rhs.size() / length);

        array_simd_t temp{};
        packet<value_t>::set(static_cast<value_t>(value), temp);

        for (size_t i = 0, size = loop_peel; i < size; i += length)
            packet<value_t>::storeu(temp, &data[i]);
#endif
        for (size_t i = loop_peel; i < rhs.size(); i++)
            data[i] = value;
    }
};
}  // namespace quarisma
//================================================================================================

namespace quarisma
{
//================================================================================================
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE double accumulate(EXPR&& expression) noexcept
{
    using E = quarisma::remove_cvref_t<EXPR>;

    double sum = 0;
    size_t i   = 0;

#if defined(VECTORIZATION_VECTORIZED)
    using value_t = typename quarisma::scalar_type<E, E>::value;

    constexpr auto length    = E::length();
    size_t         loop_peel = length * (expression.size() / length);

    if (loop_peel > 0)
    {
        using simd_t = typename simd<value_t>::simd_t;
        simd_t sum_packet;

        simd<value_t>::set(static_cast<value_t>(0.), sum_packet);

        for (size_t size = loop_peel; i < size; i += length)
        {
            const auto& temp =
                quarisma::expression_loader<E, true>::evaluate(static_cast<E const&>(expression), i);
            packet<value_t>::accumulate(temp, sum_packet);
        }
        sum += simd<value_t>::accumulate(sum_packet);
    }
#endif
    for (size_t size = expression.size(); i < size; i++)
        sum += quarisma::expression_loader<E, false>::evaluate(static_cast<E const&>(expression), i);

    return sum;
}

//================================================================================================
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE auto hmin(EXPR&& expression) noexcept
{
    using E       = quarisma::remove_cvref_t<EXPR>;
    using value_t = typename quarisma::scalar_type<E, E>::value;

    value_t ret = std::numeric_limits<value_t>::max();
    size_t  i   = 0;

#if defined(VECTORIZATION_VECTORIZED)
    constexpr auto length    = E::length();
    size_t         loop_peel = length * (expression.size() / length);

    if (loop_peel > 0)
    {
        using simd_t = typename simd<value_t>::simd_t;
        simd_t sum_packet;

        simd<value_t>::set(std::numeric_limits<value_t>::max(), sum_packet);

        for (size_t size = loop_peel; i < size; i += length)
        {
            const auto& temp =
                quarisma::expression_loader<E, true>::evaluate(static_cast<E const&>(expression), i);
            packet<value_t>::hmin(temp, sum_packet);
        }
        ret = simd<value_t>::hmin(sum_packet);
    }
#endif
    for (size_t size = expression.size(); i < size; i++)
        ret = std::fmin(
            ret, quarisma::expression_loader<E, false>::evaluate(static_cast<E&>(expression), i));

    return ret;
}

//================================================================================================
template <typename EXPR>
VECTORIZATION_FUNCTION_ATTRIBUTE auto hmax(EXPR&& expression) noexcept
{
    using E       = quarisma::remove_cvref_t<EXPR>;
    using value_t = typename quarisma::scalar_type<E, E>::value;

    auto   ret = std::numeric_limits<value_t>::min();
    size_t i   = 0;

#if defined(VECTORIZATION_VECTORIZED)
    constexpr auto length    = E::length();
    size_t         loop_peel = length * (expression.size() / length);

    if (loop_peel > 0)
    {
        using simd_t = typename simd<value_t>::simd_t;
        simd_t sum_packet;

        simd<value_t>::set(-std::numeric_limits<value_t>::max(), sum_packet);

        for (size_t size = loop_peel; i < size; i += length)
        {
            const auto& temp =
                quarisma::expression_loader<E, true>::evaluate(static_cast<E const&>(expression), i);
            packet<value_t>::hmax(temp, sum_packet);
        }
        ret = simd<value_t>::hmax(sum_packet);
    }
#endif
    for (size_t size = expression.size(); i < size; i++)
        ret = std::fmax(ret, quarisma::expression_loader<E, false>::evaluate(expression, i));

    return ret;
}
}  // namespace quarisma

