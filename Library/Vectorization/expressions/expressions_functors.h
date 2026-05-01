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

// ---------------------------------------------------------------------------
// Design notes
// ---------------------------------------------------------------------------
// * Every evaluator is a pure-static functor struct — no data, no instances.
// * VECTORIZATION_FUNCTION_ATTRIBUTE expands to __host__ __device__ when
//   building with NVCC/HIPCC, making all functor() methods callable from
//   GPU device kernels.
// * On GPU, VECTORIZATION_ON_GPU_DEVICE == 1 and is_packet<T>::value is
//   always false (no SIMD register types).  Every SIMD branch is prefixed
//   with !VECTORIZATION_ON_GPU_DEVICE so the compiler sees a single
//   unconditional scalar path on device — no dead SIMD code to parse.
// * The const-ref and rvalue functor() overloads that existed before were
//   identical in every macro.  An rvalue argument binds to const-ref in C++
//   without a separate overload, so the rvalue copies were pure bloat and
//   have been removed.
// ---------------------------------------------------------------------------

//================================================================================================
// Unary function evaluators
// Packet path : packet<value_t>::op(lhs, ret)
// Scalar path : std::op(lhs)   — helpers injected in scalar_helper_functions.h
//================================================================================================
#define MACRO_FUNCTION_EVALUATOR(op)                                                              \
    struct MACRO_EVALUATOR_SUFIX(op)                                                              \
    {                                                                                             \
        template <typename T, std::enable_if_t<vectorization::is_fundamental<T>::value, bool> = true> \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(T const& lhs) noexcept    \
        {                                                                                         \
            if constexpr (                                                                        \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                vectorization::is_packet<vectorization::remove_cvref_t<T>>::value)                \
            {                                                                                     \
                using value_t =                                                                   \
                    typename scalar_type<vectorization::remove_cvref_t<T>,                        \
                                        vectorization::remove_cvref_t<T>>::value;                 \
                using array_simd_t = typename packet<value_t>::array_simd_t;                      \
                array_simd_t ret{};                                                               \
                packet<value_t>::op(lhs, ret);                                                    \
                return ret;                                                                       \
            }                                                                                     \
            else                                                                                  \
            {                                                                                     \
                return std::op(lhs);                                                              \
            }                                                                                     \
        }                                                                                         \
    };

namespace vectorization
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
}  // namespace vectorization

//================================================================================================
// Binary evaluators — result is array_simd_t
// Covers: arithmetic (+,-,*,/), max, min, pow, hypot, copysign
// and compound-assignment helpers (madd, msub, mmul, mdiv).
//================================================================================================
#define MACRO_OPERATION_EVALUATOR(op, f)                                                          \
    struct MACRO_EVALUATOR_SUFIX(op)                                                              \
    {                                                                                             \
        template <                                                                                 \
            typename LHS,                                                                         \
            typename RHS,                                                                         \
            std::enable_if_t<                                                                     \
                vectorization::is_fundamental<LHS>::value &&                                      \
                    vectorization::is_fundamental<RHS>::value,                                    \
                bool> = true>                                                                     \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                           \
            LHS const& lhs, RHS const& rhs) noexcept                                             \
        {                                                                                         \
            if constexpr (                                                                        \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value &&                           \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, RHS>::value;                       \
                using array_simd_t = typename packet<value_t>::array_simd_t;                      \
                array_simd_t ret{};                                                               \
                packet<value_t>::f(lhs, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<RHS, RHS>::value;                       \
                using array_simd_t = typename packet<value_t>::array_simd_t;                      \
                using simd_t       = typename packet<value_t>::simd_t;                            \
                simd_t temp;                                                                      \
                simd<value_t>::set(lhs, temp);                                                    \
                array_simd_t ret{};                                                               \
                packet<value_t>::f(temp, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, LHS>::value;                       \
                using array_simd_t = typename packet<value_t>::array_simd_t;                      \
                using simd_t       = typename packet<value_t>::simd_t;                            \
                simd_t temp;                                                                      \
                simd<value_t>::set(rhs, temp);                                                    \
                array_simd_t ret{};                                                               \
                packet<value_t>::f(lhs, temp, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else                                                                                  \
                return std::f(lhs, rhs);                                                          \
        }                                                                                         \
    };

namespace vectorization
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
}  // namespace vectorization

//================================================================================================
// Binary evaluators — result is array_mask_t: logical ops (&&, ||, ^)
//================================================================================================
#define MACRO_OPERATION_EVALUATOR_MASK(op, f)                                                     \
    struct MACRO_EVALUATOR_SUFIX(op)                                                              \
    {                                                                                             \
        template <                                                                                 \
            typename LHS,                                                                         \
            typename RHS,                                                                         \
            std::enable_if_t<                                                                     \
                vectorization::is_fundamental<LHS>::value &&                                      \
                    vectorization::is_fundamental<RHS>::value,                                    \
                bool> = true>                                                                     \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                           \
            LHS const& lhs, RHS const& rhs) noexcept                                             \
        {                                                                                         \
            if constexpr (                                                                        \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value &&                           \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, RHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(lhs, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<RHS, RHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                using mask_t       = typename packet<value_t>::mask_t;                            \
                mask_t temp;                                                                      \
                simd<value_t>::set(lhs, temp);                                                    \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(temp, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, LHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                using mask_t       = typename packet<value_t>::mask_t;                            \
                mask_t temp;                                                                      \
                simd<value_t>::set(rhs, temp);                                                    \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(lhs, temp, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else                                                                                  \
                return std::f(lhs, rhs);                                                          \
        }                                                                                         \
    };

namespace vectorization
{
MACRO_OPERATION_EVALUATOR_MASK(land, land);
MACRO_OPERATION_EVALUATOR_MASK(lor, lor);
MACRO_OPERATION_EVALUATOR_MASK(lxor, lxor);
}  // namespace vectorization

//================================================================================================
// Binary evaluators — result is array_mask_t: comparisons (>, <, >=, <=, ==, !=)
//================================================================================================
#define MACRO_OPERATION_EVALUATOR_COMP(op, f)                                                     \
    struct MACRO_EVALUATOR_SUFIX(op)                                                              \
    {                                                                                             \
        template <                                                                                 \
            typename LHS,                                                                         \
            typename RHS,                                                                         \
            std::enable_if_t<                                                                     \
                vectorization::is_fundamental<LHS>::value &&                                      \
                    vectorization::is_fundamental<RHS>::value,                                    \
                bool> = true>                                                                     \
        VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(                           \
            LHS const& lhs, RHS const& rhs) noexcept                                             \
        {                                                                                         \
            if constexpr (                                                                        \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value &&                           \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, RHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(lhs, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<RHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<RHS, RHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                using simd_t       = typename packet<value_t>::simd_t;                            \
                simd_t temp;                                                                      \
                simd<value_t>::set(lhs, temp);                                                    \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(temp, rhs, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else if constexpr (                                                                   \
                !VECTORIZATION_ON_GPU_DEVICE &&                                                   \
                is_packet<vectorization::remove_cvref_t<LHS>>::value)                             \
            {                                                                                     \
                using value_t      = typename scalar_type<LHS, LHS>::value;                       \
                using array_mask_t = typename packet<value_t>::array_mask_t;                      \
                using simd_t       = typename packet<value_t>::simd_t;                            \
                simd_t temp;                                                                      \
                simd<value_t>::set(rhs, temp);                                                    \
                array_mask_t ret{};                                                               \
                packet<value_t>::f(lhs, temp, ret);                                               \
                return ret;                                                                       \
            }                                                                                     \
            else                                                                                  \
                return std::f(lhs, rhs);                                                          \
        }                                                                                         \
    };

namespace vectorization
{
MACRO_OPERATION_EVALUATOR_COMP(cmpgt, gt);
MACRO_OPERATION_EVALUATOR_COMP(cmplt, lt);
MACRO_OPERATION_EVALUATOR_COMP(cmpge, ge);
MACRO_OPERATION_EVALUATOR_COMP(cmple, le);
MACRO_OPERATION_EVALUATOR_COMP(cmpeq, eq);
MACRO_OPERATION_EVALUATOR_COMP(cmpne, neq);
}  // namespace vectorization

//================================================================================================
// Compound-assignment evaluators used by +=, -=, *=, /=.
// The builder wraps LHS + RHS in a binary_expression<LHS,RHS,Xevaluator>
// and assigns it back to the terminal via operator=.  The evaluator only
// computes the new value — it does NOT modify its inputs in place.
//================================================================================================
namespace vectorization
{
MACRO_OPERATION_EVALUATOR(madd, add);
MACRO_OPERATION_EVALUATOR(msub, sub);
MACRO_OPERATION_EVALUATOR(mmul, mul);
MACRO_OPERATION_EVALUATOR(mdiv, div);
}  // namespace vectorization

//================================================================================================
// if_else(mask, true_val, false_val)
// All combinations of packet/scalar for the three arguments are handled,
// including the previously missing case: packet mask + both scalar branches
// (which caused fall-through UB in the original).
//================================================================================================
namespace vectorization
{
struct if_else_evaluator
{
    template <
        typename LHS,
        typename MHS,
        typename RHS,
        std::enable_if_t<
            vectorization::is_fundamental<LHS>::value && vectorization::is_fundamental<MHS>::value &&
                vectorization::is_fundamental<RHS>::value,
            bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(
        LHS const& lhs, MHS const& mhs, RHS const& rhs) noexcept
    {
        using rmv_lhs = vectorization::remove_cvref_t<LHS>;
        using rmv_mhs = vectorization::remove_cvref_t<MHS>;
        using rmv_rhs = vectorization::remove_cvref_t<RHS>;

        if constexpr (!VECTORIZATION_ON_GPU_DEVICE && vectorization::is_packet<rmv_lhs>::value)
        {
            using value_t      = typename scalar_type<rmv_lhs, rmv_lhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;

            if constexpr (is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
            {
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, mhs, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_rhs>::value)
            {
                // mask SIMD, true-branch scalar, false-branch SIMD
                simd_t temp;
                simd<value_t>::set(mhs, temp);
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, temp, rhs, ret);
                return ret;
            }
            else if constexpr (is_packet<rmv_mhs>::value)
            {
                // mask SIMD, true-branch SIMD, false-branch scalar
                simd_t temp;
                simd<value_t>::set(rhs, temp);
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, mhs, temp, ret);
                return ret;
            }
            else
            {
                // mask SIMD, both branches scalar — broadcast both
                simd_t temp_t, temp_f;
                simd<value_t>::set(mhs, temp_t);
                simd<value_t>::set(rhs, temp_f);
                array_simd_t ret{};
                packet<value_t>::if_else(lhs, temp_t, temp_f, ret);
                return ret;
            }
        }
        else
            return std::if_else(lhs, mhs, rhs);
    }
};
}  // namespace vectorization

//================================================================================================
// fma(a, b, c) — fused multiply-add: a*b + c
// All combinations of packet/scalar for the three operands are handled.
// The duplicate const-ref/rvalue overload pair from the original has been
// collapsed into one canonical overload (rvalue binds to const-ref in C++).
//================================================================================================
namespace vectorization
{
struct fma_evaluator
{
    template <
        typename LHS,
        typename MHS,
        typename RHS,
        std::enable_if_t<
            vectorization::is_fundamental<LHS>::value && vectorization::is_fundamental<MHS>::value &&
                vectorization::is_fundamental<RHS>::value,
            bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr auto functor(
        LHS const& lhs, MHS const& mhs, RHS const& rhs) noexcept
    {
        using rmv_lhs = vectorization::remove_cvref_t<LHS>;
        using rmv_mhs = vectorization::remove_cvref_t<MHS>;
        using rmv_rhs = vectorization::remove_cvref_t<RHS>;

        if constexpr (!VECTORIZATION_ON_GPU_DEVICE && vectorization::is_packet<rmv_lhs>::value)
        {
            // lhs is a SIMD packet
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
                // lhs SIMD, mhs and rhs both scalar — broadcast both
                simd_t temp_mhs, temp_rhs;
                simd<value_t>::set(mhs, temp_mhs);
                simd<value_t>::set(rhs, temp_rhs);
                array_simd_t ret{};
                packet<value_t>::fma(lhs, temp_mhs, temp_rhs, ret);
                return ret;
            }
        }
        else if constexpr (
            !VECTORIZATION_ON_GPU_DEVICE && is_packet<rmv_mhs>::value && is_packet<rmv_rhs>::value)
        {
            // lhs scalar, mhs SIMD, rhs SIMD
            using value_t      = typename scalar_type<rmv_mhs, rmv_rhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;
            simd_t temp;
            simd<value_t>::set(lhs, temp);
            array_simd_t ret{};
            packet<value_t>::fma(temp, mhs, rhs, ret);
            return ret;
        }
        else if constexpr (!VECTORIZATION_ON_GPU_DEVICE && is_packet<rmv_rhs>::value)
        {
            // lhs scalar, mhs scalar, rhs SIMD — pre-multiply scalars, add to vector
            using value_t      = typename scalar_type<rmv_rhs, rmv_rhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;
            simd_t temp;
            simd<value_t>::set(lhs * mhs, temp);
            array_simd_t ret{};
            packet<value_t>::add(temp, rhs, ret);
            return ret;
        }
        else if constexpr (!VECTORIZATION_ON_GPU_DEVICE && is_packet<rmv_mhs>::value)
        {
            // lhs scalar, mhs SIMD, rhs scalar
            using value_t      = typename scalar_type<rmv_mhs, rmv_mhs>::value;
            using array_simd_t = typename packet<value_t>::array_simd_t;
            using simd_t       = typename packet<value_t>::simd_t;
            simd_t temp_lhs, temp_rhs;
            simd<value_t>::set(lhs, temp_lhs);
            simd<value_t>::set(rhs, temp_rhs);
            array_simd_t ret{};
            packet<value_t>::fma(temp_lhs, mhs, temp_rhs, ret);
            return ret;
        }
        else
            return std::fma(lhs, mhs, rhs);
    }
};
}  // namespace vectorization
