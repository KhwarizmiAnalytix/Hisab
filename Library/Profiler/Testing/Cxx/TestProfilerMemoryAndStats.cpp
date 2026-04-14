#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Profiler: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "ProfilerTest.h"
#include "native/session/profiler.h"

using namespace profiler;

// ============================================================================
// Memory Tracking and Statistical Analysis Tests
// ============================================================================

PROFILERTEST(Profiler, memory_tracking_enabled)
{
    profiler_options opts;
    opts.enable_memory_tracking_ = true;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope   scope("memory_test", session.get());
        std::vector<int> data(1000);
        for (int i = 0; i < 1000; ++i)
        {
            data[i] = i;
        }
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, memory_tracking_disabled)
{
    profiler_options opts;
    opts.enable_memory_tracking_ = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope   scope("no_memory_test", session.get());
        std::vector<int> data(1000);
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, statistical_analysis_enabled)
{
    profiler_options opts;
    opts.enable_statistical_analysis_ = true;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("stats_test", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, statistical_analysis_disabled)
{
    profiler_options opts;
    opts.enable_statistical_analysis_ = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("no_stats_test", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, hierarchical_profiling_enabled)
{
    profiler_options opts;
    opts.enable_hierarchical_profiling_ = true;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope outer("outer", session.get());
        {
            profiler_scope inner("inner", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());

    std::string json = session->generate_chrome_trace_json();
    EXPECT_NE(json.find("outer"), std::string::npos);
    EXPECT_NE(json.find("inner"), std::string::npos);
}

PROFILERTEST(Profiler, hierarchical_profiling_disabled)
{
    profiler_options opts;
    opts.enable_hierarchical_profiling_ = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope outer("outer", session.get());
        {
            profiler_scope inner("inner", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, timing_enabled)
{
    profiler_options opts;
    opts.enable_timing_ = true;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("timing_test", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, timing_disabled)
{
    profiler_options opts;
    opts.enable_timing_ = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("no_timing_test", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}

PROFILERTEST(Profiler, multiple_scopes_same_name)
{
    auto session = std::make_unique<profiler_session>(profiler_options());
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 5; ++i)
    {
        profiler_scope scope("repeated_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    std::string json = session->generate_chrome_trace_json();
    // Should contain multiple instances of the same scope name
    size_t count = 0;
    size_t pos   = 0;
    while ((pos = json.find("repeated_scope", pos)) != std::string::npos)
    {
        count++;
        pos += 14;
    }
    EXPECT_GE(count, 5);
}

PROFILERTEST(Profiler, scope_duration_measurement)
{
    auto session = std::make_unique<profiler_session>(profiler_options());
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    auto start = std::chrono::high_resolution_clock::now();
    {
        profiler_scope scope("duration_test", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto end = std::chrono::high_resolution_clock::now();

    EXPECT_TRUE(session->stop());

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration_ms, 10);
}

PROFILERTEST(Profiler, all_features_enabled)
{
    profiler_options opts;
    opts.enable_timing_                 = true;
    opts.enable_memory_tracking_        = true;
    opts.enable_hierarchical_profiling_ = true;
    opts.enable_statistical_analysis_   = true;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope   outer("outer", session.get());
        std::vector<int> data(1000);
        {
            profiler_scope inner("inner", session.get());
            for (int i = 0; i < 1000; ++i)
            {
                data[i] = i * 2;
            }
        }
    }

    EXPECT_TRUE(session->stop());

    std::string json = session->generate_chrome_trace_json();
    EXPECT_NE(json.find("outer"), std::string::npos);
    EXPECT_NE(json.find("inner"), std::string::npos);
}

PROFILERTEST(Profiler, all_features_disabled)
{
    profiler_options opts;
    opts.enable_timing_                 = false;
    opts.enable_memory_tracking_        = false;
    opts.enable_hierarchical_profiling_ = false;
    opts.enable_statistical_analysis_   = false;

    auto session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("disabled_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
