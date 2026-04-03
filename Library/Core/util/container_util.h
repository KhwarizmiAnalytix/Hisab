#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <type_traits>
#include <vector>

#include "common/macros.h"

namespace quarisma
{

//-----------------------------------------------------------------------------
// Check if a container contains a specific value
template <typename Container, typename T>
QUARISMA_FORCE_INLINE bool contains(const Container& container, const T& value)
{
    return std::find(std::begin(container), std::end(container), value) != std::end(container);
}

//-----------------------------------------------------------------------------
// Check if a map contains a specific key
template <typename Key, typename Value>
QUARISMA_FORCE_INLINE bool contains_key(const std::map<Key, Value>& map, const Key& key)
{
    return map.find(key) != map.end();
}

//-----------------------------------------------------------------------------
template <typename T>
QUARISMA_FORCE_INLINE void sort(std::vector<T>& vec)
{
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

//-----------------------------------------------------------------------------
template <typename container>
QUARISMA_FORCE_INLINE void merge(
    const std::vector<double>& array1, const container& array2, std::vector<double>& ret)
{
    if (!array1.empty())
    {
        ret.insert(ret.end(), array1.begin(), array1.end());
    }
    if (!array2.empty())
    {
        ret.insert(ret.end(), array2.begin(), array2.end());
    }

    sort(ret);
}

//------------------------------------------------------------------------------
template <typename container, typename T>
size_t closest_index(const container& element_array, T element)
{
    const auto& it = std::lower_bound(element_array.begin(), element_array.end(), element);

    if (it == element_array.begin())
    {
        return 0;
    }
    if (it == element_array.end())
    {
        return element_array.size() - 1;
    }
    const auto prev = std::prev(it);  //NOLINT
    return (std::abs(*prev - element) <= std::abs(*it - element))
               ? std::distance(element_array.begin(), prev)
               : std::distance(element_array.begin(), it);
}

//-----------------------------------------------------------------------------
// Convert a vector to a set
template <typename T>
QUARISMA_FORCE_INLINE std::set<T> to_set(const std::vector<T>& vec)
{
    return std::set<T>(vec.begin(), vec.end());
}

//-----------------------------------------------------------------------------
// Convert a set to a vector
template <typename T>
QUARISMA_FORCE_INLINE std::vector<T> to_vector(const std::set<T>& set)
{
    return std::vector<T>(set.begin(), set.end());
}

//-----------------------------------------------------------------------------
// Filter elements from a container based on a predicate
template <typename Container, typename Predicate>
QUARISMA_FORCE_INLINE Container filter(const Container& container, Predicate pred)
{
    Container result;
    std::copy_if(container.begin(), container.end(), std::back_inserter(result), pred);
    return result;
}

//-----------------------------------------------------------------------------
// Transform elements in a container using a function
template <typename Container, typename Function>
QUARISMA_FORCE_INLINE auto transform(const Container& container, Function func)
{
    using ResultType = std::invoke_result_t<Function, typename Container::value_type>;
    std::vector<ResultType> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(), std::back_inserter(result), func);
    return result;
}

}  // namespace quarisma