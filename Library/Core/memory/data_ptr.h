#pragma once

#include <cstddef>

#include "common/macros.h"
#include "memory/allocator.h"
#include "memory/device.h"

namespace quarisma
{
template <typename value_t, bool deepcopy>
struct data_ptr
{
    using allocator_t = allocator<value_t>;

    QUARISMA_FORCE_INLINE data_ptr() = default;

    QUARISMA_FORCE_INLINE data_ptr(size_t size, device_enum type)
        : data_(allocator_t::allocate(size, type)), size_(size), type_(type), allocated_(true)
    {
    }

    QUARISMA_FORCE_INLINE data_ptr(value_t* data, size_t size, device_enum type)
        : size_(size), type_(type), allocated_(deepcopy)
    {
        if constexpr (deepcopy)
        {
            data_ = allocator_t::allocate(size, type),
            allocator_t::copy(data, size_, data_, type, type_);
        }
        else
        {
            data_ = data;
        }
    }

    QUARISMA_FORCE_INLINE data_ptr(
        value_t* data, size_t size, device_enum from_type, device_enum to_type)
        : size_(size), type_(to_type), allocated_(deepcopy)
    {
        if constexpr (deepcopy)
        {
            data_ = allocator_t::allocate(size, to_type),
            allocator_t::copy(data, size_, data_, from_type, to_type);
        }
        else
        {
            data_ = data;
        }
    }

    QUARISMA_FORCE_INLINE data_ptr(data_ptr const& rhs)
        : size_(rhs.size_), type_(rhs.type_), allocated_(deepcopy)
    {
        if constexpr (deepcopy)
        {
            data_ = allocator_t::allocate(size_, type_),
            allocator_t::copy(rhs.data_, size_, data_, rhs.type_, type_);
        }
        else
        {
            data_ = rhs.data_;
        }
    };

    QUARISMA_FORCE_INLINE data_ptr& operator=(data_ptr const& rhs)
    {
        size_      = rhs.size_;
        allocated_ = deepcopy;

        if constexpr (deepcopy)
        {
            data_ = allocator_t::allocate(size_, type_),
            allocator_t::copy(rhs.data_, size_, data_, rhs.type_, type_);
        }
        else
        {
            data_ = rhs.data_;
            type_ = rhs.type_;
        }

        return *this;
    }

    QUARISMA_FORCE_INLINE data_ptr(data_ptr&& rhs)
        : data_(std::move(rhs.data_)),
          size_(std::move(rhs.size_)),
          type_(std::move(rhs.type_)),
          allocated_(std::move(rhs.allocated_))
    {
        rhs.data_ = nullptr;
    };

    QUARISMA_FORCE_INLINE data_ptr& operator=(data_ptr&& rhs) noexcept
    {
        data_      = std::move(rhs.data_);
        size_      = std::move(rhs.size_);
        type_      = std::move(rhs.type_);
        allocated_ = std::move(rhs.allocated_);

        rhs.data_ = nullptr;

        return *this;
    }

    QUARISMA_FORCE_INLINE ~data_ptr()
    {
        if (allocated_ && data_ != nullptr)
        {
            allocator_t::free(data_);
            data_ = nullptr;
        }
    }

    QUARISMA_FORCE_INLINE void copy(data_ptr const& rhs)
    {
        if (data_ == nullptr)
        {
            data_      = allocator_t::allocate(rhs.size_, rhs.type_);
            type_      = rhs.type_;
            size_      = rhs.size_;
            allocated_ = true;
        }
        if (data_ != rhs.data_)
        {
            allocator_t::copy(rhs.data_, rhs.size_, data_, rhs.type_, type_);
        }
    }

    QUARISMA_FORCE_INLINE const value_t* data() const { return data_; }
    QUARISMA_FORCE_INLINE const value_t* get() const { return data_; }
    QUARISMA_FORCE_INLINE const value_t* begin() const { return data(); }
    QUARISMA_FORCE_INLINE const value_t* end() const { return data() + size_; }

    QUARISMA_FORCE_INLINE value_t* data() { return data_; }
    QUARISMA_FORCE_INLINE value_t* get() { return data_; }
    QUARISMA_FORCE_INLINE value_t* begin() { return data(); }
    QUARISMA_FORCE_INLINE value_t* end() { return data() + size_; }

    QUARISMA_FORCE_INLINE size_t size() const { return size_; }

    value_t*    data_{nullptr};
    size_t      size_{0};
    device_enum type_{device_enum::CPU};
    bool        allocated_;
};
}  // namespace quarisma
