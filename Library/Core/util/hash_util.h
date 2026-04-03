#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include "common/macros.h"

namespace quarisma
{
/**
 * @brief Combines a seed hash value with the hash of another value
 *
 * This implementation is based on Boost's hash_combine function, which is known
 * for its good distribution properties. The magic number 0x9e3779b9 is derived
 * from the golden ratio and helps ensure good distribution.
 *
 * @tparam T Type of the value to hash
 * @param seed The seed hash value to combine with
 * @param v The value to hash and combine
 */
template <typename T>
QUARISMA_FORCE_INLINE void hash_combine(std::size_t& seed, const T& v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/**
 * @brief Specialized version of hash_combine for size_t values
 *
 * This specialized version avoids the overhead of calling std::hash<size_t>
 * on values that are already hash values.
 *
 * @param seed The seed hash value to combine with
 * @param v The hash value to combine
 */
QUARISMA_FORCE_INLINE void hash_combine(std::size_t& seed, const std::size_t& v)
{
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/**
 * @brief Computes a hash value for a pair of values
 *
 * @tparam T1 Type of the first value
 * @tparam T2 Type of the second value
 * @param p Pair of values to hash
 * @return Hash value for the pair
 */
template <typename T1, typename T2>
QUARISMA_FORCE_INLINE std::size_t hash_pair(const std::pair<T1, T2>& p)
{
    std::size_t seed = std::hash<T1>()(p.first);
    hash_combine(seed, p.second);
    return seed;
}

/**
 * @brief Computes a hash value for a pair of values
 *
 * @tparam T1 Type of the first value
 * @tparam T2 Type of the second value
 * @param v1 First value
 * @param v2 Second value
 * @return Hash value for the pair
 */
template <typename T1, typename T2>
QUARISMA_FORCE_INLINE std::size_t hash_pair(const T1& v1, const T2& v2)
{
    std::size_t seed = std::hash<T1>()(v1);
    hash_combine(seed, v2);
    return seed;
}

/**
 * @brief Computes a hash value for a sequence of values
 *
 * @tparam Iterator Type of the iterator
 * @param first Iterator to the first element
 * @param last Iterator past the last element
 * @return Hash value for the sequence
 */
template <typename Iterator>
QUARISMA_FORCE_INLINE std::size_t hash_range(Iterator first, Iterator last)
{
    std::size_t seed = 0;
    for (; first != last; ++first)
    {
        hash_combine(seed, *first);
    }
    return seed;
}

/**
 * @brief Computes a hash value for a variadic list of values
 *
 * @tparam T Type of the first value
 * @tparam Rest Types of the rest of the values
 * @param v First value
 * @param rest Rest of the values
 * @return Hash value for all the values
 */
template <typename T, typename... Rest>
QUARISMA_FORCE_INLINE std::size_t hash_values(const T& v, const Rest&... rest)
{
    std::size_t seed = std::hash<T>()(v);
    (hash_combine(seed, rest), ...);
    return seed;
}

/**
 * @brief Specialized version of hash_values for size_t as the first parameter
 *
 * This specialized version avoids the overhead of calling std::hash<size_t>
 * on the first value when it's already a hash value.
 *
 * @tparam Rest Types of the rest of the values
 * @param v First value (already a hash)
 * @param rest Rest of the values
 * @return Hash value for all the values
 */
template <typename... Rest>
QUARISMA_FORCE_INLINE std::size_t hash_values(const std::size_t& v, const Rest&... rest)
{
    std::size_t seed = v;  // No need to hash, it's already a hash value
    (hash_combine(seed, rest), ...);
    return seed;
}

/**
 * @brief Specialized version that computes a hash value for a variadic list of size_t values
 *
 * This specialized version is more efficient when combining pre-computed hash values
 * because it avoids the overhead of calling std::hash<size_t> on values that are already hashes.
 *
 * @param h1 First hash value
 * @param rest Rest of the hash values
 * @return Combined hash value
 */
template <typename... Rest>
QUARISMA_FORCE_INLINE std::size_t hash_values(std::size_t h1, std::size_t h2, Rest... rest)
{
    std::size_t seed = h1;
    hash_combine(seed, h2);

    if constexpr (sizeof...(rest) > 0)
    {
        seed = hash_values(seed, rest...);
    }
    return seed;
}

/**
 * @brief Specialized version that computes a hash value for a single size_t value
 *
 * This is a base case for the recursive template above.
 *
 * @param h Hash value
 * @return The same hash value (identity function)
 */
QUARISMA_FORCE_INLINE std::size_t hash_values(std::size_t h)
{
    return h;
}

}  // namespace quarisma

// Specializations for std::pair
namespace std
{
template <typename T1, typename T2>
struct hash<std::pair<T1, T2>>
{
    std::size_t operator()(const std::pair<T1, T2>& p) const { return quarisma::hash_pair(p); }
};
}  // namespace std
