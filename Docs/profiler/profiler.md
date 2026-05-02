# Quarisma Profiler System - Complete User Guide

## Table of Contents
1. [Overview](#overview)
2. [Profiler backends and build configuration](#profiler-backends-and-build-configuration)
3. [Architecture](#architecture)
4. [Quick Start](#quick-start)
5. [Core Components](#core-components)
6. [API Reference](#api-reference)
7. [Usage Examples](#usage-examples)
8. [Function Pipelines](#function-pipelines)
9. [Intel ITT API Integration](#intel-itt-api-integration)
10. [Quarisma Kineto Integration](#pytorch-kineto-integration)
11. [Output Formats](#output-formats)
12. [Best Practices](#best-practices)
13. [Troubleshooting](#troubleshooting)

---

## Overview

The Quarisma Profiler System is a comprehensive, modular performance analysis framework designed for high-performance applications. It provides:

- **High-precision timing measurements** with nanosecond accuracy
- **Memory usage tracking** with allocation/deallocation monitoring
- **Hierarchical profiling** for nested function call analysis
- **Thread-safe profiling** for multi-threaded applications
- **Statistical analysis** with min/max/mean/std deviation calculations
- **Multiple output formats** (console, JSON, CSV, XML, Chrome Trace)
- **Minimal performance overhead** designed for production use
- **Intel ITT API integration** for Intel VTune profiling
- **Quarisma Kineto integration** for comprehensive profiling (when the Kineto backend is enabled at build time)

### Key Features

| Feature | Description |
|---------|-------------|
| **Timing** | Nanosecond precision using `std::chrono::high_resolution_clock` |
| **Memory Tracking** | Real-time allocation/deallocation monitoring with peak tracking |
| **Hierarchical** | Nested scope profiling with automatic hierarchy tracking |
| **Thread-Safe** | Lock-free data structures and atomic operations |
| **Statistical** | Min/max/mean/std dev/percentiles (25th, 50th, 75th, 90th, 95th, 99th) |
| **Exportable** | JSON, CSV, XML, Chrome Trace, XPlane formats |
| **Low Overhead** | < 100 nanoseconds per scope, < 1KB per active scope |

---

## Profiler backends and build configuration

Quarisma selects **one** profiler backend at configure time. The preprocessor flags `PROFILER_HAS_NATIVE`, `PROFILER_HAS_KINETO`, and `PROFILER_HAS_ITT` are mutually exclusive.

### Native backend (standalone)

The **native** backend compiles only sources under `Library/Profiler/native/` (plus shared `common/` utilities). It does **not** link libkineto or compile `bespoke/kineto` or `bespoke/itt` sources. Use it for lightweight Chrome Trace / XPlane / session APIs without Kineto.

**CMake**

- Cache variable: `-DQUARISMA_PROFILER_TYPE=NATIVE` (valid values: `KINETO`, `NATIVE`, `ITT`; default is `KINETO`).
- Helper: from `Scripts/`, `python setup.py config.build.ninja.clang.release --profiler.native` sets the same option.

**Bazel**

- Add `--config=native_profiler` to your build or test command (see root `.bazelrc`: `build:native_profiler --define=profiler_type=native`).
- Equivalent: `--define=profiler_type=native`.
- `//Library/Profiler:Profiler` then exposes include roots for `native/**` only; do not include `bespoke/*` headers unless you also enable Kineto or ITT.

**Headers and API (native mode)**

- Primary session API: `#include "native/session/profiler.h"` (add `Library/Profiler` to your include path, or depend on `Quarisma::Profiler` / `//Library/Profiler:Profiler`).
- Types and macros live in namespace `quarisma` (for example `quarisma::profiler_session_builder`, `quarisma::profiler_session`, `quarisma::profiler_scope`). Scope macros are in `common/profiler_macros.h` (for example `PROFILER_PROFILE_SCOPE`).

**Tests**

- `ProfilerCxxTests` under native mode excludes Kineto-only sources; shared translation units that reference `bespoke/*` must guard includes and code with `PROFILER_HAS_KINETO` or `PROFILER_HAS_ITT` so Bazel’s stricter include graph matches CMake (where headers may still exist on disk).

### Kineto backend (default in CMake and Bazel)

Default: `QUARISMA_PROFILER_TYPE=KINETO` and Bazel without `native_profiler` / `itt`. Sets `PROFILER_HAS_KINETO=1`, links Kineto where applicable, and compiles `bespoke/kineto` and related sources.

### ITT backend

CMake: `-DQUARISMA_PROFILER_TYPE=ITT`. Bazel: `--config=itt`. Enables `bespoke/itt` and `PROFILER_HAS_ITT=1`.

---

## Architecture

### System Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (User Code with PROFILER_PROFILE_SCOPE macros)             │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Session Management                        │
│  profiler_session, profiler_scope, profiler_report        │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     Core Framework                          │
│  profiler_interface, profiler_controller, profiler_factory │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Plugin Implementations                    │
│     CPU Profiler  │  Memory Profiler  │  Custom Profilers │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                   Data Export & Analysis                    │
│  XPlane │ JSON │ CSV │ XML │ Chrome Trace │ Statistical    │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                    Platform Abstraction                     │
│        Windows │ Linux │ macOS │ Time APIs │ Memory APIs    │
└─────────────────────────────────────────────────────────────┘
```

### Component Classification

#### Required Components (Core)
- **profiler_interface.h**: Abstract profiler interface
- **profiler_controller.***: Profiler lifecycle management
- **profiler_factory.***: Factory pattern for profiler creation
- **profiler_options.h**: Configuration options
- **profiler_lock.***: Thread synchronization primitives
- **profiler_session.***: Main profiler session class
- **profiler_report.***: Report generation

#### Optional Components (Extensions)
- **host_tracer.***: Host CPU activity tracing
- **memory_tracker.***: Memory allocation tracking
- **statistical_analyzer.***: Statistical analysis
- **chrome_trace_exporter.***: Chrome Trace format export
- **xplane.***: XPlane format support

---

## Quick Start

### Basic Usage

```cpp
#include "native/session/profiler.h"

using namespace quarisma;

int main() {
    // Create profiler session with builder pattern
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .with_output_format(profiler_options::output_format_enum::JSON)
        .build();

    // Start profiling
    session->start();

    // Profile a function
    {
        PROFILER_PROFILE_FUNCTION();

        // Your code here
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Profile nested operations
        {
            PROFILER_PROFILE_SCOPE("nested_operation");
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

---

## Core Components

### profiler_session

Main profiler session class managing the entire profiling lifecycle.

**Key Methods:**
- `start()` - Start profiling session
- `stop()` - Stop profiling session
- `is_active()` - Check if session is active
- `create_scope(name)` - Create a new profiling scope
- `generate_report()` - Generate profiling report
- `export_report(filename)` - Export report to file
- `print_report()` - Print report to console

### profiler_scope

RAII profiler scope for automatic timing and memory tracking.

```cpp
{
    profiler_scope scope("operation_name", session.get());
    // Automatic timing and memory tracking
    // Measurements recorded when scope exits
}
```

### profiler_session_builder

Builder pattern for flexible session configuration.

**Builder Methods:**
- `with_timing(bool)` - Enable/disable timing
- `with_memory_tracking(bool)` - Enable/disable memory tracking
- `with_hierarchical_profiling(bool)` - Enable/disable hierarchical profiling
- `with_statistical_analysis(bool)` - Enable/disable statistical analysis
- `with_thread_safety(bool)` - Enable/disable thread safety
- `with_output_format(format)` - Set output format
- `with_output_file(path)` - Set output file path
- `with_max_samples(count)` - Set maximum samples
- `with_percentiles(bool)` - Enable/disable percentiles
- `with_peak_memory_tracking(bool)` - Enable/disable peak memory tracking
- `with_memory_deltas(bool)` - Enable/disable memory deltas
- `with_thread_pool_size(size)` - Set thread pool size
- `build()` - Build and return session

### profiler_report

Generates comprehensive profiling reports.

**Report Contents:**
- Timing statistics (min, max, mean, std dev, percentiles)
- Memory statistics (current, peak, total allocations)
- Hierarchical scope information
- Thread-specific data
- Statistical analysis results

---

## API Reference

### Profiling Macros

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
memory_tracker tracker;
tracker.start_tracking();

// Track custom allocations
void* ptr = malloc(1024);
tracker.track_allocation(ptr, 1024, "custom_allocation");

// ... use memory ...

tracker.track_deallocation(ptr);
free(ptr);

// Get statistics
auto stats = tracker.get_current_stats();
std::cout << "Current: " << stats.current_usage_ << " bytes" << std::endl;
std::cout << "Peak: " << stats.peak_usage_ << " bytes" << std::endl;

tracker.stop_tracking();
```

### Statistical Analysis

```cpp
statistical_analyzer analyzer;
analyzer.start_analysis();

// Add timing samples
for (int i = 0; i < 100; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    // ... do work ...
    auto end = std::chrono::high_resolution_clock::now();

    double duration_ms = std::chrono::duration_cast<
        std::chrono::microseconds>(end - start).count() / 1000.0;
    analyzer.add_timing_sample("my_function", duration_ms);
}

// Calculate statistics
auto stats = analyzer.calculate_timing_stats("my_function");
std::cout << "Mean: " << stats.mean << " ms" << std::endl;
std::cout << "Std Dev: " << stats.std_deviation << " ms" << std::endl;
std::cout << "95th percentile: " << stats.percentiles[4] << " ms" << std::endl;

analyzer.stop_analysis();
```

---

## Usage Examples

### Example 1: Basic Profiling

```cpp
auto session = profiler_session_builder()
    .with_timing(true)
    .build();

session->start();

{
    PROFILER_PROFILE_SCOPE("compute");
    // Your computation
}

session->stop();
session->print_report();
```

### Example 2: Memory Profiling

```cpp
auto session = profiler_session_builder()
    .with_memory_tracking(true)
    .with_peak_memory_tracking(true)
    .build();

session->start();

{
    PROFILER_PROFILE_SCOPE("memory_intensive");
    std::vector<int> data(1000000);
    // Process data
}

session->stop();
session->print_report();
```

### Example 3: Hierarchical Profiling

```cpp
auto session = profiler_session_builder()
    .with_hierarchical_profiling(true)
    .build();

session->start();

{
    PROFILER_PROFILE_SCOPE("level_1");
    {
        PROFILER_PROFILE_SCOPE("level_2");
        {
            PROFILER_PROFILE_SCOPE("level_3");
            // Nested work
        }
    }
}

session->stop();
session->print_report();
```

### Example 4: Multi-threaded Profiling

```cpp
auto session = profiler_session_builder()
    .with_thread_safety(true)
    .build();

session->start();

std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([&session, i]() {
        PROFILER_PROFILE_SCOPE("thread_" + std::to_string(i));
        // Thread-specific work
    });
}

for (auto& t : threads) {
    t.join();
}

session->stop();
session->print_report();
```

### Example 5: Statistical Analysis

```cpp
auto session = profiler_session_builder()
    .with_statistical_analysis(true)
    .with_percentiles(true)
    .build();

session->start();

for (int i = 0; i < 100; ++i) {
    PROFILER_PROFILE_SCOPE("repeated_operation");
    // Work that varies in duration
}

session->stop();
auto report = session->generate_report();
// Report includes percentiles and statistical analysis
```

---

## Function Pipelines

### Profiling Pipeline

```
User Code
    ↓
PROFILER_PROFILE_SCOPE macro
    ↓
profiler_scope constructor
    ↓
profiler_session::create_scope()
    ↓
Timing measurement starts
Memory tracking starts (if enabled)
    ↓
User code executes
    ↓
profiler_scope destructor
    ↓
Timing measurement stops
Memory tracking stops (if enabled)
    ↓
Data recorded in profiler_session
    ↓
Statistical analysis (if enabled)
    ↓
Report generation on demand
```

### Report Generation Pipeline

```
profiler_session::generate_report()
    ↓
Collect all scope data
    ↓
Calculate statistics (if enabled)
    ↓
Organize hierarchically (if enabled)
    ↓
Format according to output_format_
    ↓
profiler_report object
    ↓
Export to file or console
```

### Export Pipeline

```
profiler_session::export_report(filename)
    ↓
generate_report()
    ↓
Format selection:
  - JSON: JSON serialization
  - CSV: Comma-separated values
  - XML: XML markup
  - CONSOLE: Human-readable text
    ↓
Write to file
    ↓
Return success/failure
```

---

## Intel ITT API Integration

### Enabling ITT API

ITT API is enabled by default in Quarisma builds:

```bash
cd Scripts
python setup.py config.build.ninja.clang.debug
```

### Using ITT API with Profiler

```cpp
#ifdef PROFILER_HAS_ITT
#include <ittnotify.h>
#endif

#include "native/session/profiler.h"

void profile_with_itt() {
#ifdef PROFILER_HAS_ITT
    __itt_domain* domain = __itt_domain_create("QuarismaProfiler");
    auto handle = __itt_string_handle_create("ProfiledTask");
    __itt_task_begin(domain, __itt_null, __itt_null, handle);
#endif

    auto session = profiler_session_builder()
        .with_timing(true)
        .build();

    session->start();
    {
        PROFILER_PROFILE_SCOPE("task");
        // Your code
    }
    session->stop();

#ifdef PROFILER_HAS_ITT
    __itt_task_end(domain);
#endif
}
```

### ITT API Features

- Task annotations for Intel VTune
- Frame-based profiling
- Domain-based organization
- Thread-safe operation
- Minimal overhead (~1-2%)

---

## Quarisma Kineto Integration

### Enabling Kineto

Kineto is enabled by default in Quarisma builds.

### Using Kineto Profiler

```cpp
#ifdef PROFILER_HAS_KINETO
#include "profiler/kineto_profiler.h"

void profile_with_kineto() {
    auto profiler = quarisma::kineto_profiler::create();
    if (profiler) {
        if (profiler->start_profiling()) {
            // Your code
            profiler->stop_profiling();
        }
    }
}
#endif
```

### Kineto Configuration

```cpp
#ifdef PROFILER_HAS_KINETO
quarisma::kineto_profiler::profiling_config config;
config.enable_cpu_tracing = true;
config.enable_gpu_tracing = false;
config.enable_memory_profiling = false;
config.output_dir = "./kineto_profiles";
config.trace_name = "quarisma_trace";
config.max_activities = 0;  // Unlimited

auto profiler = quarisma::kineto_profiler::create_with_config(config);
#endif
```

### Kineto Features

- Comprehensive profiling capabilities
- Session management
- Wrapper mode support (graceful degradation)
- Thread-safe operation
- Error handling via return values

---

## Output Formats

### JSON Format

```json
{
  "profiling_data": {
    "total_duration_ns": 1000000,
    "scopes": [
      {
        "name": "main_scope",
        "duration_ns": 1000000,
        "memory_allocated": 1024000,
        "memory_freed": 512000,
        "children": [
          {
            "name": "nested_scope",
            "duration_ns": 500000,
            "memory_allocated": 512000,
            "memory_freed": 256000
          }
        ]
      }
    ],
    "statistics": {
      "min_ns": 100000,
      "max_ns": 2000000,
      "mean_ns": 1000000,
      "std_dev_ns": 250000,
      "percentiles": {
        "p25": 750000,
        "p50": 1000000,
        "p75": 1250000,
        "p90": 1500000,
        "p95": 1750000,
        "p99": 1900000
      }
    }
  }
}
```

### CSV Format

```
scope_name,duration_ns,memory_allocated,memory_freed,thread_id
main_scope,1000000,1024000,512000,1
nested_scope,500000,512000,256000,1
```

### Chrome Trace Format

```json
{
  "traceEvents": [
    {"name": "process_name", "ph": "M", "pid": 1, "args": {"name": "Host"}},
    {"name": "thread_name", "ph": "M", "pid": 1, "tid": 100, "args": {"name": "Worker-1"}},
    {"name": "main_scope", "ph": "X", "pid": 1, "tid": 100, "ts": 1000, "dur": 1000000},
    {"name": "nested_scope", "ph": "X", "pid": 1, "tid": 100, "ts": 1500, "dur": 500000}
  ],
  "displayTimeUnit": "ns"
}
```

### XPlane Format

XPlane is a structured format used internally by Quarisma for comprehensive profiling data representation with planes, lines, and events.

---

## Best Practices

### 1. Use RAII Scopes

Prefer `PROFILER_PROFILE_SCOPE` over manual start/stop:

```cpp
// Good
{
    PROFILER_PROFILE_SCOPE("operation");
    // Work
}

// Avoid
profiler->start_scope("operation");
// Work
profiler->stop_scope();
```

### 2. Minimize Scope Names

Use short, descriptive names to reduce overhead:

```cpp
// Good
PROFILER_PROFILE_SCOPE("compute");

// Avoid
PROFILER_PROFILE_SCOPE("very_long_descriptive_name_for_computation");
```

### 3. Configure Appropriately

Only enable features you need:

```cpp
auto session = profiler_session_builder()
    .with_timing(true)
    .with_memory_tracking(false)  // Disable if not needed
    .with_statistical_analysis(false)  // Disable if not needed
    .build();
```

### 4. Profile in Release Builds

The profiler is designed for production use with minimal overhead.

### 5. Export Results

Save profiling data for later analysis:

```cpp
session->export_report("profile_results.json");
```

### 6. Monitor Overhead

Use built-in overhead measurement tests to verify performance impact.

### 7. Thread Safety

Enable thread safety when using multiple threads:

```cpp
auto session = profiler_session_builder()
    .with_thread_safety(true)
    .build();
```

---

## Troubleshooting

### High Overhead

**Problem**: Profiling adds too much overhead.

**Solutions**:
1. Disable unnecessary features
2. Reduce sample sizes
3. Use coarser-grained scopes
4. Profile only critical sections

### Memory Leaks

**Problem**: Memory usage grows during profiling.

**Solutions**:
1. Ensure proper session cleanup
2. Check scope management
3. Verify memory tracking is disabled when not needed
4. Review memory allocation patterns

### Thread Safety Issues

**Problem**: Crashes or data corruption in multi-threaded code.

**Solutions**:
1. Enable thread safety: `.with_thread_safety(true)`
2. Ensure all threads use the same session
3. Verify proper synchronization

### Missing Data

**Problem**: Profiling data is incomplete or missing.

**Solutions**:
1. Verify profiling session is active
2. Check that scopes are properly created
3. Ensure session is not stopped prematurely
4. Verify output format is correct

### Performance Regression

**Problem**: Application performance degrades with profiling enabled.

**Solutions**:
1. Use statistical analysis to identify bottlenecks
2. Profile only critical sections
3. Reduce profiling frequency
4. Consider sampling-based profiling

---

## Performance Characteristics

The Enhanced Profiler is designed for minimal overhead:

- **Timing overhead**: < 100 nanoseconds per scope
- **Memory overhead**: < 1KB per active scope
- **Thread contention**: Lock-free data structures minimize blocking
- **Statistical calculations**: Performed on-demand to reduce runtime cost

---

## Platform Support

- ✅ **Windows** - Full support with high-resolution timing
- ✅ **Linux** - Full support with POSIX timing
- ✅ **macOS** - Full support with Mach timing

---

## Heavy Function Profiling Examples

### Example: Matrix Multiplication Profiling

```cpp
#include "native/session/profiler.h"
#include <vector>

std::vector<std::vector<double>> matrix_multiply(
    const std::vector<std::vector<double>>& a,
    const std::vector<std::vector<double>>& b) {
    PROFILER_PROFILE_SCOPE("matrix_multiply");

    const size_t rows_a = a.size();
    const size_t cols_a = a[0].size();
    const size_t cols_b = b[0].size();

    std::vector<std::vector<double>> result(
        rows_a, std::vector<double>(cols_b, 0.0));

    {
        PROFILER_PROFILE_SCOPE("matrix_computation");
        for (size_t i = 0; i < rows_a; ++i) {
            PROFILER_PROFILE_SCOPE("row_" + std::to_string(i));
            for (size_t j = 0; j < cols_b; ++j) {
                for (size_t k = 0; k < cols_a; ++k) {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
    }

    return result;
}

int main() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .with_output_format(profiler_options::output_format_enum::JSON)
        .build();

    session->start();

    // Create test matrices
    std::vector<std::vector<double>> a(100, std::vector<double>(100, 1.0));
    std::vector<std::vector<double>> b(100, std::vector<double>(100, 2.0));

    // Profile matrix multiplication
    auto result = matrix_multiply(a, b);

    session->stop();
    session->export_report("matrix_profile.json");
    session->print_report();

    return 0;
}
```

### Example: Sorting Algorithm Profiling

```cpp
void merge_sort(std::vector<double>& arr, size_t left, size_t right, int depth = 0) {
    PROFILER_PROFILE_SCOPE("merge_sort_depth_" + std::to_string(depth));

    if (left >= right) return;

    size_t mid = left + (right - left) / 2;

    {
        PROFILER_PROFILE_SCOPE("sort_left");
        merge_sort(arr, left, mid, depth + 1);
    }

    {
        PROFILER_PROFILE_SCOPE("sort_right");
        merge_sort(arr, mid + 1, right, depth + 1);
    }

    {
        PROFILER_PROFILE_SCOPE("merge");
        std::vector<double> temp(right - left + 1);
        size_t i = left, j = mid + 1, k = 0;

        while (i <= mid && j <= right) {
            if (arr[i] <= arr[j]) {
                temp[k++] = arr[i++];
            } else {
                temp[k++] = arr[j++];
            }
        }

        while (i <= mid) temp[k++] = arr[i++];
        while (j <= right) temp[k++] = arr[j++];

        for (size_t i = 0; i < temp.size(); ++i) {
            arr[left + i] = temp[i];
        }
    }
}

int main() {
    auto session = profiler_session_builder()
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .build();

    session->start();

    std::vector<double> data(10000);
    std::generate(data.begin(), data.end(), []() {
        return rand() % 1000;
    });

    merge_sort(data, 0, data.size() - 1);

    session->stop();
    session->export_report("sort_profile.json");

    return 0;
}
```

### Example Output: JSON Format

```json
{
  "profiling_session": {
    "total_duration_ns": 5234567890,
    "scopes": [
      {
        "name": "matrix_multiply",
        "duration_ns": 5000000000,
        "memory_allocated": 80000,
        "memory_freed": 0,
        "call_count": 1,
        "children": [
          {
            "name": "matrix_computation",
            "duration_ns": 4999000000,
            "memory_allocated": 80000,
            "memory_freed": 0,
            "call_count": 1,
            "children": [
              {
                "name": "row_0",
                "duration_ns": 50000000,
                "memory_allocated": 0,
                "memory_freed": 0,
                "call_count": 1
              },
              {
                "name": "row_1",
                "duration_ns": 50000000,
                "memory_allocated": 0,
                "memory_freed": 0,
                "call_count": 1
              }
            ]
          }
        ]
      }
    ],
    "statistics": {
      "total_scopes": 102,
      "max_depth": 3,
      "timing_stats": {
        "min_ns": 1000000,
        "max_ns": 5000000000,
        "mean_ns": 50000000,
        "std_dev_ns": 25000000,
        "percentiles": {
          "p25": 25000000,
          "p50": 50000000,
          "p75": 75000000,
          "p90": 90000000,
          "p95": 95000000,
          "p99": 99000000
        }
      },
      "memory_stats": {
        "total_allocated": 80000,
        "total_freed": 0,
        "peak_usage": 80000,
        "current_usage": 80000
      }
    }
  }
}
```

### Example Output: Chrome Trace Format

```json
{
  "traceEvents": [
    {
      "name": "process_name",
      "ph": "M",
      "pid": 1,
      "args": {"name": "QuarismaProfiler"}
    },
    {
      "name": "thread_name",
      "ph": "M",
      "pid": 1,
      "tid": 100,
      "args": {"name": "MainThread"}
    },
    {
      "name": "matrix_multiply",
      "ph": "X",
      "pid": 1,
      "tid": 100,
      "ts": 1000000,
      "dur": 5000000000,
      "args": {
        "memory_allocated": 80000,
        "memory_freed": 0
      }
    },
    {
      "name": "matrix_computation",
      "ph": "X",
      "pid": 1,
      "tid": 100,
      "ts": 1500000,
      "dur": 4999000000,
      "args": {
        "memory_allocated": 80000,
        "memory_freed": 0
      }
    },
    {
      "name": "row_0",
      "ph": "X",
      "pid": 1,
      "tid": 100,
      "ts": 2000000,
      "dur": 50000000
    },
    {
      "name": "row_1",
      "ph": "X",
      "pid": 1,
      "tid": 100,
      "ts": 52000000,
      "dur": 50000000
    }
  ],
  "displayTimeUnit": "ns"
}
```

### Example Output: CSV Format

```csv
scope_name,duration_ns,memory_allocated,memory_freed,call_count,thread_id,depth
matrix_multiply,5000000000,80000,0,1,100,0
matrix_computation,4999000000,80000,0,1,100,1
row_0,50000000,0,0,1,100,2
row_1,50000000,0,0,1,100,2
row_2,50000000,0,0,1,100,2
```

### Example Output: Console Format

```
Quarisma Profiler Report
======================

Total Duration: 5.23 seconds
Total Scopes: 102
Maximum Depth: 3

Scope Hierarchy:
├─ matrix_multiply (5.00s)
│  ├─ matrix_computation (4.99s)
│  │  ├─ row_0 (50.00ms)
│  │  ├─ row_1 (50.00ms)
│  │  ├─ row_2 (50.00ms)
│  │  └─ ... (97 more rows)

Memory Statistics:
  Total Allocated: 80.00 KB
  Total Freed: 0 B
  Peak Usage: 80.00 KB
  Current Usage: 80.00 KB

Timing Statistics:
  Min: 1.00ms
  Max: 5.00s
  Mean: 50.00ms
  Std Dev: 25.00ms

  Percentiles:
    25th: 25.00ms
    50th: 50.00ms
    75th: 75.00ms
    90th: 90.00ms
    95th: 95.00ms
    99th: 99.00ms
```

---

## See Also

- [Intel ITT API Documentation](https://github.com/intel/ittapi)
- [Quarisma Kineto](https://github.com/pytorch/kineto)
- [Chrome Trace Format](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU)
- [XPlane format](Library/Profiler/native/exporters/xplane/)
