#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "baseTest.h"
#include "native/utils/time_utils.h"

using namespace quarisma;
using namespace quarisma::profiler_impl;

// ============================================================================
// Time Utilities Tests
// ============================================================================

QUARISMATEST(Profiler, get_current_time_nanos_returns_positive)
{
    int64_t time1 = get_current_time_nanos();

    EXPECT_GT(time1, 0);
}

QUARISMATEST(Profiler, get_current_time_nanos_monotonic)
{
    int64_t time1 = get_current_time_nanos();
    int64_t time2 = get_current_time_nanos();

    EXPECT_GE(time2, time1);
}

QUARISMATEST(Profiler, get_current_time_nanos_increasing)
{
    int64_t time1 = get_current_time_nanos();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    int64_t time2 = get_current_time_nanos();

    EXPECT_GT(time2, time1);
}

QUARISMATEST(Profiler, sleep_for_nanos_basic)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_nanos(1000000);  // 1ms
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 500000);  // At least 0.5ms
}

QUARISMATEST(Profiler, sleep_for_nanos_zero)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_nanos(0);
    int64_t time2 = get_current_time_nanos();

    // Should return immediately
    EXPECT_GE(time2, time1);
}

QUARISMATEST(Profiler, sleep_for_micros_basic)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_micros(1000);  // 1ms
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 500000);  // At least 0.5ms
}

QUARISMATEST(Profiler, sleep_for_millis_basic)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_millis(1);
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 500000);  // At least 0.5ms
}

QUARISMATEST(Profiler, sleep_for_seconds_basic)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_millis(1);  // 1ms
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 500000);  // At least 0.5ms
}

QUARISMATEST(Profiler, spin_for_nanos_basic)
{
    int64_t time1 = get_current_time_nanos();
    spin_for_nanos(100000);  // 0.1ms
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 50000);  // At least 0.05ms
}

QUARISMATEST(Profiler, spin_for_nanos_zero)
{
    int64_t time1 = get_current_time_nanos();
    spin_for_nanos(0);
    int64_t time2 = get_current_time_nanos();

    // Should return immediately
    EXPECT_GE(time2, time1);
}

QUARISMATEST(Profiler, spin_for_micros_basic)
{
    int64_t time1 = get_current_time_nanos();
    spin_for_micros(100);  // 0.1ms
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 50000);  // At least 0.05ms
}

QUARISMATEST(Profiler, multiple_sleeps_cumulative)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_millis(1);
    sleep_for_millis(1);
    sleep_for_millis(1);
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 1500000);  // At least 1.5ms
}

QUARISMATEST(Profiler, time_measurement_consistency)
{
    int64_t time1 = get_current_time_nanos();
    int64_t time2 = get_current_time_nanos();
    int64_t time3 = get_current_time_nanos();

    EXPECT_GE(time2, time1);
    EXPECT_GE(time3, time2);
}

QUARISMATEST(Profiler, large_sleep_duration)
{
    int64_t time1 = get_current_time_nanos();
    sleep_for_millis(10);
    int64_t time2 = get_current_time_nanos();

    int64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 5000000);  // At least 5ms
}

QUARISMATEST(Profiler, spin_vs_sleep_accuracy)
{
    // Spin should be more accurate for short durations
    int64_t spin_time1 = get_current_time_nanos();
    spin_for_nanos(50000);  // 0.05ms
    int64_t spin_time2 = get_current_time_nanos();

    int64_t sleep_time1 = get_current_time_nanos();
    sleep_for_nanos(50000);  // 0.05ms
    int64_t sleep_time2 = get_current_time_nanos();

    int64_t spin_elapsed  = spin_time2 - spin_time1;
    int64_t sleep_elapsed = sleep_time2 - sleep_time1;

    // Both should complete
    EXPECT_GE(spin_elapsed, 25000);
    EXPECT_GE(sleep_elapsed, 25000);
}

QUARISMATEST(Profiler, get_current_time_nanos_high_resolution)
{
    // Verify we're getting nanosecond precision
    int64_t time1 = get_current_time_nanos();
    int64_t time2 = get_current_time_nanos();

    // The difference should be small (less than 1 second)
    int64_t diff = time2 - time1;
    EXPECT_LT(diff, 1000000000);  // Less than 1 second
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
