#if QUARISMA_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Test suite for timespan class
 * Tests time interval utilities for profiling
 */

#include "Testing/baseTest.h"
#include "native/core/timespan.h"

using namespace quarisma;

// Test default constructor
QUARISMATEST(Profiler, timespan_default_constructor)
{
    timespan ts;
    EXPECT_EQ(ts.begin_ps(), 0);
    EXPECT_EQ(ts.duration_ps(), 0);
}

// Test constructor with begin and duration
QUARISMATEST(Profiler, timespan_constructor_with_values)
{
    timespan ts(100, 50);
    EXPECT_EQ(ts.begin_ps(), 100);
    EXPECT_EQ(ts.duration_ps(), 50);
}

// Test begin_ps accessor
QUARISMATEST(Profiler, timespan_begin_ps)
{
    timespan ts(1000, 500);
    EXPECT_EQ(ts.begin_ps(), 1000);
}

// Test duration_ps accessor
QUARISMATEST(Profiler, timespan_duration_ps)
{
    timespan ts(1000, 500);
    EXPECT_EQ(ts.duration_ps(), 500);
}

// Test end_ps calculation
QUARISMATEST(Profiler, timespan_end_ps)
{
    timespan ts(1000, 500);
    EXPECT_EQ(ts.end_ps(), 1500);
}

// Test middle_ps calculation
QUARISMATEST(Profiler, timespan_middle_ps)
{
    timespan ts(1000, 500);
    EXPECT_EQ(ts.middle_ps(), 1250);
}

// Test instant detection
QUARISMATEST(Profiler, timespan_instant)
{
    timespan instant(1000, 0);
    timespan non_instant(1000, 100);

    EXPECT_TRUE(instant.instant());
    EXPECT_FALSE(non_instant.instant());
}

// Test empty detection
QUARISMATEST(Profiler, timespan_empty)
{
    timespan empty;
    timespan non_empty(100, 50);

    EXPECT_TRUE(empty.empty());
    EXPECT_FALSE(non_empty.empty());
}

// Test overlaps - overlapping timespans
QUARISMATEST(Profiler, timespan_overlaps_true)
{
    timespan ts1(100, 100);  // [100, 200]
    timespan ts2(150, 100);  // [150, 250]

    EXPECT_TRUE(ts1.overlaps(ts2));
    EXPECT_TRUE(ts2.overlaps(ts1));
}

// Test overlaps - non-overlapping timespans
QUARISMATEST(Profiler, timespan_overlaps_false)
{
    timespan ts1(100, 50);  // [100, 150]
    timespan ts2(200, 50);  // [200, 250]

    EXPECT_FALSE(ts1.overlaps(ts2));
    EXPECT_FALSE(ts2.overlaps(ts1));
}

// Test overlaps - adjacent timespans
QUARISMATEST(Profiler, timespan_overlaps_adjacent)
{
    timespan ts1(100, 50);  // [100, 150]
    timespan ts2(150, 50);  // [150, 200]

    EXPECT_TRUE(ts1.overlaps(ts2));
}

// Test includes - timespan includes another
QUARISMATEST(Profiler, timespan_includes_timespan_true)
{
    timespan outer(100, 200);  // [100, 300]
    timespan inner(150, 50);   // [150, 200]

    EXPECT_TRUE(outer.includes(inner));
    EXPECT_FALSE(inner.includes(outer));
}

// Test includes - timespan does not include another
QUARISMATEST(Profiler, timespan_includes_timespan_false)
{
    timespan ts1(100, 50);  // [100, 150]
    timespan ts2(200, 50);  // [200, 250]

    EXPECT_FALSE(ts1.includes(ts2));
}

// Test includes - point in timespan
QUARISMATEST(Profiler, timespan_includes_point_true)
{
    timespan ts(100, 100);  // [100, 200]

    EXPECT_TRUE(ts.includes(100));
    EXPECT_TRUE(ts.includes(150));
    EXPECT_TRUE(ts.includes(200));
}

// Test includes - point not in timespan
QUARISMATEST(Profiler, timespan_includes_point_false)
{
    timespan ts(100, 100);  // [100, 200]

    EXPECT_FALSE(ts.includes(50));
    EXPECT_FALSE(ts.includes(250));
}

// Test overlapped_duration_ps - overlapping
QUARISMATEST(Profiler, timespan_overlapped_duration_overlapping)
{
    timespan ts1(100, 100);  // [100, 200]
    timespan ts2(150, 100);  // [150, 250]

    uint64_t overlap = ts1.overlapped_duration_ps(ts2);
    EXPECT_EQ(overlap, 50);
}

// Test overlapped_duration_ps - non-overlapping
QUARISMATEST(Profiler, timespan_overlapped_duration_non_overlapping)
{
    timespan ts1(100, 50);  // [100, 150]
    timespan ts2(200, 50);  // [200, 250]

    uint64_t overlap = ts1.overlapped_duration_ps(ts2);
    EXPECT_EQ(overlap, 0);
}

// Test expand_to_include
QUARISMATEST(Profiler, timespan_expand_to_include)
{
    timespan ts1(100, 50);  // [100, 150]
    timespan ts2(200, 50);  // [200, 250]

    ts1.expand_to_include(ts2);

    EXPECT_EQ(ts1.begin_ps(), 100);
    EXPECT_EQ(ts1.end_ps(), 250);
}

// Test operator< - by begin time
QUARISMATEST(Profiler, timespan_operator_less_begin)
{
    timespan ts1(100, 50);
    timespan ts2(200, 50);

    EXPECT_TRUE(ts1 < ts2);
    EXPECT_FALSE(ts2 < ts1);
}

// Test operator< - same begin, different duration
QUARISMATEST(Profiler, timespan_operator_less_duration)
{
    timespan ts1(100, 100);  // longer duration
    timespan ts2(100, 50);   // shorter duration

    // When begin times are equal, longer duration comes first (ts1 < ts2 is false)
    EXPECT_TRUE(ts1 < ts2);  // longer duration comes first
    EXPECT_FALSE(ts2 < ts1);
}

// Test operator==
QUARISMATEST(Profiler, timespan_operator_equal)
{
    timespan ts1(100, 50);
    timespan ts2(100, 50);
    timespan ts3(100, 100);

    EXPECT_TRUE(ts1 == ts2);
    EXPECT_FALSE(ts1 == ts3);
}

// Test debug_string
QUARISMATEST(Profiler, timespan_debug_string)
{
    timespan    ts(100, 50);
    std::string debug = ts.debug_string();

    EXPECT_TRUE(debug.find("100") != std::string::npos);
    EXPECT_TRUE(debug.find("150") != std::string::npos);
}

// Test from_end_points - normal case
QUARISMATEST(Profiler, timespan_from_end_points_normal)
{
    timespan ts = timespan::from_end_points(100, 200);

    EXPECT_EQ(ts.begin_ps(), 100);
    EXPECT_EQ(ts.end_ps(), 200);
    EXPECT_EQ(ts.duration_ps(), 100);
}

// Test from_end_points - begin > end
QUARISMATEST(Profiler, timespan_from_end_points_invalid)
{
    timespan ts = timespan::from_end_points(200, 100);

    EXPECT_EQ(ts.begin_ps(), 200);
    EXPECT_EQ(ts.duration_ps(), 0);
}

// Test by_duration comparator
QUARISMATEST(Profiler, timespan_by_duration)
{
    timespan ts1(100, 50);
    timespan ts2(100, 100);

    EXPECT_TRUE(timespan::by_duration(ts1, ts2));
    EXPECT_FALSE(timespan::by_duration(ts2, ts1));
}

// Test milli_to_pico conversion
QUARISMATEST(Profiler, timespan_milli_to_pico)
{
    int64_t picos = timespan::milli_to_pico(1);
    EXPECT_EQ(picos, 1000000000LL);
}

// Test pico_span helper
QUARISMATEST(Profiler, timespan_pico_span)
{
    timespan ts = pico_span(100, 200);

    EXPECT_EQ(ts.begin_ps(), 100);
    EXPECT_EQ(ts.end_ps(), 200);
}

// Test milli_span helper
QUARISMATEST(Profiler, timespan_milli_span)
{
    timespan ts = milli_span(1.0, 2.0);

    EXPECT_EQ(ts.begin_ps(), 1000000000LL);
    EXPECT_EQ(ts.end_ps(), 2000000000LL);
}

// Test large timespan values
QUARISMATEST(Profiler, timespan_large_values)
{
    uint64_t large_begin    = 1000000000000ULL;
    uint64_t large_duration = 500000000000ULL;

    timespan ts(large_begin, large_duration);

    EXPECT_EQ(ts.begin_ps(), large_begin);
    EXPECT_EQ(ts.duration_ps(), large_duration);
    EXPECT_EQ(ts.end_ps(), large_begin + large_duration);
}

// Test zero duration timespan
QUARISMATEST(Profiler, timespan_zero_duration)
{
    timespan ts(1000, 0);

    EXPECT_TRUE(ts.instant());
    EXPECT_EQ(ts.begin_ps(), ts.end_ps());
}
#endif  // QUARISMA_HAS_NATIVE_PROFILER
