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
#include <vector>

#include "common/packet.h"
#include "expressions/expressions.h"
#include "memory/allocator.h"
#include "memory/data_ptr.h"
#include "terminals/matrix.h"

namespace quarisma
{
template <typename value_t>
class tensor
{
public:
    using value_type      = value_t;
    using size_type       = size_t;
    using packet_t        = packet<value_t, packet_size<value_t>::value>;
    using evaluator       = expressions_evaluator;
    using dimensions_type = std::vector<size_t>;
    using array_simd_t    = typename packet_t::array_simd_t;
    using data_t          = data_ptr<value_t, false>;

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor() = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t*               data,
        const dimensions_type& dimensions,
        quarisma::device_enum    type = quarisma::device_enum::CPU)
        : dimensions_(dimensions),
          sizes_(accumulate_array(dimensions)),
          storage_(data, sizes_.front(), type)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        value_t*            data,
        dimensions_type&&   dimensions,
        quarisma::device_enum type = quarisma::device_enum::CPU)
        : dimensions_(std::move(dimensions)),
          sizes_(accumulate_array(dimensions_)),
          storage_(data, sizes_.front(), type)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        void*                  data,
        const dimensions_type& dimensions,
        quarisma::device_enum    type = quarisma::device_enum::CPU)
        : tensor((value_t*)data, dimensions, type)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        const value_t*         data,
        const dimensions_type& dimensions,
        quarisma::device_enum    type = quarisma::device_enum::CPU)
        : tensor(const_cast<value_t*>(data), dimensions, type)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(
        const dimensions_type& v, quarisma::device_enum type = quarisma::device_enum::CPU)  // NOLINT
        : dimensions_(v), sizes_(accumulate_array(dimensions_)), storage_(sizes_.front(), type)
    {
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto operator[](size_type idx) const noexcept
    {
        if (sizes_.size() == 1)
            return *this;

        dimensions_type tmp(dimensions_.begin() + 1, dimensions_.end());
        return tensor(data() + idx * sizes_[1], tmp);
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto operator[](size_type idx) noexcept
    {
        if (sizes_.size() == 1)
            return *this;

        dimensions_type tmp(dimensions_.begin() + 1, dimensions_.end());
        return tensor(data() + idx * sizes_[1], tmp);
    };

    VECTORIZATION_CUDA_FUNCTION_TYPE void deepcopy(tensor const& rhs) noexcept
    {
        dimensions_ = rhs.dimensions_;
        sizes_      = rhs.sizes_;

        storage_.copy(rhs.storage_);
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE ~tensor() = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(const tensor& rhs)
        : dimensions_(rhs.dimensions_), sizes_(rhs.sizes_), storage_(rhs.storage_) {};

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(const tensor& rhs)
    {
        storage_    = rhs.storage_;
        dimensions_ = rhs.dimensions_;
        sizes_      = rhs.sizes_;

        return *this;
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(tensor&& rhs)
        : storage_(std::move(rhs.storage_)),
          dimensions_(std::move(rhs.dimensions_)),
          sizes_(std::move(rhs.sizes_)) {};

    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(tensor&& rhs)
    {
        storage_    = std::move(rhs.storage_);
        dimensions_ = std::move(rhs.dimensions_);
        sizes_      = std::move(rhs.sizes_);

        return *this;
    };

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length() noexcept
    {
        return packet_t::length();
    };

    // Metadata
    VECTORIZATION_FUNCTION_ATTRIBUTE size_t rank() const { return dimensions_.size(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t dimension(std::size_t n) const { return dimensions_[n]; }

    VECTORIZATION_FUNCTION_ATTRIBUTE const dimensions_type& dimensions() const { return dimensions_; }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto* data() const { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto* begin() const { return storage_.begin(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE const auto* end() const { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto* data() { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto* begin() { return storage_.begin(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto* end() { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto size() const { return storage_.size(); }

    // normal indices
    VECTORIZATION_FUNCTION_ATTRIBUTE const value_t& at(const dimensions_type& indices) const
    {
        return data()[linearized_index(indices)];
    }

    // normal indices
    VECTORIZATION_FUNCTION_ATTRIBUTE value_t& at(const dimensions_type& indices)
    {
        return data()[linearized_index(indices)];
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix<value_t> get_matrix(size_t indice) const
    {
        VECTORIZATION_CHECK(3 == dimensions_.size(), "number of idx is defferent from dimension!");

        return matrix<value_t>(
            &data()[indice * sizes_[1]],
            static_cast<size_t>(dimensions_[1]),
            static_cast<size_t>(dimensions_[2]));
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE matrix<value_t> get_matrix(const dimensions_type& indices) const
    {
        auto n = dimensions_.size();
        VECTORIZATION_CHECK(indices.size() + 2 == n, "number of idx is defferent from dimension!");

        if (n == 3)
        {
            return matrix<value_t>(
                &data()[indices[0] * sizes_[1]],
                static_cast<size_t>(dimensions_[1]),
                static_cast<size_t>(dimensions_[2]));
        }
        else
        {
            return matrix<value_t>(
                &data()[linearized_index(indices)], dimensions_[n - 2], dimensions_[n - 1]);
        }
    }

    template <
        typename E,
        typename std::enable_if<quarisma::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E const& expr)
    {
        storage_ = data_t(expr.size(), device_enum::CPU);
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <
        typename E,
        typename std::enable_if<quarisma::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor(E&& expr)  // NOLINT
    {
        storage_ = data_t(expr.size(), device_enum::CPU);
        evaluator::template run<E, tensor>(expr, *this);
    }

    template <
        typename E,
        typename std::enable_if<quarisma::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E const& expr)
    {
        evaluator::template run<E, tensor>(expr, *this);
        return *this;
    }

    template <
        typename E,
        typename std::enable_if<quarisma::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(E&& expr)
    {
        evaluator::template run<E, tensor>(static_cast<E const&>(expr), *this);
        return *this;
    }

    template <
        typename T2,
        typename std::enable_if<std::is_fundamental<T2>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE tensor& operator=(T2 value) noexcept
    {
        evaluator::template fill<value_t, quarisma::tensor<value_t> >(
            static_cast<value_t>(value), *this);
        return *this;
    }


private:
    static auto accumulate_array(const std::vector<size_t>& dimensions)
    {
        auto                size_tmp = dimensions.size();
        std::vector<size_t> ret(size_tmp);

        size_t prod = dimensions.back();
        ret.back()  = prod;
        for (int i = static_cast<int>(size_tmp) - 2; i >= 0; i--)
        {
            prod *= dimensions[i];
            ret[i] = prod;
        }

        return ret;
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE size_t linearized_index(const dimensions_type& indices) const
    {
        auto n = dimensions_.size();
        auto m = indices.size();

        VECTORIZATION_CHECK(
            indices.size() <= dimensions_.size(), "number of idx is defferent from dimension!");

        if (n == m)
        {
            size_t ret = indices.back();
            for (int i = static_cast<int>(n) - 2; i >= 0; i--)
            {
                ret += sizes_[i + 1] * indices[i];
            }

            return ret;
        }
        else
        {
            size_t ret = 0;
            for (size_t i = 0; i < m; i++)
            {
                ret += sizes_[i + 1] * indices[i];
            }

            return ret;
        }
    }

    dimensions_type dimensions_;
    dimensions_type sizes_;
    data_t          storage_{};
};
}  // namespace quarisma
