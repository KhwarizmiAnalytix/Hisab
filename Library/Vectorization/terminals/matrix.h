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

#if defined(_MSC_VER) && !defined(VECTORIZATION_DISPLAY_WIN32_WARNINGS)
#pragma warning(push)
#pragma warning(disable : 4267)
#endif  // Windows Warnings

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <sstream>

//#include "common/constants.h"
#include "common/vectorization_type_traits.h"
#include "matrix_operation/matrix_multiplication.h"
#include "matrix_operation/matrix_transpose.h"
//#include "serialization_impl.h"
#include "terminals/vector.h"

namespace vectorization
{

template <typename T>
inline constexpr bool is_almost_zero(T x, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
{
    return (std::fabs(x) < epsilon);
}

template <typename value_t>
class matrix
{
public:
    using value_type             = value_t;
    using size_type              = std::size_t;
    using vector_type            = vector<value_t>;
    using evaluator              = expressions_evaluator;
    using packet_t               = packet<value_t, packet_size<value_t>::value>;
    using array_simd_t           = typename packet_t::array_simd_t;
    using data_t                 = data_ptr<value_t, false>;
    using iterator               = value_t*;
    using const_iterator         = const value_t*;
    using reverse_iterator       = std::reverse_iterator<value_t*>;
    using const_reverse_iterator = std::reverse_iterator<const value_t*>;

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t dimensions() noexcept { return 2; };

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length() noexcept
    {
        return packet_t::length();
    };

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(
        size_type           rows,
        size_type           columns,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : storage_(rows * columns, type), rows_(rows), columns_(columns) {};

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(
        void*               data,
        size_type           rows,
        size_type           columns,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : matrix((value_t*)data, rows, columns, type) {};

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(
        std::initializer_list<std::initializer_list<value_t>> list,
        vectorization::device_enum type = vectorization::device_enum::CPU)
        : storage_((list.begin())->size() * list.size(), type),
          rows_(list.size()),
          columns_((list.begin())->size())
    {
        for (size_t i = 0; i < rows_; i++)
            for (size_t j = 0; j < columns_; j++)
                (*this)[i][j] = ((list.begin() + i)->begin())[j];
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(
        value_t*            data,
        size_type           rows,
        size_type           columns,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : storage_(data, rows * columns, type), rows_(rows), columns_(columns) {};

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(
        const value_t*      data,
        size_type           rows,
        size_type           columns,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : matrix(const_cast<value_t*>(data), rows, columns, type) {};

    VECTORIZATION_CUDA_FUNCTION_TYPE void deepcopy(matrix const& rhs) noexcept
    {
        rows_    = rhs.rows_;
        columns_ = rhs.columns_;

        storage_.copy(rhs.storage_);
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto* data() const { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto*       data() { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto        size() const { return storage_.size(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE iterator begin() { return storage_.begin(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE iterator end() { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator begin() const { return storage_.begin(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator end() const { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE reverse_iterator rbegin() { return reverse_iterator(end()); }

    VECTORIZATION_FUNCTION_ATTRIBUTE reverse_iterator rend() { return reverse_iterator(begin()); }

    VECTORIZATION_FUNCTION_ATTRIBUTE const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }


    VECTORIZATION_FUNCTION_ATTRIBUTE auto rows() const noexcept { return rows_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto columns() const noexcept { return columns_; }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto operator[](size_type idx) const
    {
        VECTORIZATION_CHECK_DEBUG(idx < rows_, "row index out of range");
        return vector_type(data() + columns_ * idx, columns_);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto operator[](size_type idx)
    {
        VECTORIZATION_CHECK_DEBUG(idx < rows_, "row index out of range");
        return vector_type(data() + columns_ * idx, columns_);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto at(size_type idx) const
    {
        VECTORIZATION_CHECK_DEBUG(idx < rows_, "row index out of range");
        return vector_type(data() + columns_ * idx, columns_);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto at(size_type idx)
    {
        VECTORIZATION_CHECK_DEBUG(idx < rows_, "row index out of range");
        return vector_type(data() + columns_ * idx, columns_);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto at(size_type i, size_type j) const
    {
        VECTORIZATION_CHECK_DEBUG(i < rows_, "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < columns_, "column index out of range");
        return *(data() + columns_ * i + j);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto& at(size_type i, size_type j)
    {
        VECTORIZATION_CHECK_DEBUG(i < rows_, "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < columns_, "column index out of range");
        return *(data() + columns_ * i + j);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto* data(size_type idx) const noexcept
    {
        return data() + columns_ * idx;
    };

    bool operator==(const matrix<value_t>& rhs) const
    {
        if (size() != rhs.size())
            return false;

        if (rows_ != rhs.rows_)
            return false;

        if (columns_ != rhs.columns_)
            return false;

        for (size_t i = 0; i < size(); i++)
        {
            if (data()[i] != rhs.data()[i])
                return false;
        }

        return true;
    }

    bool operator!=(const matrix<value_t>& rhs) const { return !(*this == rhs); }

    bool empty() const { return (storage_.size() == 0); }

    bool is_zero() const
    {
        for (size_type i = 0; i < size(); ++i)
            if (!vectorization::is_almost_zero(data()[i]))
                return false;

        return true;
    }

    bool is_correlation() const
    {
        if (!symmetric())
            return false;

        for (size_type i = 0; i < rows_; ++i)
        {
            for (size_type j = 0; j < i; ++j)
            {
                if (std::fabs(at(i, j)) > 1.)
                    return false;
            }
            if (!is_almost_zero(at(i, i) - 1.))
                return false;
        }

        return true;
    }

    bool identity() const
    {
        if (columns_ != rows_)
            return false;

        for (size_type i = 0; i < rows_; ++i)
        {
            for (size_type j = 0; j < columns_; ++j)
            {
                if (at(i, j) != static_cast<double>(i == j))
                    return false;
            }
        }
        return true;
    }

    bool non_negative() const
    {
        for (size_type i = 0; i < size(); ++i)
        {
            if (data()[i] < -std::numeric_limits<double>::epsilon())
                return false;
        }

        return true;
    }

    bool positive() const
    {
        for (size_type i = 0; i < size(); ++i)
        {
            if (data()[i] < std::numeric_limits<double>::epsilon())
                return false;
        }

        return true;
    }

    bool symmetric() const
    {
        if (columns_ != rows_)
            return false;

        for (size_type i = 0; i < rows_; ++i)
        {
            for (size_type j = 0; j < i; ++j)
            {
                auto a  = at(i, j);
                auto ta = at(j, i);
                if (!vectorization::is_almost_zero(a - ta))
                    return false;
            }
        }

        return true;
    }

    auto trace() const
    {
        value_t tmp = 0.;
        for (size_t f = 0; f < columns_; ++f)
        {
            tmp += at(f, f);
        }
        return tmp;
    }

    static void swap(const matrix& A, const matrix& B) { std::swap(A.begin(), B.begin()); }

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(vector_type const& rhs, bool transpose) noexcept
        : storage_(rhs.storage_),
          rows_(transpose ? 1 : rhs.size()),
          columns_(transpose ? rhs.size() : 1) {};

    VECTORIZATION_CUDA_FUNCTION_TYPE matrix(matrix&& rhs) noexcept
        : storage_(std::move(rhs.storage_)),
          rows_(std::move(rhs.rows_)),
          columns_(std::move(rhs.columns_)) {};

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix& operator=(matrix&& rhs) noexcept
    {
        if (this != &rhs)
        {
            storage_ = std::move(rhs.storage_);
            rows_    = std::move(rhs.rows_);
            columns_ = std::move(rhs.columns_);
        }

        return *this;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix(matrix const& rhs) noexcept
        : storage_(rhs.storage_), rows_(rhs.rows_), columns_(rhs.columns_) {};

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix& operator=(matrix const& rhs) noexcept  // NOLINT
    {
        if (this != &rhs)
        {
            storage_ = rhs.storage_;
            rows_    = rhs.rows_;
            columns_ = rhs.columns_;
        }
        return *this;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE void matrix_multiplication(
        bool                   transpose_lhs,
        bool                   transpose_rhs,
        matrix<value_t> const& lhs,
        matrix<value_t> const& rhs) noexcept
    {
        size_t ldlhs = lhs.columns();
        size_t ldrhs = rhs.columns();
        size_t depth = transpose_lhs ? lhs.rows() : lhs.columns();

        vectorization::matrix_multiplication(
            transpose_lhs,
            transpose_rhs,
            rows(),
            columns(),
            depth,
            lhs.begin(),
            ldlhs,
            rhs.begin(),
            ldrhs,
            begin(),
            columns());
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE void matrix_transpose(matrix const& A)
    {
        this->deepcopy(A);  //fixme!
        vectorization::matrix_transpose(A.rows(), A.columns(), begin());
        std::swap(columns_, rows_);
    }

    friend auto transpose(matrix const& rhs) { return matrix_transpose_expression<matrix>(rhs); }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE matrix& operator=(E const& expr)
    {
        evaluator::template run<E, matrix>(expr, *this);
        return *this;
    }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE matrix& operator=(E&& expr)
    {
        evaluator::template run<E, matrix>(static_cast<E const&>(expr), *this);
        return *this;
    }

    template <typename T2, typename = typename std::enable_if<std::is_fundamental<T2>::value>::type>
    VECTORIZATION_FUNCTION_ATTRIBUTE matrix& operator=(T2 value) noexcept
    {
        evaluator::template fill<value_t, vectorization::matrix<value_t>>(
            static_cast<value_t>(value), *this);
        return *this;
    }


    VECTORIZATION_FUNCTION_ATTRIBUTE matrix() noexcept = default;  // NOLINT

    std::string to_string() const
    {
        if (this->empty())
            return "";

        // compute the largest width
        size_t width = 0;
        for (size_t j = 0; j < this->columns(); ++j)
            for (size_t i = 0; i < this->rows(); ++i)
            {
                std::stringstream sstr;
                sstr << this->at(i, j);
                width = std::max<size_t>(width, size_t(sstr.str().length()));
            }

        std::ostringstream s;
        s << "[";
        for (size_t i = 0; i < this->rows(); ++i)
        {
            if (i)
                s << ";\n";
            if (width)
                s.width(width);

            s << this->at(i, 0);
            for (size_t j = 1; j < this->columns(); ++j)
            {
                s << ", ";
                if (width)
                    s.width(width);
                s << this->at(i, j);
            }
        }
        s << "]";
        return s.str();
    }

private:
    data_t    storage_{};
    size_type rows_{0};
    size_type columns_{0};
};

}  // namespace vectorization

namespace vectorization
{
template <typename value_t>
std::ostream& operator<<(std::ostream& s, const vectorization::matrix<value_t>& m)
{
    s << m.to_string();
    return s;
}
}  // namespace vectorization

#if defined(_MSC_VER) && !defined(VECTORIZATION_DISPLAY_WIN32_WARNINGS)
#pragma warning(pop)
#endif