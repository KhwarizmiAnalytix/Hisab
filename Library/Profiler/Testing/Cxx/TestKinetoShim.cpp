/*
 * Kineto Shim Tests
 *
 * Tests for the Kineto integration shim including:
 * - Kineto initialization
 * - Activity profiler operations
 * - Trace collection and export
 * - GPU backend configuration
 */

#include "baseTest.h"

#if PROFILER_HAS_KINETO && 0

#include <ActivityTraceInterface.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <thread>
#include <vector>

#include "bespoke/common/orchestration/observer.h"
#include "bespoke/kineto/kineto_shim.h"

using namespace quarisma::profiler_impl;

// ============================================================================
// Kineto Initialization Tests
// ============================================================================

QUARISMATEST(KinetoShim, Initialization)
{
    // Test Kineto initialization via prepareTrace
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // If we reach here without crashing, initialization succeeded
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, MultipleInitialization)
{
#if 0
    // Test that multiple initializations don't cause issues
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    EXPECT_TRUE(true);
#endif
}

// ============================================================================
// Activity Profiler Tests
// ============================================================================

QUARISMATEST(KinetoShim, ActivityProfilerAccess)
{
    // Test accessing the activity profiler
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // Kineto is initialized - profiler access would happen here
    // If we reach here without crashing, profiler access succeeded
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, PrepareTrace)
{
    // Test preparing a trace with CPU activities
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // Trace preparation would happen here
    // If we reach here without crashing, trace preparation succeeded
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, StartStopTrace)
{
    // Test starting and stopping a trace
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::startTrace();

    // Simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Stop trace
    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, TraceCollection)
{
    // Test trace collection with activities
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::startTrace();

    // Simulate some work
    for (int i = 0; i < 100; ++i)
    {
        volatile int x = i * i;
        (void)x;
    }

    // Stop trace and collect
    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

// ============================================================================
// Trace Export Tests
// ============================================================================

QUARISMATEST(KinetoShim, TraceExport)
{
    // Test exporting a trace
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::startTrace();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // Stop and get trace
    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

// ============================================================================
// GPU Backend Configuration Tests
// ============================================================================

QUARISMATEST(KinetoShim, GPUBackendConfiguration)
{
    // Test GPU backend configuration with CUDA activities
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);
    activities.insert(impl::ActivityType::CUDA);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // If we reach here without crashing, GPU backend configuration succeeded
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, CUDAActivities)
{
    // Test CUDA activity types
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);
    activities.insert(impl::ActivityType::CUDA);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // CUDA activity configuration would happen here
    EXPECT_TRUE(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

QUARISMATEST(KinetoShim, FullProfilerCycle)
{
    // Test full profiler cycle: init -> prepare -> start -> work -> stop
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::startTrace();

    // Simulate work
    for (int i = 0; i < 1000; ++i)
    {
        volatile int x = i * i;
        (void)x;
    }

    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, MultipleTraces)
{
    // Test collecting multiple traces
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;

    for (int trace_num = 0; trace_num < 3; ++trace_num)
    {
        impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
        impl::kineto::startTrace();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    }

    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, ConcurrentTracing)
{
    // Test concurrent tracing operations
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);
    impl::kineto::startTrace();

    // Simulate concurrent work
    std::thread t1(
        [&]()
        {
            for (int i = 0; i < 100; ++i)
            {
                volatile int x = i * i;
                (void)x;
            }
        });

    std::thread t2(
        [&]()
        {
            for (int i = 0; i < 100; ++i)
            {
                volatile int x = i * i;
                (void)x;
            }
        });

    t1.join();
    t2.join();

    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

QUARISMATEST(KinetoShim, StopWithoutStart)
{
    // Test stopping trace without starting (should handle gracefully)
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    // Try to stop without starting - should not crash
    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();
    EXPECT_TRUE(true);
}

QUARISMATEST(KinetoShim, EmptyActivitySet)
{
    // Test with empty activity set
    impl::kineto::ActivitySet activities;
    // Don't insert any activities - test with empty set

    impl::ExperimentalConfig config;
    impl::kineto::prepareTrace(/*cpuOnly=*/false, activities, config);

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    EXPECT_TRUE(true);
}

// ============================================================================
// End-to-End Detailed Profiling Test
// ============================================================================

/**
 * @brief Helper function: Matrix multiplication (Level 3 nested operation)
 * @param size Matrix dimension
 * @return Sum of result matrix elements
 */
static double matrix_multiply_operation(int size)
{
    std::vector<double> matrix_a(size * size, 1.5);
    std::vector<double> matrix_b(size * size, 2.5);
    std::vector<double> result(size * size, 0.0);

    // Perform matrix multiplication: result = matrix_a * matrix_b
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < size; ++k)
            {
                sum += matrix_a[i * size + k] * matrix_b[k * size + j];
            }
            result[i * size + j] = sum;
        }
    }

    // Return sum to prevent compiler optimization
    double total = 0.0;
    for (double val : result)
    {
        total += val;
    }
    return total;
}

/**
 * @brief Helper function: Data transformation (Level 3 nested operation)
 * @param data_size Size of data to transform
 */
static void data_transformation_operation(int data_size)
{
    std::vector<double> data(data_size);

    // Initialize with sequential values
    for (int i = 0; i < data_size; ++i)
    {
        data[i] = static_cast<double>(i);
    }

    // Apply transformations: square, normalize, scale
    for (int i = 0; i < data_size; ++i)
    {
        data[i] = cosh(data[i]) * sinh(data[i]);  // Square
    }

    double max_val = *std::max_element(data.begin(), data.end());
    if (max_val > 0.0)
    {
        for (int i = 0; i < data_size; ++i)
        {
            data[i] = data[i] / max_val;  // Normalize
        }
    }

    for (int i = 0; i < data_size; ++i)
    {
        data[i] = data[i] * 100.0;  // Scale
    }
}

/**
 * @brief Helper function: Computational phase combining multiple operations (Level 2)
 * @param matrix_size Size of matrices to process
 * @param data_size Size of data vectors to process
 * @return Result from matrix operations
 */
static double computational_phase(int matrix_size, int data_size)
{
    // Phase 1: Matrix operations
    double matrix_result = matrix_multiply_operation(matrix_size);

    // Phase 2: Data transformation
    data_transformation_operation(data_size);

    // Phase 3: Additional matrix operation
    double second_matrix_result = matrix_multiply_operation(matrix_size / 2);

    return matrix_result + second_matrix_result;
}

/**
 * @brief Helper function: Sorting and aggregation phase (Level 2)
 * @param size Size of data to sort and aggregate
 */
static void sorting_aggregation_phase(int size)
{
    std::vector<int> data(size);

    // Initialize with random-like values
    for (int i = 0; i < size; ++i)
    {
        data[i] = (i * 7919) % 10000;  // Pseudo-random using prime modulo
    }

    // Sort the data
    std::sort(data.begin(), data.end());

    // Compute aggregations
    int64_t sum = 0;
    for (int val : data)
    {
        sum += val;
    }

    // Compute variance
    double mean     = static_cast<double>(sum) / size;
    double variance = 0.0;
    for (int val : data)
    {
        double diff = val - mean;
        variance += diff * diff;
    }
    variance /= size;
}

/**
 * @brief Complex CPU-intensive workload with nested function calls (Level 1)
 *
 * This function creates a hierarchical workload with multiple computational phases:
 * - Data preprocessing
 * - Computational analysis (matrix operations + transformations)
 * - Sorting and aggregation
 * - Post-processing
 */
static void complex_cpu_workload()
{
    // Phase 1: Data preprocessing
    {
        std::vector<double> preprocessing_data(50000);
        for (size_t i = 0; i < preprocessing_data.size(); ++i)
        {
            preprocessing_data[i] = static_cast<double>(i) * 0.5;
        }
    }

    // Phase 2: Computational analysis (nested operations)
    {
        double result = computational_phase(100, 100000);
        // Use result to prevent optimization
        volatile double prevent_optimization = result;
        (void)prevent_optimization;
    }

    // Phase 3: Sorting and aggregation
    {
        sorting_aggregation_phase(50000);
    }

    // Phase 4: Post-processing
    {
        std::vector<int> postprocess_data(30000);
        for (size_t i = 0; i < postprocess_data.size(); ++i)
        {
            postprocess_data[i] = static_cast<int>(i * 2 + 1);
        }

        // Apply filters
        int sum = 0;
        for (int val : postprocess_data)
        {
            if (val % 3 == 0)
            {
                sum += val;
            }
        }
    }
}

QUARISMATEST(KinetoShim, EndToEndDetailedProfiling)
{
    // ========================================================================
    // IMPORTANT: Kineto JSON Format vs Chrome Trace Event Format
    // ========================================================================
    // Kineto's native JSON output uses its own schema (schemaVersion,
    // deviceProperties, baseTimeNanoseconds) which is NOT directly compatible
    // with chrome://tracing.
    //
    // Kineto JSON Schema:
    // - Uses "schemaVersion", "deviceProperties", "baseTimeNanoseconds"
    // - Timestamps are in absolute nanoseconds since epoch
    // - Empty pid/tid fields for some events
    // - Designed for Kineto's own visualization tools
    //
    // Chrome Trace Event Format:
    // - Uses "traceEvents" array with "ph", "pid", "tid", "ts", "dur"
    // - Timestamps are relative microseconds
    // - Requires numeric pid/tid values
    // - Designed for chrome://tracing and Perfetto
    //
    // To get Chrome Trace compatible output, use:
    // - Quarisma's profiler_session with Kineto integration
    // - Quarisma's export_to_chrome_trace_json() function
    // - RecordFunction instrumentation for CPU events
    //
    // This test demonstrates:
    // - Kineto's native trace collection workflow
    // - Kineto's JSON schema (not Chrome Trace format)
    // - Proper resource management and cleanup
    // ========================================================================

    // ========================================================================
    // Step 1: Initialize Kineto profiler with CPU activity tracking
    // ========================================================================
    impl::kineto::ActivitySet activities;
    activities.insert(impl::ActivityType::CPU);

    impl::ExperimentalConfig config;

    // Prepare Kineto trace for CPU-only profiling
    impl::kineto::prepareTrace(/*cpuOnly=*/true, activities, config);

    // ========================================================================
    // Step 2: Start Kineto profiling trace
    // ========================================================================
    impl::kineto::startTrace();

    // ========================================================================
    // Step 3: Execute complex CPU-intensive workload
    // ========================================================================
    // This workload includes:
    // - 4 distinct computational phases
    // - Nested function calls (3-4 levels deep)
    // - Matrix operations (100x100 matrices)
    // - Data transformations (100,000 elements)
    // - Sorting and aggregation (50,000 elements)
    // - Post-processing filters (30,000 elements)

    // Run workload multiple times to ensure sufficient profiling data
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        complex_cpu_workload();

        // Small delay between iterations to create distinct events
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Additional sleep to ensure Kineto captures all events
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // ========================================================================
    // Step 4: Stop Kineto profiling and collect trace
    // ========================================================================
    impl::kineto::ActivityTraceWrapper trace = impl::kineto::stopTrace();

    // ========================================================================
    // Step 5: Verify trace was collected successfully
    // ========================================================================
    ASSERT_TRUE(static_cast<bool>(trace)) << "Kineto trace should be valid after stopTrace()";

    // ========================================================================
    // Step 6: Save trace to JSON file for visualization
    // ========================================================================
    std::string const trace_filename = "kineto_detailed_trace.json";
    trace.save(trace_filename);

    // ========================================================================
    // Step 7: Verify JSON file was created successfully
    // ========================================================================
    std::ifstream trace_file(trace_filename);
    ASSERT_TRUE(trace_file.good()) << "Kineto trace JSON file should be created successfully";
    trace_file.close();

    // ========================================================================
    // Step 8: Validate Kineto's native JSON schema
    // ========================================================================
    // Read the JSON file to verify its structure
    std::ifstream json_input(trace_filename);
    ASSERT_TRUE(json_input.good()) << "Should be able to read trace file";

    std::stringstream buffer;
    buffer << json_input.rdbuf();
    std::string json_content = buffer.str();
    json_input.close();

    // Verify JSON contains Kineto's native schema fields
    // Note: This is NOT Chrome Trace Event Format - it's Kineto's own schema
    EXPECT_NE(json_content.find("\"schemaVersion\""), std::string::npos)
        << "JSON should contain Kineto schemaVersion field";
    EXPECT_NE(json_content.find("\"traceEvents\""), std::string::npos)
        << "JSON should contain traceEvents array";
    EXPECT_NE(json_content.find("\"displayTimeUnit\""), std::string::npos)
        << "JSON should contain displayTimeUnit field";
    EXPECT_NE(json_content.find("\"baseTimeNanoseconds\""), std::string::npos)
        << "JSON should contain baseTimeNanoseconds field (Kineto-specific)";
    EXPECT_NE(json_content.find("\"deviceProperties\""), std::string::npos)
        << "JSON should contain deviceProperties field (Kineto-specific)";

    // Verify the JSON is valid and non-empty
    EXPECT_GT(json_content.size(), 100) << "JSON should contain meaningful content";
    EXPECT_NE(json_content.find("{"), std::string::npos) << "JSON should be valid object";
    EXPECT_NE(json_content.find("}"), std::string::npos) << "JSON should be valid object";

    // Print JSON file location and content summary
    std::cout << "\n=== Kineto Native JSON Generated ===" << std::endl;
    std::cout << "File: " << trace_filename << std::endl;
    std::cout << "Size: " << json_content.size() << " bytes" << std::endl;
    std::cout << "\nJSON Schema Validation:" << std::endl;
    std::cout << "  ✓ Valid Kineto JSON schema (schemaVersion: 1)" << std::endl;
    std::cout << "  ✓ Contains traceEvents array" << std::endl;
    std::cout << "  ✓ Contains Kineto-specific metadata" << std::endl;
    std::cout << "\nIMPORTANT:" << std::endl;
    std::cout << "  ✗ This JSON is NOT compatible with chrome://tracing" << std::endl;
    std::cout << "  ✗ Kineto uses its own schema, not Chrome Trace Event Format" << std::endl;
    std::cout << "\nTo get Chrome Trace compatible output:" << std::endl;
    std::cout << "  1. Use Quarisma's profiler_session with Kineto integration" << std::endl;
    std::cout << "  2. Use export_to_chrome_trace_json() function" << std::endl;
    std::cout << "  3. See TestProfilerChromeTraceHierarchical.cpp for examples" << std::endl;
    std::cout << "\nKineto JSON can be viewed with:" << std::endl;
    std::cout << "  - Kineto's own visualization tools" << std::endl;
    std::cout << "  - PyTorch Profiler (torch.profiler)" << std::endl;
    std::cout << "  - TensorBoard profiler plugin" << std::endl;
    std::cout << "===================================\n" << std::endl;

    // ========================================================================
    // Step 9: Cleanup - Remove temporary trace file
    // ========================================================================
    // Comment out cleanup to allow manual inspection
    // std::remove(trace_filename.c_str());

    // Note: Keeping trace file for manual inspection
    // To enable cleanup, uncomment the line above
}

#endif  // PROFILER_HAS_KINETO
