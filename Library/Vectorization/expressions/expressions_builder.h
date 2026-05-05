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
    template <typename LHS, std::enable_if_t<vectorization::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto op(LHS ref expr)                                           \
    {                                                                                         \
        return vectorization::unary_expression<LHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
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
    template <typename LHS, std::enable_if_t<vectorization::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS && expr)                             \
    {                                                                                         \
        return vectorization::unary_expression<LHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
    }

#define MACRO_EXPRESSION_OPERATOR_1_ARG_COPY(op, symbole, ref)                                \
    template <typename LHS, std::enable_if_t<vectorization::is_expression<LHS>::value, bool> = true> \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS const& expr)                         \
    {                                                                                         \
        return vectorization::unary_expression<LHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>(expr);        \
    }

#define MACRO_EXPRESSION_OPERATOR_1_ARG(op, symbole)           \
    MACRO_EXPRESSION_OPERATOR_1_ARG_COPY(op, symbole, const&); \
    MACRO_EXPRESSION_OPERATOR_1_ARG_MOVE(op, symbole, &&);
//------------------------------------------------------------------------------------------------
namespace vectorization
{
MACRO_EXPRESSION_OPERATOR_1_ARG(neg, -);
MACRO_EXPRESSION_OPERATOR_1_ARG(lnot, !);
}  // namespace vectorization

//================================================================================================
#define MACRO_EXPRESSION_FUNCTION_2_ARG_(op, ref)                                                \
    template <                                                                                   \
        typename LHS,                                                                            \
        typename RHS,                                                                            \
        std::enable_if_t<                                                                        \
            vectorization::is_expression<LHS>::value || vectorization::is_expression<RHS>::value,              \
            bool> = true>                                                                        \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto op(LHS ref lhs, RHS ref rhs)                                  \
    {                                                                                            \
        return vectorization::binary_expression<LHS, RHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>(lhs, rhs); \
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
        return vectorization::trinary_expression<LHS, MHS, RHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>( \
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
            vectorization::is_expression<LHS>::value || vectorization::is_expression<RHS>::value,              \
            bool> = true>                                                                        \
    VECTORIZATION_FUNCTION_ATTRIBUTE auto OPERATOR(symbole)(LHS ref lhs, RHS ref rhs) noexcept          \
    {                                                                                            \
        return vectorization::binary_expression<LHS, RHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>(lhs, rhs); \
    }

#define MACRO_EXPRESSION_OPERATOR_2_ARG(op, symbole)      \
    EXPRESSION_ARG2_OPERATOR_HELPER(op, symbole, const&); \
    EXPRESSION_ARG2_OPERATOR_HELPER(op, symbole, &&);
//------------------------------------------------------------------------------------------------
namespace vectorization
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
MACRO_EXPRESSION_OPERATOR_2_ARG(mul, *);
MACRO_EXPRESSION_OPERATOR_2_ARG(div, /);

}  // namespace vectorization

//================================================================================================
#define EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, ref)                            \
    template <                                                                        \
        typename LHS,                                                                 \
        typename RHS,                                                                 \
        std::enable_if_t<vectorization::is_base_expression<LHS>::value, int> = 0>            \
    VECTORIZATION_FUNCTION_ATTRIBUTE void OPERATOR(symbole)(LHS ref lhs, RHS const ref rhs)  \
    {                                                                                 \
        lhs = vectorization::binary_expression<LHS, RHS, vectorization::MACRO_EVALUATOR_SUFIX(op)>( \
            static_cast<LHS const&>(lhs), static_cast<RHS const&>(rhs));              \
    }

#define MACRO_EXPRESSION_MOPERATOR_2_ARG(op, symbole) \
    EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, &); \
    EXPRESSION_ARG2_MOPERATOR_HELPER(op, symbole, &&);

namespace vectorization
{
MACRO_EXPRESSION_MOPERATOR_2_ARG(madd, +=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(msub, -=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(mmul, *=);
MACRO_EXPRESSION_MOPERATOR_2_ARG(mdiv, /=);
}  // namespace vectorization
#endif
