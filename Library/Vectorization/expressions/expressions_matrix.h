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
Do_not_include_expression_matrix_directly_use_expression_it;
#endif

#include "expressions/expression_interface_loader.h"

namespace vectorization
{
//================================================================================================
template <typename MAT>
class matrix_transpose_expression
{
public:
    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_transpose_expression(MAT const& mat) noexcept : mat_(mat) {}

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_transpose_expression(MAT&& mat) noexcept : mat_(std::move(mat)) {}

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_transpose_expression(matrix_transpose_expression const& expr) noexcept = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_transpose_expression(matrix_transpose_expression&& expr) noexcept = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& mat() const { return mat_; }

    template <typename E>
    VECTORIZATION_FUNCTION_ATTRIBUTE auto evaluate(E& c) const noexcept
    {
        c.matrix_transpose(mat_);
    }

private:
    MAT mat_;
};

//================================================================================================
template <typename LHS, typename RHS>
class matrix_multiplication_expression
{
    using rmv_lhs = vectorization::remove_cvref_t<LHS>;
    using rmv_rhs = vectorization::remove_cvref_t<RHS>;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_multiplication_expression(rmv_lhs const& lhs, rmv_rhs const& rhs) noexcept
        : lhs_(lhs), rhs_(rhs)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_multiplication_expression(rmv_lhs&& lhs, rmv_rhs&& rhs) noexcept
        : lhs_(std::move(lhs)), rhs_(std::move(rhs))
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_multiplication_expression(rmv_lhs const& lhs, rmv_rhs&& rhs) noexcept
        : lhs_(lhs), rhs_(std::move(rhs))
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_multiplication_expression(rmv_lhs&& lhs, rmv_rhs const& rhs) noexcept
        : lhs_(std::move(lhs)), rhs_(rhs)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix_multiplication_expression(
        matrix_multiplication_expression<LHS, RHS> const& rhs) noexcept = delete;

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix_multiplication_expression(
        matrix_multiplication_expression<LHS, RHS>&& rhs) noexcept
        : lhs_(std::move(rhs.lhs_)), rhs_(std::move(rhs.rhs_))
    {
    }

    template <typename E>
    VECTORIZATION_FUNCTION_ATTRIBUTE void evaluate(E& c) const noexcept
    {
        if constexpr (
            !is_transpose_expression<rmv_lhs>::value && !is_transpose_expression<rmv_rhs>::value)
        {
            c.matrix_multiplication(false, false, lhs_, rhs_);
            return;
        }
        else if constexpr (
            is_transpose_expression<rmv_lhs>::value && !is_transpose_expression<rmv_rhs>::value)
        {
            c.matrix_multiplication(true, false, lhs_.mat(), rhs_);
            return;
        }
        else if constexpr (
            !is_transpose_expression<rmv_lhs>::value && is_transpose_expression<rmv_rhs>::value)
        {
            c.matrix_multiplication(false, true, lhs_, rhs_.mat());
            return;
        }
        else
        {
            c.matrix_multiplication(true, true, lhs_.mat(), rhs_.mat());
            return;
        }
    }

private:
    LHS lhs_;
    RHS rhs_;
};

//================================================================================================
template <typename LHS, typename RHS>
class matrix_vector_multiplication_expression
{
    using rmv_lhs      = vectorization::remove_cvref_t<LHS>;
    using rmv_rhs      = vectorization::remove_cvref_t<RHS>;
    using value_t      = typename scalar_type<rmv_lhs, rmv_rhs>::value;
    using packet_t     = packet<value_t, packet_size<value_t>::value>;
    using array_simd_t = typename packet<value_t>::array_simd_t;
    using simd_t       = typename packet<value_t>::simd_t;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_vector_multiplication_expression(rmv_lhs const& lhs, rmv_rhs const& rhs)
        : lhs_(lhs), rhs_(rhs)
    {
        validate();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE
    matrix_vector_multiplication_expression(rmv_lhs&& lhs, rmv_rhs&& rhs)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs))
    {
        validate();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix_vector_multiplication_expression(
        matrix_vector_multiplication_expression<LHS, RHS> const& expr) noexcept = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix_vector_multiplication_expression(
        matrix_vector_multiplication_expression<LHS, RHS>&& expr) noexcept = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        static_assert(rmv_lhs::length() == rmv_rhs::length(), "expresions have different strides!");
        return rmv_rhs::length();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept { return lhs_.rows(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; };

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        matrix_vector_multiplication_expression const& expr, size_t index) noexcept
    {
        if constexpr (vectorize)
        {
            const auto peel = length() * (expr.lhs().columns() / length());

            array_simd_t t{};
            packet<value_t>::setzero(t);

            for (size_t column = 0; column < peel; column += length())
            {
                const auto& t1 =
                    expression_loader<rmv_rhs, vectorize>::evaluate(expr.rhs(), column);
                matrix_vector_multiplication(expr.lhs(), index, column, t1, t);
            }
            for (size_t column = peel; column < expr.lhs().columns(); column++)
            {
                auto t1 = expression_loader<rmv_rhs, false>::evaluate(expr.rhs(), column);
                matrix_vector_multiplication(expr.lhs(), index, column, t1, t);
            }

            return t;
        }
        else
        {
            value_t t = 0;
            for (size_t column = 0; column < expr.lhs().columns(); column++)
            {
                auto t1 = expression_loader<rmv_rhs, false>::evaluate(expr.rhs(), column);
                matrix_vector_multiplication(expr.lhs(), index, column, t1, t);
            }
            return t;
        }
    }

private:
    template <typename T, typename R>
    static void matrix_vector_multiplication(
        rmv_lhs const& rhs, size_t row, size_t column, T const& t, R& ret) noexcept
    {
        if constexpr (vectorization::is_packet<R>::value)
        {
            array_simd_t tmp{};

            if constexpr (vectorization::is_packet<T>::value)
            {
                constexpr uint32_t n = packet_t::length();
                simd_t             sum_t;

                vectorization::array<value_t, packet_t::length()> data_tmp{};

                for (size_t r = 0; r < n; ++r)
                {
                    const auto* ptr = rhs.data() + (row + r) * rhs.columns() + column;
                    packet<value_t>::loadu(ptr, tmp);
                    packet<value_t>::mul(tmp, t, tmp);

                    simd<value_t>::setzero(sum_t);
                    packet<value_t>::accumulate(tmp, sum_t);

                    data_tmp.data_[r] = static_cast<value_t>(simd<value_t>::accumulate(sum_t));
                }
                packet<value_t>::loadu(&data_tmp.data_[0], tmp);
            }
            else
            {
                const auto* ptr = rhs.data() + row * rhs.columns() + column;
                packet<value_t>::gather(ptr, static_cast<int>(rhs.columns()), tmp);
                simd_t temp;
                simd<value_t>::set(t, temp);
                packet<value_t>::mul(tmp, temp, tmp);
            }
            packet<value_t>::add(ret, tmp, ret);
        }
        else
        {
            ret += t * rhs.at(row, column);
        }
    }

    void validate() const
    {
        static_assert(
            vectorization::is_expression<rmv_rhs>::value && vectorization::is_expression<rmv_lhs>::value,
            "are not expresions!");

        VECTORIZATION_CHECK_DEBUG(
            lhs_.columns() == rhs_.size(),
            "matrix_vector_multiplication vector size {} is different from matrix number of columns {}",
            rhs_.size(),
            lhs_.columns());
    }

    LHS lhs_;
    RHS rhs_;
};

//================================================================================================
template <typename LHS, typename RHS>
class vector_matrix_multiplication_expression
{
    using rmv_lhs      = vectorization::remove_cvref_t<LHS>;
    using rmv_rhs      = vectorization::remove_cvref_t<RHS>;
    using value_t      = typename scalar_type<rmv_lhs, rmv_rhs>::value;
    using packet_t     = packet<value_t, packet_size<value_t>::value>;
    using array_simd_t = typename packet<value_t>::array_simd_t;
    using simd_t       = typename packet<value_t>::simd_t;

public:
    VECTORIZATION_FUNCTION_ATTRIBUTE
    vector_matrix_multiplication_expression(rmv_lhs const& lhs, rmv_rhs const& rhs)
        : lhs_(lhs), rhs_(rhs)
    {
        validate();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE
    vector_matrix_multiplication_expression(rmv_lhs&& lhs, rmv_rhs&& rhs)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs))
    {
        validate();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE vector_matrix_multiplication_expression(
        vector_matrix_multiplication_expression<LHS, RHS> const& rhs) = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE vector_matrix_multiplication_expression(
        vector_matrix_multiplication_expression<LHS, RHS>&& rhs) = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length()
    {
        static_assert(rmv_lhs::length() == rmv_rhs::length(), "expresions have different strides");
        return rmv_rhs::length();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& lhs() const { return lhs_; }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& rhs() const { return rhs_; };

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t size() const noexcept { return rhs_.columns(); }

    template <bool vectorize>
    VECTORIZATION_FUNCTION_ATTRIBUTE static auto evaluate(
        vector_matrix_multiplication_expression const& expr, size_t column) noexcept
    {
        if constexpr (vectorize)
        {
            const auto rows = expr.rhs().rows();
            auto       peel = length() * (rows / length());

            array_simd_t t{};
            packet<value_t>::setzero(t);

            size_t row = 0;
            for (; row < peel; row += length())
            {
                const auto& t1 = expression_loader<rmv_lhs, vectorize>::evaluate(expr.lhs(), row);
                vector_matrix_multiplication(expr.rhs(), row, column, t1, t);
            }
            for (; row < rows; row++)
            {
                auto t1 = expression_loader<rmv_lhs, false>::evaluate(expr.lhs(), row);
                vector_matrix_multiplication(expr.rhs(), row, column, t1, t);
            }
            return t;
        }
        else
        {
            const auto rows = expr.rhs().rows();
            value_t    t    = 0;
            for (size_t row = 0; row < rows; row++)
            {
                auto t1 = expression_loader<rmv_lhs, false>::evaluate(expr.lhs(), row);
                vector_matrix_multiplication(expr.rhs(), row, column, t1, t);
            }
            return t;
        }
    }

private:
    template <typename T, typename R>
    static void vector_matrix_multiplication(
        rmv_rhs const& rhs, size_t row, size_t column, T const& t, R& ret) noexcept
    {
        if constexpr (vectorization::is_packet<R>::value)
        {
            array_simd_t tmp{};

            if constexpr (vectorization::is_packet<T>::value)
            {
                simd_t sum_t;

                vectorization::array<value_t, packet_t::length()> data_tmp{};

                for (size_t c = 0; c < packet_t::length(); ++c)
                {
                    packet<value_t>::gather(
                        rhs.data() + row * rhs.columns() + column + c,
                        static_cast<int>(rhs.columns()),
                        tmp);
                    packet<value_t>::mul(tmp, t, tmp);

                    simd<value_t>::setzero(sum_t);
                    packet<value_t>::accumulate(tmp, sum_t);

                    data_tmp.data_[c] = static_cast<value_t>(simd<value_t>::accumulate(sum_t));
                }
                packet<value_t>::load(&data_tmp.data_[0], tmp);
            }
            else
            {
                packet<value_t>::loadu(rhs.data() + row * rhs.columns() + column, tmp);
                simd_t temp;
                simd<value_t>::set(t, temp);
                packet<value_t>::mul(tmp, temp, tmp);
            }
            packet<value_t>::add(ret, tmp, ret);
        }
        else
        {
            ret += t * rhs.at(row, column);
        }
    }

    void validate() const
    {
        static_assert(
            is_expression<RHS>::value && is_expression<LHS>::value,
            " LHS or RHS is not expression");

        VECTORIZATION_CHECK_DEBUG(
            rhs_.rows() == lhs_.size(),
            "vector_matrix_multiplication_expression vector size {} is different from matrix number of rows {}",
            lhs_.size(),
            rhs_.rows());
    }

    LHS lhs_;
    RHS rhs_;
};
}  // namespace vectorization

