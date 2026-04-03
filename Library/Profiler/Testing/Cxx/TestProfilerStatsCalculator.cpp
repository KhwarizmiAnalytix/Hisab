#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Test suite for stats_calculator class
 * Tests statistics calculation, aggregation, and output formatting
 */

#include "Testing/baseTest.h"
#include "native/analysis/stat_summarizer_options.h"
#include "native/analysis/stats_calculator.h"

using namespace quarisma;

// Test stats_calculator initialization
QUARISMATEST(Profiler, stats_calculator_default_initialization)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    EXPECT_EQ(calc.num_runs(), 0);
    EXPECT_TRUE(calc.get_details().empty());
}

// Test stats_calculator with custom options
QUARISMATEST(Profiler, stats_calculator_custom_options)
{
    stat_summarizer_options options;
    options.show_time     = false;
    options.show_memory   = false;
    options.format_as_csv = true;

    stats_calculator calc(options);
    EXPECT_EQ(calc.num_runs(), 0);
}

// Test update_run_total_us single update
QUARISMATEST(Profiler, stats_calculator_update_run_total_single)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.update_run_total_us(1000);
    EXPECT_EQ(calc.num_runs(), 1);
    EXPECT_EQ(calc.run_total_us().sum(), 1000);
}

// Test update_run_total_us multiple updates
QUARISMATEST(Profiler, stats_calculator_update_run_total_multiple)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.update_run_total_us(1000);
    calc.update_run_total_us(2000);
    calc.update_run_total_us(3000);

    EXPECT_EQ(calc.num_runs(), 3);
    EXPECT_EQ(calc.run_total_us().sum(), 6000);
    EXPECT_EQ(calc.run_total_us().min(), 1000);
    EXPECT_EQ(calc.run_total_us().max(), 3000);
}

// Test update_memory_used
QUARISMATEST(Profiler, stats_calculator_update_memory_used)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.update_memory_used(1024);
    calc.update_memory_used(2048);

    EXPECT_EQ(calc.run_total_us().count(), 0);  // Memory doesn't affect run count
}

// Test add_node_stats single node
QUARISMATEST(Profiler, stats_calculator_add_node_stats_single)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);

    const auto& details = calc.get_details();
    EXPECT_EQ(details.size(), 1);
    EXPECT_NE(details.find("node1"), details.end());
}

// Test add_node_stats multiple nodes
QUARISMATEST(Profiler, stats_calculator_add_node_stats_multiple)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type2", 1, 2000, 1024);
    calc.add_node_stats("node3", "type1", 2, 1500, 768);

    const auto& details = calc.get_details();
    EXPECT_EQ(details.size(), 3);
}

// Test add_node_stats accumulation
QUARISMATEST(Profiler, stats_calculator_add_node_stats_accumulation)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node1", "type1", 1, 2000, 1024);

    const auto& details = calc.get_details();
    EXPECT_EQ(details.size(), 1);

    const auto& detail = details.at("node1");
    EXPECT_EQ(detail.elapsed_time.count(), 2);
    EXPECT_EQ(detail.elapsed_time.sum(), 3000);
}

// Test get_output_string empty
QUARISMATEST(Profiler, stats_calculator_get_output_string_empty)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    std::string output = calc.get_output_string();
    EXPECT_FALSE(output.empty());  // Should have header even if empty
}

// Test get_output_string with data
QUARISMATEST(Profiler, stats_calculator_get_output_string_with_data)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type2", 1, 2000, 1024);
    calc.update_run_total_us(3000);

    // Just verify the method can be called without crashing
    std::string output = calc.get_output_string();
    EXPECT_FALSE(output.empty());
}

// Test get_short_summary
QUARISMATEST(Profiler, stats_calculator_get_short_summary)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);

    std::string summary = calc.get_short_summary();
    EXPECT_FALSE(summary.empty());
}

// Test get_stats_by_node_type empty
QUARISMATEST(Profiler, stats_calculator_get_stats_by_node_type_empty)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    std::string stats = calc.get_stats_by_node_type();
    EXPECT_FALSE(stats.empty());  // Should have header
}

// Test get_stats_by_node_type with data
QUARISMATEST(Profiler, stats_calculator_get_stats_by_node_type_with_data)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type1", 1, 2000, 1024);
    calc.add_node_stats("node3", "type2", 2, 1500, 768);
    calc.update_run_total_us(4500);

    std::string stats = calc.get_stats_by_node_type();
    EXPECT_FALSE(stats.empty());
}

// Test compute_stats_by_type
QUARISMATEST(Profiler, stats_calculator_compute_stats_by_type)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type1", 1, 2000, 1024);
    calc.add_node_stats("node3", "type2", 2, 1500, 768);
    calc.update_run_total_us(4500);

    std::map<std::string, int64_t> type_count;
    std::map<std::string, int64_t> type_time;
    std::map<std::string, int64_t> type_memory;
    std::map<std::string, int64_t> type_times_called;
    int64_t                        accumulated_us = 0;

    calc.compute_stats_by_type(
        &type_count, &type_time, &type_memory, &type_times_called, &accumulated_us);

    EXPECT_EQ(type_count.size(), 2);
}

// Test get_stats_by_metric BY_NAME
QUARISMATEST(Profiler, stats_calculator_get_stats_by_metric_by_name)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node_a", "type1", 0, 1000, 512);
    calc.add_node_stats("node_b", "type1", 1, 2000, 1024);
    calc.update_run_total_us(3000);

    std::string stats =
        calc.get_stats_by_metric("By Name", stats_calculator::sorting_metric_enum::BY_NAME, 10);
    EXPECT_FALSE(stats.empty());
}

// Test get_stats_by_metric BY_TIME
QUARISMATEST(Profiler, stats_calculator_get_stats_by_metric_by_time)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type1", 1, 5000, 1024);
    calc.update_run_total_us(6000);

    std::string stats =
        calc.get_stats_by_metric("By Time", stats_calculator::sorting_metric_enum::BY_TIME, 10);
    EXPECT_FALSE(stats.empty());
}

// Test get_stats_by_metric BY_MEMORY
QUARISMATEST(Profiler, stats_calculator_get_stats_by_metric_by_memory)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type1", 1, 2000, 2048);
    calc.update_run_total_us(3000);

    std::string stats =
        calc.get_stats_by_metric("By Memory", stats_calculator::sorting_metric_enum::BY_MEMORY, 10);
    EXPECT_FALSE(stats.empty());
}

// Test get_stats_by_metric BY_TYPE
QUARISMATEST(Profiler, stats_calculator_get_stats_by_metric_by_type)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type2", 1, 2000, 1024);
    calc.update_run_total_us(3000);

    std::string stats =
        calc.get_stats_by_metric("By Type", stats_calculator::sorting_metric_enum::BY_TYPE, 10);
    EXPECT_FALSE(stats.empty());
}

// Test large dataset
QUARISMATEST(Profiler, stats_calculator_large_dataset)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    for (int i = 0; i < 100; ++i)
    {
        calc.add_node_stats(
            "node" + std::to_string(i),
            "type" + std::to_string(i % 5),
            i,
            1000 + i * 100,
            512 + i * 10);
    }

    EXPECT_EQ(calc.get_details().size(), 100);
}

// Test CSV format output
QUARISMATEST(Profiler, stats_calculator_csv_format)
{
    stat_summarizer_options options;
    options.format_as_csv = true;
    stats_calculator calc(options);

    calc.add_node_stats("node1", "type1", 0, 1000, 512);
    calc.add_node_stats("node2", "type2", 1, 2000, 1024);
    calc.update_run_total_us(3000);

    std::string output = calc.get_output_string();
    EXPECT_FALSE(output.empty());
}

// Test run_total_us statistics
QUARISMATEST(Profiler, stats_calculator_run_total_us_statistics)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.update_run_total_us(1000);
    calc.update_run_total_us(2000);
    calc.update_run_total_us(3000);

    const auto& run_stats = calc.run_total_us();
    EXPECT_EQ(run_stats.count(), 3);
    EXPECT_EQ(run_stats.min(), 1000);
    EXPECT_EQ(run_stats.max(), 3000);
    EXPECT_DOUBLE_EQ(run_stats.avg(), 2000.0);
}

// Test node detail access
QUARISMATEST(Profiler, stats_calculator_node_detail_access)
{
    stat_summarizer_options options;
    stats_calculator        calc(options);

    calc.add_node_stats("test_node", "test_type", 5, 1500, 768);

    const auto& details = calc.get_details();
    const auto& detail  = details.at("test_node");

    EXPECT_EQ(detail.name, "test_node");
    EXPECT_EQ(detail.type, "test_type");
    EXPECT_EQ(detail.run_order, 5);
    EXPECT_EQ(detail.times_called, 1);
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
