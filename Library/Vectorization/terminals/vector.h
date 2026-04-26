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

#include <functional>
#include <utility>
#include <vector>

#include "common/packet.h"
#include "expressions/expressions.h"

namespace vectorization
{
template <typename value_t>
class vector
{
public:
    using value_type             = value_t;
    using size_type              = size_t;
    using packet_t               = packet<value_t, packet_size<value_t>::value>;
    using evaluator              = expressions_evaluator;
    using array_simd_t           = typename packet_t::array_simd_t;
    using allocator_t            = allocator<value_t>;
    using data_t                 = data_ptr<value_t, false>;
    using iterator               = value_t*;
    using const_iterator         = const value_t*;
    using reverse_iterator       = std::reverse_iterator<value_t*>;
    using const_reverse_iterator = std::reverse_iterator<const value_t*>;

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t dimensions() noexcept { return 1; };

    VECTORIZATION_FUNCTION_ATTRIBUTE static constexpr size_t length() noexcept
    {
        return packet_t::length();
    };

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        void*                 data,
        size_type             length,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : vector((value_t*)data, length, type)
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        size_type length, vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : storage_(length, type)
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        value_t               start,
        value_t               end,
        size_type             length,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : vector(length, type)
    {
        auto dx = (end - start) / (length - 1);

        for (size_t i = 0; i < length; ++i)
            storage_.data_[i] = i * dx + start;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        std::initializer_list<value_t> list,
        vectorization::device_enum          type = vectorization::device_enum::CPU) noexcept
        : vector(list.size(), type)
    {
        const auto& ptr = list.begin();
        allocator_t::copy(ptr, storage_.size_, storage_.data_, type, type);
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        std::vector<value_t> const& rhs,
        vectorization::device_enum       type = vectorization::device_enum::CPU) noexcept
        : vector(const_cast<value_t*>(rhs.data()), rhs.size(), type)
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        value_t*              data,
        size_type             length,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : storage_(data, length, type)
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(
        const value_t*        data,
        size_type             length,
        vectorization::device_enum type = vectorization::device_enum::CPU) noexcept
        : vector(const_cast<value_t*>(data), length, type)
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(vector&& rhs) noexcept
        : storage_(std::move(rhs.storage_))
    {
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector& operator=(vector&& rhs) noexcept
    {
        storage_ = std::move(rhs.storage_);
        return *this;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE vector(vector const& rhs) noexcept : storage_(rhs.storage_) {}

    VECTORIZATION_CUDA_FUNCTION_TYPE vector& operator=(vector const& rhs) noexcept
    {
        storage_ = rhs.storage_;
        return *this;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE void deepcopy(vector const& rhs) noexcept
    {
        storage_.copy(rhs.storage_);
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE ~vector() = default;

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto* data() const { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto        size() const { return storage_.size(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto*    data() { return storage_.data(); }
    VECTORIZATION_FUNCTION_ATTRIBUTE iterator begin() { return storage_.data(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE iterator end() { return storage_.end(); }

    VECTORIZATION_FUNCTION_ATTRIBUTE const_iterator begin() const { return storage_.data(); }

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

    VECTORIZATION_CUDA_FUNCTION_TYPE const value_t* data(size_type i) const noexcept
    {
        return data() + i;
    }

    VECTORIZATION_CUDA_FUNCTION_TYPE value_t* data(size_type i) noexcept { return data() + i; }

    VECTORIZATION_FUNCTION_ATTRIBUTE const auto& operator[](size_type i) const noexcept
    {
        return data()[i];
    }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto& operator[](size_type i) noexcept { return data()[i]; }

    VECTORIZATION_FUNCTION_ATTRIBUTE auto at(size_type i) const noexcept { return data()[i]; };

    VECTORIZATION_FUNCTION_ATTRIBUTE auto rows() const noexcept { return storage_.size_; }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto columns() const noexcept { return 1; }
    VECTORIZATION_FUNCTION_ATTRIBUTE auto empty() const noexcept { return storage_.size_ == 0; }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE vector(E const& expr)
    {
        storage_ = data_t(expr.size(), device_enum::CPU);
        evaluator::template run<E, vector>(expr, *this);
    }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE vector(E&& expr)  // NOLINT
    {
        storage_ = data_t(expr.size(), device_enum::CPU);
        evaluator::template run<E, vector>(expr, *this);
    }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE vector& operator=(E const& expr)
    {
        evaluator::template run<E, vector>(expr, *this);
        return *this;
    }

    template <
        typename E,
        typename std::enable_if<vectorization::is_pure_expression<E>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE vector& operator=(E&& expr)
    {
        evaluator::template run<E, vector>(std::move(expr), *this);
        return *this;
    }

    template <
        typename T2,
        typename std::enable_if<std::is_fundamental<T2>::value, bool>::type = true>
    VECTORIZATION_FUNCTION_ATTRIBUTE vector& operator=(T2 value) noexcept
    {
        evaluator::template fill<value_t, vectorization::vector<value_t>>(
            static_cast<value_t>(value), *this);
        return *this;
    }

    std::string to_string() const
    {
        // compute the largest width
        size_t width = 0;
        for (auto t : *this)
        {
            std::stringstream sstr;
            sstr << t;
            width = std::max<size_t>(width, size_t(sstr.str().length()));
        }

        std::ostringstream s;
        s << "[";
        for (size_t i = 0; i < size(); ++i)
        {
            if (i)
                s << ",\n ";
            if (width)
                s.width(width);
            s << (*this)[i];
        }
        s << "]";
        return s.str();
    }

    vector() = default;

private:
    data_t storage_{};
};

template <typename value_t>
std::ostream& operator<<(std::ostream& s, const vectorization::vector<value_t>& v)
{
    s << v.to_string();
    return s;
}
}  // namespace vectorization