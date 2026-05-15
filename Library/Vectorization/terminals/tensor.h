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
#include <cstdint>
#include <limits>
#include <numeric>
#include <sstream>
#include <vector>

#include "common/packet.h"
#include "expressions/expressions.h"
#include "sizes_and_strides.h"

namespace vectorization
{

template <typename T>
inline constexpr bool is_almost_zero(T x, T epsilon = std::numeric_limits<T>::epsilon()) noexcept
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
template <typename value_t, bool deep_copy>
class tensor
{
public:
    using value_type                          = value_t;
    using size_type                           = std::size_t;
    static constexpr size_type alignment      = VECTORIZATION_ALIGNMENT;
    static constexpr size_type scalar_size    = sizeof(value_type);
    static constexpr size_type alignment_size = alignment / scalar_size;
    static constexpr size_type alignment_mask = alignment_size - 1;
    using dimensions_type                     = std::vector<int64_t>;
    using evaluator                           = expressions_evaluator;
    using packet_t                            = packet<value_t, packet_size<value_t>::value>;
    using array_simd_t                        = typename packet_t::array_simd_t;
    using simd_t                              = typename simd<value_t>::simd_t;
    using data_t                              = data_ptr<value_t, deep_copy>;
    using iterator                            = value_t*;
    using const_iterator                      = const value_t*;
    using reverse_iterator                    = std::reverse_iterator<iterator>;
    using const_reverse_iterator              = std::reverse_iterator<const_iterator>;

    VECTORIZATION_FORCE_INLINE static size_type first_aligned(const value_t* array, size_type size)
    {
        if constexpr (alignment_size <= 1)
        {
            return 0;
        }
        if constexpr (alignment > alignof(simd_t))
        {
            return 0;
        }
        if constexpr ((alignment % scalar_size) != 0)
        {
            return size;
        }

        if (reinterpret_cast<std::uintptr_t>(array) & (scalar_size - 1))
        {
            return size;
        }

        size_type first = (alignment_size - (reinterpret_cast<std::uintptr_t>(array) / scalar_size &
                                             alignment_mask)) &
                          alignment_mask;
        return (first < size) ? first : size;
    }

    VECTORIZATION_FORCE_INLINE static size_type last_aligned(
        size_type aligned_start, size_type size, size_type packet_size)
    {
        return aligned_start + ((size - aligned_start) / packet_size) * packet_size;
    }

    // SIMD stride — identical for every rank; used by expression_loader
    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length() noexcept
    {
        return packet_t::length();
    }

    /// First element index where CPU SIMD lanes are memory-aligned (scalar prologue is \c [0, align_start) ).
    VECTORIZATION_FUNCTION_ATTRIBUTE std::size_t align_start() const noexcept
    {
        return align_start_;
    }

    /// Exclusive end index: for \c i in <tt>[align_start, align_end)</tt> at stride \ref length(), use \c load / \c store.
    VECTORIZATION_FUNCTION_ATTRIBUTE std::size_t align_end() const noexcept { return align_end_; }

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor() noexcept { recompute_cpu_simd_alignment_state(); }

    // --- 1-D constructors (vector-like) ------------------------------------

    VECTORIZATION_CUDA_FUNCTION_TYPE explicit tensor(
        size_type n, device_enum type = device_enum::CPU) noexcept
        : storage_(n, type)
    {
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(n);
        sizes_and_strides_.stride_at_unchecked(0) = 1;
        recompute_cpu_simd_alignment_state();
    }

    // 1D view constructor — wraps an existing contiguous buffer without owning it
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t* ptr, size_type n, device_enum type = device_enum::CPU) noexcept
        : storage_(ptr, n, type)
    {
        static_assert(!deep_copy, "1D view constructor requires tensor<T, false>");
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(n);
        sizes_and_strides_.stride_at_unchecked(0) = 1;
        recompute_cpu_simd_alignment_state();
    }

    // 2D view constructor — wraps an existing contiguous buffer as a rows×cols matrix
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t* ptr, size_type rows, size_type cols, device_enum type = device_enum::CPU) noexcept
        : storage_(ptr, rows * cols, type)
    {
        static_assert(!deep_copy, "2D view constructor requires tensor<T, false>");
        sizes_and_strides_.resize(2);
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(rows);
        sizes_and_strides_.size_at_unchecked(1)   = static_cast<int64_t>(cols);
        sizes_and_strides_.stride_at_unchecked(0) = static_cast<int64_t>(cols);
        sizes_and_strides_.stride_at_unchecked(1) = 1;
        recompute_cpu_simd_alignment_state();
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        value_t start, value_t end, size_type n, device_enum type = device_enum::CPU) noexcept
        : tensor(n, type)
    {
        const auto dx = (end - start) / static_cast<value_t>(n - 1);
        for (size_t i = 0; i < n; ++i)
            data()[i] = static_cast<value_t>(i) * dx + start;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        std::initializer_list<value_t> list, device_enum type = device_enum::CPU) noexcept
        : tensor(list.size(), type)
    {
        std::copy(list.begin(), list.end(), data());
    }

    // --- 2-D constructors (matrix-like) ------------------------------------

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        size_type rows, size_type cols, device_enum type = device_enum::CPU) noexcept
        : storage_(rows * cols, type)
    {
        sizes_and_strides_.resize(2);
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(rows);
        sizes_and_strides_.size_at_unchecked(1)   = static_cast<int64_t>(cols);
        sizes_and_strides_.stride_at_unchecked(0) = static_cast<int64_t>(cols);
        sizes_and_strides_.stride_at_unchecked(1) = 1;
        recompute_cpu_simd_alignment_state();
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE tensor(
        std::initializer_list<std::initializer_list<value_t>> list,
        device_enum                                           type = device_enum::CPU)
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
        : storage_(compute_total(dims), type)
    {
        set_shape(dims);
        recompute_cpu_simd_alignment_state();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        dimensions_type&& dims, device_enum type = device_enum::CPU)
        : storage_(compute_total(dims), type)
    {
        set_shape(dims);
        recompute_cpu_simd_alignment_state();
    }

    // Wrap external memory. clone=false: non-owning view. clone=true: copies data.
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t* data, const dimensions_type& dims, device_enum type = device_enum::CPU)
        : storage_(data, compute_total(dims), type)
    {
        set_shape(dims);
        recompute_cpu_simd_alignment_state();
    }

    // -----------------------------------------------------------------------
    // Copy / move
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(tensor const& rhs)
        : sizes_and_strides_(rhs.sizes_and_strides_),
          storage_(rhs.storage_),
          misalign_(rhs.misalign_),
          align_start_(rhs.align_start_),
          align_end_(rhs.align_end_)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(tensor&& rhs) noexcept
        : sizes_and_strides_(std::move(rhs.sizes_and_strides_)),
          storage_(std::move(rhs.storage_)),
          misalign_(rhs.misalign_),
          align_start_(rhs.align_start_),
          align_end_(rhs.align_end_)
    {
        rhs.misalign_ = rhs.align_start_ = rhs.align_end_ = 0;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(tensor const& rhs)
    {
        if (this != &rhs)
        {
            sizes_and_strides_ = rhs.sizes_and_strides_;
            storage_           = rhs.storage_;
            misalign_          = rhs.misalign_;
            align_start_       = rhs.align_start_;
            align_end_         = rhs.align_end_;
        }
        return *this;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(tensor&& rhs) noexcept
    {
        if (this != &rhs)
        {
            sizes_and_strides_ = std::move(rhs.sizes_and_strides_);
            storage_           = std::move(rhs.storage_);
            misalign_          = rhs.misalign_;
            align_start_       = rhs.align_start_;
            align_end_         = rhs.align_end_;
            rhs.misalign_ = rhs.align_start_ = rhs.align_end_ = 0;
        }
        return *this;
    }

    // Returns a new tensor that is a deep, contiguous copy of *this.
    // Non-contiguous sources (transpose views, strided slices, …) have their
    // logical elements gathered in C-order so the result is always contiguous.
    // Mirrors torch::Tensor::clone() semantics.
    VECTORIZATION_CUDA_FUNCTION_TYPE tensor clone() const noexcept
    {
        const size_t total = size();
        const size_t n     = rank();

        // Allocate fresh 1-D storage, then reshape below.
        tensor result(total, storage_.type_);

        if (is_contiguous())
        {
            std::copy_n(data(), total, result.data());
        }
        else
        {
#if !VECTORIZATION_ON_GPU_DEVICE
            const value_t*  src = data();
            value_t*        dst = result.data();
            dimensions_type idx(n, 0);
            for (size_t flat = 0; flat < total; ++flat)
            {
                size_t off = 0;
                for (size_t k = 0; k < n; ++k)
                {
                    off += static_cast<size_t>(idx[k]) *
                           static_cast<size_t>(sizes_and_strides_.stride_at_unchecked(k));
                }
                dst[flat] = src[off];
                for (int k = static_cast<int>(n) - 1; k >= 0; --k)
                {
                    if (++idx[k] < sizes_and_strides_.size_at_unchecked(k))
                        break;
                    idx[k] = 0;
                }
            }
#else
            // Non-contiguous GPU tensor clone falls back to flat copy.
            std::copy_n(data(), total, result.data());
#endif
        }

        // Stamp the shape onto the result with contiguous C-order strides.
        result.sizes_and_strides_.resize(n);
        for (size_t i = 0; i < n; ++i)
        {
            result.sizes_and_strides_.size_at_unchecked(i) =
                sizes_and_strides_.size_at_unchecked(i);
        }
        if (n > 0)
        {
            result.sizes_and_strides_.stride_at_unchecked(n - 1) = 1;
            for (int i = static_cast<int>(n) - 2; i >= 0; --i)
                result.sizes_and_strides_.stride_at_unchecked(i) =
                    result.sizes_and_strides_.stride_at_unchecked(i + 1) *
                    result.sizes_and_strides_.size_at_unchecked(i + 1);
        }
        result.recompute_cpu_simd_alignment_state();
        return result;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE ~tensor() = default;

    // -----------------------------------------------------------------------
    // Expression constructors / assignment
    // -----------------------------------------------------------------------

    template <
        typename E,
        std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E const& expr) : storage_(expr.size(), device_enum::CPU)
    {
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(expr.size());
        sizes_and_strides_.stride_at_unchecked(0) = 1;
        recompute_cpu_simd_alignment_state();
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <
        typename E,
        std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E&& expr)  // NOLINT
        : storage_(expr.size(), device_enum::CPU)
    {
        sizes_and_strides_.size_at_unchecked(0)   = static_cast<int64_t>(expr.size());
        sizes_and_strides_.stride_at_unchecked(0) = 1;
        recompute_cpu_simd_alignment_state();
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <
        typename E,
        std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E const& expr)
    {
        evaluator::template run<E, tensor>(expr, *this);
        return *this;
    }

    template <
        typename E,
        std::enable_if_t<vectorization::is_pure_expression<E>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E&& expr)
    {
        evaluator::template run<E, tensor>(static_cast<E const&>(expr), *this);
        return *this;
    }

    template <typename T2, std::enable_if_t<std::is_fundamental<T2>::value, bool> = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(T2 value) noexcept
    {
        evaluator::template fill<value_t, tensor>(static_cast<value_t>(value), *this);
        return *this;
    }

    // -----------------------------------------------------------------------
    // Raw data / size
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t* data() const noexcept
    {
        return storage_.data();
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t* data() noexcept { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t   size() const noexcept
    {
        return size_ == 0 ? storage_.size() : size_;
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE bool empty() const noexcept { return size() == 0; }

    VECTORIZATION_FUNCTION_ATTRIBUTE iterator       begin() noexcept { return storage_.begin(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE iterator       end() noexcept { return storage_.end(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator begin() const noexcept
    {
        return storage_.begin();
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator end() const noexcept { return storage_.end(); }

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

    // -----------------------------------------------------------------------
    // Shape / rank
    // -----------------------------------------------------------------------

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t rank() const noexcept
    {
        return sizes_and_strides_.size();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE std::span<const int64_t> dimensions() const noexcept
    {
        return sizes_and_strides_.sizes_arrayref();
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t dimension(size_t n) const noexcept
    {
        return static_cast<size_t>(sizes_and_strides_.size_at_unchecked(n));
    }

    // Per-dimension size and stride
    VECTORIZATION_FUNCTION_ATTRIBUTE int64_t size(size_t dim) const noexcept
    {
        return sizes_and_strides_.size_at_unchecked(dim);
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE int64_t stride(size_t dim) const noexcept
    {
        return sizes_and_strides_.stride_at_unchecked(dim);
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE std::span<const int64_t> strides() const noexcept
    {
        return sizes_and_strides_.strides_arrayref();
    }

    // C-order contiguity check: strides[n-1]==1 and strides[i]==strides[i+1]*sizes[i+1].
    VECTORIZATION_FUNCTION_ATTRIBUTE bool is_contiguous() const noexcept
    {
        const size_t n = rank();
        if (n == 0)
        {
            return true;
        }
        if (sizes_and_strides_.stride_at_unchecked(n - 1) != 1)
        {
            return false;
        }
        for (int i = static_cast<int>(n) - 2; i >= 0; --i)
        {
            if (sizes_and_strides_.stride_at_unchecked(i) !=
                sizes_and_strides_.stride_at_unchecked(i + 1) *
                    sizes_and_strides_.size_at_unchecked(i + 1))
            {
                return false;
            }
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Element access
    // -----------------------------------------------------------------------

    // Flat indexed access (1-D semantic, element by element)
    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t& operator[](size_type i) const noexcept
    {
        return data()[i];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& operator[](size_type i) noexcept { return data()[i]; }

    // 1-D scalar element
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t  at(size_type i) const noexcept { return data()[i]; }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(size_type i) noexcept { return data()[i]; }

    // 2-D scalar element (row-major). Not noexcept: debug bounds checks may throw (like std::vector::at).
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t at(size_type i, size_type j) const
    {
        VECTORIZATION_CHECK_DEBUG(i < dimension(0), "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < dimension(1), "column index out of range");
        return data()[i * dimension(1) + j];
    }
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(size_type i, size_type j)
    {
        VECTORIZATION_CHECK_DEBUG(i < dimension(0), "row index out of range");
        VECTORIZATION_CHECK_DEBUG(j < dimension(1), "column index out of range");
        return data()[i * dimension(1) + j];
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

    // -----------------------------------------------------------------------
    // Views — metadata-only transforms (no data copy)
    //
    // All return tensor<value_t, false> — a non-owning view sharing the same
    // backing buffer.  The caller must ensure the source outlives the view.
    // -----------------------------------------------------------------------

    // Transpose (rank-2): swap the two axes and their strides.
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor<value_t, false> t() const
    {
        VECTORIZATION_CHECK_DEBUG(rank() == 2, "t() requires a rank-2 tensor");
        sizes_and_strides sas = sizes_and_strides_;
        std::swap(sas.size_at_unchecked(0), sas.size_at_unchecked(1));
        std::swap(sas.stride_at_unchecked(0), sas.stride_at_unchecked(1));
        return tensor<value_t, false>(
            const_cast<value_t*>(data()), storage_.size(), sas, storage_.type_);
    }

    // Permute: reorder all axes by the given index list (no repeats, length == rank).
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor<value_t, false> permute(
        const dimensions_type& order) const
    {
        const size_t n = rank();
        VECTORIZATION_CHECK_DEBUG(order.size() == n, "permute: order length must equal rank");
        sizes_and_strides sas;
        sas.resize(n);
        for (size_t i = 0; i < n; ++i)
        {
            const size_t src           = static_cast<size_t>(order[i]);
            sas.size_at_unchecked(i)   = sizes_and_strides_.size_at_unchecked(src);
            sas.stride_at_unchecked(i) = sizes_and_strides_.stride_at_unchecked(src);
        }
        return tensor<value_t, false>(
            const_cast<value_t*>(data()), storage_.size(), sas, storage_.type_);
    }

    // View: reinterpret shape with new contiguous strides.  Total element count
    // must be preserved and the source tensor must be contiguous.
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor<value_t, false> view(
        const dimensions_type& new_dims) const
    {
        VECTORIZATION_CHECK_DEBUG(is_contiguous(), "view() requires a contiguous tensor");
        VECTORIZATION_CHECK_DEBUG(
            compute_total(new_dims) == size(), "view: element count must not change");
        sizes_and_strides sas;
        make_contiguous_sas(sas, new_dims);
        return tensor<value_t, false>(
            const_cast<value_t*>(data()), storage_.size(), sas, storage_.type_);
    }

    // Reshape: same as view (contiguous source required; for non-contiguous call
    // contiguous() first, then reshape).
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor<value_t, false> reshape(
        const dimensions_type& new_dims) const
    {
        VECTORIZATION_CHECK_DEBUG(
            is_contiguous(), "reshape: call contiguous() first for non-contiguous tensors");
        VECTORIZATION_CHECK_DEBUG(
            compute_total(new_dims) == size(), "reshape: element count must not change");
        sizes_and_strides sas;
        make_contiguous_sas(sas, new_dims);
        return tensor<value_t, false>(
            const_cast<value_t*>(data()), storage_.size(), sas, storage_.type_);
    }

    // Slice along one dimension.  stop==-1 means "to end".  step may be negative
    // only when start > stop (reverse slice); step==0 is invalid.
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor
    slice(size_t dim, int64_t start, int64_t stop = -1, int64_t step = 1) const
    {
        VECTORIZATION_CHECK_DEBUG(dim < rank(), "slice: dim out of range");
        VECTORIZATION_CHECK_DEBUG(step != 0, "slice: step must not be zero");
        const int64_t dim_size = sizes_and_strides_.size_at_unchecked(dim);
        if (stop < 0)
        {
            stop = dim_size;
        }
        stop                         = std::min(stop, dim_size);
        start                        = std::min(start, dim_size);
        const int64_t     span       = stop - start;
        auto              new_size   = (span > 0 && step > 0) || (span < 0 && step < 0)
                                           ? (std::abs(span) + std::abs(step) - 1) / std::abs(step)
                                           : 0;
        sizes_and_strides sas        = sizes_and_strides_;
        sas.size_at_unchecked(dim)   = new_size;
        sas.stride_at_unchecked(dim) = sizes_and_strides_.stride_at_unchecked(dim) * step;
        const size_t offset          = static_cast<size_t>(start) *
                              static_cast<size_t>(sizes_and_strides_.stride_at_unchecked(dim));
        value_t* new_ptr = const_cast<value_t*>(data()) + offset;
        // Storage accessible from new_ptr is the original buffer minus the prefix we skipped.
        const size_t storage_size = storage_.size() > offset ? storage_.size() - offset : 0;

        auto t = tensor(new_ptr, storage_size, sas, storage_.type_);

        t.size_ = new_size;
        return t;
    }

#if 0
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
        const size_t colsA = A.rank() >= 2 ? A.dimension(1) : 1;
        const size_t colsB = B.rank() >= 2 ? B.dimension(1) : 1;
        const size_t ldA   = trA ? 1 : colsA;
        const size_t ldB   = trB ? 1 : colsB;

        vectorization::matrix_multiplication(
            trA, trB,
            dimension(0), dimension(1),
            trA ? A.dimension(0) : colsA,
            A.data(), ldA,
            B.data(), ldB,
            data(), dimension(1));
    }

    // Called by matrix_transpose_expression::evaluate()
    VECTORIZATION_FUNCTION_ATTRIBUTE void matrix_transpose(tensor const& A)
    {
        clone(A);
        vectorization::matrix_transpose(A.dimension(0), A.dimension(1), data());
        if (sizes_and_strides_.size() >= 2)
        {
            // Swap shape dims 0 and 1, then recompute strides.
            const int64_t tmp = sizes_and_strides_.size_at_unchecked(0);
            sizes_and_strides_.size_at_unchecked(0) = sizes_and_strides_.size_at_unchecked(1);
            sizes_and_strides_.size_at_unchecked(1) = tmp;
            dimensions_type dims(sizes_and_strides_.sizes_begin(), sizes_and_strides_.sizes_end());
            set_shape(dims);
        }
    }

    friend auto transpose(tensor const& rhs)
    {
        return matrix_transpose_expression<tensor>(rhs);
    }
#endif

    // -----------------------------------------------------------------------
    // Comparison / predicates
    // -----------------------------------------------------------------------

    bool operator==(tensor const& rhs) const noexcept
    {
        if (sizes_and_strides_ != rhs.sizes_and_strides_)
            return false;
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] != rhs.data()[i])
                return false;
        return true;
    }

    bool operator!=(tensor const& rhs) const noexcept { return !(*this == rhs); }

    bool is_zero() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (!is_almost_zero(data()[i]))
                return false;
        return true;
    }

    bool non_negative() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] < -std::numeric_limits<value_t>::epsilon())
                return false;
        return true;
    }

    bool positive() const noexcept
    {
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] < std::numeric_limits<value_t>::epsilon())
                return false;
        return true;
    }

    bool symmetric() const
    {
        if (rank() < 2 || dimension(0) != dimension(1))
            return false;
        const size_t   r = dimension(0), c = dimension(1);
        const value_t* p = data();
        for (size_t i = 0; i < r; ++i)
            for (size_t j = 0; j < i; ++j)
                if (!is_almost_zero(p[i * c + j] - p[j * c + i]))
                    return false;
        return true;
    }

    bool identity() const
    {
        if (rank() < 2 || dimension(0) != dimension(1))
            return false;
        const size_t   r = dimension(0), c = dimension(1);
        const value_t* p = data();
        for (size_t i = 0; i < r; ++i)
            for (size_t j = 0; j < c; ++j)
                if (p[i * c + j] != static_cast<value_t>(i == j))
                    return false;
        return true;
    }

    bool is_correlation() const
    {
        if (rank() < 2 || dimension(0) != dimension(1))
            return false;
        const size_t   r = dimension(0), c = dimension(1);
        const value_t* p = data();
        for (size_t i = 0; i < r; ++i)
        {
            if (!is_almost_zero(p[i * c + i] - value_t(1)))
                return false;
            for (size_t j = 0; j < i; ++j)
            {
                const value_t v = p[i * c + j];
                if (std::fabs(v) > value_t(1) || !is_almost_zero(v - p[j * c + i]))
                    return false;
            }
        }
        return true;
    }

    value_t trace() const
    {
        VECTORIZATION_CHECK_DEBUG(
            rank() >= 2 && dimension(0) == dimension(1), "trace requires square rank-2 tensor");
        const size_t   c = dimension(1);
        const value_t* p = data();
        value_t        t = 0;
        for (size_t i = 0; i < c; ++i)
            t += p[i * c + i];
        return t;
    }

    // -----------------------------------------------------------------------
    // Formatting
    // -----------------------------------------------------------------------

    std::string to_string() const
    {
        if (empty())
            return "[]";

        // Compute field width for alignment — reuse a single stream to avoid per-element alloc.
        size_t width = 0;
        {
            std::ostringstream tmp;
            for (size_t i = 0; i < size(); ++i)
            {
                tmp.str(std::string{});
                tmp << data()[i];
                width = std::max(width, tmp.str().size());
            }
        }

        std::ostringstream s;
        if (rank() <= 1)
        {
            // 1-D: [v0, v1, ...]
            s << "[";
            for (size_t i = 0; i < size(); ++i)
            {
                if (i)
                    s << ",\n ";
                if (width)
                    s.width(static_cast<std::streamsize>(width));
                s << data()[i];
            }
            s << "]";
        }
        else
        {
            // 2-D (and higher — print as matrix of last two dims)
            const size_t r = dimension(0), c = dimension(1);
            s << "[";
            for (size_t i = 0; i < r; ++i)
            {
                if (i)
                    s << ";\n ";
                if (width)
                    s.width(static_cast<std::streamsize>(width));
                s << at(i, 0);
                for (size_t j = 1; j < c; ++j)
                {
                    s << ", ";
                    if (width)
                        s.width(static_cast<std::streamsize>(width));
                    s << at(i, j);
                }
            }
            s << "]";
        }
        return s.str();
    }

private:
    template <typename V, bool c>
    friend class tensor;

    // View constructor — creates a non-owning tensor over an existing buffer
    // with explicit shape/strides.  Only valid for clone=false instantiations.
    tensor(
        value_t*          ptr,
        size_t            storage_size,
        sizes_and_strides sas,
        device_enum       type = device_enum::CPU)
        : sizes_and_strides_(std::move(sas)), storage_(ptr, storage_size, type)
    {
        static_assert(!deep_copy, "view constructor is only valid for tensor<T, false>");
        recompute_cpu_simd_alignment_state();
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Populate sizes_and_strides_ with the given shape and derived C-order strides.
    void set_shape(const dimensions_type& dims) { make_contiguous_sas(sizes_and_strides_, dims); }

    static size_type compute_total(const dimensions_type& dims) noexcept
    {
        size_type total = 1;
        for (auto d : dims)
            total *= static_cast<size_type>(d);
        return total;
    }

    // Build a sizes_and_strides with the given shape and C-order strides.
    static void make_contiguous_sas(sizes_and_strides& sas, const dimensions_type& dims)
    {
        const size_t n = dims.size();
        sas.resize(n);
        if (n == 0)
            return;
        sas.stride_at_unchecked(n - 1) = 1;
        for (int i = static_cast<int>(n) - 2; i >= 0; --i)
            sas.stride_at_unchecked(i) = sas.stride_at_unchecked(i + 1) * dims[i + 1];
        for (size_t i = 0; i < n; ++i)
            sas.size_at_unchecked(i) = dims[i];
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t linearized_index(const dimensions_type& indices) const
    {
        const size_t n = sizes_and_strides_.size();
        const size_t m = indices.size();
        VECTORIZATION_CHECK_DEBUG(m <= n, "number of indices exceeds tensor rank");

        size_t ret = 0;
        for (size_t i = 0; i < m; ++i)
            ret += static_cast<size_t>(sizes_and_strides_.stride_at_unchecked(i)) *
                   static_cast<size_t>(indices[i]);
        return ret;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE void recompute_cpu_simd_alignment_state() noexcept
    {
#if !VECTORIZATION_VECTORIZED || VECTORIZATION_ON_GPU_DEVICE
        misalign_    = 0;
        align_start_ = 0;
        align_end_   = 0;
#else
        if (storage_.type_ != device_enum::CPU || storage_.size() == 0 || !is_contiguous())
        {
            misalign_    = 0;
            align_start_ = 0;
            align_end_   = 0;
            return;
        }
        const size_t   n         = storage_.size();
        value_t const* base      = storage_.data();
        const auto     data_addr = reinterpret_cast<std::uintptr_t>(base);

        static_assert(
            (scalar_size & (scalar_size - 1)) == 0, "SIMD alignment must be a power of two");
        misalign_ = static_cast<std::size_t>(data_addr % alignment);

        align_start_ = first_aligned(base, n);
        align_end_   = last_aligned(align_start_, n, packet_t::length());
#endif
    }

    std::size_t misalign_{0};
    std::size_t align_start_{0};
    /// Exclusive end index for the aligned SIMD body (matches \c expressions_evaluator::run peeling).
    std::size_t align_end_{0};

    sizes_and_strides sizes_and_strides_;
    size_t            size_ = 0;
    data_t            storage_{};
};

template <typename V>
VECTORIZATION_FUNCTION_ATTRIBUTE bool expr_cpu_simd_lane_aligned_at(
    tensor<V> const& t, std::size_t index) noexcept
{
    return t.cpu_simd_lane_aligned_at(index);
}

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
