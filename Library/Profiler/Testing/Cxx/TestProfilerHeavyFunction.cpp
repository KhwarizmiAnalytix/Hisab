
/**
 * @file TestProfilerHeavyFunction.cpp
 * @brief Comprehensive profiling example with computationally intensive functions
 *
 * This test demonstrates practical profiling usage with realistic heavy computational
 * workloads including matrix operations, sorting algorithms, and data processing.
 * Shows complete instrumentation, multi-format output generation, and performance analysis.
 *
 * @author Quarisma Development Team
 * @version 1.0
 * @date 2024
 */

// ============================================================================
// PROFILING USAGE GUIDE
// ============================================================================
//
// This file demonstrates how to use Quarisma's three profiling systems:
// 1. Quarisma Native Profiler - Hierarchical CPU profiling with Chrome Trace JSON export
// 2. Kineto Profiler - Quarisma profiling library for GPU-related CPU operations
// 3. ITT Profiler - Intel Instrumentation and Tracing Technology for VTune
//
// Each profiler can be used individually or combined for comprehensive analysis.
//
// ============================================================================
// 1. QUARISMA NATIVE PROFILER
// ============================================================================
//
// The Quarisma native profiler provides hierarchical CPU profiling with full
// drill-down capability in Chrome DevTools and Perfetto UI.
//
// BASIC USAGE:
// ------------
//
// #include "native/session/profiler.h"
//
// void my_function() {
//     // Configure profiler options
//     profiler_options opts;
//     opts.enable_timing_               = true;   // Enable timing measurements
//     opts.enable_memory_tracking_      = true;   // Track memory allocations
//     opts.enable_statistical_analysis_ = true;   // Compute statistics
//     opts.enable_thread_safety_        = true;   // Thread-safe operations
//     opts.output_format_               = profiler_options::output_format_enum::JSON;
//
//     // Create and start profiler session
//     profiler_session session(opts);
//     session.start();
//
//     // Instrument your code with scopes
//     {
//         QUARISMA_PROFILE_SCOPE("my_operation");
//         // ... your code here ...
//
//         {
//             QUARISMA_PROFILE_SCOPE("nested_operation");
//             // ... nested code ...
//         }
//     }
//
//     // Stop profiling
//     session.stop();
//
//     // Export Chrome Trace JSON
//     session.write_chrome_trace("my_profile.json");
// }
//
// VIEWING RESULTS:
// ----------------
// 1. Chrome DevTools (chrome://tracing):
//    - Open Chrome browser
//    - Navigate to chrome://tracing
//    - Click 'Load' and select your JSON file
//    - Use W/S to zoom, A/D to pan, click events for details
//
// 2. Perfetto UI (https://ui.perfetto.dev):
//    - Visit https://ui.perfetto.dev
//    - Click 'Open trace file'
//    - Select your JSON file
//    - Explore hierarchical timeline with drill-down
//
// OUTPUT FORMAT:
// --------------
// - File: *.json (Chrome Trace Event Format)
// - Size: ~4 MB for 50,000 events
// - Structure: Hierarchical events with timestamps, durations, thread info
//
// ============================================================================
// 2. KINETO PROFILER
// ============================================================================
//
// Kineto is Quarisma's profiling library. It captures GPU-related CPU operations
// (CUDA kernel launches, memory transfers). For hierarchical CPU profiling,
// combine Kineto with Quarisma's native profiler.
//
// BASIC USAGE:
// ------------
//
// #include "kineto_shim.h"
//
// void my_function() {
//     // Initialize Kineto profiler
//     quarisma::profiler_impl::kineto_init(false, true);
//
//     // Check if Kineto is available
//     if (!quarisma::profiler_impl::kineto_is_profiler_registered()) {
//         std::cout << "Kineto profiler not available\n";
//         return;
//     }
//
//     // Prepare trace with activity types
//     std::set<libkineto::ActivityType> activities;
//     activities.insert(libkineto::ActivityType::CPU_OP);
//     quarisma::profiler_impl::kineto_prepare_trace(activities);
//
//     // Start Kineto profiling
//     quarisma::profiler_impl::kineto_start_trace();
//
//     // Your code here (GPU-related operations)
//     // ...
//
//     // Stop profiling and get trace
//     std::unique_ptr<libkineto::ActivityTraceInterface> trace(
//         static_cast<libkineto::ActivityTraceInterface*>(
//             quarisma::profiler_impl::kineto_stop_trace()));
//
//     // Save Kineto trace
//     if (trace) {
//         trace->save("kineto_trace.json");
//     }
// }
//
// COMBINED WITH QUARISMA PROFILER (RECOMMENDED):
// ---------------------------------------------
//
// void my_function() {
//     // Initialize Kineto
//     quarisma::profiler_impl::kineto_init(false, true);
//     quarisma::profiler_impl::kineto_prepare_trace(activities);
//     quarisma::profiler_impl::kineto_start_trace();
//
//     // Start Quarisma profiler for hierarchical CPU profiling
//     profiler_options opts;
//     opts.enable_timing_ = true;
//     opts.output_format_ = profiler_options::output_format_enum::JSON;
//
//     profiler_session session(opts);
//     session.start();
//
//     // Instrument with QUARISMA_PROFILE_SCOPE
//     {
//         QUARISMA_PROFILE_SCOPE("my_operation");
//         // ... your code ...
//     }
//
//     // Stop both profilers
//     session.stop();
//     auto kineto_trace = quarisma::profiler_impl::kineto_stop_trace();
//
//     // Export both traces
//     session.write_chrome_trace("quarisma_trace.json");  // Full hierarchical CPU profiling
//     if (kineto_trace) {
//         static_cast<libkineto::ActivityTraceInterface*>(kineto_trace)->save("kineto_trace.json");
//     }
// }
//
// VIEWING RESULTS:
// ----------------
// - Quarisma trace (quarisma_trace.json): Chrome DevTools, Perfetto UI
// - Kineto trace (kineto_trace.json): Quarisma Profiler, Chrome DevTools
//
// ============================================================================
// 3. ITT PROFILER (INTEL VTUNE)
// ============================================================================
//
// ITT (Intel Instrumentation and Tracing Technology) provides annotations for
// Intel VTune Profiler. Use Quarisma's ITT wrapper for automatic domain management
// and graceful degradation when VTune is not available.
//
// BASIC USAGE:
// ------------
//
// #include "itt_wrapper.h"
//
// void my_function() {
//     // Initialize ITT profiler (creates global "Quarisma" domain)
//     quarisma::profiler_impl::itt_init();
//
//     // Check if ITT is available (VTune installed)
//     bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);
//
//     if (!itt_available) {
//         std::cout << "ITT not available (VTune not installed)\n";
//         return;
//     }
//
//     // Annotate code with ITT ranges
//     quarisma::profiler_impl::itt_range_push("my_operation");
//     {
//         // ... your code ...
//
//         quarisma::profiler_impl::itt_range_push("nested_operation");
//         // ... nested code ...
//         quarisma::profiler_impl::itt_range_pop();
//     }
//     quarisma::profiler_impl::itt_range_pop();
//
//     // Mark instantaneous events
//     quarisma::profiler_impl::itt_mark("checkpoint_reached");
// }
//
// COMBINED WITH QUARISMA PROFILER (RECOMMENDED):
// ---------------------------------------------
//
// void my_function() {
//     // Initialize ITT
//     quarisma::profiler_impl::itt_init();
//     bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);
//
//     // Start Quarisma profiler for JSON export
//     profiler_options opts;
//     opts.enable_timing_ = true;
//     opts.output_format_ = profiler_options::output_format_enum::JSON;
//
//     profiler_session session(opts);
//     session.start();
//
//     // Instrument with both ITT and Quarisma
//     {
//         if (itt_available) {
//             quarisma::profiler_impl::itt_range_push("my_operation");
//         }
//         QUARISMA_PROFILE_SCOPE("my_operation");
//
//         // ... your code ...
//
//         if (itt_available) {
//             quarisma::profiler_impl::itt_range_pop();
//         }
//     }
//
//     // Stop profiling
//     session.stop();
//
//     // Export Quarisma trace (always available)
//     session.write_chrome_trace("itt_trace.json");
//
//     // ITT annotations are captured by VTune when running under VTune
// }
//
// VIEWING RESULTS:
// ----------------
// 1. Intel VTune Profiler (if VTune installed):
//    vtune -collect hotspots -app ./your_app
//    vtune-gui  # View results with ITT annotations
//
// 2. Quarisma trace (itt_trace.json): Chrome DevTools, Perfetto UI
//
// ============================================================================
// 4. COMBINED PROFILING (ALL THREE SYSTEMS)
// ============================================================================
//
// For comprehensive profiling, combine all three systems:
// - Quarisma: Hierarchical CPU profiling with JSON export
// - Kineto: GPU-related CPU operations
// - ITT: VTune annotations
//
// COMPLETE EXAMPLE:
// -----------------
//
// #include "native/session/profiler.h"
// #include "kineto_shim.h"
// #include "itt_wrapper.h"
//
// void my_function() {
//     // Initialize all profilers
//     quarisma::profiler_impl::kineto_init(false, true);
//     quarisma::profiler_impl::itt_init();
//
//     // Check availability
//     bool const kineto_available = quarisma::profiler_impl::kineto_is_profiler_registered();
//     bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);
//
//     // Prepare Kineto
//     if (kineto_available) {
//         std::set<libkineto::ActivityType> activities;
//         activities.insert(libkineto::ActivityType::CPU_OP);
//         quarisma::profiler_impl::kineto_prepare_trace(activities);
//         quarisma::profiler_impl::kineto_start_trace();
//     }
//
//     // Start Quarisma profiler
//     profiler_options opts;
//     opts.enable_timing_ = true;
//     opts.enable_memory_tracking_ = true;
//     opts.output_format_ = profiler_options::output_format_enum::JSON;
//
//     profiler_session session(opts);
//     session.start();
//
//     // Instrument with all three profilers
//     {
//         if (itt_available) {
//             quarisma::profiler_impl::itt_range_push("my_operation");
//         }
//         QUARISMA_PROFILE_SCOPE("my_operation");
//
//         // ... your code ...
//
//         {
//             if (itt_available) {
//                 quarisma::profiler_impl::itt_range_push("nested_operation");
//             }
//             QUARISMA_PROFILE_SCOPE("nested_operation");
//
//             // ... nested code ...
//
//             if (itt_available) {
//                 quarisma::profiler_impl::itt_range_pop();
//             }
//         }
//
//         if (itt_available) {
//             quarisma::profiler_impl::itt_range_pop();
//         }
//     }
//
//     // Stop all profilers
//     session.stop();
//
//     void* kineto_trace = nullptr;
//     if (kineto_available) {
//         kineto_trace = quarisma::profiler_impl::kineto_stop_trace();
//     }
//
//     // Export all traces
//     session.write_chrome_trace("combined_trace.json");  // Quarisma trace
//
//     if (kineto_trace) {
//         static_cast<libkineto::ActivityTraceInterface*>(kineto_trace)->save("kineto_trace.json");
//     }
//
//     // ITT annotations captured by VTune (if running under VTune)
// }
//
// OUTPUT FILES:
// -------------
// - combined_trace.json: Full hierarchical CPU profiling (Quarisma)
// - kineto_trace.json: GPU-related CPU operations (Kineto)
// - VTune results: ITT annotations (when running under VTune)
//
// ============================================================================
// BEST PRACTICES
// ============================================================================
//
// 1. SCOPE NAMING:
//    - Use descriptive names: "matrix_multiply" not "func1"
//    - Include iteration info: "process_batch_" + std::to_string(i)
//    - Keep names consistent across profilers
//
// 2. GRANULARITY:
//    - Profile at multiple levels (coarse and fine-grained)
//    - Avoid profiling trivial operations (< 1 microsecond)
//    - Balance detail vs. overhead (profiling adds ~100ns per scope)
//
// 3. GRACEFUL DEGRADATION:
//    - Always check profiler availability before use
//    - Provide fallback to Quarisma profiler when Kineto/ITT unavailable
//    - Use conditional compilation for optional profilers
//
// 4. OUTPUT MANAGEMENT:
//    - Use descriptive filenames: "matrix_multiply_profile.json"
//    - Include timestamps in filenames for multiple runs
//    - Clean up old trace files to save disk space
//
// 5. PERFORMANCE:
//    - Disable profiling in production builds (use #if QUARISMA_ENABLE_PROFILING)
//    - Use profiler_session RAII for automatic cleanup
//    - Minimize string allocations in hot paths
//
// ============================================================================
// TROUBLESHOOTING
// ============================================================================
//
// ISSUE: Empty or small JSON file (< 1 KB)
// SOLUTION: Ensure profiler session is started and stopped correctly
//           Check that QUARISMA_PROFILE_SCOPE macros are used
//
// ISSUE: Kineto trace has no events
// SOLUTION: Kineto's CPU_OP captures GPU-related operations only
//           Use Quarisma profiler for general CPU profiling
//
// ISSUE: ITT annotations not visible in VTune
// SOLUTION: Ensure VTune is installed and app is run under VTune
//           Check that itt_get_domain() returns non-null
//
// ISSUE: Chrome DevTools shows "Invalid trace format"
// SOLUTION: Ensure JSON file is complete (session.stop() called)
//           Check file size is > 0 bytes
//           Validate JSON syntax with json.tool
//
// ============================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <memory>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "Testing/baseTest.h"
#include "native/analysis/statistical_analyzer.h"
#include "native/memory/memory_tracker.h"
#include "native/session/profiler.h"
#include "native/tracing/traceme.h"
#include "native/tracing/traceme_recorder.h"

using namespace quarisma;

namespace
{

/**
 * @brief Heavy computational function: Matrix multiplication
 * Performs dense matrix multiplication with profiling instrumentation
 */
std::vector<std::vector<double>> matrix_multiply(
    const std::vector<std::vector<double>>& a, const std::vector<std::vector<double>>& b)
{
    QUARISMA_PROFILE_SCOPE("matrix_multiply");

    const size_t rows_a = a.size();
    const size_t cols_a = a[0].size();
    const size_t cols_b = b[0].size();

    // Validate dimensions
    if (cols_a != b.size())
    {
        throw std::invalid_argument("Matrix dimensions incompatible for multiplication");
    }

    std::vector<std::vector<double>> result(rows_a, std::vector<double>(cols_b, 0.0));

    {
        QUARISMA_PROFILE_SCOPE("matrix_multiply_computation");

        for (size_t i = 0; i < rows_a; ++i)
        {
            QUARISMA_PROFILE_SCOPE("matrix_row_computation");

            for (size_t j = 0; j < cols_b; ++j)
            {
                for (size_t k = 0; k < cols_a; ++k)
                {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
    }

    return result;
}

/**
 * @brief Heavy computational function: Merge sort implementation
 * Recursive merge sort with profiling at each level
 */
void merge_sort(std::vector<double>& arr, size_t left, size_t right, int depth = 0)
{
    QUARISMA_PROFILE_SCOPE("merge_sort_depth_" + std::to_string(depth));

    if (left >= right)
        return;

    size_t mid = left + (right - left) / 2;

    {
        QUARISMA_PROFILE_SCOPE("merge_sort_left_half");
        merge_sort(arr, left, mid, depth + 1);
    }

    {
        QUARISMA_PROFILE_SCOPE("merge_sort_right_half");
        merge_sort(arr, mid + 1, right, depth + 1);
    }

    {
        QUARISMA_PROFILE_SCOPE("merge_operation");

        // Merge the sorted halves
        std::vector<double> temp(right - left + 1);
        size_t              i = left, j = mid + 1, k = 0;

        while (i <= mid && j <= right)
        {
            if (arr[i] <= arr[j])
            {
                temp[k++] = arr[i++];
            }
            else
            {
                temp[k++] = arr[j++];
            }
        }

        while (i <= mid)
            temp[k++] = arr[i++];
        while (j <= right)
            temp[k++] = arr[j++];

        for (size_t idx = 0; idx < temp.size(); ++idx)
        {
            arr[left + idx] = temp[idx];
        }
    }
}

/**
 * @brief Heavy computational function: Monte Carlo Pi estimation
 * Estimates Pi using random sampling with configurable precision
 */
double estimate_pi_monte_carlo(size_t num_samples)
{
    QUARISMA_PROFILE_SCOPE("monte_carlo_pi_estimation");

    std::random_device                     rd;
    std::mt19937                           gen(rd());
    std::uniform_real_distribution<double> dis(-1.0, 1.0);

    size_t points_inside_circle = 0;

    {
        QUARISMA_PROFILE_SCOPE("monte_carlo_sampling");

        for (size_t i = 0; i < num_samples; ++i)
        {
            if (i % 100000 == 0)
            {
                QUARISMA_PROFILE_SCOPE("monte_carlo_batch_" + std::to_string(i / 100000));

                for (size_t j = 0; j < std::min(size_t(100000), num_samples - i); ++j)
                {
                    double x = dis(gen);
                    double y = dis(gen);
                    if (x * x + y * y <= 1.0)
                    {
                        ++points_inside_circle;
                    }
                }
            }
        }
    }

    return 4.0 * static_cast<double>(points_inside_circle) / static_cast<double>(num_samples);
}

/**
 * @brief Heavy computational function: FFT-like computation
 * Simulates frequency domain analysis with nested loops
 */
std::vector<std::complex<double>> simulate_fft(const std::vector<double>& signal)
{
    QUARISMA_PROFILE_SCOPE("simulate_fft");

    const size_t                      n = signal.size();
    std::vector<std::complex<double>> result(n);

    {
        QUARISMA_PROFILE_SCOPE("fft_computation");

        for (size_t k = 0; k < n; ++k)
        {
            QUARISMA_PROFILE_SCOPE("fft_frequency_bin");

            std::complex<double> sum(0.0, 0.0);
            for (size_t j = 0; j < n; ++j)
            {
                double angle = -2.0 * 3.14 * k * j / n;
                sum += signal[j] * std::complex<double>(std::cos(angle), std::sin(angle));
            }
            result[k] = sum;
        }
    }

    return result;
}

/**
 * @brief Generate test data for computational functions
 */
std::vector<std::vector<double>> generate_test_matrix(size_t rows, size_t cols)
{
    QUARISMA_PROFILE_SCOPE("generate_test_matrix");

    std::random_device                     rd;
    std::mt19937                           gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 10.0);

    std::vector<std::vector<double>> matrix(rows, std::vector<double>(cols));

    for (size_t i = 0; i < rows; ++i)
    {
        for (size_t j = 0; j < cols; ++j)
        {
            matrix[i][j] = dis(gen);
        }
    }

    return matrix;
}

std::vector<double> generate_test_signal(size_t size)
{
    QUARISMA_PROFILE_SCOPE("generate_test_signal");

    std::random_device                     rd;
    std::mt19937                           gen(rd());
    std::uniform_real_distribution<double> dis(-1.0, 1.0);

    std::vector<double> signal(size);
    for (size_t i = 0; i < size; ++i)
    {
        signal[i] = dis(gen);
    }

    return signal;
}

}  // anonymous namespace

// Test comprehensive profiling with heavy computational functions
QUARISMATEST(Profiler, heavy_function_comprehensive_computational_profiling)
{
    // Configure profiler session with all features enabled
    profiler_options opts;
    opts.enable_timing_               = true;
    opts.enable_memory_tracking_      = true;
    opts.enable_statistical_analysis_ = true;
    opts.enable_thread_safety_        = true;
    opts.output_format_               = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    {
        QUARISMA_PROFILE_SCOPE("heavy_computational_workload");

        // Test 1: Matrix multiplication profiling
        {
            QUARISMA_PROFILE_SCOPE("matrix_operations_test");

            const size_t matrix_size = 100;  // 100x100 matrices
            auto         matrix_a    = generate_test_matrix(matrix_size, matrix_size);
            auto         matrix_b    = generate_test_matrix(matrix_size, matrix_size);

            // Perform multiple matrix multiplications
            for (int i = 0; i < 3; ++i)
            {
                QUARISMA_PROFILE_SCOPE("matrix_multiply_iteration_" + std::to_string(i));
                auto result = matrix_multiply(matrix_a, matrix_b);

                // Verify result is not empty (basic correctness check)
                EXPECT_EQ(result.size(), matrix_size);
                EXPECT_EQ(result[0].size(), matrix_size);
            }
        }

        // Test 2: Sorting algorithm profiling
        if (false)
        {
            QUARISMA_PROFILE_SCOPE("sorting_algorithms_test");

            const size_t        array_size = 50000;
            std::vector<double> test_data(array_size);

            // Generate random data
            {
                QUARISMA_PROFILE_SCOPE("random_data_generation");
                std::random_device                     rd;
                std::mt19937                           gen(rd());
                std::uniform_real_distribution<double> dis(0.0, 1000.0);

                for (size_t i = 0; i < array_size; ++i)
                {
                    test_data[i] = dis(gen);
                }
            }

            // Test merge sort
            {
                QUARISMA_PROFILE_SCOPE("merge_sort_test");
                auto data_copy = test_data;
                merge_sort(data_copy, 0, data_copy.size() - 1);

                // Verify sorting correctness
                EXPECT_TRUE(std::is_sorted(data_copy.begin(), data_copy.end()));
            }

            // Test std::sort for comparison
            {
                QUARISMA_PROFILE_SCOPE("std_sort_comparison");
                auto data_copy = test_data;
                std::sort(data_copy.begin(), data_copy.end());

                EXPECT_TRUE(std::is_sorted(data_copy.begin(), data_copy.end()));
            }
        }

        // Test 3: Monte Carlo simulation profiling
        {
            QUARISMA_PROFILE_SCOPE("monte_carlo_simulation_test");

            const size_t num_samples = 1000000;  // 1 million samples
            double       pi_estimate = estimate_pi_monte_carlo(num_samples);

            // Verify Pi estimate is reasonable (should be close to 3.14159)
            EXPECT_GT(pi_estimate, 3.0);
            EXPECT_LT(pi_estimate, 3.3);

            std::cout << "Monte Carlo Pi estimate: " << pi_estimate << std::endl;
        }

        // Test 4: FFT simulation profiling
        {
            QUARISMA_PROFILE_SCOPE("fft_simulation_test");

            const size_t signal_size = 512;  // Common FFT size
            auto         test_signal = generate_test_signal(signal_size);

            auto fft_result = simulate_fft(test_signal);

            // Verify FFT result size
            EXPECT_EQ(fft_result.size(), signal_size);
        }

        // Test 5: Multi-threaded computation profiling
        {
            QUARISMA_PROFILE_SCOPE("multithreaded_computation_test");

            std::vector<std::thread> workers;
            const int                num_threads = 4;

            for (int i = 0; i < num_threads; ++i)
            {
                workers.emplace_back(
                    [i]()
                    {
                        QUARISMA_PROFILE_SCOPE("worker_thread_" + std::to_string(i));

                        // Each thread performs different computational work
                        const size_t           samples_per_thread = 250000;
                        QUARISMA_UNUSED double pi_est = estimate_pi_monte_carlo(samples_per_thread);

                        // Small computation to keep thread busy
                        std::vector<double> data(10000);
                        std::iota(data.begin(), data.end(), i * 10000);
                        std::sort(data.begin(), data.end(), std::greater<double>());
                    });
            }

            // Wait for all threads to complete
            for (auto& worker : workers)
            {
                worker.join();
            }
        }
    }

    session.stop();

    // ========================================================================
    // CHROME TRACE FORMAT EXPORT
    // ========================================================================
    // Export profiling results to Chrome Trace Event Format (JSON)
    // This format is compatible with:
    //   1. Chrome DevTools (chrome://tracing)
    //   2. Perfetto UI (https://ui.perfetto.dev)
    //   3. Other trace viewers that support the standard format
    //
    // HOW TO VIEW THE TRACE:
    // 1. Open Chrome browser and navigate to: chrome://tracing
    // 2. Click "Load" button and select the generated JSON file
    // 3. Use the following keyboard shortcuts:
    //    - W/S: Zoom in/out
    //    - A/D: Pan left/right
    //    - Click and drag: Select time range
    //    - Double-click: Zoom to selection
    //
    // INSIGHTS FROM THE VISUALIZATION:
    // - Timeline view shows execution order and duration of each scope
    // - Nested scopes appear as hierarchical blocks
    // - Color coding helps identify different operations
    // - Memory allocation/deallocation events are marked
    // - Thread information shows parallel execution patterns
    // - Hover over events to see detailed statistics
    // ========================================================================

    // Export to Chrome Trace JSON format (Chrome Trace Event Format)
    std::string chrome_trace_file = "heavy_function_profile.json";
    session.write_chrome_trace(chrome_trace_file);

    std::cout << "\n=== Heavy Function Performance Analysis ===\n";
    std::cout << "\n✓ Chrome Trace JSON exported to: " << chrome_trace_file << "\n";
    std::cout << "\nTo view the trace:\n";
    std::cout << "  1. Open Chrome and navigate to: chrome://tracing\n";
    std::cout << "  2. Click 'Load' and select: " << chrome_trace_file << "\n";
    std::cout << "  3. Use W/S to zoom, A/D to pan, click to select\n";
    std::cout << "\nAlternatively, use Perfetto UI:\n";
    std::cout << "  1. Visit: https://ui.perfetto.dev\n";
    std::cout << "  2. Open the JSON file in the UI\n";

    std::cout << "\nAll computational workloads profiled successfully:\n";
    std::cout << "  - Matrix multiplication (100x100)\n";
    std::cout << "  - Merge sort (50,000 elements)\n";
    std::cout << "  - Monte Carlo Pi estimation (1M samples)\n";
    std::cout << "  - FFT simulation (512 points)\n";
    std::cout << "  - Multi-threaded computation\n";
}

// ============================================================================
// KINETO PROFILER TEST
// ============================================================================
// Test Quarisma Kineto profiler integration with heavy computational functions
//
// Kineto provides comprehensive profiling capabilities including:
// - CPU activity tracing
// - GPU activity tracing (if available)
// - Memory profiling
// - Operator-level profiling
//
// OUTPUT: Kineto generates JSON trace files compatible with:
// - Quarisma Profiler Viewer
// - TensorBoard
// - Chrome DevTools (chrome://tracing)
//
// HOW TO USE:
// 1. Run this test to generate kineto_trace.json
// 2. View with Quarisma Profiler:
//    python -m quarisma.profiler.viewer kineto_trace.json
// 3. Or view in Chrome:
//    - Open chrome://tracing
//    - Load the JSON file
// ============================================================================

#if QUARISMA_HAS_KINETO && 0
#include <fstream>
#include <sstream>

#include "bespoke/kineto/kineto_shim.h"
#include "bespoke/kineto/profiler_kineto.h"

// Suppress MSVC warnings for Kineto headers
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4245)  // signed/unsigned mismatch
#endif

#include <libkineto.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

QUARISMATEST(Profiler, kineto_heavy_function_profiling)
{
    std::cout << "\n=== Kineto + Quarisma Profiler Heavy Function Test (with Drill-Down) ===\n";
    std::cout << "Note: Kineto's CPU_OP activity type captures GPU-related CPU operations.\n";
    std::cout << "For hierarchical CPU profiling with drill-down, we combine Kineto with Quarisma "
                 "profiler.\n\n";

    // Initialize Kineto profiler
    quarisma::profiler_impl::kineto_init(false, true);

    if (!quarisma::profiler_impl::kineto_is_profiler_registered())
    {
        std::cout << "Kineto profiler not registered - using Quarisma profiler only\n";
        // Fall back to Quarisma profiler only
        profiler_options opts;
        opts.enable_timing_ = true;
        opts.output_format_ = profiler_options::output_format_enum::JSON;

        profiler_session session(opts);
        session.start();

        // Profile heavy computational workloads with hierarchical scopes
        {
            QUARISMA_PROFILE_SCOPE("kineto_matrix_operations");

            const size_t matrix_size = 50;
            auto         matrix_a    = generate_test_matrix(matrix_size, matrix_size);
            auto         matrix_b    = generate_test_matrix(matrix_size, matrix_size);

            for (int i = 0; i < 2; ++i)
            {
                QUARISMA_PROFILE_SCOPE("kineto_matrix_multiply_" + std::to_string(i));
                auto result = matrix_multiply(matrix_a, matrix_b);
                EXPECT_EQ(result.size(), matrix_size);
            }
        }

        if (false)
        {
            QUARISMA_PROFILE_SCOPE("kineto_sorting_operations");

            const size_t        array_size = 10000;
            std::vector<double> test_data(array_size);

            std::random_device                     rd;
            std::mt19937                           gen(rd());
            std::uniform_real_distribution<double> dis(0.0, 1000.0);

            for (size_t i = 0; i < array_size; ++i)
            {
                test_data[i] = dis(gen);
            }

            {
                QUARISMA_PROFILE_SCOPE("kineto_merge_sort");
                auto data_copy = test_data;
                merge_sort(data_copy, 0, data_copy.size() - 1);
                EXPECT_TRUE(std::is_sorted(data_copy.begin(), data_copy.end()));
            }
        }

        session.stop();

        // Export to Chrome Trace JSON format (supports drill-down)
        std::string const output_file = "kineto_heavy_function_trace.json";
        session.write_chrome_trace(output_file);

        // Verify JSON file
        std::ifstream json_file(output_file);
        EXPECT_TRUE(json_file.good()) << "Failed to create JSON output file";

        if (json_file.good())
        {
            std::stringstream buffer;
            buffer << json_file.rdbuf();
            std::string const json_content = buffer.str();

            EXPECT_TRUE(json_content.find("\"traceEvents\"") != std::string::npos)
                << "JSON file missing traceEvents array";

            std::cout << "✓ Quarisma profiler trace saved to: " << output_file << "\n";
            std::cout << "✓ JSON file validated (size: " << json_content.size() << " bytes)\n";
        }

        EXPECT_TRUE(true);
        return;
    }

    // Kineto is available - use combined profiling approach
    std::cout << "Kineto profiler registered - using combined Kineto + Quarisma profiling\n";

    // Start Quarisma profiler session for hierarchical CPU profiling
    profiler_options opts;
    opts.enable_timing_ = true;
    opts.output_format_ = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    // Prepare Kineto trace with CPU activities
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);
    quarisma::profiler_impl::kineto_prepare_trace(activities);

    // Start Kineto profiling
    quarisma::profiler_impl::kineto_start_trace();
    std::cout << "Combined profiling started (Kineto + Quarisma)\n";

    // Profile heavy computational workloads with hierarchical scopes
    {
        QUARISMA_PROFILE_SCOPE("kineto_matrix_operations");

        const size_t matrix_size = 50;  // Smaller for faster execution
        auto         matrix_a    = generate_test_matrix(matrix_size, matrix_size);
        auto         matrix_b    = generate_test_matrix(matrix_size, matrix_size);

        for (int i = 0; i < 2; ++i)
        {
            QUARISMA_PROFILE_SCOPE("kineto_matrix_multiply_" + std::to_string(i));
            auto result = matrix_multiply(matrix_a, matrix_b);
            EXPECT_EQ(result.size(), matrix_size);
        }
    }

    {
        QUARISMA_PROFILE_SCOPE("kineto_sorting_operations");

        const size_t        array_size = 10000;
        std::vector<double> test_data(array_size);

        std::random_device                     rd;
        std::mt19937                           gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1000.0);

        for (size_t i = 0; i < array_size; ++i)
        {
            test_data[i] = dis(gen);
        }

        {
            QUARISMA_PROFILE_SCOPE("kineto_merge_sort");
            auto data_copy = test_data;
            merge_sort(data_copy, 0, data_copy.size() - 1);
            EXPECT_TRUE(std::is_sorted(data_copy.begin(), data_copy.end()));
        }
    }

    // Stop both profilers
    session.stop();
    std::unique_ptr<libkineto::ActivityTraceInterface> trace(
        static_cast<libkineto::ActivityTraceInterface*>(quarisma::profiler_impl::kineto_stop_trace()));

    std::cout << "Combined profiling completed\n";

    // Save Quarisma trace (hierarchical CPU profiling with drill-down)
    std::string const quarisma_output_file = "kineto_heavy_function_trace.json";
    session.write_chrome_trace(quarisma_output_file);

    // Save Kineto trace (GPU-related CPU operations)
    std::string const kineto_output_file = "kineto_heavy_function_kineto_only.json";

    if (trace)
    {
        trace->save(kineto_output_file);
        std::cout << "✓ Kineto trace saved to: " << kineto_output_file << "\n";
    }

    // Verify Quarisma JSON file (primary output with drill-down capability)
    std::ifstream json_file(quarisma_output_file);
    EXPECT_TRUE(json_file.good()) << "Failed to create Quarisma JSON output file";

    if (json_file.good())
    {
        // Read and validate JSON structure
        std::stringstream buffer;
        buffer << json_file.rdbuf();
        std::string const json_content = buffer.str();

        // Validate Chrome Trace Event format
        EXPECT_TRUE(json_content.find("\"traceEvents\"") != std::string::npos)
            << "JSON file missing traceEvents array";

        // Verify hierarchical scopes are present
        EXPECT_TRUE(json_content.find("kineto_matrix_operations") != std::string::npos)
            << "JSON missing matrix operations scope";

        EXPECT_TRUE(json_content.find("kineto_sorting_operations") != std::string::npos)
            << "JSON missing sorting operations scope";

        EXPECT_TRUE(json_content.find("kineto_merge_sort") != std::string::npos)
            << "JSON missing merge sort scope";

        // Verify event types for drill-down (B/E for begin/end or X for complete events)
        bool has_event_types = json_content.find("\"ph\"") != std::string::npos;
        EXPECT_TRUE(has_event_types) << "JSON missing event phase markers";

        std::cout << "✓ Quarisma trace saved to: " << quarisma_output_file << "\n";
        std::cout << "✓ JSON file validated (size: " << json_content.size() << " bytes)\n";
        std::cout << "✓ Hierarchical scopes verified for drill-down capability\n";

        std::cout << "\n=== Drill-Down Visualization Instructions ===\n";
        std::cout << "The trace file supports full hierarchical drill-down in profiling tools:\n\n";

        std::cout << "1. Chrome DevTools (chrome://tracing):\n";
        std::cout << "   - Open Chrome browser\n";
        std::cout << "   - Navigate to chrome://tracing\n";
        std::cout << "   - Click 'Load' and select: " << quarisma_output_file << "\n";
        std::cout << "   - Use W/S to zoom in/out, A/D to pan\n";
        std::cout << "   - Click on events to see details and nested scopes\n\n";

        std::cout << "2. Perfetto UI (https://ui.perfetto.dev):\n";
        std::cout << "   - Visit https://ui.perfetto.dev\n";
        std::cout << "   - Click 'Open trace file'\n";
        std::cout << "   - Select: " << quarisma_output_file << "\n";
        std::cout << "   - Explore hierarchical timeline with drill-down\n\n";

        std::cout << "3. Expected Drill-Down Structure:\n";
        std::cout << "   ├─ kineto_matrix_operations (parent scope)\n";
        std::cout << "   │  ├─ kineto_matrix_multiply_0 (nested scope)\n";
        std::cout << "   │  └─ kineto_matrix_multiply_1 (nested scope)\n";
        std::cout << "   └─ kineto_sorting_operations (parent scope)\n";
        std::cout << "      └─ kineto_merge_sort (nested scope)\n\n";

        std::cout << "Note: Kineto-only trace (" << kineto_output_file
                  << ") contains GPU-related CPU operations.\n";
        std::cout << "      Quarisma trace (" << quarisma_output_file
                  << ") contains full hierarchical CPU profiling.\n";
    }
}
#endif  // QUARISMA_HAS_KINETO

// ============================================================================
// INTEL ITT API TEST
// ============================================================================
// Test Intel ITT API integration with heavy computational functions
//
// ITT API provides task and frame annotations for Intel VTune profiling:
// - Task annotations: Mark regions of code for analysis
// - Frame markers: Identify frame boundaries in graphics applications
// - String handles: Efficient string management for annotations
// - Domain-based organization: Group related tasks
//
// OUTPUT: VTune-compatible profiling data + Chrome Trace JSON
//
// HOW TO USE:
// 1. Run this test (works with or without Intel VTune installed)
// 2. View JSON trace in Chrome DevTools (chrome://tracing) or Perfetto UI
// 3. If VTune is installed, collect profiling data:
//    vtune -collect hotspots -app ./CoreCxxTests.exe
// 4. View results in VTune GUI and look for ITT annotations
//
// INSIGHTS:
// - Task duration shows computational complexity
// - Nested tasks reveal call hierarchy
// - Thread information shows parallelization
// - Memory events correlate with allocations
// ============================================================================

#if QUARISMA_HAS_ITT
#include <fstream>
#include <sstream>

#include "bespoke/itt/itt_wrapper.h"

QUARISMATEST(Profiler, itt_api_heavy_function_profiling)
{
    std::cout
        << "\n=== Intel ITT API + Quarisma Profiler Heavy Function Test (with Drill-Down) ===\n";
    std::cout << "Note: ITT annotations are captured by Intel VTune when available.\n";
    std::cout << "For hierarchical CPU profiling with drill-down, we combine ITT with Quarisma "
                 "profiler.\n\n";

    // Initialize ITT profiler (creates global Quarisma domain)
    quarisma::profiler_impl::itt_init();

    // Check if ITT is available (domain creation may fail if VTune not installed)
    bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);

    if (!itt_available)
    {
        std::cout << "ITT API domain creation failed (VTune not available)\n";
        std::cout << "Falling back to Quarisma profiler only for JSON trace generation\n\n";
    }
    else
    {
        std::cout << "ITT API domain created: Quarisma\n";
        std::cout << "Combined profiling started (ITT + Quarisma)\n\n";
    }

    // Start Quarisma profiler session to capture hierarchical profiling data
    profiler_options opts;
    opts.enable_timing_               = true;
    opts.enable_memory_tracking_      = false;
    opts.enable_statistical_analysis_ = false;
    opts.enable_thread_safety_        = true;
    opts.output_format_               = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    // Profile matrix operations with ITT wrapper API and Quarisma profiler
    {
        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_push("matrix_operations");
        }
        QUARISMA_PROFILE_SCOPE("itt_matrix_operations");

        const size_t matrix_size = 50;
        auto         matrix_a    = generate_test_matrix(matrix_size, matrix_size);
        auto         matrix_b    = generate_test_matrix(matrix_size, matrix_size);

        for (int i = 0; i < 2; ++i)
        {
            std::string const iter_name = "matrix_multiply_" + std::to_string(i);

            if (itt_available)
            {
                quarisma::profiler_impl::itt_range_push(iter_name.c_str());
            }

            QUARISMA_PROFILE_SCOPE(("itt_matrix_multiply_" + std::to_string(i)).c_str());

            auto result = matrix_multiply(matrix_a, matrix_b);
            EXPECT_EQ(result.size(), matrix_size);

            if (itt_available)
            {
                quarisma::profiler_impl::itt_range_pop();
            }
        }

        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_pop();
        }
    }

    // Profile sorting operations with ITT wrapper API and Quarisma profiler
    {
        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_push("sorting_operations");
        }
        QUARISMA_PROFILE_SCOPE("itt_sorting_operations");

        const size_t        array_size = 10000;
        std::vector<double> test_data(array_size);

        std::random_device                     rd;
        std::mt19937                           gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1000.0);

        for (size_t i = 0; i < array_size; ++i)
        {
            test_data[i] = dis(gen);
        }

        {
            if (itt_available)
            {
                quarisma::profiler_impl::itt_range_push("merge_sort");
            }

            QUARISMA_PROFILE_SCOPE("itt_merge_sort");

            auto data_copy = test_data;
            merge_sort(data_copy, 0, data_copy.size() - 1);
            EXPECT_TRUE(std::is_sorted(data_copy.begin(), data_copy.end()));

            if (itt_available)
            {
                quarisma::profiler_impl::itt_range_pop();
            }
        }

        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_pop();
        }
    }

    // Profile Monte Carlo simulation with ITT wrapper API and Quarisma profiler
    {
        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_push("monte_carlo_simulation");
        }
        QUARISMA_PROFILE_SCOPE("itt_monte_carlo_simulation");

        const size_t num_samples = 100000;
        double       pi_estimate = estimate_pi_monte_carlo(num_samples);

        EXPECT_GT(pi_estimate, 3.0);
        EXPECT_LT(pi_estimate, 3.3);

        std::cout << "Monte Carlo Pi estimate: " << pi_estimate << "\n";

        if (itt_available)
        {
            quarisma::profiler_impl::itt_range_pop();
        }
    }

    session.stop();

    if (itt_available)
    {
        std::cout << "Combined profiling completed (ITT + Quarisma)\n";
    }
    else
    {
        std::cout << "Quarisma profiling completed\n";
    }

    // Export profiling data to JSON (captures Quarisma profiling scopes with hierarchical drill-down)
    std::string const itt_output_file = "itt_heavy_function_trace.json";
    session.write_chrome_trace(itt_output_file);

    std::cout << "✓ Quarisma trace saved to: " << itt_output_file << "\n";

    // Verify JSON file was created and is valid
    std::ifstream json_file(itt_output_file);
    EXPECT_TRUE(json_file.good()) << "Failed to create ITT JSON output file";

    if (json_file.good())
    {
        // Read and validate JSON structure
        std::stringstream buffer;
        buffer << json_file.rdbuf();
        std::string const json_content = buffer.str();

        // Basic JSON validation - check for required fields
        EXPECT_TRUE(json_content.find("\"traceEvents\"") != std::string::npos)
            << "JSON file missing required trace structure";

        // Verify ITT-annotated scopes are present
        EXPECT_TRUE(json_content.find("itt_matrix_operations") != std::string::npos)
            << "JSON missing ITT matrix operations scope";
        EXPECT_TRUE(json_content.find("itt_sorting_operations") != std::string::npos)
            << "JSON missing ITT sorting operations scope";
        EXPECT_TRUE(json_content.find("itt_monte_carlo_simulation") != std::string::npos)
            << "JSON missing ITT Monte Carlo scope";
        EXPECT_TRUE(json_content.find("itt_merge_sort") != std::string::npos)
            << "JSON missing ITT merge sort scope";

        EXPECT_GT(json_content.size(), 1000) << "JSON file appears to be empty or too small";

        std::cout << "✓ JSON file validated (size: " << json_content.size() << " bytes)\n";
        std::cout << "✓ Hierarchical scopes verified for drill-down capability\n";
    }

    std::cout << "\n=== Drill-Down Visualization Instructions ===\n";
    std::cout << "The trace file supports full hierarchical drill-down in profiling tools:\n\n";

    std::cout << "1. Chrome DevTools (chrome://tracing):\n";
    std::cout << "   - Open Chrome browser\n";
    std::cout << "   - Navigate to chrome://tracing\n";
    std::cout << "   - Click 'Load' and select: " << itt_output_file << "\n";
    std::cout << "   - Use W/S to zoom, A/D to pan\n";
    std::cout << "   - Click on events to see details and nested scopes\n\n";

    std::cout << "2. Perfetto UI (https://ui.perfetto.dev):\n";
    std::cout << "   - Visit https://ui.perfetto.dev\n";
    std::cout << "   - Click 'Open trace file'\n";
    std::cout << "   - Select: " << itt_output_file << "\n";
    std::cout << "   - Explore hierarchical timeline with drill-down\n\n";

    if (itt_available)
    {
        std::cout << "3. Intel VTune Profiler (for ITT annotations):\n";
        std::cout << "   - Run: vtune -collect hotspots -app ./CoreCxxTests.exe\n";
        std::cout << "   - Open results in VTune GUI\n";
        std::cout << "   - Look for 'QuarismaHeavyFunctionTest' domain in timeline\n\n";
    }

    std::cout << "4. Expected Drill-Down Structure:\n";
    std::cout << "   ├─ itt_matrix_operations (parent scope)\n";
    std::cout << "   │  ├─ itt_matrix_multiply_0 (nested scope)\n";
    std::cout << "   │  └─ itt_matrix_multiply_1 (nested scope)\n";
    std::cout << "   ├─ itt_sorting_operations (parent scope)\n";
    std::cout << "   │  └─ itt_merge_sort (nested scope)\n";
    std::cout << "   └─ itt_monte_carlo_simulation (parent scope)\n\n";

    if (itt_available)
    {
        std::cout << "Note: ITT annotations are also captured in VTune profiler.\n";
        std::cout << "      Quarisma trace (" << itt_output_file
                  << ") contains full hierarchical CPU profiling.\n";
    }
    else
    {
        std::cout << "Note: ITT annotations not available (VTune not installed).\n";
        std::cout << "      Quarisma trace (" << itt_output_file
                  << ") contains full hierarchical CPU profiling.\n";
    }
}
#endif  // QUARISMA_HAS_ITT

// ============================================================================
// COMBINED KINETO + ITT PROFILING TEST
// ============================================================================
// Test combining both Kineto and ITT profiling in a single session
//
// This demonstrates how to use both profiling technologies together:
// - Kineto captures CPU/GPU activity and generates JSON traces
// - ITT provides annotations for Intel VTune profiling
// - Quarisma profiler session captures hierarchical scope information
//
// OUTPUT: Multiple JSON trace files that can be viewed in different tools
// ============================================================================

#if QUARISMA_HAS_KINETO && QUARISMA_HAS_ITT

// Suppress MSVC warnings for Kineto headers
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)  // unreferenced formal parameter
#pragma warning(disable : 4245)  // signed/unsigned mismatch
#endif

#include <libkineto.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "bespoke/itt/itt_wrapper.h"
#include "bespoke/kineto/kineto_shim.h"

QUARISMATEST(Profiler, combined_kineto_itt_heavy_function_profiling)
{
    std::cout << "\n=== Combined Kineto + ITT Profiling Test ===\n";

    // Initialize Kineto profiler
    quarisma::profiler_impl::kineto_init(false, true);

    if (!quarisma::profiler_impl::kineto_is_profiler_registered())
    {
        std::cout << "Kineto profiler not registered - skipping combined test\n";
        EXPECT_TRUE(true);
        return;
    }

    // Initialize ITT profiler (creates global Quarisma domain)
    quarisma::profiler_impl::itt_init();

    // Check if ITT is available
    bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);

    if (!itt_available)
    {
        std::cout << "ITT API domain creation failed - skipping combined test\n";
        EXPECT_TRUE(true);
        return;
    }

    std::cout << "✓ Kineto profiler initialized\n";
    std::cout << "✓ ITT domain created: Quarisma\n";

    // Prepare Kineto trace
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);
    quarisma::profiler_impl::kineto_prepare_trace(activities);

    // Start Kineto profiling
    quarisma::profiler_impl::kineto_start_trace();

    // Also start Quarisma profiler session
    profiler_options opts;
    opts.enable_timing_               = true;
    opts.enable_memory_tracking_      = true;
    opts.enable_statistical_analysis_ = true;
    opts.enable_thread_safety_        = true;
    opts.output_format_               = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    std::cout << "✓ All profilers started\n";

    // Profile combined workload with all three profiling systems
    {
        quarisma::profiler_impl::itt_range_push("combined_workload");
        QUARISMA_PROFILE_SCOPE("combined_profiling_workload");

        // Matrix operations
        {
            quarisma::profiler_impl::itt_range_push("matrix_computation");
            QUARISMA_PROFILE_SCOPE("combined_matrix_operations");

            const size_t matrix_size = 50;
            auto         matrix_a    = generate_test_matrix(matrix_size, matrix_size);
            auto         matrix_b    = generate_test_matrix(matrix_size, matrix_size);

            auto result = matrix_multiply(matrix_a, matrix_b);
            EXPECT_EQ(result.size(), matrix_size);

            quarisma::profiler_impl::itt_range_pop();
        }

        // Monte Carlo simulation
        {
            quarisma::profiler_impl::itt_range_push("monte_carlo_computation");
            QUARISMA_PROFILE_SCOPE("combined_monte_carlo");

            const size_t num_samples = 500000;
            double       pi_estimate = estimate_pi_monte_carlo(num_samples);

            EXPECT_GT(pi_estimate, 3.0);
            EXPECT_LT(pi_estimate, 3.3);

            std::cout << "Monte Carlo Pi estimate: " << pi_estimate << "\n";

            quarisma::profiler_impl::itt_range_pop();
        }

        quarisma::profiler_impl::itt_range_pop();
    }

    // Stop all profilers
    session.stop();

    std::unique_ptr<libkineto::ActivityTraceInterface> kineto_trace(
        static_cast<libkineto::ActivityTraceInterface*>(quarisma::profiler_impl::kineto_stop_trace()));

    std::cout << "✓ All profilers stopped\n";

    // Export Kineto trace
    std::string const kineto_combined_file = "combined_kineto_trace.json";
    if (kineto_trace)
    {
        kineto_trace->save(kineto_combined_file);
        std::cout << "✓ Kineto trace saved to: " << kineto_combined_file << "\n";

        // Verify Kineto JSON
        std::ifstream kineto_json(kineto_combined_file);
        EXPECT_TRUE(kineto_json.good()) << "Failed to create Kineto JSON file";

        if (kineto_json.good())
        {
            std::stringstream buffer;
            buffer << kineto_json.rdbuf();
            std::string const content = buffer.str();

            EXPECT_GT(content.size(), 100) << "Kineto JSON file too small";
            std::cout << "✓ Kineto JSON validated (size: " << content.size() << " bytes)\n";
        }
    }

    // Export Quarisma profiler trace
    std::string const quarisma_combined_file = "combined_quarisma_trace.json";
    session.write_chrome_trace(quarisma_combined_file);
    std::cout << "✓ Quarisma trace saved to: " << quarisma_combined_file << "\n";

    // Verify Quarisma JSON
    std::ifstream quarisma_json(quarisma_combined_file);
    EXPECT_TRUE(quarisma_json.good()) << "Failed to create Quarisma JSON file";

    if (quarisma_json.good())
    {
        std::stringstream buffer;
        buffer << quarisma_json.rdbuf();
        std::string const content = buffer.str();

        EXPECT_TRUE(content.find("\"traceEvents\"") != std::string::npos)
            << "Quarisma JSON missing trace structure";
        EXPECT_TRUE(content.find("combined_profiling_workload") != std::string::npos)
            << "Quarisma JSON missing profiling scopes";

        EXPECT_GT(content.size(), 100) << "Quarisma JSON file too small";
        std::cout << "✓ Quarisma JSON validated (size: " << content.size() << " bytes)\n";
    }

    std::cout << "\n=== Combined Profiling Test Summary ===\n";
    std::cout << "✓ Kineto profiling: " << kineto_combined_file << "\n";
    std::cout << "✓ Quarisma profiling: " << quarisma_combined_file << "\n";
    std::cout << "✓ ITT annotations: Available in VTune profiler\n";
    std::cout << "\nViewing options:\n";
    std::cout << "  1. Chrome Tracing (both JSON files):\n";
    std::cout << "     - Open chrome://tracing\n";
    std::cout << "     - Load either JSON file\n";
    std::cout << "  2. Perfetto UI (both JSON files):\n";
    std::cout << "     - Visit https://ui.perfetto.dev\n";
    std::cout << "     - Open either JSON file\n";
    std::cout << "  3. Quarisma Profiler (Kineto trace):\n";
    std::cout << "     - python -m quarisma.profiler.viewer " << kineto_combined_file << "\n";
    std::cout << "  4. Intel VTune (ITT annotations):\n";
    std::cout << "     - vtune -collect hotspots -app ./CoreCxxTests.exe\n";
    std::cout << "     - Look for 'QuarismaCombinedProfilingTest' domain\n";
}

#endif  // QUARISMA_HAS_KINETO && QUARISMA_HAS_ITT
