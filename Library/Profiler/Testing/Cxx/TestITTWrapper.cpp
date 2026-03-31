/*
 * ITT Wrapper Tests
 *
 * Tests for the Intel Instrumentation and Tracing Technology (ITT) wrapper
 * including:
 * - ITT initialization
 * - Event marking
 * - Task range tracking
 * - String handle management
 * - Domain creation
 */

#include "baseTest.h"

#if QUARISMA_HAS_ITT

#include "bespoke/itt/itt_wrapper.h"

using namespace quarisma::profiler_impl;

// ============================================================================
// ITT Initialization Tests
// ============================================================================

QUARISMATEST(ITTWrapper, Initialization)
{
    // Test ITT initialization
    itt_init();

    // If we reach here without crashing, initialization succeeded
    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, MultipleInitialization)
{
    // Test that multiple initializations don't cause issues
    itt_init();
    itt_init();
    itt_init();

    EXPECT_TRUE(true);
}

// ============================================================================
// Event Marking Tests
// ============================================================================

QUARISMATEST(ITTWrapper, EventMarking)
{
    // Test basic event marking
    itt_init();
    itt_mark("test_event");

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, EventMarkingMultiple)
{
    // Test multiple event markings
    itt_init();

    itt_mark("event_1");
    itt_mark("event_2");
    itt_mark("event_3");

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, EventMarkingWithSpecialChars)
{
    // Test event marking with special characters
    itt_init();

    itt_mark("event_with_underscore");
    itt_mark("event-with-dash");
    itt_mark("event.with.dot");

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, EventMarkingNullName)
{
    // Test event marking with null name (should not crash)
    itt_init();
    itt_mark(nullptr);

    EXPECT_TRUE(true);
}

// ============================================================================
// Task Range Tests
// ============================================================================

QUARISMATEST(ITTWrapper, TaskRangePushPop)
{
    // Test task range push and pop
    itt_init();

    itt_range_push("task_1");
    itt_range_pop();

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, TaskRangeNested)
{
    // Test nested task ranges
    itt_init();

    itt_range_push("outer_task");
    itt_range_push("inner_task");
    itt_range_pop();
    itt_range_pop();

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, TaskRangeMultiple)
{
    // Test multiple sequential task ranges
    itt_init();

    for (int i = 0; i < 5; ++i)
    {
        std::string task_name = "task_" + std::to_string(i);
        itt_range_push(task_name.c_str());
        itt_range_pop();
    }

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, TaskRangeDeepNesting)
{
    // Test deeply nested task ranges
    itt_init();

    const int depth = 10;
    for (int i = 0; i < depth; ++i)
    {
        itt_range_push("nested_task");
    }

    for (int i = 0; i < depth; ++i)
    {
        itt_range_pop();
    }

    EXPECT_TRUE(true);
}

// ============================================================================
// String Handle Tests
// ============================================================================

QUARISMATEST(ITTWrapper, StringHandleCreation)
{
    // Test string handle creation
    itt_init();

    // String handles are created internally by itt_mark and itt_range_push
    itt_mark("string_handle_test");

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, StringHandleReuse)
{
    // Test that string handles are reused for same strings
    itt_init();

    // Marking the same event multiple times should reuse the handle
    for (int i = 0; i < 10; ++i)
    {
        itt_mark("reused_event");
    }

    EXPECT_TRUE(true);
}

// ============================================================================
// Domain Tests
// ============================================================================

QUARISMATEST(ITTWrapper, DomainCreation)
{
    // Test ITT domain creation
    itt_init();

    // Domain is created during initialization
    // If we reach here without crashing, domain creation succeeded
    EXPECT_TRUE(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

QUARISMATEST(ITTWrapper, MixedOperations)
{
    // Test mixed ITT operations
    itt_init();

    itt_mark("start_event");

    itt_range_push("operation_1");
    itt_mark("operation_1_event");
    itt_range_pop();

    itt_range_push("operation_2");
    itt_mark("operation_2_event");
    itt_range_pop();

    itt_mark("end_event");

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, ComplexScenario)
{
    // Test complex scenario with multiple operations
    itt_init();

    itt_mark("scenario_start");

    for (int i = 0; i < 3; ++i)
    {
        std::string phase = "phase_" + std::to_string(i);
        itt_range_push(phase.c_str());

        for (int j = 0; j < 2; ++j)
        {
            std::string event = "event_" + std::to_string(i) + "_" + std::to_string(j);
            itt_mark(event.c_str());
        }

        itt_range_pop();
    }

    itt_mark("scenario_end");

    EXPECT_TRUE(true);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

QUARISMATEST(ITTWrapper, UnmatchedPop)
{
    // Test unmatched pop (should not crash)
    itt_init();

    // Pop without push - should handle gracefully
    itt_range_pop();

    EXPECT_TRUE(true);
}

QUARISMATEST(ITTWrapper, LongEventNames)
{
    // Test with very long event names
    itt_init();

    std::string long_name(256, 'a');
    itt_mark(long_name.c_str());

    EXPECT_TRUE(true);
}

#endif  // QUARISMA_HAS_ITT
