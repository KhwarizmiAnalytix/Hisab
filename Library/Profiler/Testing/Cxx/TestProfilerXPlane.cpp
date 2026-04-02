#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include "baseTest.h"
#include "native/exporters/xplane/xplane.h"
#include "native/exporters/xplane/xplane_builder.h"

using namespace quarisma;

// ============================================================================
// XPlane Schema Tests
// ============================================================================

QUARISMATEST(Profiler, xspace_empty_initialization)
{
    x_space space;

    EXPECT_EQ(space.planes().size(), 0);
}

QUARISMATEST(Profiler, xspace_add_plane)
{
    x_space space;
    auto*   plane = space.add_planes();

    EXPECT_EQ(space.planes().size(), 1);
    EXPECT_NE(plane, nullptr);
}

QUARISMATEST(Profiler, xplane_set_id_and_name)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    EXPECT_EQ(plane->id(), 1);
    EXPECT_EQ(plane->name(), "CPU");
}

QUARISMATEST(Profiler, xplane_add_line)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();

    EXPECT_EQ(plane->lines_size(), 1);
    EXPECT_NE(line, nullptr);
}

QUARISMATEST(Profiler, xline_set_properties)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();

    line->set_id(100);
    line->set_name("Thread-1");
    line->set_timestamp_ns(1000000);

    EXPECT_EQ(line->id(), 100);
    EXPECT_EQ(line->name(), "Thread-1");
    EXPECT_EQ(line->timestamp_ns(), 1000000);
}

QUARISMATEST(Profiler, xline_add_event)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();
    auto*   event = line->add_events();

    EXPECT_EQ(line->events_size(), 1);
    EXPECT_NE(event, nullptr);
}

QUARISMATEST(Profiler, xevent_set_properties)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();
    auto*   event = line->add_events();

    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    EXPECT_EQ(event->offset_ps(), 0);
    EXPECT_EQ(event->duration_ps(), 500000);
    EXPECT_EQ(event->metadata_id(), 1);
}

QUARISMATEST(Profiler, xstat_set_int64_value)
{
    xstat stat;
    stat.set_value(int64_t(42));

    EXPECT_EQ(stat.int64_value(), 42);
}

QUARISMATEST(Profiler, xstat_set_uint64_value)
{
    xstat stat;
    stat.set_value(uint64_t(100));

    EXPECT_EQ(stat.uint64_value(), 100);
}

QUARISMATEST(Profiler, xstat_set_double_value)
{
    xstat stat;
    stat.set_value(3.14);

    EXPECT_DOUBLE_EQ(stat.double_value(), 3.14);
}

QUARISMATEST(Profiler, xstat_set_string_value)
{
    xstat stat;
    stat.set_value(std::string("test"));

    EXPECT_EQ(stat.str_value(), "test");
}

QUARISMATEST(Profiler, xevent_add_stats)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();
    auto*   event = line->add_events();

    for (int i = 0; i < 5; ++i)
    {
        auto* stat = event->add_stats();
        stat->set_value(int64_t(i * 10));
    }

    EXPECT_EQ(event->stats().size(), 5);
}

QUARISMATEST(Profiler, xplane_multiple_lines)
{
    x_space space;
    auto*   plane = space.add_planes();

    for (int i = 0; i < 3; ++i)
    {
        auto* line = plane->add_lines();
        line->set_id(100 + i);
        line->set_name("Thread-" + std::to_string(i));
    }

    EXPECT_EQ(plane->lines_size(), 3);
}

QUARISMATEST(Profiler, xspace_multiple_planes)
{
    x_space space;

    for (int i = 0; i < 2; ++i)
    {
        auto* plane = space.add_planes();
        plane->set_id(i + 1);
        plane->set_name("Plane-" + std::to_string(i));
    }

    EXPECT_EQ(space.planes().size(), 2);
}

QUARISMATEST(Profiler, xplane_complex_structure)
{
    x_space space;

    // Create 2 planes
    for (int p = 0; p < 2; ++p)
    {
        auto* plane = space.add_planes();
        plane->set_id(p + 1);
        plane->set_name("Plane-" + std::to_string(p));

        // Create 3 lines per plane
        for (int l = 0; l < 3; ++l)
        {
            auto* line = plane->add_lines();
            line->set_id(100 + l);
            line->set_name("Thread-" + std::to_string(l));

            // Create 2 events per line
            for (int e = 0; e < 2; ++e)
            {
                auto* event = line->add_events();
                event->set_offset_ps(e * 1000);
                event->set_duration_ps(500);
                event->set_metadata_id(1);
            }
        }
    }

    EXPECT_EQ(space.planes().size(), 2);
    EXPECT_EQ(space.planes(0).lines_size(), 3);
    EXPECT_EQ(space.planes(0).lines(0).events_size(), 2);
}

QUARISMATEST(Profiler, xstat_ref_value)
{
    xstat stat;
    stat.set_ref_value(12345);

    EXPECT_EQ(stat.int64_value(), 12345);
}

QUARISMATEST(Profiler, xline_duration_ps)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();

    line->set_duration_ps(1000000);

    EXPECT_EQ(line->duration_ps(), 1000000);
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
