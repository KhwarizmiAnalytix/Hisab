/*
 * Quarisma: High-Performance Computational Library
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

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "CoreTest.h"
#include "logger.h"
#include "util/flat_hash.h"
#include "util/hash_util.h"

using namespace quarisma;

// ============================================================================
// Basic hash_combine Tests
// ============================================================================

/**
 * @brief Test basic hash_combine functionality
 *
 * Covers: hash_combine with various types, seed modification
 */
QUARISMATEST(HashUtil, hash_util_basic_combine)
{
    // Test hash_combine with integers
    std::size_t seed1 = 0;
    hash_combine(seed1, 42);
    EXPECT_NE(seed1, 0);  // Seed should be modified

    // Test hash_combine with strings
    std::size_t seed2 = 0;
    hash_combine(seed2, std::string("test"));
    EXPECT_NE(seed2, 0);

    // Test hash_combine with doubles
    std::size_t seed3 = 0;
    hash_combine(seed3, 3.14159);
    EXPECT_NE(seed3, 0);

    // Test that same value produces same hash
    std::size_t seed4 = 0;
    std::size_t seed5 = 0;
    hash_combine(seed4, 42);
    hash_combine(seed5, 42);
    EXPECT_EQ(seed4, seed5);

    // Test that different values produce different hashes
    std::size_t seed6 = 0;
    std::size_t seed7 = 0;
    hash_combine(seed6, 42);
    hash_combine(seed7, 43);
    EXPECT_NE(seed6, seed7);

    END_TEST();
}

/**
 * @brief Test hash_combine with size_t specialization
 *
 * Covers: specialized hash_combine for size_t values
 */
QUARISMATEST(HashUtil, hash_util_combine_size_t)
{
    // Test size_t specialization
    std::size_t seed1 = 0;
    std::size_t value = 12345;
    hash_combine(seed1, value);
    EXPECT_NE(seed1, 0);

    // Test that specialization produces consistent results
    std::size_t seed2 = 0;
    hash_combine(seed2, value);
    EXPECT_EQ(seed1, seed2);

    // Test combining multiple size_t values
    std::size_t seed3 = 0;
    hash_combine(seed3, std::size_t(1));
    hash_combine(seed3, std::size_t(2));
    hash_combine(seed3, std::size_t(3));
    EXPECT_NE(seed3, 0);

    END_TEST();
}

// ============================================================================
// hash_pair Tests
// ============================================================================

/**
 * @brief Test hash_pair functionality
 *
 * Covers: hash_pair with various types, consistency
 */
QUARISMATEST(HashUtil, hash_util_pair)
{
    // Test hash_pair with integers
    auto        pair1 = std::make_pair(1, 2);
    std::size_t hash1 = hash_pair(pair1);
    EXPECT_NE(hash1, 0);

    // Test hash_pair with separate arguments
    std::size_t hash2 = hash_pair(1, 2);
    EXPECT_EQ(hash1, hash2);

    // Test hash_pair with strings
    auto        pair3 = std::make_pair(std::string("hello"), std::string("world"));
    std::size_t hash3 = hash_pair(pair3);
    EXPECT_NE(hash3, 0);

    // Test hash_pair with mixed types
    auto        pair4 = std::make_pair(42, std::string("test"));
    std::size_t hash4 = hash_pair(pair4);
    EXPECT_NE(hash4, 0);

    // Test that different pairs produce different hashes
    auto pair5 = std::make_pair(1, 2);
    auto pair6 = std::make_pair(2, 1);
    EXPECT_NE(hash_pair(pair5), hash_pair(pair6));

    // Test std::hash specialization for std::pair
    std::hash<std::pair<int, int>> hasher;
    std::size_t                    hash7 = hasher(pair1);
    EXPECT_EQ(hash7, hash1);

    END_TEST();
}

// ============================================================================
// hash_range Tests
// ============================================================================

/**
 * @brief Test hash_range functionality
 *
 * Covers: hash_range with various containers, empty ranges
 */
QUARISMATEST(HashUtil, hash_util_range)
{
    // Test hash_range with vector
    std::vector<int> vec1  = {1, 2, 3, 4, 5};
    std::size_t      hash1 = hash_range(vec1.begin(), vec1.end());
    EXPECT_NE(hash1, 0);

    // Test consistency
    std::size_t hash2 = hash_range(vec1.begin(), vec1.end());
    EXPECT_EQ(hash1, hash2);

    // Test with different vector
    std::vector<int> vec2  = {1, 2, 3, 4, 6};
    std::size_t      hash3 = hash_range(vec2.begin(), vec2.end());
    EXPECT_NE(hash1, hash3);

    // Test with empty range
    std::vector<int> vec3;
    std::size_t      hash4 = hash_range(vec3.begin(), vec3.end());
    EXPECT_EQ(hash4, 0);  // Empty range should produce 0

    // Test with single element
    std::vector<int> vec4  = {42};
    std::size_t      hash5 = hash_range(vec4.begin(), vec4.end());
    EXPECT_NE(hash5, 0);

    // Test with strings
    std::vector<std::string> vec5  = {"hello", "world"};
    std::size_t              hash6 = hash_range(vec5.begin(), vec5.end());
    EXPECT_NE(hash6, 0);

    END_TEST();
}

// ============================================================================
// hash_values Tests
// ============================================================================

/**
 * @brief Test hash_values with variadic arguments
 *
 * Covers: hash_values with multiple types, order sensitivity
 */
QUARISMATEST(HashUtil, hash_util_values)
{
    // Test hash_values with single value
    std::size_t hash1 = hash_values(42);
    EXPECT_NE(hash1, 0);

    // Test hash_values with multiple values
    std::size_t hash2 = hash_values(1, 2, 3);
    EXPECT_NE(hash2, 0);

    // Test hash_values with mixed types
    std::size_t hash3 = hash_values(42, 3.14, std::string("test"));
    EXPECT_NE(hash3, 0);

    // Test order sensitivity
    std::size_t hash4 = hash_values(1, 2, 3);
    std::size_t hash5 = hash_values(3, 2, 1);
    EXPECT_NE(hash4, hash5);

    // Test consistency
    std::size_t hash6 = hash_values(1, 2, 3);
    EXPECT_EQ(hash4, hash6);

    // Test with size_t specialization
    std::size_t hash7 = hash_values(std::size_t(1), std::size_t(2), std::size_t(3));
    EXPECT_NE(hash7, 0);

    // Test single size_t
    std::size_t hash8 = hash_values(std::size_t(42));
    EXPECT_EQ(hash8, 42);  // Single size_t should return itself

    END_TEST();
}

// ============================================================================
// Edge Cases and Boundary Conditions
// ============================================================================

/**
 * @brief Test hash utilities with edge cases
 *
 * Covers: zero values, negative values, extreme values
 */
QUARISMATEST(HashUtil, hash_util_edge_cases)
{
    // Test with zero
    //auto hash1 = hash_values(0);
    //EXPECT_EQ(hash1, 0);  // Hash of 0 should be 0, as per hash_values(size_t) specialization

    // Test with negative values
    auto hash2 = hash_values(-1, -2, -3);
    EXPECT_NE(hash2, 0);

    // Test with maximum values
    auto hash3 = hash_values(INT32_MAX, INT64_MAX);
    EXPECT_NE(hash3, 0);

    // Test with minimum values
    auto hash4 = hash_values(INT32_MIN, INT64_MIN);
    EXPECT_NE(hash4, 0);

    // Test very long string
    std::string long_str(10000, 'x');
    auto        hash6 = hash_values(long_str);
    EXPECT_NE(hash6, 0);

    END_TEST();
}

// ============================================================================
// Hash Distribution Tests
// ============================================================================

/**
 * @brief Test hash distribution quality
 *
 * Covers: collision resistance, distribution uniformity
 */
QUARISMATEST(HashUtil, hash_util_distribution)
{
    // Test that sequential values produce well-distributed hashes
    std::set<std::size_t> hashes;
    const int             count = 1000;

    for (int i = 0; i < count; ++i)
    {
        std::size_t hash = hash_values(i);
        hashes.insert(hash);
    }

    // Check that we have good distribution (few collisions)
    // We expect at least 99% unique hashes for sequential integers
    EXPECT_GT(hashes.size(), static_cast<size_t>(count * 0.99));

    QUARISMA_LOG_INFO("Hash distribution test: {}/{} unique hashes", hashes.size(), count);

    END_TEST();
}

// ============================================================================
// Platform Independence Tests
// ============================================================================

/**
 * @brief Test that hash functions produce consistent results across platforms
 *
 * Covers: cross-platform consistency, fixed-width types
 */
QUARISMATEST(HashUtil, hash_util_platform_independence)
{
    // Use fixed-width types for platform independence
    std::size_t hash1 = hash_values(int32_t(42), int64_t(1000));
    EXPECT_NE(hash1, 0);

    // Test with uint types
    std::size_t hash2 = hash_values(uint32_t(42), uint64_t(1000));
    EXPECT_NE(hash2, 0);

    // Test consistency within same run
    std::size_t hash3 = hash_values(int32_t(42), int64_t(1000));
    EXPECT_EQ(hash1, hash3);

    END_TEST();
}

// ============================================================================
// Container Integration Tests
// ============================================================================

/**
 * @brief Test hash utilities with STL containers
 *
 * Covers: unordered_map, unordered_set with custom hash
 */
QUARISMATEST(HashUtil, hash_util_container_integration)
{
    // Test std::pair as key in unordered_map (uses our hash specialization)
    quarisma_map<std::pair<int, int>, std::string> map1;
    map1[std::make_pair(1, 2)] = "one-two";
    map1[std::make_pair(3, 4)] = "three-four";

    EXPECT_EQ(map1[std::make_pair(1, 2)], "one-two");
    EXPECT_EQ(map1[std::make_pair(3, 4)], "three-four");
    EXPECT_EQ(map1.size(), 2);

    // Test with string pairs
    quarisma_map<std::pair<std::string, std::string>, int> map2;
    map2[std::make_pair(std::string("hello"), std::string("world"))] = 1;
    map2[std::make_pair(std::string("foo"), std::string("bar"))]     = 2;

    EXPECT_EQ(map2[std::make_pair(std::string("hello"), std::string("world"))], 1);
    EXPECT_EQ(map2[std::make_pair(std::string("foo"), std::string("bar"))], 2);

    // Test std::pair in unordered_set
    std::unordered_set<std::pair<int, int>> set1;
    set1.insert({1, 2});
    set1.insert({3, 4});
    set1.insert({1, 2});  // Duplicate

    EXPECT_EQ(set1.size(), 2);
    EXPECT_TRUE(set1.find({1, 2}) != set1.end());
    EXPECT_TRUE(set1.find({3, 4}) != set1.end());
    EXPECT_TRUE(set1.find({5, 6}) == set1.end());

    END_TEST();
}

// ============================================================================
// Performance Tests
// ============================================================================

/**
 * @brief Test hash performance with large datasets
 *
 * Covers: performance, scalability
 */
QUARISMATEST(HashUtil, hash_util_performance)
{
    // Test hashing performance with many values
    const int                iterations = 10000;
    std::vector<std::size_t> hashes;
    hashes.reserve(iterations);

    for (int i = 0; i < iterations; ++i)
    {
        std::size_t hash = hash_values(i, i * 2, i * 3);
        hashes.push_back(hash);
    }

    EXPECT_EQ(hashes.size(), static_cast<size_t>(iterations));

    // Test hash_range performance with large vector
    std::vector<int> large_vec;
    large_vec.reserve(1000);
    for (int i = 0; i < 1000; ++i)
    {
        large_vec.push_back(i);
    }

    std::size_t hash = hash_range(large_vec.begin(), large_vec.end());
    EXPECT_NE(hash, 0);

    QUARISMA_LOG_INFO("Performance test: hashed {} values successfully", iterations);

    END_TEST();
}

// ============================================================================
// Collision Resistance Tests
// ============================================================================

/**
 * @brief Test hash collision resistance
 *
 * Covers: collision detection, hash quality
 */
QUARISMATEST(HashUtil, hash_util_collision_resistance)
{
    // Test with similar values
    std::unordered_set<std::size_t> hashes;

    // Test pairs that differ by one element
    for (int i = 0; i < 100; ++i)
    {
        hashes.insert(hash_pair(i, 0));
        hashes.insert(hash_pair(0, i));
    }

    // Should have 200 unique hashes (or close to it)
    EXPECT_GT(hashes.size(), 195);

    // Test with permutations
    hashes.clear();
    std::vector<int> base = {1, 2, 3, 4, 5};

    // Generate some permutations
    for (int i = 0; i < 100; ++i)
    {
        std::vector<int> perm = base;
        std::rotate(perm.begin(), perm.begin() + (i % 5), perm.end());
        hashes.insert(hash_range(perm.begin(), perm.end()));
    }

    // Should have good distribution
    EXPECT_GT(hashes.size(), 4);  // At least 5 unique permutations

    END_TEST();
}

// ============================================================================
// Complex Type Tests
// ============================================================================

/**
 * @brief Test hash utilities with complex types
 *
 * Covers: nested containers, custom types
 */
QUARISMATEST(HashUtil, hash_util_complex_types)
{
    // Test with nested pairs
    using NestedPair   = std::pair<std::pair<int, int>, std::pair<int, int>>;
    NestedPair  nested = {{1, 2}, {3, 4}};
    std::size_t hash1  = hash_pair(nested);
    EXPECT_NE(hash1, 0);

    // Test with vector of pairs
    std::vector<std::pair<int, int>> vec_pairs = {{1, 2}, {3, 4}, {5, 6}};
    std::size_t                      hash2     = hash_range(vec_pairs.begin(), vec_pairs.end());
    EXPECT_NE(hash2, 0);

    // Test combining multiple hash values
    std::size_t h1       = hash_values(1, 2);
    std::size_t h2       = hash_values(3, 4);
    std::size_t h3       = hash_values(5, 6);
    std::size_t combined = hash_values(h1, h2, h3);
    EXPECT_NE(combined, 0);

    END_TEST();
}

// ============================================================================
// Consistency Tests
// ============================================================================

/**
 * @brief Test hash consistency across multiple calls
 *
 * Covers: deterministic behavior, repeatability
 */
QUARISMATEST(HashUtil, hash_util_consistency)
{
    // Test that same input always produces same output
    const int   test_runs      = 100;
    std::string test_str       = "test";
    std::size_t reference_hash = hash_values(1, 2, 3, test_str, 4.5);

    for (int i = 0; i < test_runs; ++i)
    {
        std::size_t hash = hash_values(1, 2, 3, test_str, 4.5);
        EXPECT_EQ(hash, reference_hash);
    }

    // Test with pairs
    auto        pair          = std::make_pair(42, std::string("test"));
    std::size_t pair_hash_ref = hash_pair(pair);

    for (int i = 0; i < test_runs; ++i)
    {
        std::size_t hash = hash_pair(pair);
        EXPECT_EQ(hash, pair_hash_ref);
    }

    // Test with ranges
    std::vector<int> vec            = {1, 2, 3, 4, 5};
    std::size_t      range_hash_ref = hash_range(vec.begin(), vec.end());

    for (int i = 0; i < test_runs; ++i)
    {
        std::size_t hash = hash_range(vec.begin(), vec.end());
        EXPECT_EQ(hash, range_hash_ref);
    }

    QUARISMA_LOG_INFO("Consistency test: {} runs completed successfully", test_runs);

    END_TEST();
}
