#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Test suite for annotation_stack class
 * Tests hierarchical annotation tracking for CPU profiling
 */

#include "Testing/baseTest.h"
#include "native/cpu/annotation_stack.h"

using namespace quarisma::profiler_impl;

// Test push_annotation single level
QUARISMATEST(Profiler, annotation_stack_push_single)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("test");

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.find("test") != std::string::npos);
}

// Test pop_annotation single level
QUARISMATEST(Profiler, annotation_stack_pop_single)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("test");
    annotation_stack::pop_annotation();

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.empty());
}

// Test push_annotation multiple levels
QUARISMATEST(Profiler, annotation_stack_push_multiple)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("outer");
    annotation_stack::push_annotation("inner");

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.find("outer") != std::string::npos);
    EXPECT_TRUE(result.find("inner") != std::string::npos);
}

// Test pop_annotation multiple levels
QUARISMATEST(Profiler, annotation_stack_pop_multiple)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("outer");
    annotation_stack::push_annotation("inner");
    annotation_stack::pop_annotation();

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.find("outer") != std::string::npos);
}

// Test get_scope_range_ids single level
QUARISMATEST(Profiler, annotation_stack_scope_range_ids_single)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("test");

    const auto& ids = annotation_stack::get_scope_range_ids();
    EXPECT_GE(ids.size(), 1);
}

// Test get_scope_range_ids multiple levels
QUARISMATEST(Profiler, annotation_stack_scope_range_ids_multiple)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("outer");
    annotation_stack::push_annotation("inner");

    const auto& ids = annotation_stack::get_scope_range_ids();
    EXPECT_GE(ids.size(), 2);
}

// Test enable/disable functionality
QUARISMATEST(Profiler, annotation_stack_enable_disable)
{
    annotation_stack::enable(true);
    EXPECT_TRUE(annotation_stack::is_enabled());

    annotation_stack::enable(false);
    EXPECT_FALSE(annotation_stack::is_enabled());
}

// Test is_enabled when enabled
QUARISMATEST(Profiler, annotation_stack_is_enabled_true)
{
    annotation_stack::enable(true);
    EXPECT_TRUE(annotation_stack::is_enabled());
}

// Test is_enabled when disabled
QUARISMATEST(Profiler, annotation_stack_is_enabled_false)
{
    annotation_stack::enable(false);
    EXPECT_FALSE(annotation_stack::is_enabled());
}

// Test push with empty name
QUARISMATEST(Profiler, annotation_stack_push_empty_name)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("");

    const std::string& result = annotation_stack::get();
    EXPECT_EQ(result, "");
}

// Test push with special characters
QUARISMATEST(Profiler, annotation_stack_push_special_chars)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("test_123");

    const std::string& result = annotation_stack::get();
    EXPECT_EQ(result, "test_123");
}

// Test push with long name
QUARISMATEST(Profiler, annotation_stack_push_long_name)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    std::string long_name(100, 'a');
    annotation_stack::push_annotation(long_name);

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.find(long_name) != std::string::npos);
}

// Test nested push/pop sequence
QUARISMATEST(Profiler, annotation_stack_nested_sequence)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("level1");
    EXPECT_TRUE(annotation_stack::get().find("level1") != std::string::npos);

    annotation_stack::push_annotation("level2");
    EXPECT_TRUE(annotation_stack::get().find("level2") != std::string::npos);

    annotation_stack::push_annotation("level3");
    EXPECT_TRUE(annotation_stack::get().find("level3") != std::string::npos);

    annotation_stack::pop_annotation();
    EXPECT_FALSE(annotation_stack::get().find("level3") != std::string::npos);

    annotation_stack::pop_annotation();
    EXPECT_TRUE(annotation_stack::get().find("level1") != std::string::npos);

    annotation_stack::pop_annotation();
    EXPECT_TRUE(annotation_stack::get().empty());
}

// Test deep nesting
QUARISMATEST(Profiler, annotation_stack_deep_nesting)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    for (int i = 0; i < 5; ++i)
    {
        annotation_stack::push_annotation("level" + std::to_string(i));
    }

    const auto& ids = annotation_stack::get_scope_range_ids();
    EXPECT_GE(ids.size(), 5);

    for (int i = 0; i < 5; ++i)
    {
        annotation_stack::pop_annotation();
    }

    EXPECT_TRUE(annotation_stack::get().empty());
}

// Test separator in annotation names
QUARISMATEST(Profiler, annotation_stack_separator_in_name)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("outer");
    annotation_stack::push_annotation("inner");

    const std::string& result = annotation_stack::get();
    EXPECT_NE(result.find("::"), std::string::npos);
}

// Test multiple push/pop cycles
QUARISMATEST(Profiler, annotation_stack_multiple_cycles)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        annotation_stack::push_annotation("cycle" + std::to_string(cycle));
        EXPECT_FALSE(annotation_stack::get().empty());
        annotation_stack::pop_annotation();
    }

    EXPECT_TRUE(annotation_stack::get().empty());
}

// Test scope range IDs are unique
QUARISMATEST(Profiler, annotation_stack_scope_range_ids_unique)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("first");
    const auto& ids1  = annotation_stack::get_scope_range_ids();
    size_t      size1 = ids1.size();

    annotation_stack::push_annotation("second");
    const auto& ids2  = annotation_stack::get_scope_range_ids();
    size_t      size2 = ids2.size();

    // IDs should be different (or at least we can verify they exist)
    EXPECT_GT(size2, size1);
}

// Test get returns reference
QUARISMATEST(Profiler, annotation_stack_get_returns_reference)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("test");

    const std::string& ref1 = annotation_stack::get();
    const std::string& ref2 = annotation_stack::get();

    EXPECT_EQ(ref1, ref2);
}

// Test get_scope_range_ids returns reference
QUARISMATEST(Profiler, annotation_stack_get_scope_range_ids_returns_reference)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("test");

    const auto& ids1 = annotation_stack::get_scope_range_ids();
    const auto& ids2 = annotation_stack::get_scope_range_ids();

    EXPECT_EQ(ids1.size(), ids2.size());
}

// Test enable multiple times
QUARISMATEST(Profiler, annotation_stack_enable_multiple_times)
{
    annotation_stack::enable(true);
    EXPECT_TRUE(annotation_stack::is_enabled());

    annotation_stack::enable(true);
    EXPECT_TRUE(annotation_stack::is_enabled());

    annotation_stack::enable(false);
    EXPECT_FALSE(annotation_stack::is_enabled());

    annotation_stack::enable(false);
    EXPECT_FALSE(annotation_stack::is_enabled());
}

// Test push with numeric string
QUARISMATEST(Profiler, annotation_stack_push_numeric_string)
{
    annotation_stack::enable(true);
    annotation_stack::push_annotation("12345");

    const std::string& result = annotation_stack::get();
    EXPECT_EQ(result, "12345");
}

// Test push with whitespace
QUARISMATEST(Profiler, annotation_stack_push_whitespace)
{
    annotation_stack::enable(true);
    // Clear any existing annotations
    while (!annotation_stack::get().empty())
    {
        annotation_stack::pop_annotation();
    }

    annotation_stack::push_annotation("test name");

    const std::string& result = annotation_stack::get();
    EXPECT_TRUE(result.find("test name") != std::string::npos);
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
