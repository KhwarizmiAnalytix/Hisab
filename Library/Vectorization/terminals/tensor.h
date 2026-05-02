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
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <sstream>
#include <vector>

#include "common/packet.h"
#include "expressions/expressions.h"
#include "terminals/matrix_operation/matrix_multiplication.h"
#include "terminals/matrix_operation/matrix_transpose.h"

namespace vectorization
{

template <typename T>
inline constexpr bool is_almost_zero(
    T x, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
{
    return (std::fabs(x) < epsilon);
}

// ---------------------------------------------------------------------------
// tensor<T> — unified N-dimensional container
//
// Rank-1 behaves like the former vector<T>.
// Rank-2 behaves like the former matrix<T>.
// Higher ranks generalise to arbitrary N-D storage.
//
// tensor * tensor always produces matrix_multiplication_expression so that
// matrix algebra is preserved for rank-2 operands.  Element-wise multiply
// is available via mul(a, b) or fma().
// ---------------------------------------------------------------------------
template <typename value_t>
class tensor
{
public:
    using value_type             = value_t;
    using size_type              = std::size_t;
    using dimensions_type        = std::vector<size_t>;
    using evaluator              = expressions_evaluator;
    using packet_t               = packet<value_t, packet_size<value_t>::value>;
    using array_simd_t           = typename packet_t::array_simd_t;
    using data_t                 = data_ptr<value_t, false>;
    using iterator               = value_t*;
    using const_iterator         = const value_t*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // SIMD stride — identical for every rank; used by expression_loader
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length() noexcept
    {
        return packet_t::length();
    }

    // Static rank is 0 for tensor (rank is a runtime property).
    // Provided so that generic code that tested vector<T>::dimensions()==1 or
    // matrix<T>::dimensions()==2 can migrate to tensor<T>::static_rank()==0.
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t static_rank() noexcept
    {
        return 0;
    }

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor() = default;

    // --- 1-D constructors (vector-like) ------------------------------------

    VECTORIZATION_CUDA_FUNCTION_TYPE explicit tensor(
        size_type n, device_enum type = device_enum::CPU) noexcept
        : dimensions_({n}), sizes_({n}), storage_(n, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        value_t   start,
        value_t   end,
        size_type n,
        device_enum type = device_enum::CPU) noexcept
        : tensor(n, type)
    {
        const auto dx = (end - start) / static_cast<value_t>(n - 1);
        for (size_t i = 0; i < n; ++i)
            data()[i] = static_cast<value_t>(i) * dx + start;
    }

    // 1-D pointer views.  The device_enum overload is split from the 2-arg form
    // so that (ptr, rows, cols) never ambiguously matches (ptr, n, device_enum)
    // when device_enum is an unscoped enum implicitly convertible from int.
    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        value_t* ptr, size_type n) noexcept
        : dimensions_({n}), sizes_({n}), storage_(ptr, n, device_enum::CPU)
    {}
    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        value_t* ptr, size_type n, device_enum type) noexcept
        : dimensions_({n}), sizes_({n}), storage_(ptr, n, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        const value_t* ptr, size_type n) noexcept
        : tensor(const_cast<value_t*>(ptr), n)
    {}
    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        const value_t* ptr, size_type n, device_enum type) noexcept
        : tensor(const_cast<value_t*>(ptr), n, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        void* ptr, size_type n) noexcept
        : tensor(static_cast<value_t*>(ptr), n)
    {}
    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        void* ptr, size_type n, device_enum type) noexcept
        : tensor(static_cast<value_t*>(ptr), n, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        std::vector<value_t> const& v,
        device_enum type = device_enum::CPU) noexcept
        : tensor(const_cast<value_t*>(v.data()), v.size(), type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        std::initializer_list<value_t> list,
        device_enum type = device_enum::CPU) noexcept
        : tensor(list.size(), type)
    {
        std::copy(list.begin(), list.end(), data());
    }

    // --- 2-D constructors (matrix-like) ------------------------------------

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        size_type rows,
        size_type cols,
        device_enum type = device_enum::CPU) noexcept
        : dimensions_({rows, cols}),
          sizes_({rows * cols, cols}),
          storage_(rows * cols, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        value_t*    ptr,
        size_type   rows,
        size_type   cols,
        device_enum type = device_enum::CPU) noexcept
        : dimensions_({rows, cols}),
          sizes_({rows * cols, cols}),
          storage_(ptr, rows * cols, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        const value_t* ptr,
        size_type      rows,
        size_type      cols,
        device_enum    type = device_enum::CPU) noexcept
        : tensor(const_cast<value_t*>(ptr), rows, cols, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        void*       ptr,
        size_type   rows,
        size_type   cols,
        device_enum type = device_enum::CPU) noexcept
        : tensor(static_cast<value_t*>(ptr), rows, cols, type)
    {}

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        std::initializer_list<std::initializer_list<value_t>> list,
        device_enum type = device_enum::CPU)
        : tensor(list.size(), list.begin()->size(), type)
    {
        size_t i = 0;
        for (auto const& row : list)
        {
            size_t j = 0;
            for (auto const& v : row)
                at(i, j++) = v;
            ++i;
        }
    }

    // --- N-D constructors (general tensor) ---------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        const dimensions_type& dims, device_enum type = device_enum::CPU)
        : dimensions_(dims),
          sizes_(compute_sizes(dims)),
          storage_(sizes_.front(), type)
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        dimensions_type&& dims, device_enum type = device_enum::CPU)
        : dimensions_(std::move(dims)),
          sizes_(compute_sizes(dimensions_)),
          storage_(sizes_.front(), type)
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t*               ptr,
        const dimensions_type& dims,
        device_enum            type = device_enum::CPU)
        : dimensions_(dims),
          sizes_(compute_sizes(dims)),
          storage_(ptr, sizes_.front(), type)
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        void*                  ptr,
        const dimensions_type& dims,
        device_enum            type = device_enum::CPU)
        : tensor(static_cast<value_t*>(ptr), dims, type)
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        const value_t*         ptr,
        const dimensions_type& dims,
        device_enum            type = device_enum::CPU)
        : tensor(const_cast<value_t*>(ptr), dims, type)
    {}

    // -----------------------------------------------------------------------
    // Copy / move
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(tensor const& rhs)
        : dimensions_(rhs.dimensions_), sizes_(rhs.sizes_), storage_(rhs.storage_)
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(tensor&& rhs) noexcept
        : dimensions_(std::move(rhs.dimensions_)),
          sizes_(std::move(rhs.sizes_)),
          storage_(std::move(rhs.storage_))
    {}

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(tensor const& rhs)
    {
        if (this != &rhs)
        {
            dimensions_ = rhs.dimensions_;
            sizes_      = rhs.sizes_;
            storage_    = rhs.storage_;
        }
        return *this;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(tensor&& rhs) noexcept
    {
        if (this != &rhs)
        {
            dimensions_ = std::move(rhs.dimensions_);
            sizes_      = std::move(rhs.sizes_);
            storage_    = std::move(rhs.storage_);
        }
        return *this;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE void deepcopy(tensor const& rhs) noexcept
    {
        dimensions_ = rhs.dimensions_;
        sizes_      = rhs.sizes_;
        storage_.copy(rhs.storage_);
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE ~tensor() = default;

    // -----------------------------------------------------------------------
    // Expression constructors / assignment
    // -----------------------------------------------------------------------

    template <typename E,
              std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E const& expr)
        : dimensions_({expr.size()}),
          sizes_({expr.size()}),
          storage_(expr.size(), device_enum::CPU)
    {
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <typename E,
              std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E&& expr)  // NOLINT
        : dimensions_({expr.size()}),
          sizes_({expr.size()}),
          storage_(expr.size(), device_enum::CPU)
    {
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <typename E,
              std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E const& expr)
    {
        evaluator::template run<E, tensor>(expr, *this);
        return *this;
    }

    template <typename E,
              std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E&& expr)
    {
        evaluator::template run<E, tensor>(static_cast<E const&>(expr), *this);
        return *this;
    }

    template <typename T2,
              std::enable_if_t<std::is_fundamental<T2>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(T2 value) noexcept
    {
        evaluator::template fill<value_t, tensor>(static_cast<value_t>(value), *this);
        return *this;
    }

    // -----------------------------------------------------------------------
    // Raw data / size
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t* data()  const noexcept { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE       value_t* data()        noexcept { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t         size()  const noexcept { return storage_.size(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE bool           empty() const noexcept { return size() == 0; }

    VECTORIZATION_FUNCTION_ATTRIBUTE iterator       begin()       noexcept { return storage_.begin(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE iterator       end()         noexcept { return storage_.end(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator begin() const noexcept { return storage_.begin(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator end()   const noexcept { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

    // -----------------------------------------------------------------------
    // Shape / rank
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t rank()             const noexcept { return dimensions_.size(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const dimensions_type& dimensions() const noexcept { return dimensions_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t dimension(size_t n) const noexcept { return dimensions_[n]; }

    // 1-D / 2-D convenience (mirrors former vector<T>::rows() and matrix<T>::rows/columns())
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t rows()    const noexcept
    {
        return dimensions_.empty() ? 0 : dimensions_[0];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t columns() const noexcept
    {
        return dimensions_.size() < 2 ? 1 : dimensions_[1];
    }

    // -----------------------------------------------------------------------
    // Element access
    // -----------------------------------------------------------------------

    // Flat indexed access (1-D semantic, element by element)
    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t& operator[](size_type i) const noexcept
    {
        return data()[i];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& operator[](size_type i) noexcept
    {
        return data()[i];
    }

    // 1-D scalar element
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t  at(size_type i) const noexcept { return data()[i]; }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(size_type i)       noexcept { return data()[i]; }

    // 2-D scalar element (row-major). Not noexcept: debug bounds checks may throw (like std::vector::at).
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t at(size_type i, size_type j) const
    {
        VECTORIZATION_CHECK_DEBUG(i < rows(),    "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < columns(), "column index out of range");
        return data()[i * columns() + j];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(size_type i, size_type j)
    {
        VECTORIZATION_CHECK_DEBUG(i < rows(),    "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < columns(), "column index out of range");
        return data()[i * columns() + j];
    }

    // N-D element by multi-index
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t at(const dimensions_type& indices) const
    {
        return data()[linearized_index(indices)];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(const dimensions_type& indices)
    {
        return data()[linearized_index(indices)];
    }

    // Row view as sub-tensor (for rank-2: returns a rank-1 row; for rank-N: reduces outer dim)
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor row(size_type i) const
    {
        VECTORIZATION_CHECK_DEBUG(rank() >= 2, "row() requires rank >= 2");
        VECTORIZATION_CHECK_DEBUG(i < rows(), "row index out of range");
        return tensor(data() + i * columns(), columns());
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor row(size_type i)
    {
        VECTORIZATION_CHECK_DEBUG(rank() >= 2, "row() requires rank >= 2");
        VECTORIZATION_CHECK_DEBUG(i < rows(), "row index out of range");
        return tensor(data() + i * columns(), columns());
    }

    // data() by row index (raw pointer, for internal use by expressions_matrix)
    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t* data(size_type row_idx) const noexcept
    {
        return data() + columns() * row_idx;
    }

    // get_matrix() — returns a rank-2 sub-tensor (matrix slice) from a rank≥3
    // tensor.  For a rank-3 tensor with shape [K, R, C], get_matrix(k) returns
    // the [R, C] matrix at index k.  For higher-rank tensors, provide all but
    // the last two indices in the multi-index form.
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor get_matrix(size_type indice) const
    {
        VECTORIZATION_CHECK(dimensions_.size() >= 3, "get_matrix(i) requires rank >= 3");
        return tensor(
            data() + indice * sizes_[1],
            static_cast<size_type>(dimensions_[1]),
            static_cast<size_type>(dimensions_[2]));
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor get_matrix(const dimensions_type& indices) const
    {
        const size_t n = dimensions_.size();
        VECTORIZATION_CHECK(indices.size() + 2 == n, "index count must be rank - 2");
        if (n == 3)
        {
            return tensor(
                data() + indices[0] * sizes_[1],
                static_cast<size_type>(dimensions_[1]),
                static_cast<size_type>(dimensions_[2]));
        }
        return tensor(
            data() + linearized_index(indices),
            static_cast<size_type>(dimensions_[n - 2]),
            static_cast<size_type>(dimensions_[n - 1]));
    }

    // -----------------------------------------------------------------------
    // Matrix operations (2-D)
    // -----------------------------------------------------------------------

    // Called by matrix_multiplication_expression::evaluate() with *this as output
    VECTORIZATION_FUNCTION_ATTRIBUTE void matrix_multiplication(
        bool          trA,
        bool          trB,
        tensor const& A,
        tensor const& B) noexcept
    {
        const size_t colsA = A.rank() >= 2 ? A.columns() : 1;
        const size_t colsB = B.rank() >= 2 ? B.columns() : 1;
        const size_t ldA   = trA ? 1 : colsA;
        const size_t ldB   = trB ? 1 : colsB;

        vectorization::matrix_multiplication(
            trA, trB,
            rows(), columns(),
            trA ? A.rows() : colsA,
            A.data(), ldA,
            B.data(), ldB,
            data(), columns());
    }

    // Called by matrix_transpose_expression::evaluate()
    VECTORIZATION_FUNCTION_ATTRIBUTE void matrix_transpose(tensor const& A)
    {
        deepcopy(A);
        vectorization::matrix_transpose(A.rows(), A.columns(), data());
        if (dimensions_.size() >= 2)
        {
            std::swap(dimensions_[0], dimensions_[1]);
            sizes_ = compute_sizes(dimensions_);
        }
    }

    friend auto transpose(tensor const& rhs)
    {
        return matrix_transpose_expression<tensor>(rhs);
    }

    // -----------------------------------------------------------------------
    // Comparison / predicates
    // -----------------------------------------------------------------------

    bool operator==(tensor const& rhs) const noexcept
    {
        if (dimensions_ != rhs.dimensions_) return false;
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] != rhs.data()[i]) return false;
        return true;
    }
    bool operator!=(tensor const& rhs) const noexcept { return !(*this == rhs); }

    bool is_zero() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (!is_almost_zero(data()[i])) return false;
        return true;
    }

    bool non_negative() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] < -std::numeric_limits<value_t>::epsilon()) return false;
        return true;
    }

    bool positive() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] < std::numeric_limits<value_t>::epsilon()) return false;
        return true;
    }

    bool symmetric() const
    {
        if (rank() < 2 || rows() != columns()) return false;
        for (size_t i = 0; i < rows(); ++i)
            for (size_t j = 0; j < i; ++j)
                if (!is_almost_zero(at(i, j) - at(j, i))) return false;
        return true;
    }

    bool identity() const
    {
        if (rank() < 2 || rows() != columns()) return false;
        for (size_t i = 0; i < rows(); ++i)
            for (size_t j = 0; j < columns(); ++j)
                if (at(i, j) != static_cast<value_t>(i == j)) return false;
        return true;
    }

    bool is_correlation() const
    {
        if (!symmetric()) return false;
        for (size_t i = 0; i < rows(); ++i)
        {
            if (!is_almost_zero(at(i, i) - value_t(1))) return false;
            for (size_t j = 0; j < i; ++j)
                if (std::fabs(at(i, j)) > value_t(1)) return false;
        }
        return true;
    }

    value_t trace() const
    {
        VECTORIZATION_CHECK_DEBUG(rank() >= 2 && rows() == columns(), "trace requires square rank-2 tensor");
        value_t t = 0;
        for (size_t i = 0; i < columns(); ++i) t += at(i, i);
        return t;
    }

    // -----------------------------------------------------------------------
    // Formatting
    // -----------------------------------------------------------------------

    std::string to_string() const
    {
        if (empty()) return "[]";

        // Compute field width for alignment
        size_t width = 0;
        for (size_t i = 0; i < size(); ++i)
        {
            std::ostringstream tmp;
            tmp << data()[i];
            width = std::max(width, tmp.str().size());
        }

        std::ostringstream s;
        if (rank() <= 1)
        {
            // 1-D: [v0, v1, ...]
            s << "[";
            for (size_t i = 0; i < size(); ++i)
            {
                if (i) s << ",\n ";
                if (width) s.width(static_cast<std::streamsize>(width));
                s << data()[i];
            }
            s << "]";
        }
        else
        {
            // 2-D (and higher — print as matrix of last two dims)
            const size_t r = rows(), c = columns();
            s << "[";
            for (size_t i = 0; i < r; ++i)
            {
                if (i) s << ";\n ";
                if (width) s.width(static_cast<std::streamsize>(width));
                s << at(i, 0);
                for (size_t j = 1; j < c; ++j)
                {
                    s << ", ";
                    if (width) s.width(static_cast<std::streamsize>(width));
                    s << at(i, j);
                }
            }
            s << "]";
        }
        return s.str();
    }

private:
    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    static dimensions_type compute_sizes(const dimensions_type& dims)
    {
        if (dims.empty()) return {};
        const size_t n = dims.size();
        dimensions_type ret(n);
        size_t prod = dims.back();
        ret.back()  = prod;
        for (int i = static_cast<int>(n) - 2; i >= 0; --i)
        {
            prod  *= dims[i];
            ret[i] = prod;
        }
        return ret;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t linearized_index(const dimensions_type& indices) const
    {
        const size_t n = dimensions_.size();
        const size_t m = indices.size();
        VECTORIZATION_CHECK(
            m <= n, "number of indices exceeds tensor rank");

        if (n == m)
        {
            size_t ret = indices.back();
            for (int i = static_cast<int>(n) - 2; i >= 0; --i)
                ret += sizes_[i + 1] * indices[i];
            return ret;
        }
        else
        {
            size_t ret = 0;
            for (size_t i = 0; i < m; ++i)
                ret += sizes_[i + 1] * indices[i];
            return ret;
        }
    }

    dimensions_type dimensions_;
    dimensions_type sizes_;
    data_t          storage_{};
};

// ---------------------------------------------------------------------------
// Stream output
// ---------------------------------------------------------------------------
template <typename T>
std::ostream& operator<<(std::ostream& s, const tensor<T>& t)
{
    s << t.to_string();
    return s;
}

}  // namespace vectorization

#if defined(_MSC_VER) && !defined(VECTORIZATION_DISPLAY_WIN32_WARNINGS)
#pragma warning(pop)
#endif
