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

// ---------------------------------------------------------------------------
// is_packet
// ---------------------------------------------------------------------------
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

#if VECTORIZATION_HAS_AVX512 || VECTORIZATION_HAS_NEON || VECTORIZATION_HAS_SVE
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
#endif  // AVX512 / NEON / SVE mask packet traits

}  // namespace vectorization

// ---------------------------------------------------------------------------
// remove_cvref
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// is_fundamental (extends std::is_fundamental with SIMD packet types)
// ---------------------------------------------------------------------------
namespace vectorization
{
template <typename T>
struct is_fundamental
{
    static constexpr bool value =
        (std::is_fundamental<vectorization::remove_cvref_t<T>>::value ||
         is_packet<vectorization::remove_cvref_t<T>>::value);
};
}  // namespace vectorization

// ---------------------------------------------------------------------------
// Forward declarations
//
// tensor<T> is the sole container type.  vector<T> and matrix<T> are
// transparent type aliases so that existing code continues to compile while
// emitting a deprecation diagnostic.  All trait specialisations are written
// once for tensor<T> and automatically cover the aliases.
// ---------------------------------------------------------------------------
namespace vectorization
{

template <typename value_t>
class tensor;

// Deprecated aliases — prefer tensor<T> directly.
template <typename value_t>
using vector [[deprecated("use tensor<value_t> directly")]] = tensor<value_t>;

template <typename value_t>
using matrix [[deprecated("use tensor<value_t> directly")]] = tensor<value_t>;

// Expression node types
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

// ---------------------------------------------------------------------------
// is_base_expression / is_pure_expression / is_expression
// ---------------------------------------------------------------------------
namespace vectorization
{

template <typename T>
struct is_base_expression
{
    static constexpr bool value = false;
};

template <typename value_t>
struct is_base_expression<tensor<value_t>>
{
    static constexpr bool value = true;
};

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

// ---------------------------------------------------------------------------
// scalar_type — maps an expression type to its underlying scalar (float/double)
// ---------------------------------------------------------------------------
namespace vectorization
{

template <typename T, typename V, typename Dummy = void>
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

#if VECTORIZATION_HAS_AVX512 || VECTORIZATION_HAS_NEON || VECTORIZATION_HAS_SVE
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
#endif  // AVX512 / NEON / SVE mask scalar_type

// tensor<value_t> — covers former vector<T> and matrix<T> since they alias tensor
template <typename value_t, typename T>
struct scalar_type<tensor<value_t>, T>
{
    using value = value_t;
};

template <typename T, typename value_t>
struct scalar_type<
    T,
    tensor<value_t>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
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
    using value =
        typename scalar_type<vectorization::remove_cvref_t<E>,
                             vectorization::remove_cvref_t<E>>::value;
};

template <typename LHS, typename RHS, typename EVALUATOR, typename T>
struct scalar_type<binary_expression<LHS, RHS, EVALUATOR>, T>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<LHS>,
                             vectorization::remove_cvref_t<RHS>>::value;
};

template <typename T, typename LHS, typename RHS, typename EVALUATOR>
struct scalar_type<
    T,
    binary_expression<LHS, RHS, EVALUATOR>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<LHS>,
                             vectorization::remove_cvref_t<RHS>>::value;
};

template <typename LHS, typename MHS, typename RHS, typename EVALUATOR, typename T>
struct scalar_type<trinary_expression<LHS, MHS, RHS, EVALUATOR>, T>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<MHS>,
                             vectorization::remove_cvref_t<RHS>>::value;
};

template <typename T, typename LHS, typename MHS, typename RHS, typename EVALUATOR>
struct scalar_type<
    T,
    trinary_expression<LHS, MHS, RHS, EVALUATOR>,
    typename std::enable_if_t<vectorization::is_fundamental<T>::value>>
{
    using value =
        typename scalar_type<vectorization::remove_cvref_t<MHS>,
                             vectorization::remove_cvref_t<RHS>>::value;
};

}  // namespace vectorization

// ---------------------------------------------------------------------------
// is_matrix_operation
//
// True for tensor<T> (covers all ranks — runtime rank dispatch happens inside
// tensor::matrix_multiplication()).  Also true for expressions derived from
// tensor (transpose, matrix-multiply chains).
// ---------------------------------------------------------------------------
namespace vectorization
{

template <typename T>
struct is_matrix_operation
{
    static constexpr bool value = false;
};

// tensor<T> is NOT a matrix operation — tensor * tensor is element-wise.
// Matrix multiply is performed explicitly via matmul(A, B) which creates
// matrix_multiplication_expression.  is_matrix_operation remains false for
// tensor so that all arithmetic operators on tensors are element-wise.

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

// is_matrix_evalute — kept for expression system compatibility
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

// is_matrix_compute — true for expression types that write their result into an
// output container in a single call (evaluate(output)), rather than producing
// one element at a time.  expressions_evaluator::run dispatches on this trait.
// This is SEPARATE from is_matrix_operation: tensor<T> is not a matrix_operation
// (so tensor*tensor is element-wise), but matrix_multiplication_expression and
// matrix_transpose_expression always use the bulk-output evaluate protocol.
template <typename T>
struct is_matrix_compute
{
    static constexpr bool value = false;
};

template <typename LHS, typename RHS>
struct is_matrix_compute<matrix_multiplication_expression<LHS, RHS>>
{
    static constexpr bool value = true;
};

template <typename T>
struct is_matrix_compute<matrix_transpose_expression<T>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_matrix_compute<matrix_vector_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

template <typename T1, typename T2>
struct is_matrix_compute<vector_matrix_multiplication_expression<T1, T2>>
{
    static constexpr bool value = true;
};

}  // namespace vectorization

// ---------------------------------------------------------------------------
// is_transpose_expression
// ---------------------------------------------------------------------------
namespace vectorization
{

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

// ---------------------------------------------------------------------------
// is_matrix / is_vector
// Both are true for tensor<T> since a tensor can act as either.
// ---------------------------------------------------------------------------
namespace vectorization
{

template <typename M>
struct is_matrix
{
    static constexpr bool value = false;
};

template <typename T>
struct is_matrix<tensor<T>>
{
    static constexpr bool value = true;
};

template <typename M>
struct is_vector
{
    static constexpr bool value = false;
};

template <typename T>
struct is_vector<tensor<T>>
{
    static constexpr bool value = true;
};

}  // namespace vectorization
