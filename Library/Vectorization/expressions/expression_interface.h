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

#include "common/packet.h"
#include "common/scalar_helper_functions.h"
#include "common/vectorization_macros.h"
#include "common/vectorization_type_traits.h"
#include "expressions/expression_interface_loader.h"

namespace vectorization
{
/*!
 * \brief An unary expression
 *
 * This expression applies an unary operator on each element of a sub expression
 */
template <typename LHS, typename EVALUATOR>
class unary_expression final
{
    using rmv_lhs = vectorization::remove_cvref_t<LHS>;
    rmv_lhs lhs_;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept { return lhs_.size(); }

    static constexpr size_t length() { return rmv_lhs::length(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE explicit unary_expression(rmv_lhs const& lhs) noexcept
        : lhs_(lhs)
    {
    }  // NOLINT

    VECTORIZATION_FUNCTION_ATTRIBUTE explicit unary_expression(rmv_lhs&& lhs) noexcept
        : lhs_(std::move(lhs))
    {
    }

    // Defaulted copy/move must carry __host__ __device__ for CUDA/HIP device
    // code to be able to copy expression nodes (e.g. when capturing in a kernel).
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression(unary_expression const& e)     = default;
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression(unary_expression&& e) noexcept = default;

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression& operator=(unary_expression const& e) =
        delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE unary_expression& operator=(unary_expression&& e) = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return lhs_; }

    template <bool vectorize, bool aligned>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        unary_expression const& expr, size_t index) noexcept
    {
        const auto& rhs = expression_loader<rmv_lhs, vectorize, aligned>::evaluate(expr.rhs(), index);
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
    using rmv_lhs = vectorization::remove_cvref_t<LHS>;
    using rmv_rhs = vectorization::remove_cvref_t<RHS>;

    rmv_lhs lhs_;
    rmv_rhs rhs_;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept
    {
        if constexpr (vectorization::is_expression<rmv_rhs>::value)  // NOLINT
            return rhs_.size();
        else if constexpr (vectorization::is_expression<rmv_lhs>::value)  // NOLINT
            return lhs_.size();
        else
            return 0;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        if constexpr (
            vectorization::is_expression<rmv_rhs>::value &&
            vectorization::is_expression<rmv_lhs>::value)
        {
            static_assert(
                rmv_rhs::length() == rmv_lhs::length(), "expresions have different strides!");
            return rmv_rhs::length();
        }
        else if constexpr (vectorization::is_expression<rmv_rhs>::value)
        {
            return rmv_rhs::length();
        }
        else if constexpr (vectorization::is_expression<rmv_lhs>::value)
        {
            return rmv_lhs::length();
        }
        else
        {
            static_assert(
                vectorization::is_expression<rmv_lhs>::value ||
                    vectorization::is_expression<rmv_rhs>::value,
                "binary_expression: neither operand is an expression");
            return 0;  // unreachable; satisfies the compiler
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

    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression(binary_expression const& e)     = default;
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression(binary_expression&& e) noexcept = default;

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression& operator=(binary_expression const& e) =
        delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE binary_expression& operator=(binary_expression&& e) = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; }

    template <bool vectorize, bool aligned>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        binary_expression const& expr, size_t index) noexcept
    {
        const auto& lhs = expression_loader<rmv_lhs, vectorize, aligned>::evaluate(expr.lhs(), index);
        const auto& rhs = expression_loader<rmv_rhs, vectorize, aligned>::evaluate(expr.rhs(), index);
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
    using rmv_lhs = vectorization::remove_cvref_t<LHS>;
    using rmv_mhs = vectorization::remove_cvref_t<MHS>;
    using rmv_rhs = vectorization::remove_cvref_t<RHS>;

    rmv_lhs lhs_;
    rmv_mhs mhs_;
    rmv_rhs rhs_;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept
    {
        if constexpr (vectorization::is_expression<rmv_lhs>::value)
            return lhs_.size();
        else if constexpr (vectorization::is_expression<rmv_mhs>::value)
            return mhs_.size();
        else if constexpr (vectorization::is_expression<rmv_rhs>::value)
            return rhs_.size();
        else
        {
            static_assert(
                vectorization::is_expression<rmv_lhs>::value ||
                    vectorization::is_expression<rmv_mhs>::value ||
                    vectorization::is_expression<rmv_rhs>::value,
                "trinary_expression: no operand is an expression");
            return 0;  // unreachable; satisfies the compiler
        }
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        if constexpr (vectorization::is_expression<rmv_lhs>::value)
            return rmv_lhs::length();
        else if constexpr (vectorization::is_expression<rmv_mhs>::value)
            return rmv_mhs::length();
        else if constexpr (vectorization::is_expression<rmv_rhs>::value)
            return rmv_rhs::length();
        else
        {
            static_assert(
                vectorization::is_expression<rmv_lhs>::value ||
                    vectorization::is_expression<rmv_mhs>::value ||
                    vectorization::is_expression<rmv_rhs>::value,
                "trinary_expression: no operand is an expression");
            return 0;  // unreachable; satisfies the compiler
        }
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

    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression(trinary_expression const& e)     = default;
    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression(trinary_expression&& e) noexcept = default;

    // Expressions are invariant
    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression& operator=(trinary_expression const& e) =
        delete;
    VECTORIZATION_FUNCTION_ATTRIBUTE trinary_expression& operator=(trinary_expression&& e) = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& mhs() const { return mhs_; };
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; }

    template <bool vectorize, bool aligned>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        trinary_expression const& expr, size_t index) noexcept
    {
        const auto& lhs = expression_loader<rmv_lhs, vectorize, aligned>::evaluate(expr.lhs(), index);
        const auto& mhs = expression_loader<rmv_mhs, vectorize, aligned>::evaluate(expr.mhs(), index);
        const auto& rhs = expression_loader<rmv_rhs, vectorize, aligned>::evaluate(expr.rhs(), index);
        return EVALUATOR::functor(lhs, mhs, rhs);
    }
};
}  // namespace vectorization
