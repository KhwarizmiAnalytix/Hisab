#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

#include "baseTest.h"
#include "native/analysis/stats_calculator.h"

using namespace quarisma;

// ============================================================================
// Consolidated Stat Template Tests
// ============================================================================

QUARISMATEST(Profiler, stat_basic_operations_and_initialization)
{
    // Test 1: Empty initialization
    quarisma::stat<int64_t> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0);

    // Test 2: Single update
    s.update_stat(10);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.count(), 1);
    EXPECT_EQ(s.first(), 10);
    EXPECT_EQ(s.newest(), 10);

    // Test 3: Multiple updates
    s.update_stat(20);
    s.update_stat(30);
    EXPECT_EQ(s.count(), 3);
    EXPECT_EQ(s.first(), 10);
    EXPECT_EQ(s.newest(), 30);

    // Test 4: Sum calculation
    EXPECT_EQ(s.sum(), 60);

    // Test 5: Average calculation
    double avg = s.avg();
    EXPECT_DOUBLE_EQ(avg, 20.0);
}

QUARISMATEST(Profiler, stat_min_max_and_aggregations)
{
    quarisma::stat<int64_t> s;

    // Test min/max tracking
    s.update_stat(10);
    s.update_stat(5);
    s.update_stat(20);
    EXPECT_EQ(s.min(), 5);
    EXPECT_EQ(s.max(), 20);

    // Test squared_sum calculation (4 + 9 + 16 = 29)
    quarisma::stat<int64_t> s2;
    s2.update_stat(2);
    s2.update_stat(3);
    s2.update_stat(4);
    double squared_sum = s2.squared_sum();
    EXPECT_EQ(squared_sum, 29.0);
}

QUARISMATEST(Profiler, stat_variance_and_statistical_calculations)
{
    // Test 1: All same values - variance should be 0
    quarisma::stat<int64_t> s1;
    s1.update_stat(10);
    s1.update_stat(10);
    s1.update_stat(10);
    EXPECT_TRUE(s1.all_same());
    EXPECT_EQ(s1.variance(), 0);
    EXPECT_EQ(s1.std_deviation(), 0);

    // Test 2: Different values - variance should be > 0
    quarisma::stat<int64_t> s2;
    s2.update_stat(10);
    s2.update_stat(20);
    EXPECT_FALSE(s2.all_same());

    // Test 3: Variance calculation with multiple values
    quarisma::stat<int64_t> s3;
    s3.update_stat(1);
    s3.update_stat(2);
    s3.update_stat(3);
    s3.update_stat(4);
    s3.update_stat(5);
    double variance = s3.variance();
    EXPECT_GT(variance, 0);

    // Test 4: Sample variance
    quarisma::stat<int64_t> s4;
    s4.update_stat(1);
    s4.update_stat(2);
    s4.update_stat(3);
    int64_t sample_var = s4.sample_variance();
    EXPECT_GE(sample_var, 0);
}

QUARISMATEST(Profiler, stat_reset_and_state_management)
{
    quarisma::stat<int64_t> s;
    s.update_stat(10);
    s.update_stat(20);
    s.update_stat(30);
    EXPECT_EQ(s.count(), 3);

    // Test reset functionality
    s.reset();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.count(), 0);
    EXPECT_EQ(s.sum(), 0);
}

QUARISMATEST(Profiler, stat_output_stream_operations)
{
    // Test 1: Empty stat output
    quarisma::stat<int64_t> s1;
    std::ostringstream      oss1;
    s1.output_to_stream(&oss1);
    std::string output1 = oss1.str();
    EXPECT_NE(output1.find("count=0"), std::string::npos);

    // Test 2: Output with data
    quarisma::stat<int64_t> s2;
    s2.update_stat(10);
    s2.update_stat(20);
    std::ostringstream oss2;
    s2.output_to_stream(&oss2);
    std::string output2 = oss2.str();
    EXPECT_NE(output2.find("count=2"), std::string::npos);

    // Test 3: Output with all same values
    quarisma::stat<int64_t> s3;
    s3.update_stat(10);
    s3.update_stat(10);
    s3.update_stat(10);
    std::ostringstream oss3;
    s3.output_to_stream(&oss3);
    std::string output3 = oss3.str();
    EXPECT_NE(output3.find("all same"), std::string::npos);

    // Test 4: Output stream operator
    quarisma::stat<int64_t> s4;
    s4.update_stat(10);
    s4.update_stat(20);
    std::ostringstream oss4;
    oss4 << s4;
    std::string output4 = oss4.str();
    EXPECT_NE(output4.find("count=2"), std::string::npos);
}

QUARISMATEST(Profiler, stat_edge_cases_and_special_values)
{
    // Test 1: Negative values
    quarisma::stat<int64_t> s1;
    s1.update_stat(-10);
    s1.update_stat(-5);
    s1.update_stat(0);
    s1.update_stat(5);
    s1.update_stat(10);
    EXPECT_EQ(s1.min(), -10);
    EXPECT_EQ(s1.max(), 10);
    EXPECT_EQ(s1.sum(), 0);

    // Test 2: Large values
    quarisma::stat<int64_t> s2;
    s2.update_stat(1000000000);
    s2.update_stat(2000000000);
    s2.update_stat(3000000000);
    EXPECT_EQ(s2.count(), 3);
    EXPECT_EQ(s2.sum(), 6000000000LL);

    // Test 3: Double type support
    quarisma::stat<double> s3;
    s3.update_stat(1.5);
    s3.update_stat(2.5);
    s3.update_stat(3.5);
    EXPECT_EQ(s3.count(), 3);
    EXPECT_DOUBLE_EQ(s3.avg(), 2.5);
    EXPECT_DOUBLE_EQ(s3.sum(), 7.5);
}

QUARISMATEST(Profiler, stat_with_percentiles_basic_operations)
{
    // Test 1: Empty initialization
    quarisma::stat_with_percentiles<int64_t> s1;
    EXPECT_TRUE(s1.empty());

    // Test 2: Multiple updates and count tracking
    quarisma::stat_with_percentiles<int64_t> s2;
    for (int i = 1; i <= 100; ++i)
    {
        s2.update_stat(i);
    }
    EXPECT_EQ(s2.count(), 100);

    // Test 3: Single value percentile
    quarisma::stat_with_percentiles<int64_t> s3;
    s3.update_stat(42);
    int64_t p50_single = s3.percentile(50);
    EXPECT_EQ(p50_single, 42);

    // Test 4: Two values percentile
    quarisma::stat_with_percentiles<int64_t> s4;
    s4.update_stat(10);
    s4.update_stat(20);
    int64_t p50_two = s4.percentile(50);
    EXPECT_GE(p50_two, 10);
    EXPECT_LE(p50_two, 20);
}

QUARISMATEST(Profiler, stat_with_percentiles_calculations)
{
    // Prepare dataset 1-100
    quarisma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    // Test 1: Percentile 0
    int64_t p0 = s.percentile(0);
    EXPECT_GT(p0, 0);

    // Test 2: Percentile 25
    int64_t p25 = s.percentile(25);
    EXPECT_GT(p25, 0);
    EXPECT_LE(p25, 100);

    // Test 3: Percentile 50 (median)
    int64_t p50 = s.percentile(50);
    EXPECT_GT(p50, 0);

    // Test 4: Percentile 75
    int64_t p75 = s.percentile(75);
    EXPECT_GT(p75, 0);
    EXPECT_LE(p75, 100);

    // Test 5: Percentile 99
    int64_t p99 = s.percentile(99);
    EXPECT_GT(p99, 0);
    EXPECT_LE(p99, 100);

    // Test 6: Percentile 100 (max)
    int64_t p100 = s.percentile(100);
    EXPECT_EQ(p100, 100);

    // Test 7: Invalid percentile (should return NaN or 0)
    int64_t p_invalid = s.percentile(101);
    EXPECT_TRUE(std::isnan(static_cast<double>(p_invalid)) || p_invalid == 0);
}

QUARISMATEST(Profiler, stat_with_percentiles_output_stream)
{
    quarisma::stat_with_percentiles<int64_t> s;
    for (int i = 1; i <= 100; ++i)
    {
        s.update_stat(i);
    }

    std::ostringstream oss;
    s.output_to_stream(&oss);

    std::string output = oss.str();
    // Verify percentile markers are present in output
    EXPECT_NE(output.find("p5="), std::string::npos);
    EXPECT_NE(output.find("median="), std::string::npos);
    EXPECT_NE(output.find("p95="), std::string::npos);
}

#endif  // PROFILER_HAS_NATIVE_PROFILER
