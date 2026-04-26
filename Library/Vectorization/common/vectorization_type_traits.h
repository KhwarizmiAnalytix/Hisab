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

#include <array>
#include <cstdint>
#include <type_traits>

#include "common/intrin.h"
#include "common/packet.h"

namespace vectorization
{

template <typename T>
struct is_packet
{
    static constexpr bool value = false;
};

template <typename value_t>
using scalar_type_simd_t = typename packet<value_t>::array_simd_t;

template <>
struct is_packet<scalar_type_simd_t<double>>
{
    static constexpr bool value = true;
};

template <>
struct is_packet<scalar_type_simd_t<float>>
{
    static constexpr bool value = true;
};

#ifdef VECTORIZATION_HAS_AVX512

template <typename value_t>
using scalar_type_mask_t = typename packet<value_t>::array_mask_t;

template <>
struct is_packet<scalar_type_mask_t<double>>
{
    static constexpr bool value = true;
};

template <>
struct is_packet<scalar_type_mask_t<float>>
{
    static constexpr bool value = true;
};
#endif

}  // namespace vectorization

namespace vectorization
{
template <typename T>
struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;
}  // namespace vectorization

namespace vectorization
{
//================================================================================================
template <typename T>
struct is_fundamental
{
    static constexpr bool value =
        (std::is_fundamental<vectorization::remove_cvref_t<T>>::value ||
         is_packet<vectorization::remove_cvref_t<T>>::value);
};
}  // namespace vectorization

namespace vectorization
{
template <typename value_t>
class vector;

template <typename value_t>
class matrix;

template <typename value_t>
class tensor;

template <typename LHS, typename EVALUATOR>
class unary_expression;

template <typename LHS, typename RHS, typename EVALUATOR>
class binary_expression;

template <typename LHS, typename MHS, typename RHS, typename EVALUATOR>
class trinary_expression;

template <typename MAT>
class matrix_transpose_expression;

template <typename LHS, typename RHS>
class matrix_multiplication_expression;

template <typename LHS, typename RHS>
class matrix_vector_multiplication_expression;

template <typename LHS, typename RHS>
class vector_matrix_multiplication_expression;
}  // namespace vectorization

namespace vectorization
{
//================================================================================================
/*!
 * \brief Traits indicating if the given value is a base expr.
 * \tparam T The value to test
 */
template <typename T>
struct is_base_expression
{
    static constexpr bool value = false;
};

template <typename value_t>
struct is_base_expression<vector<value_t>>
{
    static constexpr bool value = true;
};

template <typename value_t>
struct is_base_expression<matrix<value_t>>
{
    static constexpr bool value = true;
};

template <typename value_t>
struct is_base_expression<tensor<value_t>>
{
    static constexpr bool value = true;
};

/*!
 * \brief Traits indicating if the given value is a base expr.
 * \tparam T The value to test
 */
template <typename T>
struct is_pure_expression
{
    static constexpr bool value = false;
};

template <typename T, typename E>
struct is_pure_expression<unary_expression<T, E>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2, typename E>
struct is_pure_expression<binary_expression<T1, T2, E>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2, typename T3, typename E>
struct is_pure_expression<trinary_expression<T1, T2, T3, E>>
{
    static constexpr bool value = true;
};

template <typename T1>
struct is_pure_expression<matrix_transpose_expression<T1>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_pure_expression<matrix_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_pure_expression<matrix_vector_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_pure_expression<vector_matrix_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

template <typename T>
struct is_expression
{
    static constexpr bool value = (is_pure_expression<T>::value || is_base_expression<T>::value);
};
}  // namespace vectorization

namespace vectorization
{
//================================================================================================
/*!
 * \brief Traits to get the scalar value of an expr.
 * \tparam T The value to test
 */
template <typename T, typename v, typename Dummy = void>
struct scalar_type
{
};

template <>
struct scalar_type<float, float>
{
    using value = float;
};

template <>
struct scalar_type<double, double>
{
    using value = double;
};

template <typename T>
struct scalar_type<scalar_type_simd_t<double>, T>
{
    using value = double;
};

template <typename T>
struct scalar_type<scalar_type_simd_t<float>, T>
{
    using value = float;
};

#ifdef VECTORIZATION_HAS_AVX512

template <typename T>
struct scalar_type<scalar_type_mask_t<double>, T>
{
    using value = double;
};

template <typename T>
struct scalar_type<scalar_type_mask_t<float>, T>
{
    using value = float;
};

#endif  // VECTORIZATION_HAS_AVX512

template <typename value_t, typename T>
struct scalar_type<vector<value_t>, T>
{
    using value = value_t;
};

template <typename T, typename value_t>
struct scalar_type<T, vector<value_t>, typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value = value_t;
};

template <typename value_t, typename T>
struct scalar_type<matrix<value_t>, T>
{
    using value = value_t;
};

template <typename T, typename value_t>
struct scalar_type<T, matrix<value_t>, typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value = value_t;
};

template <typename value_t, typename T>
struct scalar_type<tensor<value_t>, T>
{
    using value = value_t;
};

template <typename value_t, typename T>
struct scalar_type<T, tensor<value_t>, typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value = value_t;
};

template <typename E, typename EVALUATOR, typename T>
struct scalar_type<unary_expression<E, EVALUATOR>, T>
{
    using value = typename scalar_type<E, E>::value;
};

template <typename T, typename E, typename EVALUATOR>
struct scalar_type<
    T,
    unary_expression<E, EVALUATOR>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value = typename scalar_type<vectorization::remove_cvref_t<E>, vectorization::remove_cvref_t<E>>::value;
};

template <typename LHS, typename RHS, typename EVALUATOR, typename T>
struct scalar_type<binary_expression<LHS, RHS, EVALUATOR>, T>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<LHS>, vectorization::remove_cvref_t<RHS>>::value;
};

template <typename T, typename LHS, typename RHS, typename EVALUATOR>
struct scalar_type<
    T,
    binary_expression<LHS, RHS, EVALUATOR>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<LHS>, vectorization::remove_cvref_t<RHS>>::value;
};

template <typename LHS, typename MHS, typename RHS, typename EVALUATOR, typename T>
struct scalar_type<trinary_expression<LHS, MHS, RHS, EVALUATOR>, T>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<MHS>, vectorization::remove_cvref_t<RHS>>::value;
};

template <typename T, typename LHS, typename MHS, typename RHS, typename EVALUATOR>
struct scalar_type<
    T,
    trinary_expression<LHS, MHS, RHS, EVALUATOR>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<MHS>, vectorization::remove_cvref_t<RHS>>::value;
};
}  // namespace vectorization

namespace vectorization
{
//================================================================================================
/*!
 * \brief Traits indicating if the given value is a matrix expr.
 * \tparam T The value to test
 */
template <typename T>
struct is_matrix_operation
{
    static constexpr bool value = false;
};

template <typename value_t>
struct is_matrix_operation<matrix<value_t>>
{
    static constexpr bool value = true;
};

template <typename T>
struct is_matrix_operation<matrix_transpose_expression<T>>
{
    static constexpr bool value = is_matrix_operation<T>::value;
};

template <typename LHS, typename RHS>
struct is_matrix_operation<matrix_multiplication_expression<LHS, RHS>>
{
    static constexpr bool value =
        is_matrix_operation<LHS>::value && is_matrix_operation<RHS>::value;
};

//================================================================================================
/*!
 * \brief Traits indicating if the given value is a matrix evaluate.
 * \tparam T The value to test
 */
template <typename T>
struct is_matrix_evalute
{
    static constexpr bool value = false;
};

template <typename T1, typename T2>
struct is_matrix_evalute<matrix_vector_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_matrix_evalute<vector_matrix_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};
}  // namespace vectorization

namespace vectorization
{
//================================================================================================
/*!
 * \brief Traits indicating if the given value is a transpose expr.
 * \tparam T The value to test
 */
template <typename T>
struct is_transpose_expression
{
    static constexpr bool value = false;
};

template <typename T>
struct is_transpose_expression<matrix_transpose_expression<T>>
{
    static constexpr bool value = true;
};
}  // namespace vectorization

namespace vectorization
{
template <typename M>
struct is_matrix
{
    static constexpr bool value = false;
};

template <typename M>
struct is_matrix<matrix<M>>
{
    static constexpr bool value = true;
};

template <typename M>
struct is_vector
{
    static constexpr bool value = false;
};

template <typename M>
struct is_vector<vector<M>>
{
    static constexpr bool value = true;
};
}  // namespace vectorization
