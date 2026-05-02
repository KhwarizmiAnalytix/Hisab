# Quarisma Profiler library (`Library/Profiler`)

Long-form documentation for the profiler **library** target. Sources live under [`Library/Profiler/`](../../Library/Profiler/).

## Overview

The profiler is a separate CMake/Bazel target from Core. Link `Quarisma::Profiler` (and `Quarisma::Core` when using a static build or Core APIs).

### Build: native backend

Native mode builds only `native/` (plus `common/`) sources—no Kineto or ITT `bespoke/` tree.

- **CMake**: `-DQUARISMA_PROFILER_TYPE=NATIVE`, or `python setup.py ... --profiler.native` from `Scripts/`.
- **Bazel**: pass `--config=native_profiler` (or `--define=profiler_type=native`).

With native mode, include the session API as `"native/session/profiler.h"` (include root is `Library/Profiler`). The default CMake/Bazel backend is **Kineto**, not native.

The library provides:

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
#include "native/session/profiler.h"
#include "common/profiler_macros.h"

using namespace quarisma;

int main() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .with_output_format(profiler_options::output_format_enum::JSON)
        .build();

    if (!session)
    {
        return 1;
    }
    session->start();

    {
        PROFILER_PROFILE_FUNCTION();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            PROFILER_PROFILE_SCOPE("nested_operation");
            std::vector<int> data(1000, 42);
        }
    }

    session->stop();
    session->print_report();
    session->export_report("profile_results.json");

    return 0;
}
```

### Advanced Configuration

```cpp
auto session = profiler_session_builder()
    .with_timing(true)
    .with_memory_tracking(true)
    .with_hierarchical_profiling(true)
    .with_statistical_analysis(true)
    .with_thread_safety(true)
    .with_output_format(profiler_options::output_format_enum::JSON)
    .with_output_file("detailed_profile.json")
    .with_max_samples(10000)
    .with_percentiles(true)
    .with_peak_memory_tracking(true)
    .with_memory_deltas(true)
    .with_thread_pool_size(8)
    .build();
```

## API Reference

### profiler_session and profiler_session_builder

The session type is `quarisma::profiler_session`; configure it with `quarisma::profiler_session_builder` (see `native/session/profiler.h`).

#### Builder pattern (selected methods)
- `with_timing(bool)` — timing measurements
- `with_memory_tracking(bool)` — allocation tracking
- `with_hierarchical_profiling(bool)` — nested scopes
- `with_statistical_analysis(bool)` — stats aggregation
- `with_thread_safety(bool)` — thread-safe collection
- `with_output_format(...)` — JSON, CSV, console, etc.
- `with_output_file(string)` — default export path
- `with_max_samples(size_t)` — cap per series
- `with_percentiles(bool)` — percentile stats
- `with_peak_memory_tracking(bool)` — peak usage
- `with_memory_deltas(bool)` — deltas between points
- `with_thread_pool_size(size_t)` — worker pool hint

#### Session methods
- `start()` / `stop()` / `is_active()`
- `generate_report()` / `export_report(filename)` / `print_report()`
- Chrome trace: `write_chrome_trace(path)` where implemented on the session

### Profiling Macros

Convenient macros for automatic profiling:

```cpp
// Profile current scope
PROFILER_PROFILE_SCOPE("scope_name");

// Profile current function
PROFILER_PROFILE_FUNCTION();

// Profile a block of code
PROFILER_PROFILE_BLOCK("block_name") {
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
// TraceMe coexists with the native session/scopes when enabled
{
    TraceMe trace("traceme_scope");
    PROFILER_PROFILE_SCOPE("native_scope");
}
```

## Thread Safety

The profiler is fully thread-safe and supports concurrent profiling:

```cpp
auto session = profiler_session_builder()
    .with_thread_safety(true)
    .build();

session->start();

// Launch multiple threads
std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&session, i]() {
        PROFILER_PROFILE_SCOPE("thread_" + std::to_string(i));
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

1. **Use RAII scopes** - Prefer `PROFILER_PROFILE_SCOPE` over manual start/stop
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

See the tests under `Library/Profiler/Testing/Cxx/` (for example `TestEnhancedProfiler.cpp`) for usage examples.

## License

This library follows the same licensing terms as the rest of the Quarisma repository.
