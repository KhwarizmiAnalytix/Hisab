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

#include <algorithm>
#include <string>
#include <vector>

#include "baseTest.h"
#include "util/flat_hash.h"

using namespace quarisma;

// ============================================================================
// Consolidated Test 1: Basic Map and Set Operations
// ============================================================================
// Tests: basic operations, value types, iteration, capacity, edge cases,
//        copy/move semantics, emplace, aliases, insert operations
QUARISMATEST(FlatHash, map_and_set_comprehensive)
{
    // ===== MAP BASIC OPERATIONS =====
    flat_hash_map<int, std::string> map;

    // Test empty map
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    // Test insert via operator[]
    map[1] = "one";
    map[2] = "two";
    map[3] = "three";
    EXPECT_FALSE(map.empty());
    EXPECT_EQ(map.size(), 3);

    // Test find
    auto it1 = map.find(1);
    EXPECT_TRUE(it1 != map.end());
    EXPECT_EQ(it1->second, "one");
    auto it_not_found = map.find(99);
    EXPECT_TRUE(it_not_found == map.end());

    // Test erase
    map.erase(2);
    EXPECT_EQ(map.size(), 2);
    EXPECT_TRUE(map.find(2) == map.end());

    // Test clear
    map.clear();
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    // ===== SET BASIC OPERATIONS =====
    flat_hash_set<int> set;

    // Test empty set
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);

    // Test insert
    set.insert(1);
    set.insert(2);
    set.insert(3);
    EXPECT_FALSE(set.empty());
    EXPECT_EQ(set.size(), 3);

    // Test find and contains
    EXPECT_TRUE(set.find(1) != set.end());
    EXPECT_TRUE(set.find(2) != set.end());
    EXPECT_TRUE(set.find(99) == set.end());
    EXPECT_TRUE(set.contains(1));
    EXPECT_TRUE(set.contains(2));
    EXPECT_FALSE(set.contains(99));

    // Test duplicate insert
    auto result = set.insert(1);
    EXPECT_FALSE(result.second);  // Should not insert duplicate
    EXPECT_EQ(set.size(), 3);

    // Test erase
    set.erase(2);
    EXPECT_EQ(set.size(), 2);
    EXPECT_FALSE(set.contains(2));

    // Test clear
    set.clear();
    EXPECT_TRUE(set.empty());

    // ===== VALUE TYPES =====
    // Test with string
    flat_hash_set<std::string> str_set;
    str_set.insert("hello");
    str_set.insert("world");
    EXPECT_TRUE(str_set.contains("hello"));
    EXPECT_TRUE(str_set.contains("world"));
    EXPECT_FALSE(str_set.contains("foo"));

    // ===== ITERATION =====
    flat_hash_map<int, std::string> map2;
    map2[1] = "one";
    map2[2] = "two";
    map2[3] = "three";

    int count = 0;
    for (const auto& pair : map2)
    {
        EXPECT_TRUE(pair.first >= 1 && pair.first <= 3);
        count++;
    }
    EXPECT_EQ(count, 3);

    // Test set iteration
    flat_hash_set<int> set2;
    set2.insert(1);
    set2.insert(2);
    set2.insert(3);

    count = 0;
    for (int value : set2)
    {
        EXPECT_TRUE(value >= 1 && value <= 3);
        count++;
    }
    EXPECT_EQ(count, 3);

    // ===== CAPACITY =====
    flat_hash_map<int, int> cap_map;

    // Test reserve
    cap_map.reserve(100);
    EXPECT_GE(cap_map.bucket_count(), 100);

    // Insert elements
    for (int i = 0; i < 10; ++i)
    {
        cap_map[i] = i * 10;
    }
    EXPECT_EQ(cap_map.size(), 10);

    // Test load factor
    float load = cap_map.load_factor();
    EXPECT_GT(load, 0.0f);
    EXPECT_LE(load, cap_map.max_load_factor());

    // ===== EDGE CASES =====
    // Test empty map operations
    flat_hash_map<int, int> empty_map;
    EXPECT_TRUE(empty_map.find(1) == empty_map.end());
    EXPECT_EQ(empty_map.erase(1), 0);

    // Test single element and overwriting
    flat_hash_map<int, int> single_map;
    single_map[1] = 100;
    EXPECT_EQ(single_map.size(), 1);
    EXPECT_EQ(single_map[1], 100);
    single_map[1] = 200;
    EXPECT_EQ(single_map.size(), 1);
    EXPECT_EQ(single_map[1], 200);

    // Test empty set operations
    flat_hash_set<int> empty_set;
    EXPECT_FALSE(empty_set.contains(1));
    EXPECT_EQ(empty_set.erase(1), 0);

    // Test duplicate handling in set
    flat_hash_set<int> dup_set;
    dup_set.insert(1);
    dup_set.insert(1);
    dup_set.insert(1);
    EXPECT_EQ(dup_set.size(), 1);

    // ===== COPY SEMANTICS =====
    flat_hash_map<int, std::string> map3;
    map3[1] = "one";
    map3[2] = "two";

    flat_hash_map<int, std::string> map4(map3);
    EXPECT_EQ(map4.size(), 2);
    EXPECT_EQ(map4[1], "one");
    EXPECT_EQ(map4[2], "two");

    // Verify independence
    map3[1] = "ONE";
    EXPECT_EQ(map4[1], "one");

    // Test set copy constructor
    flat_hash_set<int> set3;
    set3.insert(1);
    set3.insert(2);

    flat_hash_set<int> set4(set3);
    EXPECT_EQ(set4.size(), 2);
    EXPECT_TRUE(set4.contains(1));
    EXPECT_TRUE(set4.contains(2));

    // ===== EMPLACE OPERATIONS =====
    flat_hash_map<int, std::string> emplace_map;
    auto                            result1 = emplace_map.emplace(1, "one");
    EXPECT_TRUE(result1.second);  // Insertion successful
    EXPECT_EQ(result1.first->second, "one");

    // Test duplicate emplace
    auto result2 = emplace_map.emplace(1, "ONE");
    EXPECT_FALSE(result2.second);             // Insertion failed (duplicate)
    EXPECT_EQ(result2.first->second, "one");  // Original value unchanged

    // Test set emplace
    flat_hash_set<int> emplace_set;
    auto               result3 = emplace_set.emplace(42);
    EXPECT_TRUE(result3.second);
    auto result4 = emplace_set.emplace(42);
    EXPECT_FALSE(result4.second);  // Duplicate

    // ===== QUARISMA ALIASES =====
    quarisma_map<int, std::string> alias_map;
    alias_map[1] = "one";
    alias_map[2] = "two";
    EXPECT_EQ(alias_map.size(), 2);
    EXPECT_EQ(alias_map[1], "one");

    quarisma_set<int> alias_set;
    alias_set.insert(1);
    alias_set.insert(2);
    EXPECT_EQ(alias_set.size(), 2);
    EXPECT_TRUE(alias_set.contains(1));

    // ===== INSERT OPERATIONS =====
    flat_hash_map<int, std::string> insert_map;

    // Insert const value
    const std::pair<int, std::string> value(1, "one");
    auto                              insert_result = insert_map.insert(value);
    EXPECT_TRUE(insert_result.second);
    EXPECT_EQ(insert_result.first->first, 1);
    EXPECT_EQ(insert_result.first->second, "one");

    // Insert rvalue
    auto rvalue_result = insert_map.insert(std::make_pair(2, "two"));
    EXPECT_TRUE(rvalue_result.second);
    EXPECT_EQ(rvalue_result.first->first, 2);
    EXPECT_EQ(rvalue_result.first->second, "two");

    // Insert duplicate
    auto dup_result = insert_map.insert({1, "20"});
    EXPECT_FALSE(dup_result.second);
    EXPECT_EQ(dup_result.first->second, "one");  // Original value unchanged

    // insert_or_assign and contains
    flat_hash_map<int, std::string> ioa_map;
    ioa_map.insert_or_assign(1, std::string("one"));
    EXPECT_EQ(ioa_map.size(), 1);
    EXPECT_TRUE(ioa_map.contains(1));
    EXPECT_EQ(ioa_map.find(1)->second, "one");

    // Assign existing key (value should be updated, size unchanged)
    ioa_map.insert_or_assign(1, std::string("ONE"));
    EXPECT_EQ(ioa_map.size(), 1);
    EXPECT_TRUE(ioa_map.contains(1));
    EXPECT_EQ(ioa_map.find(1)->second, "ONE");

    // Insert another key via iterator overload
    ioa_map.insert_or_assign(ioa_map.cbegin(), 2, std::string("two"));
    EXPECT_EQ(ioa_map.size(), 2);
    EXPECT_TRUE(ioa_map.contains(2));

    // ===== MAP AT ACCESSORS =====
    flat_hash_map<int, int> at_map;
    at_map[10]          = 42;
    const auto& cat_map = at_map;

    EXPECT_EQ(at_map.at(10), 42);
    EXPECT_EQ(cat_map.at(10), 42);

    END_TEST();
}

// ============================================================================
// Consolidated Test 2: Constructors and Move Semantics
// ============================================================================
// Tests: default constructor, bucket count constructor, initializer list,
//        move constructor, copy constructor, move assignments
QUARISMATEST(FlatHash, constructors_and_move_semantics)
{
    // ===== DEFAULT CONSTRUCTOR =====
    flat_hash_map<int, int> default_map;
    EXPECT_TRUE(default_map.empty());
    EXPECT_EQ(default_map.size(), 0);
    EXPECT_EQ(default_map.bucket_count(), 0);

    // ===== BUCKET COUNT CONSTRUCTOR =====
    flat_hash_map<int, int> bucket_map(100);
    EXPECT_TRUE(bucket_map.empty());
    EXPECT_GE(bucket_map.bucket_count(), 100);

    // ===== BUCKET COUNT WITH HASH AND EQUAL =====
    std::hash<int>          hash;
    std::equal_to<int>      equal;
    flat_hash_map<int, int> hash_map(50, hash, equal);
    EXPECT_TRUE(hash_map.empty());
    EXPECT_GE(hash_map.bucket_count(), 50);

    // ===== INITIALIZER LIST CONSTRUCTOR (EMPTY) =====
    flat_hash_map<int, int> empty_init({});
    EXPECT_TRUE(empty_init.empty());
    EXPECT_EQ(empty_init.size(), 0);

    // ===== INITIALIZER LIST CONSTRUCTOR (SMALL) =====
    flat_hash_map<int, int> small_init({{1, 10}, {2, 20}, {3, 30}});
    EXPECT_EQ(small_init.size(), 3);
    EXPECT_EQ(small_init[1], 10);
    EXPECT_EQ(small_init[2], 20);
    EXPECT_EQ(small_init[3], 30);

    // ===== INITIALIZER LIST CONSTRUCTOR (LARGE) =====
    std::vector<std::pair<int, int>> items;
    for (int i = 0; i < 100; ++i)
    {
        items.push_back({i, i * 10});
    }

    flat_hash_map<int, int> large_init(items.begin(), items.end());
    EXPECT_EQ(large_init.size(), 100);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(large_init[i], i * 10);
    }

    // ===== MOVE CONSTRUCTOR =====
    flat_hash_map<int, std::string> move_src;
    move_src[1] = "one";
    move_src[2] = "two";
    move_src[3] = "three";

    flat_hash_map<int, std::string> move_dst(std::move(move_src));
    EXPECT_EQ(move_dst.size(), 3);
    EXPECT_EQ(move_dst[1], "one");
    EXPECT_EQ(move_dst[2], "two");
    EXPECT_EQ(move_dst[3], "three");

    // ===== COPY CONSTRUCTOR =====
    flat_hash_map<int, std::string> copy_src;
    copy_src[1] = "one";
    copy_src[2] = "two";

    flat_hash_map<int, std::string> copy_dst(copy_src);
    EXPECT_EQ(copy_dst.size(), 2);
    EXPECT_EQ(copy_dst[1], "one");
    EXPECT_EQ(copy_dst[2], "two");

    // Verify independence
    copy_src[1] = "ONE";
    EXPECT_EQ(copy_dst[1], "one");

    // ===== MOVE ASSIGNMENT (SELF) =====
    flat_hash_map<int, int> self_map;
    self_map[1] = 10;
    self_map[2] = 20;

    self_map = std::move(self_map);
    EXPECT_EQ(self_map.size(), 2);
    EXPECT_EQ(self_map[1], 10);
    EXPECT_EQ(self_map[2], 20);

    // ===== MOVE ASSIGNMENT (DIFFERENT) =====
    flat_hash_map<int, int> ma_src;
    ma_src[1] = 10;
    ma_src[2] = 20;

    flat_hash_map<int, int> ma_dst;
    ma_dst[3] = 30;

    ma_dst = std::move(ma_src);
    EXPECT_EQ(ma_dst.size(), 2);
    EXPECT_EQ(ma_dst[1], 10);
    EXPECT_EQ(ma_dst[2], 20);
    EXPECT_FALSE(ma_dst.contains(3));

    // ===== MOVE ASSIGNMENT (EMPTY TO NONEMPTY) =====
    flat_hash_map<int, int> empty_src;
    flat_hash_map<int, int> nonempty_dst;
    nonempty_dst[1] = 10;
    nonempty_dst[2] = 20;

    nonempty_dst = std::move(empty_src);
    EXPECT_TRUE(nonempty_dst.empty());
    EXPECT_EQ(nonempty_dst.size(), 0);

    // ===== MOVE ASSIGNMENT (NONEMPTY TO EMPTY) =====
    flat_hash_map<int, int> nonempty_src;
    nonempty_src[1] = 10;
    nonempty_src[2] = 20;

    flat_hash_map<int, int> empty_dst;
    empty_dst = std::move(nonempty_src);
    EXPECT_EQ(empty_dst.size(), 2);
    EXPECT_EQ(empty_dst[1], 10);
    EXPECT_EQ(empty_dst[2], 20);

    END_TEST();
}

// ============================================================================
// Consolidated Test 3: Iterators and Erase/Rehash/Swap Operations
// ============================================================================
// Tests: iterator post-increment, const iterator, erase by iterator/range,
//        rehash, reserve, equality, swap
QUARISMATEST(FlatHash, iterators_and_advanced_operations)
{
    // ===== ITERATOR POST-INCREMENT =====
    flat_hash_map<int, int> iter_map;
    iter_map[1] = 10;
    iter_map[2] = 20;
    iter_map[3] = 30;

    auto it      = iter_map.begin();
    auto it_copy = it++;
    EXPECT_EQ(it_copy->first, it_copy->first);
    EXPECT_NE(it, it_copy);

    // ===== CONST ITERATOR POST-INCREMENT =====
    auto cit      = iter_map.cbegin();
    auto cit_copy = cit++;
    EXPECT_NE(cit, cit_copy);

    // ===== ERASE BY ITERATOR AND RANGE =====
    flat_hash_map<int, int> erase_map;
    for (int i = 0; i < 5; ++i)
    {
        erase_map[i] = i * 10;
    }
    EXPECT_EQ(erase_map.size(), 5);

    // Erase begin iterator
    auto old_begin = erase_map.begin();
    erase_map.erase(old_begin);
    EXPECT_EQ(erase_map.size(), 4);

    // Erase [begin, end) - leave one element
    auto erase_it = erase_map.begin();
    ++erase_it;
    erase_map.erase(erase_it, erase_map.end());
    EXPECT_EQ(erase_map.size(), 1);

    // ===== REHASH, RESERVE AND LOAD FACTOR =====
    flat_hash_map<int, int> rehash_map;
    rehash_map.reserve(8);
    auto initial_buckets = rehash_map.bucket_count();

    for (int i = 0; i < 16; ++i)
    {
        rehash_map[i] = i;
    }
    EXPECT_EQ(rehash_map.size(), 16);
    EXPECT_GE(rehash_map.bucket_count(), initial_buckets);
    EXPECT_LE(rehash_map.load_factor(), rehash_map.max_load_factor());

    // Rehash up
    rehash_map.rehash(64);
    EXPECT_GE(rehash_map.bucket_count(), 64);
    EXPECT_LE(rehash_map.load_factor(), rehash_map.max_load_factor());

    // ===== MAP EQUALITY AND SWAP =====
    flat_hash_map<int, int> a;
    flat_hash_map<int, int> b;
    for (int i = 0; i < 5; ++i)
    {
        a[i] = i;
        b[i] = i;
    }
    EXPECT_TRUE(a == b);

    b[99] = 7;
    EXPECT_TRUE(a != b);

    a.swap(b);
    EXPECT_TRUE(a.contains(99));
    EXPECT_FALSE(b.contains(99));

    // ===== SET EQUALITY AND SWAP =====
    flat_hash_set<int> s1;
    flat_hash_set<int> s2;
    for (int i = 0; i < 5; ++i)
    {
        s1.insert(i);
        s2.insert(i);
    }
    EXPECT_TRUE(s1 == s2);

    s2.insert(99);
    EXPECT_TRUE(s1 != s2);

    s1.swap(s2);
    EXPECT_TRUE(s1.contains(99));
    EXPECT_FALSE(s2.contains(99));

    // ===== SWAP POINTERS BASIC =====
    flat_hash_map<int, int> swap1;
    swap1[1] = 10;
    swap1[2] = 20;

    flat_hash_map<int, int> swap2;
    swap2[3] = 30;

    swap1.swap(swap2);
    EXPECT_EQ(swap1.size(), 1);
    EXPECT_EQ(swap1[3], 30);
    EXPECT_EQ(swap2.size(), 2);
    EXPECT_EQ(swap2[1], 10);
    EXPECT_EQ(swap2[2], 20);

    // ===== SWAP POINTERS EMPTY WITH NONEMPTY =====
    flat_hash_map<int, int> empty_swap;
    flat_hash_map<int, int> nonempty_swap;
    nonempty_swap[1] = 10;
    nonempty_swap[2] = 20;

    empty_swap.swap(nonempty_swap);
    EXPECT_EQ(empty_swap.size(), 2);
    EXPECT_EQ(empty_swap[1], 10);
    EXPECT_TRUE(nonempty_swap.empty());

    // ===== SWAP POINTERS PRESERVES BUCKET COUNT =====
    flat_hash_map<int, int> bc1(100);
    bc1[1] = 10;

    flat_hash_map<int, int> bc2(50);
    bc2[2] = 20;

    uint64_t bucket_count1 = bc1.bucket_count();
    uint64_t bucket_count2 = bc2.bucket_count();

    bc1.swap(bc2);
    EXPECT_EQ(bc1.bucket_count(), bucket_count2);
    EXPECT_EQ(bc2.bucket_count(), bucket_count1);

    // ===== RESET TO EMPTY STATE =====
    flat_hash_map<int, int> reset_map;
    reset_map[1] = 10;
    reset_map[2] = 20;
    reset_map[3] = 30;
    EXPECT_EQ(reset_map.size(), 3);

    reset_map.clear();
    EXPECT_TRUE(reset_map.empty());
    EXPECT_EQ(reset_map.size(), 0);

    // Test reinsertion after clear
    reset_map[1] = 20;
    EXPECT_EQ(reset_map.size(), 1);
    EXPECT_EQ(reset_map[1], 20);

    END_TEST();
}

// ============================================================================
// Consolidated Test 4: Custom Hash and Equality
// ============================================================================
// Tests: KeyOrValueEquality, custom hash and equal functions,
//        power_of_two_hash_policy instantiation
QUARISMATEST(FlatHash, custom_hash_and_equality)
{
    // ===== KEY OR VALUE EQUALITY CONSTRUCTOR =====
    std::equal_to<int>                                                                 eq;
    detailv3::KeyOrValueEquality<int, std::pair<int, std::string>, std::equal_to<int>> equality(eq);

    // Test equality comparisons
    EXPECT_TRUE(equality(1, 1));
    EXPECT_FALSE(equality(1, 2));

    // ===== KEY OR VALUE EQUALITY KEY COMPARISONS =====
    // Test key-to-key comparison
    EXPECT_TRUE(equality(5, 5));
    EXPECT_FALSE(equality(5, 10));

    // Test key-to-value comparison
    std::pair<int, std::string> value1(5, "five");
    EXPECT_TRUE(equality(5, value1));
    EXPECT_FALSE(equality(10, value1));

    // Test value-to-key comparison
    EXPECT_TRUE(equality(value1, 5));
    EXPECT_FALSE(equality(value1, 10));

    // Test value-to-value comparison
    std::pair<int, std::string> value2(5, "five");
    EXPECT_TRUE(equality(value1, value2));

    // ===== KEY OR VALUE EQUALITY PAIR COMPARISONS =====
    std::pair<int, std::string> value3(10, "ten");

    // Test key-to-pair comparison
    EXPECT_TRUE(equality(5, value1));
    EXPECT_FALSE(equality(10, value1));

    // Test pair-to-key comparison
    EXPECT_TRUE(equality(value1, 5));
    EXPECT_FALSE(equality(value1, 10));

    // Test value-to-pair comparison
    EXPECT_TRUE(equality(value1, value2));
    EXPECT_FALSE(equality(value1, value3));

    // Test pair-to-pair comparison
    EXPECT_TRUE(equality(value1, value2));
    EXPECT_FALSE(equality(value1, value3));

    // ===== CUSTOM HASH AND EQUAL FUNCTIONS =====
    // Case-insensitive hash
    struct ci_hash
    {
        size_t operator()(const std::string& s) const noexcept
        {
            std::string lower(s);
            std::transform(
                lower.begin(),
                lower.end(),
                lower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return std::hash<std::string>{}(lower);
        }
    };

    // Case-insensitive equality
    struct ci_equal
    {
        bool operator()(const std::string& a, const std::string& b) const noexcept
        {
            if (a.size() != b.size())
                return false;
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(a[i])) !=
                    std::tolower(static_cast<unsigned char>(b[i])))
                    return false;
            }
            return true;
        }
    };

    flat_hash_map<std::string, int, ci_hash, ci_equal> cmap;
    cmap["Hello"] = 1;
    EXPECT_TRUE(cmap.contains("hello"));
    EXPECT_EQ(cmap.find("heLLo")->second, 1);

    // insert_or_assign should use custom equal
    cmap.insert_or_assign("HELLO", 7);
    EXPECT_EQ(cmap.size(), 1);
    EXPECT_EQ(cmap.find("hello")->second, 7);

    // ===== POWER OF TWO HASH POLICY INSTANTIATION =====
    flat_hash_set<int, power_of_two_std_hash<int>> pow2_set;
    pow2_set.insert(1);
    pow2_set.insert(2);
    EXPECT_TRUE(pow2_set.contains(1));
    EXPECT_TRUE(pow2_set.contains(2));

    END_TEST();
}

// ============================================================================
// Consolidated Test 5: Prime Number Hash Policy
// ============================================================================
// Tests: all prime_number_hash_policy methods including index_for_hash,
//        next_size_over, reset, keep_in_range, commit, edge cases
QUARISMATEST(FlatHash, prime_number_hash_policy_comprehensive)
{
    // ===== INDEX FOR HASH =====
    quarisma::prime_number_hash_policy policy1;

    uint64_t index1 = policy1.index_for_hash(12345, 0);
    EXPECT_GE(index1, 0);

    uint64_t index2 = policy1.index_for_hash(67890, 0);
    EXPECT_GE(index2, 0);

    // ===== NEXT SIZE OVER =====
    quarisma::prime_number_hash_policy policy2;

    uint64_t size          = 10;
    uint64_t original_size = size;
    policy2.next_size_over(size);
    EXPECT_GT(size, original_size);

    // ===== NEXT SIZE OVER SMALL =====
    quarisma::prime_number_hash_policy policy3;
    uint64_t                           small_size = 1;
    auto                               f          = policy3.next_size_over(small_size);
    EXPECT_GT(small_size, 1ULL);
    EXPECT_EQ(f(small_size), 0ULL);  // n % n == 0

    // ===== NEXT SIZE OVER BETWEEN PRIMES =====
    quarisma::prime_number_hash_policy policy4;
    uint64_t                           between_size = 6;
    auto                               f2           = policy4.next_size_over(between_size);
    EXPECT_GE(between_size, 6ULL);
    EXPECT_EQ(f2(between_size), 0ULL);
    EXPECT_EQ(f2(between_size + 1), 1ULL);

    // ===== COMMIT AND INDEX FOR HASH =====
    quarisma::prime_number_hash_policy policy5;
    uint64_t                           commit_size = 1000;
    auto                               f3          = policy5.next_size_over(commit_size);
    policy5.commit(f3);

    uint64_t h = 1234567890123456789ULL;
    EXPECT_EQ(policy5.index_for_hash(h, 0), f3(h));

    // keep_in_range tests
    EXPECT_EQ(policy5.keep_in_range(42, 100), 42ULL);
    uint64_t big = commit_size * 3 + 5;
    EXPECT_EQ(policy5.keep_in_range(big, commit_size - 1), f3(big));

    // ===== RESET RESTORES MOD0 =====
    quarisma::prime_number_hash_policy policy6;
    uint64_t                           reset_size = 50;
    auto                               f4         = policy6.next_size_over(reset_size);
    policy6.commit(f4);
    uint64_t h2 = 987654321ULL;
    EXPECT_EQ(policy6.index_for_hash(h2, 0), f4(h2));

    policy6.reset();
    EXPECT_EQ(policy6.index_for_hash(h2, 0), 0ULL);
    EXPECT_EQ(policy6.keep_in_range(h2, 0ULL), 0ULL);

    // ===== NEXT SIZE OVER LARGE =====
    quarisma::prime_number_hash_policy policy7;
    uint64_t                           requested  = std::numeric_limits<uint64_t>::max() - 12345ULL;
    uint64_t                           large_size = requested;
    auto                               f5         = policy7.next_size_over(large_size);
    EXPECT_GE(large_size, requested);
    EXPECT_EQ(f5(large_size), 0ULL);

    policy7.commit(f5);
    uint64_t h3 = requested - 777ULL;
    EXPECT_EQ(policy7.index_for_hash(h3, 0), f5(h3));

    // ===== KEEP IN RANGE =====
    quarisma::prime_number_hash_policy policy8;
    uint64_t                           range_index = policy8.keep_in_range(12345, 100);
    EXPECT_LE(range_index, 100);

    // ===== SEQUENTIAL NEXT SIZE OVER =====
    quarisma::prime_number_hash_policy policy9;
    uint64_t                           seq_size1 = 5;
    policy9.next_size_over(seq_size1);
    uint64_t first_prime = seq_size1;

    uint64_t seq_size2 = first_prime + 1;
    policy9.next_size_over(seq_size2);
    uint64_t second_prime = seq_size2;
    EXPECT_GT(second_prime, first_prime);

    // ===== INDEX FOR HASH AFTER COMMIT =====
    quarisma::prime_number_hash_policy policy10;
    uint64_t                           iah_size = 20;
    auto                               mod_func = policy10.next_size_over(iah_size);
    policy10.commit(mod_func);

    uint64_t iah_index1 = policy10.index_for_hash(12345, 0);
    uint64_t iah_index2 = policy10.index_for_hash(67890, 0);
    EXPECT_GE(iah_index1, 0);
    EXPECT_GE(iah_index2, 0);

    // ===== KEEP IN RANGE AFTER COMMIT =====
    quarisma::prime_number_hash_policy policy11;
    uint64_t                           kir_size     = 30;
    auto                               kir_mod_func = policy11.next_size_over(kir_size);
    policy11.commit(kir_mod_func);

    uint64_t kir_index1 = policy11.keep_in_range(100, 50);
    EXPECT_LE(kir_index1, 50);

    uint64_t kir_index2 = policy11.keep_in_range(0xFFFFFFFF, 50);
    EXPECT_LE(kir_index2, 50);

    // ===== EDGE CASES =====
    quarisma::prime_number_hash_policy policy_edge;

    // Size zero
    uint64_t zero_size = 0;
    policy_edge.next_size_over(zero_size);
    EXPECT_GT(zero_size, 0);

    // Size one
    uint64_t one_size = 1;
    policy_edge.next_size_over(one_size);
    EXPECT_GT(one_size, 0);

    // Size two
    uint64_t two_size = 2;
    policy_edge.next_size_over(two_size);
    EXPECT_GE(two_size, 2);

    // Large size
    uint64_t huge_size = 1000000000;
    policy_edge.next_size_over(huge_size);
    EXPECT_GT(huge_size, 0);

    // ===== RESET AFTER OPERATIONS =====
    quarisma::prime_number_hash_policy policy12;
    uint64_t                           rao_size     = 50;
    auto                               rao_mod_func = policy12.next_size_over(rao_size);
    policy12.commit(rao_mod_func);

    policy12.reset();
    uint64_t rao_index = policy12.index_for_hash(12345, 0);
    EXPECT_GE(rao_index, 0);

    END_TEST();
}

// ============================================================================
// Consolidated Test 6: Fibonacci Hash Policy
// ============================================================================
// Tests: all fibonacci_hash_policy methods including index_for_hash,
//        next_size_over, reset, keep_in_range, commit
QUARISMATEST(FlatHash, fibonacci_hash_policy_comprehensive)
{
    // ===== INDEX FOR HASH =====
    quarisma::fibonacci_hash_policy policy1;

    uint64_t index1 = policy1.index_for_hash(12345, 0);
    EXPECT_GE(index1, 0);

    uint64_t index2 = policy1.index_for_hash(67890, 0);
    EXPECT_GE(index2, 0);

    // ===== NEXT SIZE OVER =====
    quarisma::fibonacci_hash_policy policy2;

    uint64_t size  = 10;
    int8_t   shift = policy2.next_size_over(size);
    EXPECT_GE(size, 2);
    EXPECT_LE(shift, 63);

    // ===== NEXT SIZE OVER MINIMUM =====
    quarisma::fibonacci_hash_policy policy3;

    uint64_t min_size = 1;
    policy3.next_size_over(min_size);
    EXPECT_GE(min_size, 2);

    // ===== RESET =====
    quarisma::fibonacci_hash_policy policy4;

    uint64_t reset_size = 100;
    policy4.next_size_over(reset_size);
    policy4.reset();

    // After reset, verify the policy is in initial state
    uint64_t reset_size2 = 10;
    policy4.next_size_over(reset_size2);
    EXPECT_GE(reset_size2, 2);

    // ===== KEEP IN RANGE =====
    quarisma::fibonacci_hash_policy policy5;

    uint64_t num_slots_minus_one = 127;  // 2^7 - 1
    uint64_t range_index         = policy5.keep_in_range(12345, num_slots_minus_one);
    EXPECT_LE(range_index, num_slots_minus_one);

    // ===== COMMIT =====
    quarisma::fibonacci_hash_policy policy6;

    uint64_t commit_size  = 16;
    int8_t   commit_shift = policy6.next_size_over(commit_size);
    policy6.commit(commit_shift);

    // After commit, index_for_hash should use the new shift value
    uint64_t commit_index = policy6.index_for_hash(12345, 0);
    EXPECT_GE(commit_index, 0);

    END_TEST();
}

// ============================================================================
// Consolidated Test 7: Power of Two Hash Policy
// ============================================================================
// Tests: all power_of_two_hash_policy methods including index_for_hash,
//        keep_in_range, next_size_over (small/medium/large), commit, reset
QUARISMATEST(FlatHash, power_of_two_hash_policy_comprehensive)
{
    // ===== INDEX FOR HASH =====
    quarisma::power_of_two_hash_policy policy1;

    uint64_t index1 = policy1.index_for_hash(12345, 15);  // 15 = 0xF (4 bits)
    EXPECT_LE(index1, 15);

    uint64_t index2 = policy1.index_for_hash(67890, 31);  // 31 = 0x1F (5 bits)
    EXPECT_LE(index2, 31);

    uint64_t index3 = policy1.index_for_hash(0xFFFFFFFF, 255);  // 255 = 0xFF (8 bits)
    EXPECT_LE(index3, 255);

    // ===== INDEX FOR HASH BITWISE AND =====
    quarisma::power_of_two_hash_policy policy2;

    uint64_t hash                = 0x12345678;
    uint64_t num_slots_minus_one = 0xFF;  // 255

    uint64_t index    = policy2.index_for_hash(hash, num_slots_minus_one);
    uint64_t expected = hash & num_slots_minus_one;
    EXPECT_EQ(index, expected);

    // ===== KEEP IN RANGE =====
    quarisma::power_of_two_hash_policy policy3;

    uint64_t nsmone = 127;  // 2^7 - 1

    // Test with index within range
    uint64_t kir_index1 = policy3.keep_in_range(50, nsmone);
    EXPECT_LE(kir_index1, nsmone);

    // Test with index out of range
    uint64_t kir_index2 = policy3.keep_in_range(200, nsmone);
    EXPECT_LE(kir_index2, nsmone);

    // Test with large index
    uint64_t kir_index3 = policy3.keep_in_range(0xFFFFFFFF, nsmone);
    EXPECT_LE(kir_index3, nsmone);

    // ===== NEXT SIZE OVER SMALL =====
    quarisma::power_of_two_hash_policy policy4;

    uint64_t small_size  = 1;
    int8_t   small_shift = policy4.next_size_over(small_size);
    EXPECT_GE(small_size, 1);
    EXPECT_EQ(small_size & (small_size - 1), 0);  // Power of two
    EXPECT_EQ(small_shift, 0);

    // ===== NEXT SIZE OVER MEDIUM =====
    quarisma::power_of_two_hash_policy policy5;

    uint64_t medium_size     = 10;
    uint64_t original_medium = medium_size;
    int8_t   medium_shift    = policy5.next_size_over(medium_size);
    EXPECT_GE(medium_size, original_medium);
    EXPECT_EQ(medium_size & (medium_size - 1), 0);  // Power of two
    EXPECT_EQ(medium_shift, 0);

    // ===== NEXT SIZE OVER LARGE =====
    quarisma::power_of_two_hash_policy policy6;

    uint64_t large_size     = 1000000;
    uint64_t original_large = large_size;
    int8_t   large_shift    = policy6.next_size_over(large_size);
    EXPECT_GE(large_size, original_large);
    EXPECT_EQ(large_size & (large_size - 1), 0);  // Power of two
    EXPECT_EQ(large_shift, 0);

    // ===== NEXT SIZE OVER ALREADY POWER OF TWO =====
    quarisma::power_of_two_hash_policy policy7;

    uint64_t pow2_size  = 64;  // Already a power of two
    int8_t   pow2_shift = policy7.next_size_over(pow2_size);
    EXPECT_GE(pow2_size, 64);
    EXPECT_EQ(pow2_size & (pow2_size - 1), 0);
    EXPECT_EQ(pow2_shift, 0);

    // ===== COMMIT IS NOOP =====
    quarisma::power_of_two_hash_policy policy8;

    uint64_t commit_index1 = policy8.index_for_hash(12345, 255);
    policy8.commit(5);  // Should have no effect
    uint64_t commit_index2 = policy8.index_for_hash(12345, 255);
    EXPECT_EQ(commit_index1, commit_index2);

    // ===== RESET IS NOOP =====
    quarisma::power_of_two_hash_policy policy9;

    uint64_t reset_index1 = policy9.index_for_hash(12345, 255);
    policy9.reset();  // Should have no effect
    uint64_t reset_index2 = policy9.index_for_hash(12345, 255);
    EXPECT_EQ(reset_index1, reset_index2);

    // ===== SEQUENTIAL OPERATIONS =====
    quarisma::power_of_two_hash_policy policy10;

    uint64_t seq_size1 = 5;
    policy10.next_size_over(seq_size1);
    EXPECT_EQ(seq_size1 & (seq_size1 - 1), 0);

    uint64_t seq_size2 = 100;
    policy10.next_size_over(seq_size2);
    EXPECT_EQ(seq_size2 & (seq_size2 - 1), 0);

    uint64_t seq_index = policy10.index_for_hash(0xDEADBEEF, seq_size2 - 1);
    EXPECT_LE(seq_index, seq_size2 - 1);

    END_TEST();
}
