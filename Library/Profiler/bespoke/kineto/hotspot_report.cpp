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

#include "bespoke/kineto/hotspot_report.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "bespoke/common/collection.h"

namespace profiler::autograd::profiler_impl
{

namespace
{

int64_t node_total_ns(const experimental_event_t& node)
{
    const auto total = node->endTimeNS() - node->start_time_ns_;
    return total > 0 ? total : 0;
}

int64_t children_total_ns(const experimental_event_t& node)
{
    return std::accumulate(
        node->children_.begin(),
        node->children_.end(),
        int64_t{0},
        [](int64_t sum, const experimental_event_t& child)
        { return sum + node_total_ns(child); });
}

// Recursively walks the RecordFunction event tree, accumulating per-name
// self/total time and call counts into `hotspots`, and remembering the first
// call path (root -> ... -> leaf) seen for each name into `call_stacks`.
void accumulate(
    const experimental_event_t&                                node,
    std::vector<std::string>&                                  path,
    std::unordered_map<std::string, hotspot_entry>&             hotspots,
    std::unordered_map<std::string, std::vector<std::string>>& call_stacks)
{
    const auto  name       = node->name();
    const auto  total_ns   = node_total_ns(node);
    const auto  children_ns = children_total_ns(node);
    const auto self_ns = total_ns > children_ns ? total_ns - children_ns : 0;

    auto& entry = hotspots[name];
    entry.name = name;
    entry.self_time_ns += static_cast<uint64_t>(self_ns);
    entry.total_time_ns += static_cast<uint64_t>(total_ns);
    entry.call_count += 1;

    path.push_back(name);
    call_stacks.try_emplace(name, path);
    for (const auto& child : node->children_)
    {
        accumulate(child, path, hotspots, call_stacks);
    }
    path.pop_back();
}

std::string format_ns(uint64_t ns)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    if (ns >= 1'000'000)
    {
        oss.precision(2);
        oss << (static_cast<double>(ns) / 1'000'000.0) << "ms";
    }
    else if (ns >= 1'000)
    {
        oss.precision(2);
        oss << (static_cast<double>(ns) / 1'000.0) << "us";
    }
    else
    {
        oss << ns << "ns";
    }
    return oss.str();
}

void render_tree(
    const experimental_event_t& node,
    int64_t                     root_total_ns,
    size_t                      depth,
    std::ostringstream&         out)
{
    const auto total_ns    = node_total_ns(node);
    const auto children_ns = children_total_ns(node);
    const auto self_ns = total_ns > children_ns ? total_ns - children_ns : 0;
    const auto pct = root_total_ns > 0
                          ? (100.0 * static_cast<double>(total_ns) / static_cast<double>(root_total_ns))
                          : 0.0;

    out << std::string(depth * 2, ' ') << "[" << std::fixed << std::setprecision(1) << pct
        << "%] " << node->name() << "  total=" << format_ns(static_cast<uint64_t>(total_ns))
        << " self=" << format_ns(static_cast<uint64_t>(self_ns)) << '\n';

    for (const auto& child : node->children_)
    {
        render_tree(child, root_total_ns, depth + 1, out);
    }
}

}  // namespace

hotspot_report::hotspot_report(const ProfilerResult& result) : roots_(result.event_tree())
{
    std::unordered_map<std::string, hotspot_entry> hotspots;
    std::vector<std::string>                        path;
    for (const auto& root : roots_)
    {
        accumulate(root, path, hotspots, call_stacks_);
    }

    hotspots_.reserve(hotspots.size());
    for (auto& [name, entry] : hotspots)
    {
        hotspots_.push_back(std::move(entry));
    }
    std::sort(
        hotspots_.begin(),
        hotspots_.end(),
        [](const hotspot_entry& a, const hotspot_entry& b)
        { return a.self_time_ns > b.self_time_ns; });
}

std::string hotspot_report::top_down_tree() const
{
    std::ostringstream out;
    for (const auto& root : roots_)
    {
        render_tree(root, node_total_ns(root), 0, out);
    }
    return out.str();
}

std::string hotspot_report::bottom_up_hotspots(size_t max_rows) const
{
    std::ostringstream out;
    out << "self time      total time     calls  name\n";
    const size_t row_count = max_rows == 0 ? hotspots_.size() : std::min(max_rows, hotspots_.size());
    for (size_t i = 0; i < row_count; ++i)
    {
        const auto& entry = hotspots_[i];
        out << std::left << std::setw(14) << format_ns(entry.self_time_ns) << ' ' << std::setw(14)
            << format_ns(entry.total_time_ns) << ' ' << std::setw(6) << entry.call_count << ' '
            << entry.name << '\n';
    }
    return out.str();
}

std::vector<std::string> hotspot_report::call_stack_for(const std::string& name) const
{
    auto it = call_stacks_.find(name);
    return it == call_stacks_.end() ? std::vector<std::string>{} : it->second;
}

}  // namespace profiler::autograd::profiler_impl
