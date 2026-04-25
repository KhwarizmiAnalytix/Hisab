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

#ifndef __expressions_builder_h__
#define __expressions_builder_h__

#include "expressions/expression_interface.h"
#include "expressions/expressions_functors.h"
#include "expressions/expressions_matrix.h"

#define OPERATOR(s) operator s

//================================================================================================
#define MACRO_EXPRESSION_FUNCTION_1_ARG_(op, ref)                                             \
    template <typename LHS, std::enable_if_t<quarisma::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto op(LHS ref expr)                                           \
    {                                                                                         \
        return quarisma::unary_expression<LHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
    }

#define MACRO_EXPRESSION_FUNCTION_1_ARG(op)       \
    MACRO_EXPRESSION_FUNCTION_1_ARG_(op, const&); \
    MACRO_EXPRESSION_FUNCTION_1_ARG_(op, &&);

MACRO_EXPRESSION_FUNCTION_1_ARG(fabs);
MACRO_EXPRESSION_FUNCTION_1_ARG(neg);
MACRO_EXPRESSION_FUNCTION_1_ARG(floor);
MACRO_EXPRESSION_FUNCTION_1_ARG(ceil);
MACRO_EXPRESSION_FUNCTION_1_ARG(sqrt);
MACRO_EXPRESSION_FUNCTION_1_ARG(sqr);
MACRO_EXPRESSION_FUNCTION_1_ARG(exp);
MACRO_EXPRESSION_FUNCTION_1_ARG(expm1);
MACRO_EXPRESSION_FUNCTION_1_ARG(exp2);
MACRO_EXPRESSION_FUNCTION_1_ARG(log);
MACRO_EXPRESSION_FUNCTION_1_ARG(log1p);
MACRO_EXPRESSION_FUNCTION_1_ARG(log2);
MACRO_EXPRESSION_FUNCTION_1_ARG(log10);
MACRO_EXPRESSION_FUNCTION_1_ARG(sin);
MACRO_EXPRESSION_FUNCTION_1_ARG(cos);
MACRO_EXPRESSION_FUNCTION_1_ARG(tan);
MACRO_EXPRESSION_FUNCTION_1_ARG(asin);
MACRO_EXPRESSION_FUNCTION_1_ARG(acos);
MACRO_EXPRESSION_FUNCTION_1_ARG(atan);
MACRO_EXPRESSION_FUNCTION_1_ARG(sinh);
MACRO_EXPRESSION_FUNCTION_1_ARG(cosh);
MACRO_EXPRESSION_FUNCTION_1_ARG(tanh);
MACRO_EXPRESSION_FUNCTION_1_ARG(asinh);
MACRO_EXPRESSION_FUNCTION_1_ARG(acosh);
MACRO_EXPRESSION_FUNCTION_1_ARG(atanh);
MACRO_EXPRESSION_FUNCTION_1_ARG(cbrt);
MACRO_EXPRESSION_FUNCTION_1_ARG(cdf)
MACRO_EXPRESSION_FUNCTION_1_ARG(inv_cdf)
MACRO_EXPRESSION_FUNCTION_1_ARG(trunc)
MACRO_EXPRESSION_FUNCTION_1_ARG(invsqrt)

//================================================================================================
#define MACRO_EXPRESSION_OPERATOR_1_ARG_MOVE(op, symbole, ref)                                \
    template <typename LHS, std::enable_if_t<quarisma::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS && expr)                             \
    {                                                                                         \
        return quarisma::unary_expression<LHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
    }

#define MACRO_EXPRESSION_OPERATOR_1_ARG_COPY(op, symbole, ref)                                \
    template <typename LHS, std::enable_if_t<quarisma::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS const& expr)                         \
    {                                                                                         \
        return quarisma::unary_expression<LHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
    }

#define MACRO_EXPRESSION_OPERATOR_1_ARG(op, symbole)           \
    MACRO_EXPRESSION_OPERATOR_1_ARG_COPY(op, symbole, const&); \
    MACRO_EXPRESSION_OPERATOR_1_ARG_MOVE(op, symbole, &&);
//------------------------------------------------------------------------------------------------
namespace quarisma
{
MACRO_EXPRESSION_OPERATOR_1_ARG(neg, -);
MACRO_EXPRESSION_OPERATOR_1_ARG(lnot, !);
}  // namespace quarisma

//================================================================================================
#define MACRO_EXPRESSION_FUNCTION_2_ARG_(op, ref)                                                \
    template <                                                                                   \
        typename LHS,                                                                            \
        typename RHS,                                                                            \
        std::enable_if_t<                                                                        \
            quarisma::is_expression<LHS>::value || quarisma::is_expression<RHS>::value,              \
            bool> = true>                                                                        \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto op(LHS ref lhs, RHS ref rhs)                                  \
    {                                                                                            \
        return quarisma::binary_expression<LHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>(lhs, rhs); \
    };

#define MACRO_EXPRESSION_FUNCTION_2_ARG(op)       \
    MACRO_EXPRESSION_FUNCTION_2_ARG_(op, const&); \
    MACRO_EXPRESSION_FUNCTION_2_ARG_(op, &&);
//------------------------------------------------------------------------------------------------
// MACRO_EXPRESSION_FUNCTION_2_ARG(gather)
MACRO_EXPRESSION_FUNCTION_2_ARG(max);
MACRO_EXPRESSION_FUNCTION_2_ARG(min);
MACRO_EXPRESSION_FUNCTION_2_ARG(pow);
MACRO_EXPRESSION_FUNCTION_2_ARG(hypot);
MACRO_EXPRESSION_FUNCTION_2_ARG(copysign);

//================================================================================================
#define MACRO_EXPRESSION_FUNCTION_3_ARG_(op, ref)                                            \
    template <typename LHS, typename MHS, typename RHS>                                      \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto op(LHS ref lhs, MHS ref mhs, RHS ref rhs)                 \
    {                                                                                        \
        return quarisma::trinary_expression<LHS, MHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>( \
            lhs, mhs, rhs);                                                                  \
    }

#define MACRO_EXPRESSION_FUNCTION_3_ARG(op)       \
    MACRO_EXPRESSION_FUNCTION_3_ARG_(op, const&); \
    MACRO_EXPRESSION_FUNCTION_3_ARG_(op, &&);
//------------------------------------------------------------------------------------------------
MACRO_EXPRESSION_FUNCTION_3_ARG(fma);
MACRO_EXPRESSION_FUNCTION_3_ARG(if_else);

//================================================================================================
#define EXPRESSION_ARG2_OPERATOR_HELPER(op, symbole, ref)                                        \
    template <                                                                                   \
        typename LHS,                                                                            \
        typename RHS,                                                                            \
        std::enable_if_t<                                                                        \
            quarisma::is_expression<LHS>::value || quarisma::is_expression<RHS>::value,              \
            bool> = true>                                                                        \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS ref lhs, RHS ref rhs) noexcept          \
    {                                                                                            \
        return quarisma::binary_expression<LHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>(lhs, rhs); \
    }

#define MACRO_EXPRESSION_OPERATOR_2_ARG(op, symbole)      \
    EXPRESSION_ARG2_OPERATOR_HELPER(op, symbole, const&); \
    EXPRESSION_ARG2_OPERATOR_HELPER(op, symbole, &&);
//------------------------------------------------------------------------------------------------
namespace quarisma
{
MACRO_EXPRESSION_OPERATOR_2_ARG(land, &&);
MACRO_EXPRESSION_OPERATOR_2_ARG(lor, ||);
MACRO_EXPRESSION_OPERATOR_2_ARG(lxor, ^);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmpgt, >);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmplt, <);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmpge, >=);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmple, <=);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmpeq, ==);
MACRO_EXPRESSION_OPERATOR_2_ARG(cmpne, !=);
MACRO_EXPRESSION_OPERATOR_2_ARG(add, +);
MACRO_EXPRESSION_OPERATOR_2_ARG(sub, -);
// MACRO_EXPRESSION_OPERATOR_2_ARG(mul, *);
MACRO_EXPRESSION_OPERATOR_2_ARG(div, /);

}  // namespace quarisma

//================================================================================================
namespace quarisma
{
template <
    typename LHS,
    typename RHS,
    std::enable_if_t<quarisma::is_expression<LHS>::value || quarisma::is_expression<RHS>::value, bool> =
        true>
VECTORIZATION_FUNCTION_ATTRIBUTE auto operator*(LHS const& lhs, RHS const& rhs) noexcept
{
    if constexpr ((!quarisma::is_matrix_operation<LHS>::value &&
                   !quarisma::is_matrix_operation<RHS>::value))
    {
        return quarisma::binary_expression<LHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(mul)>(lhs, rhs);
    }
    else if constexpr (
        quarisma::is_matrix_operation<LHS>::value && !quarisma::is_matrix_operation<RHS>::value)
    {
        return quarisma::matrix_vector_multiplication_expression<LHS, RHS>(lhs, rhs);
    }
    else if constexpr (
        !quarisma::is_matrix_operation<LHS>::value && quarisma::is_matrix_operation<RHS>::value)
    {
        if constexpr (std::is_fundamental<LHS>::value)
        {
            return quarisma::binary_expression<LHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(mul)>(
                lhs, rhs);
        }
        else
        {
            return quarisma::vector_matrix_multiplication_expression<LHS, RHS>(lhs, rhs);
        }
    }
    else if constexpr (
        quarisma::is_matrix_operation<LHS>::value && quarisma::is_matrix_operation<RHS>::value)
    {
        return quarisma::matrix_multiplication_expression<LHS, RHS>(lhs, rhs);
    }
}

template <
    typename LHS,
    typename RHS,
    std::enable_if_t<quarisma::is_expression<LHS>::value || quarisma::is_expression<RHS>::value, bool> =
        true>
VECTORIZATION_FUNCTION_ATTRIBUTE auto operator*(LHS&& lhs, RHS&& rhs) noexcept
{
    using rmv_lhs = quarisma::remove_cvref_t<LHS>;
    using rmv_rhs = quarisma::remove_cvref_t<RHS>;
    if constexpr (
        !quarisma::is_matrix_operation<rmv_lhs>::value &&
        !quarisma::is_matrix_operation<rmv_rhs>::value)
    {
        return quarisma::binary_expression<rmv_lhs, rmv_rhs, quarisma::MACRO_EVALUATOR_SUFIX(mul)>(
            lhs, rhs);
    }
    else if constexpr (
        quarisma::is_matrix_operation<rmv_lhs>::value && !quarisma::is_matrix_operation<rmv_rhs>::value)
    {
        return quarisma::matrix_vector_multiplication_expression<rmv_lhs, rmv_rhs>(lhs, rhs);
    }
    else if constexpr (
        !quarisma::is_matrix_operation<rmv_lhs>::value && quarisma::is_matrix_operation<rmv_rhs>::value)
    {
        return quarisma::vector_matrix_multiplication_expression<rmv_lhs, rmv_rhs>(lhs, rhs);
    }
    else if constexpr (
        quarisma::is_matrix_operation<rmv_lhs>::value && quarisma::is_matrix_operation<rmv_rhs>::value)
    {
        return quarisma::matrix_multiplication_expression<rmv_lhs, rmv_rhs>(lhs, rhs);
    }
}
}  // namespace quarisma

//================================================================================================
#define EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, ref)                            \
    template <                                                                        \
        typename LHS,                                                                 \
        typename RHS,                                                                 \
        std::enable_if_t<quarisma::is_base_expression<LHS>::value, int> = 0>            \
    VECTORIZATION_FUNCTION_ATTRIBUTE void OPERATOR(symbole)(LHS ref lhs, RHS const ref rhs)  \
    {                                                                                 \
        lhs = quarisma::binary_expression<LHS, RHS, quarisma::MACRO_EVALUATOR_SUFIX(op)>( \
            static_cast<LHS const&>(lhs), static_cast<RHS const&>(rhs));              \
    }

#define MACRO_EXPRESSION_MOPERATOR_2_ARG(op, symbole) \
    EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, &); \
    EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, &&);

namespace quarisma
{
MACRO_EXPRESSION_MOPERATOR_2_ARG(madd, +=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(msub, -=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(mmul, *=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(mdiv, /=);
}  // namespace quarisma

#endif
