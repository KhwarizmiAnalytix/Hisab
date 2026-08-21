/*
 * Quarisma: High-Performance Computational Library
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

#include "native/session/scope_tree_builder.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "native/exporters/xplane/xplane_schema.h"
#include "native/session/profiler.h"

namespace profiler::scope_tree_builder
{
namespace
{

struct flat_event
{
    int64_t     start_ns;
    int64_t     end_ns;
    std::string name;
};

std::chrono::high_resolution_clock::time_point to_time_point(int64_t nanos)
{
    // Reconstructed nodes don't have a real high_resolution_clock reading (the collection
    // already finished when this runs) -- only relative differences (get_duration_*()) are
    // meaningful here, so any consistent zero point works.
    return std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(nanos));
}

std::string event_name(const xplane& plane, const xevent& event)
{
    const auto& metadata = plane.event_metadata();
    if (const auto it = metadata.find(event.metadata_id()); it != metadata.end())
    {
        const auto& md = it->second;
        std::string name =
            !md.display_name().empty() ? std::string(md.display_name()) : std::string(md.name());
        if (!name.empty())
        {
            return name;
        }
    }
    return "TraceEvent";
}

std::vector<flat_event> collect_line_events(const xplane& plane, const xline& line)
{
    std::vector<flat_event> events;
    events.reserve(line.events_size());

    int64_t const line_timestamp_ns = line.timestamp_ns();
    for (const xevent& event : line.events())
    {
        if (event.data_case() == xevent::data_case_type::kNumOccurrences)
        {
            continue;  // counter event, not a duration to nest
        }
        int64_t const start_ns    = line_timestamp_ns + (event.offset_ps() / 1000);
        int64_t const duration_ns = std::max<int64_t>(0, event.duration_ps() / 1000);
        events.push_back({start_ns, start_ns + duration_ns, event_name(plane, event)});
    }

    std::stable_sort(
        events.begin(),
        events.end(),
        [](const flat_event& a, const flat_event& b) { return a.start_ns < b.start_ns; });
    return events;
}

// Nests one thread's flat, time-sorted events under `root` by interval containment: each event
// becomes a child of the innermost still-open event whose [start, end) range contains it.
void nest_line_events(
    const std::vector<flat_event>& events,
    const std::string&             thread_label,
    profiler::profiler_scope_data& root)
{
    std::vector<profiler::profiler_scope_data*> open_ancestors;  // innermost open node last

    for (const flat_event& event : events)
    {
        auto const start = to_time_point(event.start_ns);
        while (!open_ancestors.empty() && open_ancestors.back()->end_time_ <= start)
        {
            open_ancestors.pop_back();
        }
        profiler::profiler_scope_data* parent =
            open_ancestors.empty() ? &root : open_ancestors.back();

        auto node           = std::make_unique<profiler::profiler_scope_data>();
        node->name_         = event.name;
        node->start_time_   = start;
        node->end_time_     = to_time_point(event.end_ns);
        node->depth_level_  = parent->depth_level_ + 1;
        node->parent_       = parent;
        node->thread_label_ = thread_label;

        profiler::profiler_scope_data* node_ptr = node.get();
        parent->children_.push_back(std::move(node));
        open_ancestors.push_back(node_ptr);
    }
}

}  // namespace

std::unique_ptr<profiler::profiler_scope_data> build_scope_tree(const profiler::x_space& space)
{
    auto root   = std::make_unique<profiler::profiler_scope_data>();
    root->name_ = "ROOT";

    bool found_events = false;
    for (const xplane& plane : space.planes())
    {
        if (plane.name() != kHostThreadsPlaneName)
        {
            continue;
        }
        for (const xline& line : plane.lines())
        {
            std::vector<flat_event> const events = collect_line_events(plane, line);
            if (events.empty())
            {
                continue;
            }
            found_events = true;
            nest_line_events(events, xline_thread_label(line), *root);
        }
    }

    return found_events ? std::move(root) : nullptr;
}

}  // namespace profiler::scope_tree_builder
