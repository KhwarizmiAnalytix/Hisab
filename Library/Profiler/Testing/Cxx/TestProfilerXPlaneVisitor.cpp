#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Test suite for XPlane visitor classes
 * Tests visitor pattern implementation for XPlane data structures
 */

#include "Testing/baseTest.h"
#include "native/exporters/xplane/xplane.h"
#include "native/exporters/xplane/xplane_builder.h"
#include "native/exporters/xplane/xplane_visitor.h"

using namespace quarisma;

// ============================================================================
// x_stat_visitor Tests
// ============================================================================

// Test x_stat_visitor int64 value
QUARISMATEST(XPlaneVisitor, x_stat_visitor_int_value)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, int64_t(42));

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            EXPECT_EQ(stat.int_value(), 42);
                            EXPECT_EQ(stat.value_case(), xstat::value_case_type::kInt64Value);
                        });
                });
        });
}

// Test x_stat_visitor uint64 value
QUARISMATEST(XPlaneVisitor, x_stat_visitor_uint_value)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, uint64_t(123));

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            EXPECT_EQ(stat.uint_value(), 123);
                            EXPECT_EQ(stat.value_case(), xstat::value_case_type::kUint64Value);
                        });
                });
        });
}

// Test x_stat_visitor double value
QUARISMATEST(XPlaneVisitor, x_stat_visitor_double_value)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, 3.14);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            EXPECT_DOUBLE_EQ(stat.double_value(), 3.14);
                            EXPECT_EQ(stat.value_case(), xstat::value_case_type::kDoubleValue);
                        });
                });
        });
}

// Test x_stat_visitor string value
QUARISMATEST(XPlaneVisitor, x_stat_visitor_str_value)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, "test_string");

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            EXPECT_EQ(stat.str_or_ref_value(), "test_string");
                            EXPECT_EQ(stat.value_case(), xstat::value_case_type::kStrValue);
                        });
                });
        });
}

// Test x_stat_visitor bool value
QUARISMATEST(XPlaneVisitor, x_stat_visitor_bool_value)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, int64_t(1));

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat([](const x_stat_visitor& stat)
                                        { EXPECT_TRUE(stat.bool_value()); });
                });
        });
}

// Test x_stat_visitor id and name
QUARISMATEST(XPlaneVisitor, x_stat_visitor_id_and_name)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto  line_builder  = builder.get_or_create_line(1);
    auto  metadata      = builder.get_or_create_event_metadata("test_event");
    auto* stat_metadata = builder.get_or_create_stat_metadata("my_stat");
    auto  event_builder = line_builder.add_event(*metadata);
    event_builder.add_stat_value(*stat_metadata, int64_t(42));

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            EXPECT_EQ(stat.name(), "my_stat");
                            EXPECT_GE(stat.id(), 0);
                        });
                });
        });
}

// ============================================================================
// xevent_visitor Tests
// ============================================================================

// Test xevent_visitor basic properties
QUARISMATEST(XPlaneVisitor, xevent_visitor_basic_properties)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder  = builder.get_or_create_line(1);
    auto metadata      = builder.get_or_create_event_metadata("test_event");
    auto event_builder = line_builder.add_event(*metadata);
    event_builder.SetOffsetNs(1000);
    event_builder.SetDurationNs(500);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    EXPECT_EQ(event.name(), "test_event");
                    EXPECT_EQ(event.offset_ps(), 1000000);   // 1000 ns = 1000000 ps
                    EXPECT_EQ(event.duration_ps(), 500000);  // 500 ns = 500000 ps
                });
        });
}

// Test xevent_visitor timestamp calculations
QUARISMATEST(XPlaneVisitor, xevent_visitor_timestamps)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetTimestampNs(2000);
    auto metadata      = builder.get_or_create_event_metadata("test_event");
    auto event_builder = line_builder.add_event(*metadata);
    event_builder.SetOffsetNs(1000);
    event_builder.SetDurationNs(500);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    EXPECT_EQ(event.line_timestamp_ns(), 2000);
                    EXPECT_EQ(event.timestamp_ns(), 3000);      // 2000 + 1000
                    EXPECT_EQ(event.end_timestamp_ns(), 3500);  // 3000 + 500
                });
        });
}

// Test xevent_visitor timespan
QUARISMATEST(XPlaneVisitor, xevent_visitor_timespan)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetTimestampNs(2000);
    auto metadata      = builder.get_or_create_event_metadata("test_event");
    auto event_builder = line_builder.add_event(*metadata);
    event_builder.SetOffsetNs(1000);
    event_builder.SetDurationNs(500);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    timespan ts = event.get_timespan();
                    EXPECT_EQ(ts.begin_ps(), 3000000);    // (2000 + 1000) * 1000
                    EXPECT_EQ(ts.duration_ps(), 500000);  // 500 * 1000
                });
        });
}

// Test xevent_visitor aggregated events
QUARISMATEST(XPlaneVisitor, xevent_visitor_aggregated_event)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder  = builder.get_or_create_line(1);
    auto metadata      = builder.get_or_create_event_metadata("test_event");
    auto event_builder = line_builder.add_event(*metadata);
    event_builder.SetNumOccurrences(10);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    EXPECT_TRUE(event.is_aggregated_event());
                    EXPECT_EQ(event.num_occurrences(), 10);
                });
        });
}

// Test xevent_visitor comparison
QUARISMATEST(XPlaneVisitor, xevent_visitor_comparison)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetTimestampNs(1000);
    auto metadata1      = builder.get_or_create_event_metadata("event1");
    auto metadata2      = builder.get_or_create_event_metadata("event2");
    auto event_builder1 = line_builder.add_event(*metadata1);
    event_builder1.SetOffsetNs(100);
    event_builder1.SetDurationNs(50);
    auto event_builder2 = line_builder.add_event(*metadata2);
    event_builder2.SetOffsetNs(200);
    event_builder2.SetDurationNs(50);

    xplane_visitor plane_visitor(&plane);

    std::vector<xevent_visitor> events;
    plane_visitor.for_each_line(
        [&events](const xline_visitor& line)
        {
            line.for_each_event([&events](const xevent_visitor& event)
                                { events.push_back(event); });
        });

    EXPECT_EQ(events.size(), 2);
    if (events.size() == 2)
    {
        EXPECT_TRUE(events[0] < events[1]);
    }
}

// ============================================================================
// xline_visitor Tests
// ============================================================================

// Test xline_visitor basic properties
QUARISMATEST(XPlaneVisitor, xline_visitor_basic_properties)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(42);
    line_builder.SetName("test_line");
    line_builder.SetTimestampNs(1000);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            EXPECT_EQ(line.id(), 42);
            EXPECT_EQ(line.name(), "test_line");
            EXPECT_EQ(line.timestamp_ns(), 1000);
        });
}

// Test xline_visitor display name
QUARISMATEST(XPlaneVisitor, xline_visitor_display_name)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetName("internal_name");

    // Access the raw line to set display name
    (*plane.mutable_lines())[0].set_display_name("Display Name");

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            EXPECT_EQ(line.name(), "internal_name");
            EXPECT_EQ(line.display_name(), "Display Name");
        });
}

// Test xline_visitor event iteration
QUARISMATEST(XPlaneVisitor, xline_visitor_event_iteration)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    for (int i = 0; i < 5; ++i)
    {
        auto metadata = builder.get_or_create_event_metadata("event_" + std::to_string(i));
        line_builder.add_event(*metadata);
    }

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            EXPECT_EQ(line.num_events(), 5);
            int count = 0;
            line.for_each_event(
                [&count](const xevent_visitor& event)
                {
                    count++;
                    (void)event;
                });
            EXPECT_EQ(count, 5);
        });
}

// ============================================================================
// xplane_visitor Tests
// ============================================================================

// Test xplane_visitor basic properties
QUARISMATEST(XPlaneVisitor, xplane_visitor_basic_properties)
{
    xplane         plane;
    xplane_builder builder(&plane);
    builder.SetName("test_plane");

    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetName("line1");

    xplane_visitor plane_visitor(&plane);

    EXPECT_EQ(plane_visitor.name(), "test_plane");
    EXPECT_EQ(plane_visitor.num_lines(), 1);
}

// Test xplane_visitor multiple lines
QUARISMATEST(XPlaneVisitor, xplane_visitor_multiple_lines)
{
    xplane         plane;
    xplane_builder builder(&plane);

    for (int i = 0; i < 5; ++i)
    {
        auto line_builder = builder.get_or_create_line(i);
        line_builder.SetName("line_" + std::to_string(i));
    }

    xplane_visitor plane_visitor(&plane);

    EXPECT_EQ(plane_visitor.num_lines(), 5);

    int count = 0;
    plane_visitor.for_each_line(
        [&count](const xline_visitor& line)
        {
            count++;
            (void)line;
        });
    EXPECT_EQ(count, 5);
}

// Test xplane_visitor event metadata access
QUARISMATEST(XPlaneVisitor, xplane_visitor_event_metadata)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto metadata     = builder.get_or_create_event_metadata("test_event");
    auto line_builder = builder.get_or_create_line(1);
    line_builder.add_event(*metadata);

    xplane_visitor plane_visitor(&plane);

    const auto* retrieved_metadata = plane_visitor.get_event_metadata(metadata->id());
    EXPECT_NE(retrieved_metadata, nullptr);
    if (retrieved_metadata != nullptr)
    {
        EXPECT_EQ(retrieved_metadata->name(), "test_event");
    }
}

// Test xplane_visitor stat metadata access
QUARISMATEST(XPlaneVisitor, xplane_visitor_stat_metadata)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto* stat_metadata  = builder.get_or_create_stat_metadata("test_stat");
    auto  event_metadata = builder.get_or_create_event_metadata("test_event");
    auto  line_builder   = builder.get_or_create_line(1);
    auto  event_builder  = line_builder.add_event(*event_metadata);
    event_builder.add_stat_value(*stat_metadata, int64_t(42));

    xplane_visitor plane_visitor(&plane);

    const auto* retrieved_metadata = plane_visitor.get_stat_metadata(stat_metadata->id());
    EXPECT_NE(retrieved_metadata, nullptr);
    if (retrieved_metadata != nullptr)
    {
        EXPECT_EQ(retrieved_metadata->name(), "test_stat");
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

// Test complete workflow: build and read back
QUARISMATEST(XPlaneVisitor, integration_build_and_read)
{
    xplane         plane;
    xplane_builder builder(&plane);
    builder.SetName("CPU");

    // Create line with events
    auto line_builder = builder.get_or_create_line(1);
    line_builder.SetName("Thread 1");
    line_builder.SetTimestampNs(1000);

    // Add events with stats
    auto* stat_metadata1 = builder.get_or_create_stat_metadata("duration");
    auto* stat_metadata2 = builder.get_or_create_stat_metadata("count");

    auto event_metadata1 = builder.get_or_create_event_metadata("compute");
    auto event_builder1  = line_builder.add_event(*event_metadata1);
    event_builder1.SetOffsetNs(100);
    event_builder1.SetDurationNs(50);
    event_builder1.add_stat_value(*stat_metadata1, 50.0);
    event_builder1.add_stat_value(*stat_metadata2, int64_t(10));

    // Read back using visitor
    xplane_visitor plane_visitor(&plane);

    EXPECT_EQ(plane_visitor.name(), "CPU");
    EXPECT_EQ(plane_visitor.num_lines(), 1);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            EXPECT_EQ(line.name(), "Thread 1");
            EXPECT_EQ(line.timestamp_ns(), 1000);
            EXPECT_EQ(line.num_events(), 1);

            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    EXPECT_EQ(event.name(), "compute");
                    EXPECT_EQ(event.offset_ns(), 100);
                    EXPECT_EQ(event.duration_ns(), 50);

                    int stat_count = 0;
                    event.for_each_stat(
                        [&stat_count](const x_stat_visitor& stat)
                        {
                            stat_count++;
                            if (stat.name() == "duration")
                            {
                                EXPECT_DOUBLE_EQ(stat.double_value(), 50.0);
                            }
                            else if (stat.name() == "count")
                            {
                                EXPECT_EQ(stat.int_value(), 10);
                            }
                        });
                    EXPECT_EQ(stat_count, 2);
                });
        });
}

// ============================================================================
// Additional visitor method coverage tests
// ============================================================================

// Test xevent_visitor raw_event and plane methods
QUARISMATEST(XPlaneVisitor, xevent_visitor_raw_event_and_plane)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    auto metadata     = builder.get_or_create_event_metadata("test_event");
    auto event        = line_builder.add_event(*metadata);
    event.SetOffsetNs(1000);

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [&plane_visitor](const xline_visitor& line)
        {
            line.for_each_event(
                [&plane_visitor](const xevent_visitor& event)
                {
                    // The plane() method should return the same plane visitor
                    EXPECT_EQ(&event.plane(), &plane_visitor);
                    // The raw_event() method should return a valid reference
                    EXPECT_EQ(event.raw_event().offset_ps(), 1000000);  // 1000 ns = 1000000 ps
                });
        });
}

// Test x_stat_visitor raw_stat method
QUARISMATEST(XPlaneVisitor, x_stat_visitor_raw_stat)
{
    xplane         plane;
    xplane_builder builder(&plane);

    auto line_builder = builder.get_or_create_line(1);
    auto metadata     = builder.get_or_create_event_metadata("test_event");
    auto event        = line_builder.add_event(*metadata);

    auto stat_metadata = builder.get_or_create_stat_metadata("test_stat");
    event.add_stat_value(*stat_metadata, int64_t(42));

    xplane_visitor plane_visitor(&plane);

    plane_visitor.for_each_line(
        [](const xline_visitor& line)
        {
            line.for_each_event(
                [](const xevent_visitor& event)
                {
                    event.for_each_stat(
                        [](const x_stat_visitor& stat)
                        {
                            // raw_stat() should return a valid reference
                            EXPECT_EQ(stat.raw_stat().int64_value(), 42);
                        });
                });
        });
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
