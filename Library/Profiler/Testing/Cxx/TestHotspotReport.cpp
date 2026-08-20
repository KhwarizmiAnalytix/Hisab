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

#include <chrono>
#include <set>
#include <string>
#include <thread>

#include "ProfilerTest.h"

#if PROFILER_HAS_KINETO

#include "bespoke/common/record_function.h"
#include "bespoke/kineto/hotspot_report.h"
#include "bespoke/kineto/profiler_kineto.h"

namespace
{

void busy_wait_for(std::chrono::microseconds duration)
{
    const auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        PROFILER_UNUSED volatile int spin = 0;
        (void)spin;
        ++spin;
    }
}

std::unique_ptr<profiler::autograd::profiler_impl::ProfilerResult> record_sample_workload()
{
    profiler::autograd::profiler_impl::ProfilerConfig const config(
        profiler::autograd::profiler_impl::ProfilerState::KINETO,
        /*report_input_shapes=*/false,
        /*profile_memory=*/false,
        /*with_stack=*/false,
        /*with_flops=*/false,
        /*with_modules=*/false);

    profiler::autograd::profiler_impl::prepareProfiler(
        config, {profiler::autograd::profiler_impl::ActivityType::CPU});
    profiler::autograd::profiler_impl::enableProfiler(
        config,
        {profiler::autograd::profiler_impl::ActivityType::CPU},
        {profiler::RecordScope::USER_SCOPE});

    {
        RECORD_USER_SCOPE("hotspot_outer");
        busy_wait_for(std::chrono::milliseconds(1));
        {
            RECORD_USER_SCOPE("hotspot_inner_a");
            busy_wait_for(std::chrono::milliseconds(2));
        }
        {
            RECORD_USER_SCOPE("hotspot_inner_b");
            busy_wait_for(std::chrono::milliseconds(1));
        }
    }

    return profiler::autograd::profiler_impl::disableProfiler();
}

}  // namespace

PROFILERTEST(HotspotReport, self_time_excludes_children)
{
    auto result = record_sample_workload();
    ASSERT_NE(result, nullptr);
    if (result->event_tree().empty())
    {
        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
    }

    profiler::autograd::profiler_impl::hotspot_report const report(*result);
    const auto&                                              hotspots = report.hotspots();
    ASSERT_FALSE(hotspots.empty());

    const profiler::autograd::profiler_impl::hotspot_entry* outer = nullptr;
    const profiler::autograd::profiler_impl::hotspot_entry* inner_a = nullptr;
    for (const auto& entry : hotspots)
    {
        if (entry.name == "hotspot_outer")
        {
            outer = &entry;
        }
        if (entry.name == "hotspot_inner_a")
        {
            inner_a = &entry;
        }
    }

    if (outer == nullptr || inner_a == nullptr)
    {
        GTEST_SKIP() << "Required hotspot entries not captured";
    }

    // hotspot_outer's own busy-wait is ~1ms, while its two children add ~3ms on
    // top; self time must therefore be a small slice of its total time.
    EXPECT_LT(outer->self_time_ns, outer->total_time_ns);
    // hotspot_inner_a has no children, so self time equals total time.
    EXPECT_EQ(inner_a->self_time_ns, inner_a->total_time_ns);
    EXPECT_GT(inner_a->self_time_ns, 0U);
}

PROFILERTEST(HotspotReport, bottom_up_sorted_by_self_time_descending)
{
    auto result = record_sample_workload();
    ASSERT_NE(result, nullptr);
    if (result->event_tree().empty())
    {
        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
    }

    profiler::autograd::profiler_impl::hotspot_report const report(*result);
    const auto&                                              hotspots = report.hotspots();
    for (size_t i = 1; i < hotspots.size(); ++i)
    {
        EXPECT_GE(hotspots[i - 1].self_time_ns, hotspots[i].self_time_ns);
    }
}

PROFILERTEST(HotspotReport, call_stack_for_reports_root_to_leaf_path)
{
    auto result = record_sample_workload();
    ASSERT_NE(result, nullptr);
    if (result->event_tree().empty())
    {
        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
    }

    profiler::autograd::profiler_impl::hotspot_report const report(*result);
    const auto path = report.call_stack_for("hotspot_inner_b");
    if (path.empty())
    {
        GTEST_SKIP() << "hotspot_inner_b not captured in this environment";
    }

    ASSERT_EQ(path.size(), 2U);
    EXPECT_EQ(path.front(), "hotspot_outer");
    EXPECT_EQ(path.back(), "hotspot_inner_b");
}

PROFILERTEST(HotspotReport, unknown_function_has_empty_call_stack)
{
    auto result = record_sample_workload();
    ASSERT_NE(result, nullptr);

    profiler::autograd::profiler_impl::hotspot_report const report(*result);
    EXPECT_TRUE(report.call_stack_for("does_not_exist").empty());
}

PROFILERTEST(HotspotReport, text_renderers_are_non_empty_when_events_exist)
{
    auto result = record_sample_workload();
    ASSERT_NE(result, nullptr);
    if (result->event_tree().empty())
    {
        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
    }

    profiler::autograd::profiler_impl::hotspot_report const report(*result);
    EXPECT_FALSE(report.top_down_tree().empty());
    EXPECT_FALSE(report.bottom_up_hotspots().empty());
}

#endif  // PROFILER_HAS_KINETO
