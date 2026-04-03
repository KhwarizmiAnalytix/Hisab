#pragma once

// Check for C++20 span support
#if __cplusplus >= 202002L
#include <span>
#else
#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace std
{
template <typename T>
class span
{
public:
    // Type definitions
    using element_type           = T;
    using value_type             = std::remove_cv_t<T>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using pointer                = T*;
    using const_pointer          = const T*;
    using reference              = T&;
    using const_reference        = const T&;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    // Static constant for dynamic extent
    static constexpr size_type dynamic_extent = static_cast<size_type>(-1);

    // Constructors
    constexpr span() noexcept : data_(nullptr), size_(0) {}

    constexpr span(pointer ptr, size_type count) noexcept : data_(ptr), size_(count) {}

    constexpr span(pointer first, pointer last) noexcept
        : data_(first), size_(static_cast<size_type>(last - first))
    {
    }

    // Container constructors
    template <size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N)
    {
    }

    template <size_t N>
    constexpr span(std::array<value_type, N>& arr) noexcept : data_(arr.data()), size_(N)
    {
    }

    template <size_t N>
    constexpr span(const std::array<value_type, N>& arr) noexcept : data_(arr.data()), size_(N)
    {
    }

    template <typename Container>
    constexpr span(Container& cont) noexcept(noexcept(std::data(cont)) && noexcept(std::size(cont)))
        : data_(std::data(cont)), size_(std::size(cont))
    {
    }

    constexpr span(const span& other) noexcept = default;

    // Assignment
    constexpr span& operator=(const span& other) noexcept = default;

    // Iterator support
    constexpr iterator               begin() const noexcept { return data_; }
    constexpr iterator               end() const noexcept { return data_ + size_; }
    constexpr const_iterator         cbegin() const noexcept { return data_; }
    constexpr const_iterator         cend() const noexcept { return data_ + size_; }
    constexpr reverse_iterator       rbegin() const noexcept { return reverse_iterator(end()); }
    constexpr reverse_iterator       rend() const noexcept { return reverse_iterator(begin()); }
    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    constexpr const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }

    // Element access
    constexpr reference operator[](size_type idx) const noexcept
    {
        assert(idx < size_ && "span index out of range");
        return data_[idx];
    }

    constexpr reference front() const noexcept
    {
        assert(size_ > 0 && "span is empty");
        return data_[0];
    }

    constexpr reference back() const noexcept
    {
        assert(size_ > 0 && "span is empty");
        return data_[size_ - 1];
    }

    // Data access
    constexpr pointer data() const noexcept { return data_; }

    // Observers
    constexpr size_type size() const noexcept { return size_; }
    constexpr size_type size_bytes() const noexcept { return size_ * sizeof(element_type); }
    QUARISMA_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }

    // Subviews
    constexpr span<element_type> first(size_type count) const
    {
        assert(count <= size_ && "count out of range");
        return span<element_type>(data_, count);
    }

    constexpr span<element_type> last(size_type count) const
    {
        assert(count <= size_ && "count out of range");
        return span<element_type>(data_ + (size_ - count), count);
    }

    constexpr span<element_type> subspan(size_type offset, size_type count = dynamic_extent) const
    {
        assert(offset <= size_ && "offset out of range");
        if (count == dynamic_extent)
        {
            count = size_ - offset;
        }

        assert(count <= size_ - offset && "count out of range");
        return span<element_type>(data_ + offset, count);
    }

private:
    pointer   data_;
    size_type size_;
};

// Deduction guides
template <typename T, size_t N>
span(T (&)[N]) -> span<T>;

template <typename T, size_t N>
span(std::array<T, N>&) -> span<T>;

template <typename T, size_t N>
span(const std::array<T, N>&) -> span<const T>;

template <typename Container>
span(Container&) -> span<typename Container::value_type>;

// Comparison operators
template <typename T>
constexpr bool operator==(const span<T>& lhs, const span<T>& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;
    for (size_t i = 0; i < lhs.size(); ++i)
    {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

template <typename T>
constexpr bool operator!=(const span<T>& lhs, const span<T>& rhs) noexcept
{
    return !(lhs == rhs);
}

// Helper type traits
template <typename T>
struct is_span : std::false_type
{
};

template <typename T>
struct is_span<span<T> > : std::true_type
{
};

template <typename T>
inline constexpr bool is_span_v = is_span<T>::value;
}  // namespace std
#endif