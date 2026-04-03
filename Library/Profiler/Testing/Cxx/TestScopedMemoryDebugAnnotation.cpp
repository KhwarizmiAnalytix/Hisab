#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for scoped_memory_debug_annotation
 * Tests RAII-based memory debug annotations for profiling
 */

#include <string>
#include <thread>

#include "Testing/baseTest.h"
#include "native/memory/scoped_memory_debug_annotation.h"

using namespace quarisma;

// ============================================================================
// Basic Constructor Tests
// ============================================================================

QUARISMATEST(ScopedMemoryDebugAnnotation, constructor_with_op_name)
{
    {
        scoped_memory_debug_annotation annotation("test_op");
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, 0);
        EXPECT_EQ(current.pending_region_type, nullptr);
        EXPECT_EQ(current.pending_data_type, 0);
    }

    // After scope, annotation should be cleared
    const auto& current = scoped_memory_debug_annotation::current_annotation();
    EXPECT_EQ(current.pending_op_name, nullptr);
}

QUARISMATEST(ScopedMemoryDebugAnnotation, constructor_with_op_name_and_step_id)
{
    {
        scoped_memory_debug_annotation annotation("test_op", 42);
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, 42);
        EXPECT_EQ(current.pending_region_type, nullptr);
        EXPECT_EQ(current.pending_data_type, 0);
    }

    // After scope, annotation should be cleared
    const auto& current = scoped_memory_debug_annotation::current_annotation();
    EXPECT_EQ(current.pending_op_name, nullptr);
    EXPECT_EQ(current.pending_step_id, 0);
}

QUARISMATEST(ScopedMemoryDebugAnnotation, constructor_with_region_and_shape)
{
    auto shape_func = []() { return "shape[10,20]"; };

    {
        scoped_memory_debug_annotation annotation(
            "test_op", "region_type", 123, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_STREQ(current.pending_region_type, "region_type");
        EXPECT_EQ(current.pending_data_type, 123);
        EXPECT_EQ(current.pending_shape_func(), "shape[10,20]");
    }

    // After scope, annotation should be cleared
    const auto& current = scoped_memory_debug_annotation::current_annotation();
    EXPECT_EQ(current.pending_op_name, nullptr);
    EXPECT_EQ(current.pending_region_type, nullptr);
}

QUARISMATEST(ScopedMemoryDebugAnnotation, constructor_with_all_parameters)
{
    auto shape_func = []() { return "shape[5,10,15]"; };

    {
        scoped_memory_debug_annotation annotation(
            "test_op", 99, "region_type", 456, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, 99);
        EXPECT_STREQ(current.pending_region_type, "region_type");
        EXPECT_EQ(current.pending_data_type, 456);
        EXPECT_EQ(current.pending_shape_func(), "shape[5,10,15]");
    }

    // After scope, annotation should be cleared
    const auto& current = scoped_memory_debug_annotation::current_annotation();
    EXPECT_EQ(current.pending_op_name, nullptr);
    EXPECT_EQ(current.pending_step_id, 0);
}

// ============================================================================
// Nested Annotation Tests
// ============================================================================

QUARISMATEST(ScopedMemoryDebugAnnotation, nested_annotations_basic)
{
    {
        scoped_memory_debug_annotation outer("outer_op");
        const auto& current1 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current1.pending_op_name, "outer_op");

        {
            scoped_memory_debug_annotation inner("inner_op");
            const auto& current2 = scoped_memory_debug_annotation::current_annotation();
            EXPECT_STREQ(current2.pending_op_name, "inner_op");
        }

        // After inner scope, should restore outer
        const auto& current3 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current3.pending_op_name, "outer_op");
    }

    // After all scopes, should be cleared
    const auto& current = scoped_memory_debug_annotation::current_annotation();
    EXPECT_EQ(current.pending_op_name, nullptr);
}

QUARISMATEST(ScopedMemoryDebugAnnotation, nested_annotations_with_step_id)
{
    {
        scoped_memory_debug_annotation outer("outer_op", 10);
        const auto& current1 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current1.pending_op_name, "outer_op");
        EXPECT_EQ(current1.pending_step_id, 10);

        {
            scoped_memory_debug_annotation inner("inner_op", 20);
            const auto& current2 = scoped_memory_debug_annotation::current_annotation();
            EXPECT_STREQ(current2.pending_op_name, "inner_op");
            EXPECT_EQ(current2.pending_step_id, 20);
        }

        // After inner scope, should restore outer
        const auto& current3 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current3.pending_op_name, "outer_op");
        EXPECT_EQ(current3.pending_step_id, 10);
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, nested_annotations_preserve_parent_op_name)
{
    auto shape_func = []() { return "shape[1,2,3]"; };

    {
        scoped_memory_debug_annotation outer("outer_op", 5);
        const auto& current1 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current1.pending_op_name, "outer_op");
        EXPECT_EQ(current1.pending_step_id, 5);

        {
            // This constructor preserves parent op_name if it exists
            scoped_memory_debug_annotation inner("inner_op", "region", 100, std::move(shape_func));
            const auto& current2 = scoped_memory_debug_annotation::current_annotation();

            // Should preserve parent op_name
            EXPECT_STREQ(current2.pending_op_name, "outer_op");
            EXPECT_STREQ(current2.pending_region_type, "region");
            EXPECT_EQ(current2.pending_data_type, 100);
        }

        // After inner scope, should restore outer
        const auto& current3 = scoped_memory_debug_annotation::current_annotation();
        EXPECT_STREQ(current3.pending_op_name, "outer_op");
        EXPECT_EQ(current3.pending_step_id, 5);
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, nested_annotations_no_parent_op_name)
{
    auto shape_func = []() { return "shape[4,5,6]"; };

    {
        // No parent annotation
        scoped_memory_debug_annotation annotation("new_op", "region", 200, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        // Should use provided op_name since no parent exists
        EXPECT_STREQ(current.pending_op_name, "new_op");
        EXPECT_STREQ(current.pending_region_type, "region");
        EXPECT_EQ(current.pending_data_type, 200);
    }
}

// ============================================================================
// Thread-Local Storage Tests
// ============================================================================

QUARISMATEST(ScopedMemoryDebugAnnotation, thread_local_isolation)
{
    std::string thread1_op;
    std::string thread2_op;

    std::thread t1(
        [&thread1_op]()
        {
            scoped_memory_debug_annotation annotation("thread1_op");
            const auto& current = scoped_memory_debug_annotation::current_annotation();
            if (current.pending_op_name)
                thread1_op = current.pending_op_name;
        });

    std::thread t2(
        [&thread2_op]()
        {
            scoped_memory_debug_annotation annotation("thread2_op");
            const auto& current = scoped_memory_debug_annotation::current_annotation();
            if (current.pending_op_name)
                thread2_op = current.pending_op_name;
        });

    t1.join();
    t2.join();

    EXPECT_EQ(thread1_op, "thread1_op");
    EXPECT_EQ(thread2_op, "thread2_op");
}

// ============================================================================
// Shape Function Tests
// ============================================================================

QUARISMATEST(ScopedMemoryDebugAnnotation, default_shape_function)
{
    {
        scoped_memory_debug_annotation annotation("test_op");
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        // Default shape function returns empty string
        EXPECT_EQ(current.pending_shape_func(), "");
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, custom_shape_function)
{
    auto shape_func = []() { return "custom_shape[100,200,300]"; };

    {
        scoped_memory_debug_annotation annotation("test_op", "region", 1, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_EQ(current.pending_shape_func(), "custom_shape[100,200,300]");
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, shape_function_with_capture)
{
    int  dim1 = 10, dim2 = 20, dim3 = 30;
    auto shape_func = [dim1, dim2, dim3]()
    {
        return "shape[" + std::to_string(dim1) + "," + std::to_string(dim2) + "," +
               std::to_string(dim3) + "]";
    };

    {
        scoped_memory_debug_annotation annotation("test_op", 1, "region", 2, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_EQ(current.pending_shape_func(), "shape[10,20,30]");
    }
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

QUARISMATEST(ScopedMemoryDebugAnnotation, zero_step_id)
{
    {
        scoped_memory_debug_annotation annotation("test_op", 0);
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, 0);
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, negative_step_id)
{
    {
        scoped_memory_debug_annotation annotation("test_op", -1);
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, -1);
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, large_step_id)
{
    {
        scoped_memory_debug_annotation annotation("test_op", 9223372036854775807LL);
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_STREQ(current.pending_op_name, "test_op");
        EXPECT_EQ(current.pending_step_id, 9223372036854775807LL);
    }
}

QUARISMATEST(ScopedMemoryDebugAnnotation, zero_data_type)
{
    auto shape_func = []() { return ""; };

    {
        scoped_memory_debug_annotation annotation("test_op", "region", 0, std::move(shape_func));
        const auto& current = scoped_memory_debug_annotation::current_annotation();

        EXPECT_EQ(current.pending_data_type, 0);
    }
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
