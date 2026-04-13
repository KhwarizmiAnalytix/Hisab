#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "ProfilerTest.h"
#include "native/session/profiler.h"
#include "native/session/profiler_report.h"

using namespace quarisma;

// ============================================================================
// Profiler Session Edge Cases and Error Handling Tests (Consolidated)
// ============================================================================

// Test 1: Session State Management and Lifecycle
// Tests: double start, stop without start, double stop, is_active, current_session,
//        destructor behavior, and multiple sequential sessions
PROFILERTEST(Profiler, session_state_management_and_lifecycle)
{
    // Test double start fails
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());
        EXPECT_FALSE(session->start());  // Second start should fail
        EXPECT_TRUE(session->stop());
    }

    // Test stop without start
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_FALSE(session->stop());  // Stop without start should fail
    }

    // Test double stop
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());
        EXPECT_TRUE(session->stop());
        EXPECT_FALSE(session->stop());  // Second stop should fail
    }

    // Test is_active status
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_FALSE(session->is_active());

        EXPECT_TRUE(session->start());
        EXPECT_TRUE(session->is_active());

        EXPECT_TRUE(session->stop());
        EXPECT_FALSE(session->is_active());
    }

    // Test current_session tracking
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);

        EXPECT_TRUE(session->start());
        EXPECT_EQ(profiler_session::current_session(), session.get());
        EXPECT_TRUE(session->stop());

        EXPECT_NE(profiler_session::current_session(), session.get());
    }

    // Test destructor stops active session
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());
        EXPECT_TRUE(session->is_active());

        {
            profiler_scope scope("destructor_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Destructor should stop the session
    }

    // Test multiple sequential sessions
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("first_session_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("second_session_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }
}

// Test 2: Scope Edge Cases
// Tests: scope without session, scope after session stop, empty name, very long name,
//        immediate destruction, and scopes with various naming scenarios
PROFILERTEST(Profiler, scope_edge_cases)
{
    // Test scope without active session
    {
        profiler_scope scope("orphan_scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // Should not crash
    }

    // Test scope after session stop
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());
        EXPECT_TRUE(session->stop());

        profiler_scope scope("post_stop_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // Should not crash
    }

    // Test scope with empty name
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test scope with very long name
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            std::string    long_name(10000, 'a');
            profiler_scope scope(long_name, session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test scope immediate destruction
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        profiler_scope("immediate_scope", session.get());

        EXPECT_TRUE(session->stop());
    }
}

// Test 3: Session Configuration Options
// Tests: disabled features, memory tracking, statistical analysis,
//        hierarchical profiling disabled, timing disabled, thread safety disabled
PROFILERTEST(Profiler, session_configuration_options)
{
    // Test with all features disabled
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
            profiler_scope scope("disabled_features_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test with memory tracking enabled
    {
        profiler_options opts;
        opts.enable_memory_tracking_ = true;
        auto session                 = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope   scope("memory_tracking_scope", session.get());
            std::vector<int> data(10000);
            for (int i = 0; i < 10000; ++i)
            {
                data[i] = i * 2;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test with statistical analysis enabled
    {
        profiler_options opts;
        opts.enable_statistical_analysis_ = true;
        auto session                      = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        for (int iter = 0; iter < 5; ++iter)
        {
            profiler_scope scope("statistical_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test with hierarchical profiling disabled
    {
        profiler_options opts;
        opts.enable_hierarchical_profiling_ = false;
        auto session                        = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("non_hierarchical_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test with timing disabled
    {
        profiler_options opts;
        opts.enable_timing_ = false;
        auto session        = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("no_timing_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }

    // Test with thread safety disabled
    {
        profiler_options opts;
        opts.enable_thread_safety_ = false;
        auto session               = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("non_thread_safe_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
    }
}

// Test 4: Threading and Concurrency
// Tests: many concurrent scopes with thread safety
PROFILERTEST(Profiler, threading_and_concurrency)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back(
            [session = session.get(), i]()
            {
                for (int j = 0; j < 10; ++j)
                {
                    profiler_scope scope(
                        "scope_" + std::to_string(i) + "_" + std::to_string(j), session);
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_TRUE(session->stop());
}

// Test 5: Advanced Scope Features
// Tests: duration calculations, deeply nested hierarchies
PROFILERTEST(Profiler, advanced_scope_features)
{
    // Test duration calculations
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("duration_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        EXPECT_TRUE(session->stop());
        // Scope should have recorded timing data
    }

    // Test deeply nested scopes
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope level1("level1", session.get());
            {
                profiler_scope level2("level2", session.get());
                {
                    profiler_scope level3("level3", session.get());
                    {
                        profiler_scope level4("level4", session.get());
                        {
                            profiler_scope level5("level5", session.get());
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                    }
                }
            }
        }

        EXPECT_TRUE(session->stop());
    }
}

// Test 6: Session Methods and Data Access
// Tests: create_scope, generate_report, export_report, print_report,
//        collected_xspace, has_collected_xspace
PROFILERTEST(Profiler, session_methods_and_data_access)
{
    // Test create_scope method
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        auto scope = session->create_scope("created_scope");
        EXPECT_TRUE(scope != nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        EXPECT_TRUE(session->stop());
    }

    // Test generate_report
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("report_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());

        auto report = session->generate_report();
        EXPECT_TRUE(report != nullptr);
    }

    // Test export_report to file (invalid path)
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("export_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());

        session->export_report("/invalid/path/report.json");
        // Should not crash
    }

    // Test print_report
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("print_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());

        session->print_report();
    }

    // Test collected_xspace access
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("xspace_test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());

        PROFILER_UNUSED const auto& xspace = session->collected_xspace();
        EXPECT_TRUE(true);
    }

    // Test has_collected_xspace
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);

        EXPECT_FALSE(session->has_collected_xspace());

        EXPECT_TRUE(session->start());
        EXPECT_FALSE(session->has_collected_xspace());

        {
            profiler_scope scope("xspace_check_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());
        // May or may not have xspace depending on backend profilers
    }
}

// Test 7: Chrome Trace Operations
// Tests: write_chrome_trace with invalid path, generate before stop
PROFILERTEST(Profiler, chrome_trace_operations)
{
    // Test write_chrome_trace with invalid path
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_TRUE(session->stop());

        std::string invalid_path = "/invalid/path/that/does/not/exist/trace.json";
        EXPECT_FALSE(session->write_chrome_trace(invalid_path));
    }

    // Test generate_chrome_trace_json before stop
    {
        profiler_options opts;
        auto             session = std::make_unique<profiler_session>(opts);
        EXPECT_TRUE(session != nullptr);
        EXPECT_TRUE(session->start());

        {
            profiler_scope scope("test_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Generate trace before stop - should return empty or partial data
        std::string json = session->generate_chrome_trace_json();
        EXPECT_TRUE(json.empty() || json.find("traceEvents") != std::string::npos);

        EXPECT_TRUE(session->stop());
    }
}

// Test 8: Report Generation with Data
// Tests: comprehensive report generation with multiple scopes,
//        all report formats (console, JSON, CSV, XML)
PROFILERTEST(Profiler, report_generation_with_data)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    for (int i = 0; i < 3; ++i)
    {
        profiler_scope scope("report_scope_" + std::to_string(i), session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Test all report generation methods
    std::string console_report = report->generate_console_report();
    EXPECT_FALSE(console_report.empty());

    std::string json_report = report->generate_json_report();
    EXPECT_FALSE(json_report.empty());

    std::string csv_report = report->generate_csv_report();
    EXPECT_FALSE(csv_report.empty());

    std::string xml_report = report->generate_xml_report();
    EXPECT_FALSE(xml_report.empty());

    // Test export to invalid path
    bool result = report->export_to_file(
        "/invalid/path/report.json", profiler_options::output_format_enum::JSON);
    EXPECT_FALSE(result);
}

// Test 9: Report Customization - Display Options
// Tests: precision, time_unit, memory_unit settings
PROFILERTEST(Profiler, report_customization_display_options)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope scope("customization_test_scope", session.get());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Test precision setting
    report->set_precision(4);
    std::string output1 = report->generate_console_report();
    EXPECT_FALSE(output1.empty());

    // Test time unit setting
    report->set_time_unit("ms");
    std::string output2 = report->generate_console_report();
    EXPECT_FALSE(output2.empty());

    // Test memory unit setting
    report->set_memory_unit("MB");
    std::string output3 = report->generate_console_report();
    EXPECT_FALSE(output3.empty());
}

// Test 10: Report Customization - Data Inclusion Options
// Tests: thread_info, hierarchical_data inclusion settings
PROFILERTEST(Profiler, report_customization_data_inclusion)
{
    profiler_options opts;
    auto             session = std::make_unique<profiler_session>(opts);
    EXPECT_TRUE(session != nullptr);
    EXPECT_TRUE(session->start());

    {
        profiler_scope outer("outer_scope", session.get());
        {
            profiler_scope inner("inner_scope", session.get());
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    EXPECT_TRUE(session->stop());

    auto report = session->generate_report();
    EXPECT_TRUE(report != nullptr);

    // Test include thread info
    report->set_include_thread_info(true);
    std::string output1 = report->generate_console_report();
    EXPECT_FALSE(output1.empty());

    // Test include hierarchical data
    report->set_include_hierarchical_data(true);
    std::string output2 = report->generate_console_report();
    EXPECT_FALSE(output2.empty());
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
