#if PROFILER_HAS_NATIVE
/*
 * Profiler: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for XPlane Utilities
 * Tests utility functions for XPlane manipulation
 */

#include <string>
#include <vector>

#include "ProfilerTest.h"
#include "native/exporters/xplane/xplane.h"
#include "native/exporters/xplane/xplane_builder.h"
#include "native/exporters/xplane/xplane_utils.h"
#include "native/exporters/xplane/xplane_visitor.h"

using namespace profiler;

// ============================================================================
// xevent_timespan Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, xevent_timespan_basic)
{
    xevent event;
    event.set_offset_ps(1000);
    event.set_duration_ps(5000);

    timespan ts = xevent_timespan(event);
    EXPECT_EQ(ts.begin_ps(), 1000);
    EXPECT_EQ(ts.duration_ps(), 5000);
}

PROFILERTEST(XPlaneUtils, xevent_timespan_zero_duration)
{
    xevent event;
    event.set_offset_ps(1000);
    event.set_duration_ps(0);

    timespan ts = xevent_timespan(event);
    EXPECT_EQ(ts.begin_ps(), 1000);
    EXPECT_EQ(ts.duration_ps(), 0);
}

// ============================================================================
// find_mutable_plane_with_name Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, find_mutable_plane_with_name_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_name("CPU");

    xplane* found = find_mutable_plane_with_name(&space, "CPU");
    EXPECT_NE(found, nullptr);
    if (found != nullptr)
    {
        EXPECT_EQ(found->name(), "CPU");
    }
}

PROFILERTEST(XPlaneUtils, find_mutable_plane_with_name_not_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_name("CPU");

    xplane* found = find_mutable_plane_with_name(&space, "GPU");
    EXPECT_EQ(found, nullptr);
}

// ============================================================================
// find_mutable_planes_with_prefix Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, find_mutable_planes_with_prefix_found)
{
    x_space space;
    auto*   plane1 = space.add_planes();
    plane1->set_name("GPU:0");
    auto* plane2 = space.add_planes();
    plane2->set_name("GPU:1");

    std::vector<xplane*> found = find_mutable_planes_with_prefix(&space, "GPU");
    EXPECT_EQ(found.size(), 2);
}

PROFILERTEST(XPlaneUtils, find_mutable_planes_with_prefix_not_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_name("CPU:0");

    std::vector<xplane*> found = find_mutable_planes_with_prefix(&space, "TPU");
    EXPECT_EQ(found.size(), 0);
}

PROFILERTEST(XPlaneUtils, find_mutable_planes_with_prefix_empty_prefix)
{
    x_space space;
    auto*   plane1 = space.add_planes();
    plane1->set_name("CPU");
    auto* plane2 = space.add_planes();
    plane2->set_name("GPU");

    std::vector<xplane*> found = find_mutable_planes_with_prefix(&space, "");
    EXPECT_EQ(found.size(), 2);  // Empty prefix matches all
}

// ============================================================================
// find_line_with_id Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, find_line_with_id_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line1 = plane->add_lines();
    line1->set_id(100);
    auto* line2 = plane->add_lines();
    line2->set_id(200);

    const xline* found = find_line_with_id(*plane, 100);
    EXPECT_NE(found, nullptr);
    if (found != nullptr)
    {
        EXPECT_EQ(found->id(), 100);
    }
}

PROFILERTEST(XPlaneUtils, find_line_with_id_not_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();
    line->set_id(100);

    const xline* found = find_line_with_id(*plane, 999);
    EXPECT_EQ(found, nullptr);
}

PROFILERTEST(XPlaneUtils, find_line_with_id_empty_plane)
{
    x_space      space;
    auto*        plane = space.add_planes();
    const xline* found = find_line_with_id(*plane, 100);
    EXPECT_EQ(found, nullptr);
}

// ============================================================================
// find_line_with_name Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, find_line_with_name_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line1 = plane->add_lines();
    line1->set_name("Thread-1");
    auto* line2 = plane->add_lines();
    line2->set_name("Thread-2");

    const xline* found = find_line_with_name(*plane, "Thread-1");
    EXPECT_NE(found, nullptr);
    if (found != nullptr)
    {
        EXPECT_EQ(found->name(), "Thread-1");
    }
}

PROFILERTEST(XPlaneUtils, find_line_with_name_not_found)
{
    x_space space;
    auto*   plane = space.add_planes();
    auto*   line  = plane->add_lines();
    line->set_name("Thread-1");

    const xline* found = find_line_with_name(*plane, "Thread-99");
    EXPECT_EQ(found, nullptr);
}

// Note: remove_plane and remove_empty_planes functions are declared but not
// implemented in xplane_utils.cpp, so we skip testing them for now

// ============================================================================
// find_or_add_mutable_plane_with_name Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, find_or_add_mutable_plane_with_name_existing)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_name("CPU");

    xplane* found = find_or_add_mutable_plane_with_name(&space, "CPU");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(space.planes().size(), 1);  // Should not add a new plane
}

PROFILERTEST(XPlaneUtils, find_or_add_mutable_plane_with_name_new)
{
    x_space space;

    xplane* found = find_or_add_mutable_plane_with_name(&space, "GPU");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(space.planes().size(), 1);  // Should add a new plane
    if (found != nullptr)
    {
        EXPECT_EQ(found->name(), "GPU");
    }
}

// ============================================================================
// xlines_comparator_by_name Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, xlines_comparator_by_name_basic)
{
    xline line1, line2;
    line1.set_name("aaa");
    line2.set_name("bbb");

    xlines_comparator_by_name comparator;
    EXPECT_TRUE(comparator(line1, line2));
    EXPECT_FALSE(comparator(line2, line1));
}

PROFILERTEST(XPlaneUtils, xlines_comparator_by_name_display_name)
{
    xline line1, line2;
    line1.set_name("zzz");
    line1.set_display_name("aaa");
    line2.set_name("aaa");
    line2.set_display_name("bbb");

    xlines_comparator_by_name comparator;
    EXPECT_TRUE(comparator(line1, line2));  // Compares display names
}

// ============================================================================
// IsEmpty Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, is_empty_true)
{
    x_space space;
    EXPECT_TRUE(IsEmpty(space));
}

PROFILERTEST(XPlaneUtils, is_empty_false)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_name("CPU");
    auto* line = plane->add_lines();
    line->set_id(1);
    auto* event = line->add_events();
    event->set_offset_ps(1000);

    EXPECT_FALSE(IsEmpty(space));
}

// ============================================================================
// IsHostPlane / IsDevicePlane / IsCustomPlane Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, is_host_plane)
{
    xplane plane;
    plane.set_name("/host:CPU");
    EXPECT_TRUE(IsHostPlane(plane));
}

PROFILERTEST(XPlaneUtils, is_device_plane)
{
    xplane plane;
    plane.set_name("/device:GPU:0");
    EXPECT_TRUE(IsDevicePlane(plane));
}

PROFILERTEST(XPlaneUtils, is_custom_plane)
{
    xplane plane;
    plane.set_name("/custom:MyPlane");
    EXPECT_TRUE(IsCustomPlane(plane));
}

// ============================================================================
// xevents_comparator Tests
// ============================================================================

PROFILERTEST(XPlaneUtils, xevents_comparator_basic)
{
    xevent event1, event2;
    event1.set_offset_ps(1000);
    event1.set_duration_ps(500);
    event2.set_offset_ps(2000);
    event2.set_duration_ps(500);

    xevents_comparator comparator;
    EXPECT_TRUE(comparator(event1, event2));
    EXPECT_FALSE(comparator(event2, event1));
}

PROFILERTEST(XPlaneUtils, xevents_comparator_nested_events)
{
    xevent event1, event2;
    // event1: offset=1000, duration=1000 (ends at 2000)
    event1.set_offset_ps(1000);
    event1.set_duration_ps(1000);
    // event2: offset=1000, duration=500 (ends at 1500, nested inside event1)
    event2.set_offset_ps(1000);
    event2.set_duration_ps(500);

    xevents_comparator comparator;
    // Nested events: longer duration comes first (event1 < event2 is true)
    EXPECT_TRUE(comparator(event1, event2));
    EXPECT_FALSE(comparator(event2, event1));
}
#endif  // PROFILER_HAS_NATIVE
