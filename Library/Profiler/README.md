# Enhanced Profiler for Quarisma Core Module

## Overview

The Enhanced Profiler is a comprehensive performance analysis system for the Quarisma Core module that provides:

- **High-precision timing measurements** with nanosecond accuracy
- **Memory usage tracking** with allocation/deallocation monitoring
- **Hierarchical profiling** for nested function call analysis
- **Thread-safe profiling** for multi-threaded applications
- **Statistical analysis** with min/max/mean/std deviation calculations
- **Multiple output formats** (console, JSON, CSV, XML)
- **Minimal performance overhead** designed for production use

## Features

### 1. Timing Profiling
- Nanosecond precision timing using `std::chrono::high_resolution_clock`
- Automatic scope-based timing with RAII semantics
- Support for nested timing scopes with hierarchical reporting
- Statistical analysis of repeated measurements

### 2. Memory Tracking
- Real-time memory allocation and deallocation tracking
- Peak memory usage monitoring
- Memory delta calculations between profiling points
- Platform-specific system memory information (Windows/Linux)
- Memory leak detection capabilities

### 3. Statistical Analysis
- Comprehensive statistics: min, max, mean, median, standard deviation
- Percentile calculations (25th, 50th, 75th, 90th, 95th, 99th)
- Outlier detection using z-score analysis
- Time series analysis with trend detection
- Performance regression detection

### 4. Thread Safety
- Lock-free data structures for minimal contention
- Per-thread profiling data storage
- Atomic operations for statistics updates
- Thread-safe report generation

### 5. Output Formats
- **Console**: Human-readable text output
- **JSON**: Structured data for programmatic analysis
- **CSV**: Spreadsheet-compatible format
- **XML**: Structured markup for integration

#### Chrome Trace Export (JSON)

Chrome Trace Event JSON produced by the profiler uses nanoseconds for timestamps and durations and sets `displayTimeUnit` to `"ns"`. This applies to both hierarchical traces and XPlane-derived traces and ensures consistent interpretation in Chrome/Perfetto.

## Quick Start

### Basic Usage

```cpp
#include "profiler/native/core/profiler.h"

using namespace quarisma::profiler;

int main() {
    // Create profiler session with builder pattern
    auto session = EnhancedProfilerSession::Builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .with_output_format(EnhancedProfilerOptions::OutputFormat::JSON)
        .build();

    // Start profiling
    session->start();

    // Profile a function
    {
        QUARISMA_PROFILE_FUNCTION();

        // Your code here
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Profile nested operations
        {
            QUARISMA_PROFILE_SCOPE("nested_operation");
            std::vector<int> data(1000, 42);
            // More work...
        }
    }

    // Stop profiling and generate report
    session->stop();
    session->print_report();
    session->export_report("profile_results.json");

    return 0;
}
```

### Advanced Configuration

```cpp
// Create session with custom configuration
auto session = EnhancedProfilerSession::Builder()
    .with_timing(true)
    .with_memory_tracking(true)
    .with_hierarchical_profiling(true)
    .with_statistical_analysis(true)
    .with_thread_safety(true)
    .with_output_format(EnhancedProfilerOptions::OutputFormat::JSON)
    .with_output_file("detailed_profile.json")
    .with_max_samples(10000)
    .with_percentiles(true)
    .with_peak_memory_tracking(true)
    .with_memory_deltas(true)
    .with_thread_pool_size(8)
    .build();
```

## API Reference

### EnhancedProfilerSession

The main profiler session class that manages all profiling activities.

#### Builder Pattern Methods
- `with_timing(bool)` - Enable/disable timing measurements
- `with_memory_tracking(bool)` - Enable/disable memory tracking
- `with_hierarchical_profiling(bool)` - Enable/disable hierarchical profiling
- `with_statistical_analysis(bool)` - Enable/disable statistical analysis
- `with_thread_safety(bool)` - Enable/disable thread-safe operations
- `with_output_format(OutputFormat)` - Set output format
- `with_output_file(string)` - Set output file path
- `with_max_samples(size_t)` - Set maximum samples per series
- `with_percentiles(bool)` - Enable/disable percentile calculations

#### Core Methods
- `start()` - Start profiling session
- `stop()` - Stop profiling session
- `is_active()` - Check if session is active
- `generate_report()` - Generate profiling report
- `export_report(filename)` - Export report to file
- `print_report()` - Print report to console

### Profiling Macros

Convenient macros for automatic profiling:

```cpp
// Profile current scope
QUARISMA_PROFILE_SCOPE("scope_name");

// Profile current function
QUARISMA_PROFILE_FUNCTION();

// Profile a block of code
QUARISMA_PROFILE_BLOCK("block_name") {
    // Your code here
}
```

### Memory Tracking

```cpp
MemoryTracker tracker;
tracker.start_tracking();

// Track custom allocations
void* ptr = malloc(1024);
tracker.track_allocation(ptr, 1024, "custom_allocation");

// ... use memory ...

tracker.track_deallocation(ptr);
free(ptr);

// Get statistics
auto stats = tracker.get_current_stats();
std::cout << "Current usage: " << stats.current_usage << " bytes" << std::endl;
std::cout << "Peak usage: " << stats.peak_usage << " bytes" << std::endl;

tracker.stop_tracking();
```

### Statistical Analysis

```cpp
StatisticalAnalyzer analyzer;
analyzer.start_analysis();

// Add timing samples
for (int i = 0; i < 100; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    // ... do work ...
    auto end = std::chrono::high_resolution_clock::now();

    double duration_ms = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count() / 1000.0;
    analyzer.add_timing_sample("my_function", duration_ms);
}

// Calculate statistics
auto stats = analyzer.calculate_timing_stats("my_function");
std::cout << "Mean: " << stats.mean << " ms" << std::endl;
std::cout << "Std Dev: " << stats.std_deviation << " ms" << std::endl;
std::cout << "95th percentile: " << stats.percentiles[4] << " ms" << std::endl;

analyzer.stop_analysis();
```

## Performance Characteristics

The Enhanced Profiler is designed for minimal overhead:

- **Timing overhead**: < 100 nanoseconds per scope
- **Memory overhead**: < 1KB per active scope
- **Thread contention**: Lock-free data structures minimize blocking
- **Statistical calculations**: Performed on-demand to reduce runtime cost

## Integration with Existing Code

The profiler integrates seamlessly with existing Quarisma components:

### With TraceMe
```cpp
// Enhanced profiler builds on TraceMe infrastructure
{
    TraceMe trace("traceme_scope");
    QUARISMA_PROFILE_SCOPE("enhanced_scope");
    // Both profilers will capture this scope
}
```

## Thread Safety

The profiler is fully thread-safe and supports concurrent profiling:

```cpp
auto session = EnhancedProfilerSession::Builder()
    .with_thread_safety(true)
    .build();

session->start();

// Launch multiple threads
std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&session, i]() {
        QUARISMA_PROFILE_SCOPE("thread_" + std::to_string(i));
        // Thread-specific work
    });
}

// Wait for completion
for (auto& t : threads) {
    t.join();
}

session->stop();
```

## Best Practices

1. **Use RAII scopes** - Prefer `QUARISMA_PROFILE_SCOPE` over manual start/stop
2. **Minimize scope names** - Use short, descriptive names to reduce overhead
3. **Configure appropriately** - Only enable features you need
4. **Profile in release builds** - The profiler is designed for production use
5. **Export results** - Save profiling data for later analysis
6. **Monitor overhead** - Use the built-in overhead measurement tests

## Troubleshooting

### Common Issues

1. **High overhead** - Disable unnecessary features or reduce sample sizes
2. **Memory leaks** - Ensure proper session cleanup and scope management
3. **Thread safety issues** - Enable thread safety if using multiple threads
4. **Missing data** - Check that profiling session is active during measurement

### Debug Mode

Enable debug logging for troubleshooting:

```cpp
// Enable verbose logging (if available)
session->set_debug_mode(true);
```

## Examples

See the test files in `Library/Core/Testing/Cxx/TestEnhancedProfiler.cpp` for comprehensive usage examples.

## License

This profiler is part of the Quarisma Core module and follows the same licensing terms.
