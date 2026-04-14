#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Profiler: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of Profiler and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@profiler.co.uk
 * Website: https://www.profiler.co.uk
 */

#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

#include "ProfilerTest.h"
// IWYU pragma: keep
//#include "logger.h"
#include "native/core/profiler_factory.h"
#include "native/core/profiler_lock.h"
#include "native/core/profiler_options.h"
#include "native/session/profiler.h"

// ============================================================================
// ProfilerLock Tests (Skipped - requires profiler_lock.cpp linking)
// Note: ProfilerLock tests are covered in other test files
// ============================================================================

// ============================================================================
// Enhanced ProfilerOptions Tests
// ============================================================================

PROFILERTEST(Profiler, enhanced_profiler_options_default_values)
{
    profiler::profiler_options opts;
    EXPECT_TRUE(opts.enable_timing_);
    EXPECT_TRUE(opts.enable_memory_tracking_);
    EXPECT_TRUE(opts.enable_hierarchical_profiling_);
    EXPECT_TRUE(opts.enable_statistical_analysis_);
    EXPECT_TRUE(opts.enable_thread_safety_);
    EXPECT_EQ(opts.max_samples_, 1000);
    EXPECT_TRUE(opts.calculate_percentiles_);
    EXPECT_TRUE(opts.track_peak_memory_);
    EXPECT_TRUE(opts.track_memory_deltas_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_timing)
{
    profiler::profiler_options opts;
    opts.enable_timing_ = false;
    EXPECT_FALSE(opts.enable_timing_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_memory_tracking)
{
    profiler::profiler_options opts;
    opts.enable_memory_tracking_ = false;
    EXPECT_FALSE(opts.enable_memory_tracking_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_hierarchical_profiling)
{
    profiler::profiler_options opts;
    opts.enable_hierarchical_profiling_ = false;
    EXPECT_FALSE(opts.enable_hierarchical_profiling_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_statistical_analysis)
{
    profiler::profiler_options opts;
    opts.enable_statistical_analysis_ = false;
    EXPECT_FALSE(opts.enable_statistical_analysis_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_thread_safety)
{
    profiler::profiler_options opts;
    opts.enable_thread_safety_ = false;
    EXPECT_FALSE(opts.enable_thread_safety_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_set_output_format_json)
{
    profiler::profiler_options opts;
    opts.output_format_ = profiler::profiler_options::output_format_enum::JSON;
    EXPECT_EQ(opts.output_format_, profiler::profiler_options::output_format_enum::JSON);
}

PROFILERTEST(Profiler, enhanced_profiler_options_set_output_format_csv)
{
    profiler::profiler_options opts;
    opts.output_format_ = profiler::profiler_options::output_format_enum::CSV;
    EXPECT_EQ(opts.output_format_, profiler::profiler_options::output_format_enum::CSV);
}

PROFILERTEST(Profiler, enhanced_profiler_options_set_output_file_path)
{
    profiler::profiler_options opts;
    opts.output_file_path_ = "/tmp/profile.json";
    EXPECT_EQ(opts.output_file_path_, "/tmp/profile.json");
}

PROFILERTEST(Profiler, enhanced_profiler_options_set_max_samples)
{
    profiler::profiler_options opts;
    opts.max_samples_ = 5000;
    EXPECT_EQ(opts.max_samples_, 5000);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_percentiles)
{
    profiler::profiler_options opts;
    opts.calculate_percentiles_ = false;
    EXPECT_FALSE(opts.calculate_percentiles_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_peak_memory_tracking)
{
    profiler::profiler_options opts;
    opts.track_peak_memory_ = false;
    EXPECT_FALSE(opts.track_peak_memory_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_disable_memory_deltas)
{
    profiler::profiler_options opts;
    opts.track_memory_deltas_ = false;
    EXPECT_FALSE(opts.track_memory_deltas_);
}

PROFILERTEST(Profiler, enhanced_profiler_options_set_thread_pool_size)
{
    profiler::profiler_options opts;
    opts.thread_pool_size_ = 16;
    EXPECT_EQ(opts.thread_pool_size_, 16);
}

// ============================================================================
// Enhanced Profiler Session Tests
// ============================================================================

PROFILERTEST(Profiler, enhanced_profiler_session_basic_creation)
{
    profiler::profiler_options opts;
    auto                       session = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
}

PROFILERTEST(Profiler, enhanced_profiler_session_start_stop)
{
    profiler::profiler_options opts;
    auto                       session = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());
    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_with_timing_enabled)
{
    profiler::profiler_options opts;
    opts.enable_timing_ = true;
    auto session        = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope scope("test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_with_memory_tracking)
{
    profiler::profiler_options opts;
    opts.enable_memory_tracking_ = true;
    auto session                 = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope scope("memory_scope", session.get());
        std::vector<int>         data(1000);
        for (int i = 0; i < 1000; ++i)
        {
            data[i] = i * 2;
        }
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_with_hierarchical_profiling)
{
    profiler::profiler_options opts;
    opts.enable_hierarchical_profiling_ = true;
    auto session                        = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope outer("outer_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        {
            profiler::profiler_scope inner("inner_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_with_statistical_analysis)
{
    profiler::profiler_options opts;
    opts.enable_statistical_analysis_ = true;
    auto session                      = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 5; ++i)
    {
        profiler::profiler_scope scope("stat_scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_all_features_enabled)
{
    profiler::profiler_options opts;
    opts.enable_timing_                 = true;
    opts.enable_memory_tracking_        = true;
    opts.enable_hierarchical_profiling_ = true;
    opts.enable_statistical_analysis_   = true;
    opts.enable_thread_safety_          = true;

    auto session = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope outer("outer", session.get());
        std::vector<int>         data(1000);

        {
            profiler::profiler_scope inner("inner", session.get());
            for (int i = 0; i < 1000; ++i)
            {
                data[i] = i * 2;
            }
        }
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_generate_chrome_trace_json)
{
    profiler::profiler_options opts;
    opts.enable_timing_ = true;
    auto session        = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope scope("json_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    std::string json = session->generate_chrome_trace_json();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("json_test_scope"), std::string::npos);
}

PROFILERTEST(Profiler, enhanced_profiler_session_multiple_scopes)
{
    profiler::profiler_options opts;
    auto                       session = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 10; ++i)
    {
        profiler::profiler_scope scope("scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, enhanced_profiler_session_deeply_nested_scopes)
{
    profiler::profiler_options opts;
    opts.enable_hierarchical_profiling_ = true;
    auto session                        = std::make_unique<profiler::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler::profiler_scope level1("level1", session.get());
        {
            profiler::profiler_scope level2("level2", session.get());
            {
                profiler::profiler_scope level3("level3", session.get());
                {
                    profiler::profiler_scope level4("level4", session.get());
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    EXPECT_TRUE(session->stop());
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
