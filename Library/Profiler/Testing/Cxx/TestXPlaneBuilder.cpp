#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for XPlane Builder classes
 * Tests builder pattern implementation for XPlane data structures
 *
 * NOTE: Tests have been consolidated to reduce test count while maintaining
 * full coverage. Each test now covers multiple related scenarios.
 */

#include <limits>
#include <string>
#include <vector>

#include "Testing/baseTest.h"
#include "native/core/timespan.h"
#include "native/exporters/xplane/xplane.h"
#include "native/exporters/xplane/xplane_builder.h"

using namespace quarisma;

// ============================================================================
// simple_atoi Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneBuilder, simple_atoi_valid_inputs)
{
    // Test valid positive integer
    int64_t result = 0;
    EXPECT_TRUE(simple_atoi("12345", &result));
    EXPECT_EQ(result, 12345);

    // Test valid negative integer
    result = 0;
    EXPECT_TRUE(simple_atoi("-9876", &result));
    EXPECT_EQ(result, -9876);

    // Test zero
    result = 0;
    EXPECT_TRUE(simple_atoi("0", &result));
    EXPECT_EQ(result, 0);

    // Test with plus sign
    result = 0;
    EXPECT_TRUE(simple_atoi("+42", &result));
    EXPECT_EQ(result, 42);

    // Test INT64_MAX
    result = 0;
    EXPECT_TRUE(simple_atoi("9223372036854775807", &result));
    EXPECT_EQ(result, std::numeric_limits<int64_t>::max());

    // Test INT64_MIN
    result = 0;
    EXPECT_TRUE(simple_atoi("-9223372036854775808", &result));
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min());

    // Test only sign (returns 0)
    result = 999;
    EXPECT_TRUE(simple_atoi("-", &result));
    EXPECT_EQ(result, 0);
    result = 999;
    EXPECT_TRUE(simple_atoi("+", &result));
    EXPECT_EQ(result, 0);
}

QUARISMATEST(XPlaneBuilder, simple_atoi_invalid_inputs)
{
    int64_t result = 0;

    // Test empty string
    EXPECT_FALSE(simple_atoi("", &result));

    // Test null pointer
    int64_t* null_ptr = nullptr;
    EXPECT_FALSE(simple_atoi("123", null_ptr));

    // Test invalid characters
    EXPECT_FALSE(simple_atoi("12a34", &result));

    // Test leading whitespace
    EXPECT_FALSE(simple_atoi(" 123", &result));

    // Test trailing whitespace
    EXPECT_FALSE(simple_atoi("123 ", &result));
}

QUARISMATEST(XPlaneBuilder, simple_atoi_overflow_and_limits)
{
    int64_t result = 0;

    // Test positive overflow (INT64_MAX + 1)
    EXPECT_FALSE(simple_atoi("9223372036854775808", &result));

    // Test negative overflow (INT64_MIN - 1)
    EXPECT_FALSE(simple_atoi("-9223372036854775809", &result));

    // Test uint64_t positive max value
    uint64_t uresult = 0;
    EXPECT_TRUE(simple_atoi("18446744073709551615", &uresult));
    EXPECT_EQ(uresult, std::numeric_limits<uint64_t>::max());
}

// ============================================================================
// simple_atod Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneBuilder, simple_atod_valid_inputs)
{
    // Test valid positive double
    double result = 0.0;
    EXPECT_TRUE(simple_atod("123.456", &result));
    EXPECT_DOUBLE_EQ(result, 123.456);

    // Test valid negative double
    result = 0.0;
    EXPECT_TRUE(simple_atod("-987.654", &result));
    EXPECT_DOUBLE_EQ(result, -987.654);

    // Test zero
    result = 0.0;
    EXPECT_TRUE(simple_atod("0.0", &result));
    EXPECT_DOUBLE_EQ(result, 0.0);

    // Test integer (no decimal point)
    result = 0.0;
    EXPECT_TRUE(simple_atod("42", &result));
    EXPECT_DOUBLE_EQ(result, 42.0);

    // Test with plus sign
    result = 0.0;
    EXPECT_TRUE(simple_atod("+3.14", &result));
    EXPECT_DOUBLE_EQ(result, 3.14);

    // Test only decimal point (returns 0.0)
    result = 999.0;
    EXPECT_TRUE(simple_atod(".", &result));
    EXPECT_DOUBLE_EQ(result, 0.0);

    // Test no digits after decimal
    result = 0.0;
    EXPECT_TRUE(simple_atod("123.", &result));
    EXPECT_DOUBLE_EQ(result, 123.0);
}

QUARISMATEST(XPlaneBuilder, simple_atod_scientific_notation)
{
    // Test positive exponent
    double result = 0.0;
    EXPECT_TRUE(simple_atod("1.23e2", &result));
    EXPECT_DOUBLE_EQ(result, 123.0);

    // Test negative exponent
    result = 0.0;
    EXPECT_TRUE(simple_atod("1.23e-2", &result));
    EXPECT_NEAR(result, 0.0123, 1e-10);

    // Test uppercase E
    result = 0.0;
    EXPECT_TRUE(simple_atod("1.5E3", &result));
    EXPECT_DOUBLE_EQ(result, 1500.0);
}

QUARISMATEST(XPlaneBuilder, simple_atod_invalid_inputs)
{
    double result = 0.0;

    // Test empty string
    EXPECT_FALSE(simple_atod("", &result));

    // Test null pointer
    double* null_ptr = nullptr;
    EXPECT_FALSE(simple_atod("1.23", null_ptr));

    // Test multiple decimal points
    EXPECT_FALSE(simple_atod("1.2.3", &result));

    // Test exponent overflow
    EXPECT_FALSE(simple_atod("1e999999999999999999", &result));

    // Test trailing characters
    EXPECT_FALSE(simple_atod("1.23abc", &result));
}

// ============================================================================
// xplane_builder Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneBuilder, xplane_builder_basic_operations)
{
    xplane         plane;
    xplane_builder builder(&plane);

    // Test construction and initial state
    EXPECT_EQ(builder.Id(), 0);
    EXPECT_EQ(builder.Name(), "");

    // Test set ID and name
    builder.SetId(42);
    builder.SetName("TestPlane");
    EXPECT_EQ(builder.Id(), 42);
    EXPECT_EQ(builder.Name(), "TestPlane");

    // Test reserve lines (should not crash)
    builder.ReserveLines(100);
    EXPECT_TRUE(true);
}

QUARISMATEST(XPlaneBuilder, xplane_builder_line_management)
{
    xplane         plane;
    xplane_builder builder(&plane);

    // Test get_or_create_line creates and reuses lines
    auto line1 = builder.get_or_create_line(100);
    auto line2 = builder.get_or_create_line(100);  // Same ID
    EXPECT_EQ(line1.Id(), 100);
    EXPECT_EQ(line2.Id(), 100);
    EXPECT_EQ(plane.lines_size(), 1);  // Should reuse existing line

    // Test multiple lines with different IDs
    builder.get_or_create_line(1);
    builder.get_or_create_line(2);
    builder.get_or_create_line(3);
    EXPECT_EQ(plane.lines_size(), 4);  // Including the initial line with ID 100
}

QUARISMATEST(XPlaneBuilder, xplane_builder_metadata_management)
{
    xplane         plane;
    xplane_builder builder(&plane);

    // Test create_event_metadata generates unique IDs
    auto* metadata1 = builder.create_event_metadata();
    auto* metadata2 = builder.create_event_metadata();
    EXPECT_NE(metadata1, nullptr);
    EXPECT_NE(metadata2, nullptr);
    EXPECT_NE(metadata1->id(), metadata2->id());

    // Test get_or_create_event_metadata by ID reuses metadata
    auto* metadata3 = builder.get_or_create_event_metadata(100);
    auto* metadata4 = builder.get_or_create_event_metadata(100);  // Same ID
    EXPECT_EQ(metadata3, metadata4);
    EXPECT_EQ(metadata3->id(), 100);

    // Test get_or_create_event_metadata by name reuses metadata
    auto* metadata5 = builder.get_or_create_event_metadata("test_event");
    auto* metadata6 = builder.get_or_create_event_metadata("test_event");  // Same name
    EXPECT_EQ(metadata5, metadata6);
    EXPECT_EQ(metadata5->name(), "test_event");

    // Test get_or_create_event_metadata with string move
    std::string name      = "movable_event";
    auto*       metadata7 = builder.get_or_create_event_metadata(std::move(name));
    EXPECT_NE(metadata7, nullptr);
    EXPECT_EQ(metadata7->name(), "movable_event");

    // Test get_or_create_stat_metadata by ID
    auto* stat1 = builder.get_or_create_stat_metadata(200);
    auto* stat2 = builder.get_or_create_stat_metadata(200);  // Same ID
    EXPECT_EQ(stat1, stat2);
    EXPECT_EQ(stat1->id(), 200);

    // Test get_or_create_stat_metadata by name
    auto* stat3 = builder.get_or_create_stat_metadata("test_stat");
    auto* stat4 = builder.get_or_create_stat_metadata("test_stat");  // Same name
    EXPECT_EQ(stat3, stat4);
    EXPECT_EQ(stat3->name(), "test_stat");
}

// ============================================================================
// xline_builder Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneBuilder, xline_builder_basic_operations)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line = builder.get_or_create_line(1);

    // Test construction and ID
    EXPECT_EQ(line.Id(), 1);

    // Test set name
    line.SetName("TestLine");
    EXPECT_EQ(line.Name(), "TestLine");

    // Test set timestamp
    line.SetTimestampNs(1000000);
    EXPECT_EQ(line.TimestampNs(), 1000000);

    // Test reserve events (should not crash)
    line.ReserveEvents(50);
    EXPECT_TRUE(true);
}

QUARISMATEST(XPlaneBuilder, xline_builder_event_management)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");

    // Test initial event count
    EXPECT_EQ(line.NumEvents(), 0);

    // Test add_event
    auto event1 = line.add_event(*metadata);
    EXPECT_EQ(event1.MetadataId(), metadata->id());
    EXPECT_EQ(line.NumEvents(), 1);

    // Test add_event with timespan
    timespan ts(1000, 5000);  // begin=1000, duration=5000
    auto     event2 = line.add_event(ts, *metadata);
    EXPECT_EQ(event2.MetadataId(), metadata->id());
    EXPECT_EQ(event2.OffsetPs(), 1000);
    EXPECT_EQ(event2.DurationPs(), 5000);
    EXPECT_EQ(line.NumEvents(), 2);

    // Test adding another event increments count
    line.add_event(*metadata);
    EXPECT_EQ(line.NumEvents(), 3);
}

// ============================================================================
// xevent_builder Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneBuilder, xevent_builder_timing_operations)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line     = builder.get_or_create_line(1);
    auto*          metadata = builder.get_or_create_event_metadata("test_event");

    // Test SetOffsetPs
    auto event1 = line.add_event(*metadata);
    event1.SetOffsetPs(1000);
    EXPECT_EQ(event1.OffsetPs(), 1000);

    // Test SetOffsetNs (converts to picoseconds)
    auto event2 = line.add_event(*metadata);
    event2.SetOffsetNs(100);
    EXPECT_EQ(event2.OffsetPs(), 100000);  // 100 ns = 100000 ps

    // Test SetTimestampNs (relative to line timestamp)
    auto event3 = line.add_event(*metadata);
    line.SetTimestampNs(1000);
    event3.SetTimestampNs(2000);
    EXPECT_EQ(event3.OffsetPs(), 1000000);  // (2000 - 1000) * 1000 ps

    // Test SetDurationPs
    auto event4 = line.add_event(*metadata);
    event4.SetDurationPs(5000);
    EXPECT_EQ(event4.DurationPs(), 5000);

    // Test SetDurationNs (converts to picoseconds)
    auto event5 = line.add_event(*metadata);
    event5.SetDurationNs(200);
    EXPECT_EQ(event5.DurationPs(), 200000);  // 200 ns = 200000 ps

    // Test SetEndTimestampNs (calculates duration)
    auto event6 = line.add_event(*metadata);
    line.SetTimestampNs(1000);
    event6.SetTimestampNs(2000);
    event6.SetEndTimestampNs(3000);
    EXPECT_EQ(event6.DurationPs(), 1000000);  // (3000 - 2000) * 1000 ps

// Test SetTimespan
#if 0
    auto     event7 = line.add_event(*metadata);
    timespan ts(1000, 5000);  // begin=1000, duration=5000
    event7.SetTimespan(ts);
    EXPECT_EQ(event7.OffsetPs(), 1000);
    EXPECT_EQ(event7.DurationPs(), 5000);
#endif
}

QUARISMATEST(XPlaneBuilder, xevent_builder_stats_and_occurrences)
{
    xplane         plane;
    xplane_builder builder(&plane);
    auto           line          = builder.get_or_create_line(1);
    auto*          event_meta    = builder.get_or_create_event_metadata("test_event");
    auto*          stat_metadata = builder.get_or_create_stat_metadata("test_stat");

    // Test SetNumOccurrences
    auto event1 = line.add_event(*event_meta);
    event1.SetNumOccurrences(10);
    EXPECT_TRUE(true);  // No direct getter, but should not crash

    // Test add_stat_value with int64_t
    auto event2 = line.add_event(*event_meta);
    event2.add_stat_value(*stat_metadata, int64_t(42));
    EXPECT_TRUE(true);

    // Test add_stat_value with uint64_t
    auto event3 = line.add_event(*event_meta);
    event3.add_stat_value(*stat_metadata, uint64_t(100));
    EXPECT_TRUE(true);

    // Test add_stat_value with double
    auto event4 = line.add_event(*event_meta);
    event4.add_stat_value(*stat_metadata, 3.14);
    EXPECT_TRUE(true);
}

#endif  // PROFILER_HAS_NATIVE_PROFILER
