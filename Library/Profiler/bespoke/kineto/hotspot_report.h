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

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "bespoke/kineto/profiler_kineto.h"
#include "common/profiler_export.h"

namespace profiler::autograd::profiler_impl
{

/**
 * @brief One aggregated hotspot: every call site sharing the same
 * RECORD_FUNCTION/RECORD_USER_SCOPE name, merged the way VTune's "Bottom-up"
 * view merges samples by function.
 */
struct PROFILER_VISIBILITY hotspot_entry
{
    std::string name;
    uint64_t    self_time_ns  = 0;  ///< Time spent in this function, excluding children.
    uint64_t    total_time_ns = 0;  ///< Inclusive time: this call plus all descendants.
    uint64_t    call_count    = 0;
};

/**
 * @brief VTune-style top-down call tree / bottom-up hotspot breakdown, built
 * from the RecordFunction event tree captured by a Kineto profiling session.
 *
 * Unlike VTune's own sampling-based views, this is instrumentation-based:
 * nodes come from RECORD_FUNCTION/RECORD_USER_SCOPE scopes rather than PC
 * samples, so self/total time is exact for whatever call sites were
 * annotated, and the "call stack" for a hotspot is the RecordFunction parent
 * chain rather than a symbolized native stack trace.
 */
class PROFILER_VISIBILITY hotspot_report
{
public:
    PROFILER_API explicit hotspot_report(const ProfilerResult& result);

    /**
     * @brief Indented top-down call tree, one line per call site: name,
     * self/total time, percentage of that root's total time, and call count.
     * One tree is rendered per root (i.e. per thread's outermost scope).
     */
    PROFILER_API std::string top_down_tree() const;

    /**
     * @brief Functions merged across call sites, sorted by self time
     * descending -- VTune's default "Hotspots" sort order.
     * @param max_rows Maximum number of rows to include (0 = all).
     */
    PROFILER_API std::string bottom_up_hotspots(size_t max_rows = 20) const;

    /**
     * @brief The call path (root -> ... -> leaf) for the first occurrence of
     * the given function name, if it was observed. Mirrors VTune's
     * "Call Stack" pane for a selected hotspot.
     */
    PROFILER_API std::vector<std::string> call_stack_for(const std::string& name) const;

    /// Hotspots sorted by self time descending.
    const std::vector<hotspot_entry>& hotspots() const { return hotspots_; }

private:
    std::vector<experimental_event_t>                          roots_;
    std::vector<hotspot_entry>                                 hotspots_;
    std::unordered_map<std::string, std::vector<std::string>>  call_stacks_;
};

}  // namespace profiler::autograd::profiler_impl
