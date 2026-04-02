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

#ifndef QUARISMA_TEST_H
#define QUARISMA_TEST_H

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/configure.h"
#include "common/macros.h"
#include "logging/logger.h"

// Include Google Test if available
#if QUARISMA_HAS_GTEST
#include <gtest/gtest.h>
#endif

// Logging macros for tests
#if QUARISMA_HAS_GTEST
// Google Test functions are void, so END_TEST should not return anything
#define END_TEST()
#else
// Non-Google Test functions return int
#define END_TEST() return EXIT_SUCCESS
#endif

// Test assertion macros
#if QUARISMA_HAS_GTEST
// Use Google Test macros when available (Google Test uses EXPECT_*, not GTEST_EXPECT_*)
// Note: The macros are already defined by Google Test, so we don't need to redefine them
// Just ensure they're available by including gtest.h above

// Google Test main function
#define QUARISMATEST(module, name) TEST(module, name)

#define QUARISMATEST_VOID(module, name) TEST(module, name)

#define QUARISMATEST_CALL(module, name)

#else
// Simple assertion macros for non-Google Test builds
#define EXPECT_TRUE(condition)                                                             \
    do                                                                                     \
    {                                                                                      \
        if (!(condition))                                                                  \
        {                                                                                  \
            std::cerr << "EXPECT_TRUE failed: " << #condition << " at " << __FILE__ << ":" \
                      << __LINE__ << std::endl;                                            \
        }                                                                                  \
    } while (0)

#define EXPECT_FALSE(condition)                                                             \
    do                                                                                      \
    {                                                                                       \
        if (condition)                                                                      \
        {                                                                                   \
            std::cerr << "EXPECT_FALSE failed: " << #condition << " at " << __FILE__ << ":" \
                      << __LINE__ << std::endl;                                             \
        }                                                                                   \
    } while (0)

#define EXPECT_NEAR(expected, actual, tolerance)                                            \
    do                                                                                      \
    {                                                                                       \
        if (std::abs((expected) - (actual)) > tolerance)                                    \
        {                                                                                   \
            std::cerr << "EXPECT_NEAR failed: " << #expected << " != " << #actual << " at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                          \
        }                                                                                   \
    } while (0)

#define EXPECT_EQ(expected, actual)                                                       \
    do                                                                                    \
    {                                                                                     \
        if ((expected) != (actual))                                                       \
        {                                                                                 \
            std::cerr << "EXPECT_EQ failed: " << #expected << " != " << #actual << " at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                        \
        }                                                                                 \
    } while (0)

#define EXPECT_NE(expected, actual)                                                       \
    do                                                                                    \
    {                                                                                     \
        if ((expected) == (actual))                                                       \
        {                                                                                 \
            std::cerr << "EXPECT_NE failed: " << #expected << " == " << #actual << " at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                        \
        }                                                                                 \
    } while (0)

#define EXPECT_LT(val1, val2)                                                                   \
    do                                                                                          \
    {                                                                                           \
        if (!((val1) < (val2)))                                                                 \
        {                                                                                       \
            std::cerr << "EXPECT_LT failed: " << #val1 << " >= " << #val2 << " at " << __FILE__ \
                      << ":" << __LINE__ << std::endl;                                          \
        }                                                                                       \
    } while (0)

#define EXPECT_LE(val1, val2)                                                                  \
    do                                                                                         \
    {                                                                                          \
        if (!((val1) <= (val2)))                                                               \
        {                                                                                      \
            std::cerr << "EXPECT_LE failed: " << #val1 << " > " << #val2 << " at " << __FILE__ \
                      << ":" << __LINE__ << std::endl;                                         \
        }                                                                                      \
    } while (0)

#define EXPECT_GT(val1, val2)                                                                   \
    do                                                                                          \
    {                                                                                           \
        if (!((val1) > (val2)))                                                                 \
        {                                                                                       \
            std::cerr << "EXPECT_GT failed: " << #val1 << " <= " << #val2 << " at " << __FILE__ \
                      << ":" << __LINE__ << std::endl;                                          \
        }                                                                                       \
    } while (0)

#define EXPECT_GE(val1, val2)                                                                  \
    do                                                                                         \
    {                                                                                          \
        if (!((val1) >= (val2)))                                                               \
        {                                                                                      \
            std::cerr << "EXPECT_GE failed: " << #val1 << " < " << #val2 << " at " << __FILE__ \
                      << ":" << __LINE__ << std::endl;                                         \
        }                                                                                      \
    } while (0)

#define ASSERT_TRUE(condition)                                                             \
    do                                                                                     \
    {                                                                                      \
        if (!(condition))                                                                  \
        {                                                                                  \
            std::cerr << "ASSERT_TRUE failed: " << #condition << " at " << __FILE__ << ":" \
                      << __LINE__ << std::endl;                                            \
            throw std::runtime_error("Assertion failed: " #condition);                     \
        }                                                                                  \
    } while (0)

#define ASSERT_FALSE(condition)                                                             \
    do                                                                                      \
    {                                                                                       \
        if (condition)                                                                      \
        {                                                                                   \
            std::cerr << "ASSERT_FALSE failed: " << #condition << " at " << __FILE__ << ":" \
                      << __LINE__ << std::endl;                                             \
            throw std::runtime_error("Assertion failed: " #condition);                      \
        }                                                                                   \
    } while (0)

#define ASSERT_EQ(expected, actual)                                                       \
    do                                                                                    \
    {                                                                                     \
        if ((expected) != (actual))                                                       \
        {                                                                                 \
            std::cerr << "ASSERT_EQ failed: " << #expected << " != " << #actual << " at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                        \
            throw std::runtime_error("Assertion failed: " #expected " != " #actual);      \
        }                                                                                 \
    } while (0)

#define ASSERT_NE(expected, actual)                                                       \
    do                                                                                    \
    {                                                                                     \
        if ((expected) == (actual))                                                       \
        {                                                                                 \
            std::cerr << "ASSERT_NE failed: " << #expected << " == " << #actual << " at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                        \
            throw std::runtime_error("Assertion failed: " #expected " == " #actual);      \
        }                                                                                 \
    } while (0)

#define ASSERT_ANY_THROW(statement)                                                        \
    do                                                                                     \
    {                                                                                      \
        bool threw = false;                                                                \
        try                                                                                \
        {                                                                                  \
            statement;                                                                     \
        }                                                                                  \
        catch (...)                                                                        \
        {                                                                                  \
            threw = true;                                                                  \
        }                                                                                  \
        if (!threw)                                                                        \
        {                                                                                  \
            std::cerr << "ASSERT_ANY_THROW failed: " << #statement << " did not throw at " \
                      << __FILE__ << ":" << __LINE__ << std::endl;                         \
            throw std::runtime_error("Assertion failed: " #statement " did not throw");    \
        }                                                                                  \
    } while (0)

// Standard test main function
#define QUARISMATEST(module, testname) \
    int Test##testname(QUARISMA_UNUSED int argc, QUARISMA_UNUSED char* argv[])

#define QUARISMATEST_VOID(module, testname) \
    void test_##module##testname(QUARISMA_UNUSED int argc, QUARISMA_UNUSED char* argv[])

#define QUARISMATEST_CALL(module, testname) test_##module##testname(argc, argv);

#endif

namespace quarisma
{
// Helper function to check memory alignment
inline bool IsAligned(void* ptr, size_t alignment)
{ return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0; }

inline bool ValidateMemory(const void* ptr, size_t size, uint8_t pattern)
{
    const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        if (bytes[i] != pattern)
        {
            return false;
        }
    }
    return true;
}
}  // namespace quarisma

#endif  // QUARISMA_TEST_H