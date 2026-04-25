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

#include "common/macros.h"
#include "common/packet.h"
#include "common/scalar_helper_functions.h"
#include "common/vectorization_type_traits.h"
#include "expressions/expression_interface_loader.h"

namespace quarisma
{
/*!
 * \brief An unary expression
 *
 * This expression applies an unary operator on each element of a sub expression
 */
template <typename LHS, typename EVALUATOR>
class unary_expression final
{
    LHS lhs_;
    using rmv_lhs = quarisma::remove_cvref_t<LHS>;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept { return lhs_.size(); }

    static constexpr size_t length() { return rmv_lhs::length(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE explicit unary_expression(LHS const& lhs) noexcept
        : lhs_(lhs) {}  // NOLINT

    VECTORIZATION_FUNCTION_ATTRIBUTE explicit unary_expression(LHS&& lhs) noexcept : lhs_(std::move(lhs))
    {
    }

    /*!
     * \brief Copy construct a new unary expression
     * \param e The expression from which to copy
     */
    unary_expression(unary_expression const& e) = default;

    /*!
     * \brief Move construct a new unary expression
     * \param e The expression from which to move
     */
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression(unary_expression&& e) noexcept
        : lhs_(std::move(e.lhs_))
    {
    }

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression& operator=(unary_expression const& e) = delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression& operator=(unary_expression&& e)      = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return lhs_; }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        unary_expression const& expr, size_t index) noexcept
    {
        const auto rhs = expression_loader<rmv_lhs, vectorize>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(rhs);
    }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(unary_expression&& expr, size_t index) noexcept
    {
        const auto rhs = expression_loader<rmv_lhs, vectorize>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(rhs);
    }
};

/*!
 * \brief A binary expression
 *
 * A binary expression has a left hand side expression and a right hand side expression and for each
 * element applies a binary opeartor to both expressions.
 */
template <typename LHS, typename RHS, typename EVALUATOR>
class binary_expression final
{
    using rmv_lhs = quarisma::remove_cvref_t<LHS>;
    using rmv_rhs = quarisma::remove_cvref_t<RHS>;

    rmv_lhs lhs_;
    rmv_rhs rhs_;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept
    {
        size_t ret = 0;

        if constexpr (quarisma::is_expression<rmv_rhs>::value)  // NOLINT
        {
            ret = rhs_.size();
        }
        else if constexpr (quarisma::is_expression<rmv_lhs>::value)  // NOLINT
        {
            ret = lhs_.size();
        }

        return ret;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        if constexpr (
            quarisma::is_expression<rmv_rhs>::value && quarisma::is_expression<rmv_lhs>::value)
        {
            static_assert(
                rmv_rhs::length() == rmv_lhs::length(), "expresions have different strides!");
            return rmv_rhs::length();
        }
        else
        {
            if constexpr (quarisma::is_expression<rmv_rhs>::value)
            {
                return rmv_rhs::length();
            }
            else if constexpr (quarisma::is_expression<rmv_lhs>::value)
            {
                return rmv_lhs::length();
            }
        }
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression(
        rmv_lhs const& lhs, rmv_rhs const& rhs) noexcept  // NOLINT
        : lhs_(lhs), rhs_(rhs)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression(rmv_lhs&& lhs, rmv_rhs&& rhs) noexcept
        : lhs_(std::move(lhs)), rhs_(std::move(rhs))
    {
    }

    /*!
     * \brief Copy construct a new binary expression
     * \param e The expression from which to copy
     */
    binary_expression(binary_expression const& e) = default;

    /*!
     * \brief Move construct a new binary expression
     * \param e The expression from which to move
     */
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression(binary_expression&& e) noexcept
        : lhs_(std::move(e.lhs_)), rhs_(std::move(e.rhs_))
    {
    }

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression& operator=(binary_expression const& e) = delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression& operator=(binary_expression&& e)      = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        binary_expression const& expr, size_t index) noexcept
    {
        const auto lhs = expression_loader<rmv_lhs, vectorize>::evaluate(expr.lhs(), index);
        const auto rhs = expression_loader<rmv_rhs, vectorize>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(lhs, rhs);
    }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(binary_expression&& expr, size_t index) noexcept
    {
        const auto lhs = expression_loader<rmv_lhs, vectorize>::evaluate(expr.lhs(), index);
        const auto rhs = expression_loader<rmv_rhs, vectorize>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(lhs, rhs);
    }
};

/*!
 * \brief A trinary expression
 *
 * A trinary expression has a left hand side expression, middle hand side and a right hand side
 * expression and for each element applies a trinary opeartor to all expressions.
 */
template <typename LHS, typename MHS, typename RHS, typename EVALUATOR>
class trinary_expression final
{
    using rmv_lhs = quarisma::remove_cvref_t<LHS>;
    using rmv_mhs = quarisma::remove_cvref_t<MHS>;
    using rmv_rhs = quarisma::remove_cvref_t<RHS>;

    rmv_lhs lhs_;
    rmv_mhs mhs_;
    rmv_rhs rhs_;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept
    {
        if constexpr (quarisma::is_expression<rmv_lhs>::value)
            return lhs_.size();
        else if constexpr (quarisma::is_expression<rmv_mhs>::value)
            return mhs_.size();
        else if constexpr (quarisma::is_expression<rmv_rhs>::value)
            return rhs_.size();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        if constexpr (quarisma::is_expression<rmv_lhs>::value)
            return rmv_lhs::length();
        else if constexpr (quarisma::is_expression<rmv_mhs>::value)
            return rmv_mhs::length();
        else if constexpr (quarisma::is_expression<rmv_rhs>::value)
            return rmv_rhs::length();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression(
        rmv_lhs const& lhs, rmv_mhs const& e1, rmv_rhs const& e2) noexcept
        : lhs_(lhs), mhs_(e1), rhs_(e2)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression(
        rmv_lhs&& lhs, rmv_mhs&& mhs, rmv_rhs&& rhs) noexcept
        : lhs_(std::move(lhs)), mhs_(std::move(mhs)), rhs_(std::move(rhs))
    {
    }

    /*!
     * \brief Copy construct a new trinary expression
     * \param e The expression from which to copy
     */
    trinary_expression(trinary_expression const& e) = default;

    /*!
     * \brief Move construct a new trinary expression
     * \param e The expression from which to move
     */
    trinary_expression(trinary_expression&& e) noexcept = default;

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression& operator=(trinary_expression const& e) = delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression& operator=(trinary_expression&& e)      = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& mhs() const { return mhs_; };
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        trinary_expression const& expr, size_t index) noexcept
    {
        const auto lhs = expression_loader<rmv_lhs, vectorize>::evaluate(expr.lhs(), index);
        const auto mhs = expression_loader<rmv_mhs, vectorize>::evaluate(expr.mhs(), index);
        const auto rhs = expression_loader<rmv_rhs, vectorize>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(lhs, mhs, rhs);
    }
};
}  // namespace quarisma
