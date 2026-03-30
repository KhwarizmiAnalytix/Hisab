#if QUARISMA_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
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

#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

#include "baseTest.h"
#include "common/configure.h"  // IWYU pragma: keep
#include "logging/logger.h"
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

QUARISMATEST(Profiler, enhanced_profiler_options_default_values)
{
    quarisma::profiler_options opts;
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

QUARISMATEST(Profiler, enhanced_profiler_options_disable_timing)
{
    quarisma::profiler_options opts;
    opts.enable_timing_ = false;
    EXPECT_FALSE(opts.enable_timing_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_memory_tracking)
{
    quarisma::profiler_options opts;
    opts.enable_memory_tracking_ = false;
    EXPECT_FALSE(opts.enable_memory_tracking_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_hierarchical_profiling)
{
    quarisma::profiler_options opts;
    opts.enable_hierarchical_profiling_ = false;
    EXPECT_FALSE(opts.enable_hierarchical_profiling_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_statistical_analysis)
{
    quarisma::profiler_options opts;
    opts.enable_statistical_analysis_ = false;
    EXPECT_FALSE(opts.enable_statistical_analysis_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_thread_safety)
{
    quarisma::profiler_options opts;
    opts.enable_thread_safety_ = false;
    EXPECT_FALSE(opts.enable_thread_safety_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_set_output_format_json)
{
    quarisma::profiler_options opts;
    opts.output_format_ = quarisma::profiler_options::output_format_enum::JSON;
    EXPECT_EQ(opts.output_format_, quarisma::profiler_options::output_format_enum::JSON);
}

QUARISMATEST(Profiler, enhanced_profiler_options_set_output_format_csv)
{
    quarisma::profiler_options opts;
    opts.output_format_ = quarisma::profiler_options::output_format_enum::CSV;
    EXPECT_EQ(opts.output_format_, quarisma::profiler_options::output_format_enum::CSV);
}

QUARISMATEST(Profiler, enhanced_profiler_options_set_output_file_path)
{
    quarisma::profiler_options opts;
    opts.output_file_path_ = "/tmp/profile.json";
    EXPECT_EQ(opts.output_file_path_, "/tmp/profile.json");
}

QUARISMATEST(Profiler, enhanced_profiler_options_set_max_samples)
{
    quarisma::profiler_options opts;
    opts.max_samples_ = 5000;
    EXPECT_EQ(opts.max_samples_, 5000);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_percentiles)
{
    quarisma::profiler_options opts;
    opts.calculate_percentiles_ = false;
    EXPECT_FALSE(opts.calculate_percentiles_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_peak_memory_tracking)
{
    quarisma::profiler_options opts;
    opts.track_peak_memory_ = false;
    EXPECT_FALSE(opts.track_peak_memory_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_disable_memory_deltas)
{
    quarisma::profiler_options opts;
    opts.track_memory_deltas_ = false;
    EXPECT_FALSE(opts.track_memory_deltas_);
}

QUARISMATEST(Profiler, enhanced_profiler_options_set_thread_pool_size)
{
    quarisma::profiler_options opts;
    opts.thread_pool_size_ = 16;
    EXPECT_EQ(opts.thread_pool_size_, 16);
}

// ============================================================================
// Enhanced Profiler Session Tests
// ============================================================================

QUARISMATEST(Profiler, enhanced_profiler_session_basic_creation)
{
    quarisma::profiler_options opts;
    auto                       session = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
}

QUARISMATEST(Profiler, enhanced_profiler_session_start_stop)
{
    quarisma::profiler_options opts;
    auto                       session = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());
    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_with_timing_enabled)
{
    quarisma::profiler_options opts;
    opts.enable_timing_ = true;
    auto session        = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope scope("test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_with_memory_tracking)
{
    quarisma::profiler_options opts;
    opts.enable_memory_tracking_ = true;
    auto session                 = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope scope("memory_scope", session.get());
        std::vector<int>         data(1000);
        for (int i = 0; i < 1000; ++i)
        {
            data[i] = i * 2;
        }
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_with_hierarchical_profiling)
{
    quarisma::profiler_options opts;
    opts.enable_hierarchical_profiling_ = true;
    auto session                        = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope outer("outer_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        {
            quarisma::profiler_scope inner("inner_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_with_statistical_analysis)
{
    quarisma::profiler_options opts;
    opts.enable_statistical_analysis_ = true;
    auto session                      = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 5; ++i)
    {
        quarisma::profiler_scope scope("stat_scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_all_features_enabled)
{
    quarisma::profiler_options opts;
    opts.enable_timing_                 = true;
    opts.enable_memory_tracking_        = true;
    opts.enable_hierarchical_profiling_ = true;
    opts.enable_statistical_analysis_   = true;
    opts.enable_thread_safety_          = true;

    auto session = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope outer("outer", session.get());
        std::vector<int>         data(1000);

        {
            quarisma::profiler_scope inner("inner", session.get());
            for (int i = 0; i < 1000; ++i)
            {
                data[i] = i * 2;
            }
        }
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_generate_chrome_trace_json)
{
    quarisma::profiler_options opts;
    opts.enable_timing_ = true;
    auto session        = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope scope("json_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    std::string json = session->generate_chrome_trace_json();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("json_test_scope"), std::string::npos);
}

QUARISMATEST(Profiler, enhanced_profiler_session_multiple_scopes)
{
    quarisma::profiler_options opts;
    auto                       session = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 10; ++i)
    {
        quarisma::profiler_scope scope("scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

QUARISMATEST(Profiler, enhanced_profiler_session_deeply_nested_scopes)
{
    quarisma::profiler_options opts;
    opts.enable_hierarchical_profiling_ = true;
    auto session                        = std::make_unique<quarisma::profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        quarisma::profiler_scope level1("level1", session.get());
        {
            quarisma::profiler_scope level2("level2", session.get());
            {
                quarisma::profiler_scope level3("level3", session.get());
                {
                    quarisma::profiler_scope level4("level4", session.get());
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }

    EXPECT_TRUE(session->stop());
}
#endif  // QUARISMA_HAS_NATIVE_PROFILER
