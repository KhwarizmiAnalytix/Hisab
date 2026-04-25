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
Do_not_include_expression_functor_directly_use_expression_it;
#endif

#define MACRO_EVALUATOR_SUFIX(op) op##_evaluator

//================================================================================================
#define MACRO_FUNCTION_EVALUATOR(op)                                                             \
    struct MACRO_EVALUATOR_SUFIX(op)                                                             \
    {                                                                                            \
        template <typename T, std::enable_if_t<quarisma::is_fundamental<T>::value, bool> = true>   \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(T const& lhs) noexcept           \
        {                                                                                        \
            if constexpr (quarisma::is_packet<quarisma::remove_cvref_t<T>>::value)                   \
            {                                                                                    \
                using value_t =                                                                  \
                    typename scalar_type<quarisma::remove_cvref_t<T>, quarisma::remove_cvref_t<T>>:: \
                        value;                                                                   \
                using array_simd_t = typename packet<value_t>::array_simd_t;                     \
                array_simd_t ret{};                                                              \
                packet<value_t>::op(lhs, ret);                                                   \
                return ret;                                                                      \
            }                                                                                    \
            else                                                                                 \
            {                                                                                    \
                return std::op(lhs);                                                             \
            }                                                                                    \
        }                                                                                        \
        template <typename T, std::enable_if_t<quarisma::is_fundamental<T>::value, bool> = true>   \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(T&& lhs) noexcept                \
        {                                                                                        \
            if constexpr (quarisma::is_packet<quarisma::remove_cvref_t<T>>::value)                   \
            {                                                                                    \
                using value_t =                                                                  \
                    typename scalar_type<quarisma::remove_cvref_t<T>, quarisma::remove_cvref_t<T>>:: \
                        value;                                                                   \
                using array_simd_t = typename packet<value_t>::array_simd_t;                     \
                array_simd_t ret{};                                                              \
                packet<value_t>::op(lhs, ret);                                                   \
                return ret;                                                                      \
            }                                                                                    \
            else                                                                                 \
            {                                                                                    \
                return std::op(lhs);                                                             \
            }                                                                                    \
        }                                                                                        \
    };

namespace quarisma
{
MACRO_FUNCTION_EVALUATOR(lnot);
MACRO_FUNCTION_EVALUATOR(neg);
MACRO_FUNCTION_EVALUATOR(fabs);
MACRO_FUNCTION_EVALUATOR(floor);
MACRO_FUNCTION_EVALUATOR(ceil);
MACRO_FUNCTION_EVALUATOR(sqrt);
MACRO_FUNCTION_EVALUATOR(sqr);
MACRO_FUNCTION_EVALUATOR(exp);
MACRO_FUNCTION_EVALUATOR(expm1);
MACRO_FUNCTION_EVALUATOR(exp2);
MACRO_FUNCTION_EVALUATOR(log);
MACRO_FUNCTION_EVALUATOR(log1p);
MACRO_FUNCTION_EVALUATOR(log2);
MACRO_FUNCTION_EVALUATOR(log10);
MACRO_FUNCTION_EVALUATOR(sin);
MACRO_FUNCTION_EVALUATOR(cos);
MACRO_FUNCTION_EVALUATOR(tan);
MACRO_FUNCTION_EVALUATOR(asin);
MACRO_FUNCTION_EVALUATOR(acos);
MACRO_FUNCTION_EVALUATOR(atan);
MACRO_FUNCTION_EVALUATOR(sinh);
MACRO_FUNCTION_EVALUATOR(cosh);
MACRO_FUNCTION_EVALUATOR(tanh);
MACRO_FUNCTION_EVALUATOR(asinh);
MACRO_FUNCTION_EVALUATOR(acosh);
MACRO_FUNCTION_EVALUATOR(atanh);
MACRO_FUNCTION_EVALUATOR(cbrt);
MACRO_FUNCTION_EVALUATOR(cdf);
MACRO_FUNCTION_EVALUATOR(inv_cdf);
MACRO_FUNCTION_EVALUATOR(trunc);
MACRO_FUNCTION_EVALUATOR(invsqrt);
}  // namespace quarisma

//================================================================================================
#define MACRO_OPERATION_EVALUATOR(op, f)                                                       \
    struct MACRO_EVALUATOR_SUFIX(op)                                                           \
    {                                                                                          \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                               \
            LHS const& lhs, RHS const& rhs) noexcept                                           \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(LHS&& lhs, RHS&& rhs) noexcept \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_simd_t = typename packet<value_t>::array_simd_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_simd_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
    };

namespace quarisma
{
MACRO_OPERATION_EVALUATOR(add, add);
MACRO_OPERATION_EVALUATOR(mul, mul);
MACRO_OPERATION_EVALUATOR(div, div);
MACRO_OPERATION_EVALUATOR(sub, sub);
MACRO_OPERATION_EVALUATOR(max, max);
MACRO_OPERATION_EVALUATOR(min, min);
MACRO_OPERATION_EVALUATOR(pow, pow);
MACRO_OPERATION_EVALUATOR(hypot, hypot);
MACRO_OPERATION_EVALUATOR(copysign, signcopy);
}  // namespace quarisma
//================================================================================================
#define MACRO_OPERATION_EVALUATOR_MASK(op, f)                                                  \
    struct MACRO_EVALUATOR_SUFIX(op)                                                           \
    {                                                                                          \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                               \
            LHS const& lhs, RHS const& rhs) noexcept                                           \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using mask_t       = typename packet<value_t>::mask_t;                         \
                mask_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using mask_t       = typename packet<value_t>::mask_t;                         \
                mask_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(LHS&& lhs, RHS&& rhs) noexcept \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using mask_t       = typename packet<value_t>::mask_t;                         \
                mask_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using mask_t       = typename packet<value_t>::mask_t;                         \
                mask_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
    };

namespace quarisma
{
MACRO_OPERATION_EVALUATOR_MASK(land, land);
MACRO_OPERATION_EVALUATOR_MASK(lor, lor);
MACRO_OPERATION_EVALUATOR_MASK(lxor, lxor);
}  // namespace quarisma

//================================================================================================
#define MACRO_OPERATION_EVALUATOR_COMP(op, f)                                                  \
    struct MACRO_EVALUATOR_SUFIX(op)                                                           \
    {                                                                                          \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                               \
            LHS const& lhs, RHS const& rhs) noexcept                                           \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
        template <                                                                             \
            typename LHS,                                                                      \
            typename RHS,                                                                      \
            std::enable_if_t<                                                                  \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,      \
                bool> = true>                                                                  \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(LHS&& lhs, RHS&& rhs) noexcept \
        {                                                                                      \
            if constexpr (                                                                     \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                               \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                 \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, rhs, ret);                                             \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<RHS, RHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(lhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(temp, rhs, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                  \
            {                                                                                  \
                using value_t      = typename scalar_type<LHS, LHS>::value;                    \
                using array_mask_t = typename packet<value_t>::array_mask_t;                   \
                using simd_t       = typename packet<value_t>::simd_t;                         \
                simd_t temp;                                                                   \
                simd<value_t>::set(rhs, temp);                                                 \
                array_mask_t ret{};                                                            \
                packet<value_t>::f(lhs, temp, ret);                                            \
                return ret;                                                                    \
            }                                                                                  \
            else                                                                               \
                return std::f(lhs, rhs);                                                       \
        }                                                                                      \
    };

namespace quarisma
{
MACRO_OPERATION_EVALUATOR_COMP(cmpgt, gt);
MACRO_OPERATION_EVALUATOR_COMP(cmplt, lt);
MACRO_OPERATION_EVALUATOR_COMP(cmpge, ge);
MACRO_OPERATION_EVALUATOR_COMP(cmple, le);
MACRO_OPERATION_EVALUATOR_COMP(cmpeq, eq);
MACRO_OPERATION_EVALUATOR_COMP(cmpne, neq);
}  // namespace quarisma

//================================================================================================
#define MACRO_MOPERATION_EVALUATOR(op, symbole)                                                    \
    struct MACRO_EVALUATOR_SUFIX(op)                                                               \
    {                                                                                              \
        template <                                                                                 \
            typename LHS,                                                                          \
            typename RHS,                                                                          \
            std::enable_if_t<                                                                      \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,          \
                bool> = true>                                                                      \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(LHS& lhs, RHS const& rhs) noexcept \
        {                                                                                          \
            if constexpr (                                                                         \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                                   \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                     \
            {                                                                                      \
                using value_t      = typename scalar_type<LHS, RHS>::value;                        \
                using array_simd_t = typename packet<value_t>::array_simd_t;                       \
                array_simd_t ret{};                                                                \
                packet<value_t>::f(lhs, rhs, lhs);                                                 \
                return ret;                                                                        \
            }                                                                                      \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                      \
            {                                                                                      \
                using value_t      = typename scalar_type<LHS, RHS>::value;                        \
                using array_simd_t = typename packet<value_t>::array_simd_t;                       \
                using simd_t       = typename packet<value_t>::simd_t;                             \
                simd_t temp;                                                                       \
                simd<value_t>::set(rhs, temp);                                                     \
                array_simd_t ret{};                                                                \
                packet<value_t>::f(lhs, temp, lhs);                                                \
                return ret;                                                                        \
            }                                                                                      \
            else if constexpr (                                                                    \
                std::is_fundamental<quarisma::remove_cvref_t<RHS>>::value &&                         \
                std::is_fundamental<quarisma::remove_cvref_t<LHS>>::value)                           \
                return std::f(lhs, rhs);                                                           \
        }                                                                                          \
        template <                                                                                 \
            typename LHS,                                                                          \
            typename RHS,                                                                          \
            std::enable_if_t<                                                                      \
                quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<RHS>::value,          \
                bool> = true>                                                                      \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(LHS& lhs, RHS&& rhs) noexcept      \
        {                                                                                          \
            if constexpr (                                                                         \
                is_packet<quarisma::remove_cvref_t<LHS>>::value &&                                   \
                is_packet<quarisma::remove_cvref_t<RHS>>::value)                                     \
            {                                                                                      \
                using value_t      = typename scalar_type<LHS, RHS>::value;                        \
                using array_simd_t = typename packet<value_t>::array_simd_t;                       \
                array_simd_t ret{};                                                                \
                packet<value_t>::f(lhs, rhs, lhs);                                                 \
                return ret;                                                                        \
            }                                                                                      \
            else if constexpr (is_packet<quarisma::remove_cvref_t<LHS>>::value)                      \
            {                                                                                      \
                using value_t      = typename scalar_type<LHS, RHS>::value;                        \
                using array_simd_t = typename packet<value_t>::array_simd_t;                       \
                using simd_t       = typename packet<value_t>::simd_t;                             \
                simd_t temp;                                                                       \
                simd<value_t>::set(rhs, temp);                                                     \
                array_simd_t ret{};                                                                \
                packet<value_t>::f(lhs, temp, lhs);                                                \
                return ret;                                                                        \
            }                                                                                      \
            else if constexpr (                                                                    \
                std::is_fundamental<quarisma::remove_cvref_t<RHS>>::value &&                         \
                std::is_fundamental<quarisma::remove_cvref_t<LHS>>::value)                           \
                return std::f(lhs, rhs);                                                           \
        }                                                                                          \
    };

namespace quarisma
{
MACRO_OPERATION_EVALUATOR(madd, add);
MACRO_OPERATION_EVALUATOR(mmul, mul);
MACRO_OPERATION_EVALUATOR(mdiv, div);
MACRO_OPERATION_EVALUATOR(msub, sub);
}  // namespace quarisma

//================================================================================================
namespace quarisma
{
struct if_else_evaluator
{
    template <
        typename LHS,
        typename MHS,
        typename RHS,
        std::enable_if_t<
            quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<MHS>::value &&
                quarisma::is_fundamental<RHS>::value,
            bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(
        LHS&& lhs, MHS&& mhs, RHS&& rhs) noexcept
    {
        if constexpr (quarisma::is_packet<quarisma::remove_cvref_t<LHS>>::value)
        {
            using value_t =
                typename scalar_type<quarisma::remove_cvref_t<LHS>, quarisma::remove_cvref_t<LHS>>::
                    value;
            if constexpr (
                is_packet<quarisma::remove_cvref_t<MHS>>::value &&
                is_packet<quarisma::remove_cvref_t<RHS>>::value)
            {
                using array_simd_t = typename packet<value_t>::array_simd_t;
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<quarisma::remove_cvref_t<RHS>>::value)
            {
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(mhs, temp);
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, temp, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<quarisma::remove_cvref_t<MHS>>::value)
            {
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(rhs, temp);
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, mhs, temp, ret);
                return ret;
            }
        }
        else
            return std::if_else(lhs, mhs, rhs);
    };
};
}  // namespace quarisma

namespace quarisma
{
struct fma_evaluator
{
    template <
        typename LHS,
        typename MHS,
        typename RHS,
        std::enable_if_t<
            quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<MHS>::value &&
                quarisma::is_fundamental<RHS>::value,
            bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(
        LHS&& lhs, MHS&& mhs, RHS&& rhs) noexcept
    {
        using rmv_lhs = quarisma::remove_cvref_t<LHS>;
        using rmv_mhs = quarisma::remove_cvref_t<MHS>;
        using rmv_rhs = quarisma::remove_cvref_t<RHS>;

        if constexpr (quarisma::is_packet<rmv_lhs>::value)
        {
            using value_t      = typename scalar_type<rmv_lhs, rmv_lhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;

            if constexpr (is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
            {
                array_simd_t ret{};
                packet<value_t>::fma(lhs, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_rhs>::value)
            {
                simd_t temp;
                simd<value_t>::set(mhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, temp, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_mhs>::value)
            {
                simd_t temp;
                simd<value_t>::set(rhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, mhs, temp, ret);
                return ret;
            }
            else
            {
                simd_t temp1;
                simd<value_t>::set(mhs, temp1);
                simd_t temp2;
                simd<value_t>::set(rhs, temp2);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, temp1, temp2, ret);
                return ret;
            }
        }
        else
        {
            if constexpr (is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
            {
                using value_t      = typename scalar_type<rmv_mhs, rmv_rhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(temp, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_rhs>::value)
            {
                using value_t      = typename scalar_type<rmv_rhs, rmv_rhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs, temp);
                simd_t temp2;
                simd<value_t>::set(mhs, temp2);
                array_simd_t ret{};
                packet<value_t>::fma(temp, temp2, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_mhs>::value)
            {
                using value_t      = typename scalar_type<rmv_mhs, rmv_mhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs, temp);
                simd_t temp2;
                simd<value_t>::set(rhs, temp2);
                array_simd_t ret{};
                packet<value_t>::fma(temp, mhs, temp2, ret);
                return ret;
            }
            else
                return std::fma(lhs, mhs, rhs);
        }
    };

    template <
        typename LHS,
        typename MHS,
        typename RHS,
        std::enable_if_t<
            quarisma::is_fundamental<LHS>::value && quarisma::is_fundamental<MHS>::value &&
                quarisma::is_fundamental<RHS>::value,
            bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(
        LHS const& lhs, MHS const& mhs, RHS const& rhs) noexcept
    {
        using rmv_lhs = quarisma::remove_cvref_t<LHS>;
        using rmv_mhs = quarisma::remove_cvref_t<MHS>;
        using rmv_rhs = quarisma::remove_cvref_t<RHS>;

        if constexpr (quarisma::is_packet<rmv_lhs>::value)
        {
            using value_t      = typename scalar_type<rmv_lhs, rmv_lhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;

            if constexpr (is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
            {
                array_simd_t ret{};
                packet<value_t>::fma(lhs, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_rhs>::value)
            {
                simd_t temp;
                simd<value_t>::set(mhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, temp, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_mhs>::value)
            {
                simd_t temp;
                simd<value_t>::set(rhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, mhs, temp, ret);
                return ret;
            }
            else
            {
                simd_t temp1;
                simd<value_t>::set(mhs, temp1);
                simd_t temp2;
                simd<value_t>::set(rhs, temp2);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, temp1, temp2, ret);
                return ret;
            }
        }
        else
        {
            if constexpr (is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
            {
                using value_t      = typename scalar_type<rmv_mhs, rmv_rhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs, temp);
                array_simd_t ret{};
                packet<value_t>::fma(temp, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_rhs>::value)
            {
                using value_t      = typename scalar_type<rmv_rhs, rmv_rhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs * mhs, temp);
                array_simd_t ret{};
                packet<value_t>::add(temp, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_mhs>::value)
            {
                using value_t      = typename scalar_type<rmv_mhs, rmv_mhs>::value;
                using array_simd_t = typename packet<value_t>::array_simd_t;
                using simd_t       = typename packet<value_t>::simd_t;
                simd_t temp;
                simd<value_t>::set(lhs, temp);
                simd_t temp2;
                simd<value_t>::set(rhs, temp2);
                array_simd_t ret{};
                packet<value_t>::fma(temp, mhs, temp2, ret);
                return ret;
            }
            else
                return std::fma(lhs, mhs, rhs);
        }
    };
};
}  // namespace quarisma

