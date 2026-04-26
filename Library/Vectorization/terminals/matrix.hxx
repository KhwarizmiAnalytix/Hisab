#ifdef COMPILE_MATRIX_HXX

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <numeric>

//#include "common/constants.h"
#include "matrix_operation/matrix_multiplication.h"
#include "matrix_operation/matrix_transpose.h"
#include "common/vectorization_macros.h"

namespace vectorization
{
//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(size_type rows, size_type columns, vectorization::device_type type) noexcept
    : storage_(rows * columns, type), rows_(rows), columns_(columns){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(
    value_t* data, size_type rows, size_type columns, vectorization::device_type type) noexcept
    : storage_(data, rows * columns, type), rows_(rows), columns_(columns){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(
    void* data, size_type rows, size_type columns, vectorization::device_type type) noexcept
    : matrix((value_t*)data, rows, columns, type){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(
    std::initializer_list<std::initializer_list<value_t>> list, vectorization::device_type type) noexcept
    : storage_((list.begin())->size() * list.size(), type),
      rows_(list.size()),
      columns_((list.begin())->size())
{
    for (size_t i = 0; i < rows_; i++)
        for (size_t j = 0; j < columns_; j++)
            (*this)[i][j] = ((list.begin() + i)->begin())[j];
}

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(
    const value_t* data, size_type rows, size_type columns, vectorization::device_type type) noexcept
    : matrix(const_cast<value_t*>(data), rows, columns, type){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(vector_type const& rhs, bool transpose) noexcept
    : storage_(rhs.storage_),
      rows_(transpose ? 1 : rhs.size()),
      columns_(transpose ? rhs.size() : 1){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(matrix const& rhs) noexcept
    : storage_(rhs.storage_), rows_(rhs.rows_), columns_(rhs.columns_){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>& matrix<value_t>::operator=(matrix const& rhs) noexcept  // NOLINT
{
    if (this != &rhs)
    {
        storage_ = rhs.storage_;
        rows_    = rhs.rows_;
        columns_ = rhs.columns_;
    }
    return *this;
}

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>::matrix(matrix<value_t>&& rhs) noexcept
    : storage_(std::move(rhs.storage_)),
      rows_(std::move(rhs.rows_)),
      columns_(std::move(rhs.columns_)){};

//-----------------------------------------------------------------------------
template <typename value_t>
matrix<value_t>& matrix<value_t>::operator=(matrix<value_t>&& rhs) noexcept
{
    if (this != &rhs)
    {
        storage_ = std::move(rhs.storage_);
        rows_    = std::move(rhs.rows_);
        columns_ = std::move(rhs.columns_);
    }

    return *this;
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ void matrix<value_t>::deepcopy(matrix const& rhs) noexcept
{
    rows_    = rhs.rows_;
    columns_ = rhs.columns_;

    storage_.copy(rhs.storage_);
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ void matrix<value_t>::matrix_multiplication(
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

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::empty() const
{
    return (storage_.size() == 0);
}

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::is_zero() const
{
    for (size_type i = 0; i < size(); ++i)
        if (!vectorization::is_almost_zero(data()[i]))
            return false;

    return true;
}

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::is_correlation() const
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

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::identity() const
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

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::non_negative() const
{
    for (size_type i = 0; i < size(); ++i)
    {
        if (data()[i] < -std::numeric_limits<double>::epsilon())
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::positive() const
{
    for (size_type i = 0; i < size(); ++i)
    {
        if (data()[i] < std::numeric_limits<double>::epsilon())
            return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
template <typename value_t>
bool matrix<value_t>::symmetric() const
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

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ void matrix<value_t>::matrix_transpose(matrix const& A)
{
    this->deepcopy(A);  //fixme!
    vectorization::matrix_transpose(A.rows(), A.columns(), begin());
    std::swap(columns_, rows_);
}
}  // namespace vectorization
#endif  // COMPILE_MATRIX_HXX
