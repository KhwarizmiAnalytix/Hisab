#ifdef COMPILE_VECTOR_HXX
#include <utility>

#include "memory/allocator.h"

#define size_type quarisma::vector<value_t>::size_type

namespace quarisma
{
//-----------------------------------------------------------------------------
template <typename value_t>
vector<value_t>::vector(size_type length, quarisma::device_type type) noexcept
    : storage_(length, type)
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ vector<value_t>::vector(
    std::initializer_list<value_t> list, quarisma::device_type type) noexcept
    : vector(list.size(), type)
{
    const auto& ptr = list.begin();
    allocator_t::copy(ptr, storage_.size_, storage_.data_, type, type);
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ vector<value_t>::vector(
    value_t* data, size_type length, quarisma::device_type type) noexcept
    : storage_(data, length, type)
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ vector<value_t>::vector(
    void* data, size_type length, quarisma::device_type type) noexcept
    : vector((value_t*)data, length, type)
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>::vector(
    std::vector<value_t> const& rhs, quarisma::device_type type) noexcept
    : vector(const_cast<value_t*>(rhs.data()), rhs.size(), type)
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>::vector(
    value_t const* data, size_type length, quarisma::device_type type) noexcept
    : vector(const_cast<value_t*>(data), length, type)
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>::vector(
    value_t start, value_t end, size_type length, quarisma::device_type type) noexcept
    : vector(length, type)
{
    auto dx = (end - start) / (length - 1);

    for (size_t i = 0; i < length; ++i)
        storage_.data_[i] = i * dx + start;
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>& vector<value_t>::operator=(
    vector const& rhs) noexcept
{
    storage_ = rhs.storage_;
    return *this;
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>::vector(vector const& rhs) noexcept
    : storage_(rhs.storage_)
{
}
//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>& vector<value_t>::operator=(vector&& rhs) noexcept
{
    storage_ = std::move(rhs.storage_);
    return *this;
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_CUDA_FUNCTION_TYPE__ vector<value_t>::vector(vector&& rhs) noexcept
    : storage_(std::move(rhs.storage_))
{
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ void vector<value_t>::deepcopy(vector const& rhs) noexcept
{
    storage_.copy(rhs.storage_);
}

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ vector<value_t>::~vector() = default;

//-----------------------------------------------------------------------------
template <typename value_t>
__VECTORIZATION_FUNCTION_ATTRIBUTE__ value_t* vector<value_t>::data(size_type i) const noexcept
{
    return data() + i;
}

#ifndef __CUDACC__

//-----------------------------------------------------------------------------
template <typename value_t>
void vector<value_t>::binary_serialize(quarisma::multi_process_stream& buffer, const vector& v)
{
    buffer << v.size();
    buffer.Push(v.begin(), v.size());
};

//-----------------------------------------------------------------------------
template <typename value_t>
void vector<value_t>::binary_deserialize(quarisma::multi_process_stream& buffer, vector& v)
{
    size_t size;
    buffer >> size;

    v = vector(size);

    unsigned int n;
    buffer.Pop(v.begin(), n);
};

//-----------------------------------------------------------------------------
template <typename value_t>
void vector<value_t>::read_from_json(const json& root, vector& v)
{
    auto size = root["size"].get<size_t>();

    v = vector(size);

    const auto& archiver = root["data"].array();

    for (size_t i = 0; i < v.size(); ++i)
    {
        v[i] = archiver.at(i).get<value_t>();
    }
}

//-----------------------------------------------------------------------------
template <typename value_t>
void vector<value_t>::write_to_json(json& root, const vector& v)
{
    root["class"] = "quarisma.vector";
    root["size"]  = v.size();

    auto& archiver = root["data"];
    // archiver.resize(static_cast<Json::ArrayIndex>(v.size()));

    for (size_t i = 0; i < v.size(); ++i)
    {
        archiver.emplace_back(v[i]);
    }
}
#endif
}  // namespace quarisma

#undef size_type
#endif
