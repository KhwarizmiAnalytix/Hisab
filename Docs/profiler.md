# Quarisma Profiler System - Complete User Guide

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Quick Start](#quick-start)
4. [Displaying Profiler Output](#displaying-profiler-output)
5. [Core Components](#core-components)
6. [API Reference](#api-reference)
7. [Usage Examples](#usage-examples)
8. [Output Formats and Visualization](#output-formats-and-visualization)
9. [Intel ITT API Integration](#intel-itt-api-integration)
10. [Quarisma Kineto Integration](#pytorch-kineto-integration)
11. [Best Practices](#best-practices)
12. [Troubleshooting](#troubleshooting)

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
- **Quarisma Kineto integration** for comprehensive GPU/CPU profiling

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
| **Visualization** | Chrome Tracing, Perfetto UI, Intel VTune integration |

---

## Architecture

### System Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (User Code with QUARISMA_PROFILE_SCOPE macros)             │
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
- **profiler_report.***: Report generation and output formatting

#### Optional Components (Extensions)
- **host_tracer.***: Host CPU activity tracing
- **memory_tracker.***: Memory allocation tracking
- **statistical_analyzer.***: Statistical analysis
- **chrome_trace_exporter.***: Chrome Trace format export
- **xplane.***: XPlane format support
- **kineto_shim.***: Quarisma Kineto integration
- **itt_wrapper.***: Intel ITT API integration

---

## Quick Start

### Basic Usage

```cpp
#include "profiler/session/profiler.h"

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
    session->print_report();              // Display to console
    session->export_report("profile_results.json");  // Save to file

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

## Displaying Profiler Output

Quarisma provides multiple ways to display and visualize profiler data, from simple console output to advanced visualization tools.

### 1. Console Output

#### Print Report to Console

The simplest way to view profiler results is to print them directly to the console:

```cpp
session->stop();
session->print_report();  // Prints comprehensive report to console
```

**Console Output Includes:**
- Session summary (duration, scope count, max depth)
- Hierarchical scope tree with indentation
- Timing statistics (min, max, mean, std dev)
- Memory statistics (current, peak, allocations)
- Thread information
- Statistical analysis (percentiles)

**Example Console Output:**
```
================================================================================
                        PROFILER SESSION REPORT
================================================================================
Session Active: false
Total Scopes: 15
Max Depth: 3
Session Duration: 125.45 ms

================================================================================
                        HIERARCHICAL SCOPES
================================================================================
main_function (125.45 ms)
  ├─ matrix_operations (85.23 ms)
  │   ├─ matrix_multiply_0 (42.11 ms)
  │   └─ matrix_multiply_1 (43.12 ms)
  └─ sorting_operations (40.22 ms)
      ├─ merge_sort (25.15 ms)
      └─ quick_sort (15.07 ms)

================================================================================
                        TIMING STATISTICS
================================================================================
Scope Name                    Count    Min (ms)   Max (ms)   Mean (ms)  Std Dev
matrix_multiply_0                 1      42.11      42.11      42.11      0.00
matrix_multiply_1                 1      43.12      43.12      43.12      0.00
merge_sort                        1      25.15      25.15      25.15      0.00

================================================================================
                        MEMORY STATISTICS
================================================================================
Current Usage: 2.45 MB
Peak Usage: 5.67 MB
Total Allocations: 1,234
Total Deallocations: 1,100
```

#### Custom Report Generation

Generate specific sections of the report:

```cpp
auto report = session->generate_report();

// Generate and print specific sections
std::cout << report->generate_header_section();
std::cout << report->generate_timing_section();
std::cout << report->generate_memory_section();
std::cout << report->generate_statistical_section();
```

### 2. File Export

#### Export to Different Formats

```cpp
// Export as JSON
session->export_report("profile.json");

// Or use profiler_report for more control
auto report = session->generate_report();
report->export_to_file("profile.json", profiler_options::output_format_enum::JSON);
report->export_to_file("profile.csv", profiler_options::output_format_enum::CSV);
report->export_to_file("profile.xml", profiler_options::output_format_enum::STRUCTURED);
report->export_to_file("profile.txt", profiler_options::output_format_enum::CONSOLE);
```

### 3. Chrome Trace Format

The Chrome Trace format enables visualization in Chrome DevTools and Perfetto UI:

```cpp
// Export Chrome Trace JSON
session->write_chrome_trace("trace.json");

// Or generate as string
std::string trace_json = session->generate_chrome_trace_json();
```

**Viewing Chrome Traces:**
1. **Chrome DevTools**: Open `chrome://tracing` in Chrome browser
2. **Perfetto UI**: Visit https://ui.perfetto.dev/
3. Load the `trace.json` file
4. Explore timeline, zoom, filter by thread/process

**Chrome Trace Features:**
- Timeline visualization with zoom/pan
- Process and thread separation
- Event duration bars
- Nested scope visualization
- Metadata and statistics

### 4. Customizing Report Output

#### Using Report Builder

```cpp
auto report = profiler_report_builder(*session)
    .with_precision(6)                    // 6 decimal places
    .with_time_unit("us")                 // Microseconds
    .with_memory_unit("KB")               // Kilobytes
    .include_thread_info(true)            // Include thread details
    .include_hierarchical_data(true)      // Include scope hierarchy
    .include_statistical_analysis(true)   // Include percentiles
    .include_memory_details(true)         // Include memory breakdown
    .build();

// Generate custom report
std::string custom_report = report->generate_console_report();
std::cout << custom_report;
```

#### Configuring Output Format

```cpp
profiler_options opts;
opts.output_format_ = profiler_options::output_format_enum::JSON;  // JSON format
opts.output_file_path_ = "output.json";                            // Output file
opts.calculate_percentiles_ = true;                                // Enable percentiles
opts.track_peak_memory_ = true;                                    // Track peak memory
opts.track_memory_deltas_ = true;                                  // Track memory changes

profiler_session session(opts);
```

### 5. Real-Time Monitoring

#### Accessing Live Profiler Data

```cpp
// Check if profiler is active
if (session->is_active()) {
    // Get current scope count
    const auto* root = session->get_root_scope();

    // Access XSpace data (raw profiling data)
    const auto& xspace = session->get_xspace();

    // Get session timing
    auto start_time = session->session_start_time();
    auto end_time = session->session_end_time();
}
```

### 6. Output Format Details

#### JSON Format

```json
{
  "header": {
    "active": false,
    "scope_count": 15,
    "max_depth": 3,
    "duration_ms": 125.45
  },
  "scopes": [
    {
      "name": "matrix_multiply",
      "duration_ms": 42.11,
      "memory_bytes": 1048576,
      "thread_id": "0x7f8a1c000000",
      "children": []
    }
  ],
  "top_durations": [
    {"name": "matrix_multiply", "duration_ms": 42.11}
  ]
}
```

#### CSV Format

```csv
Scope Name,Duration (ms),Memory (bytes),Thread ID,Depth
matrix_multiply,42.11,1048576,0x7f8a1c000000,1
merge_sort,25.15,524288,0x7f8a1c000000,2
```

#### XML Format

```xml
<?xml version="1.0" encoding="UTF-8"?>
<profiler_report>
  <header>
    <active>false</active>
    <scope_count>15</scope_count>
    <max_depth>3</max_depth>
    <duration_ms>125.45</duration_ms>
  </header>
  <scopes>
    <scope name="matrix_multiply" duration_ms="42.11" memory_bytes="1048576"/>
  </scopes>
</profiler_report>
```

### 7. Verbosity Control

```cpp
// Enable verbose logging for debugging
quarisma::profiler_impl::ProfilerConfig config;
config.verbose = true;  // Enables detailed logging

// Or for native profiler
profiler_options opts;
opts.enable_timing_ = true;
// Verbose output controlled via logging system
```

---

## Core Components

### profiler_session

The main profiler session class that manages the profiling lifecycle.

**Key Methods:**
- `start()` - Start profiling session
- `stop()` - Stop profiling session
- `is_active()` - Check if session is active
- `generate_report()` - Generate profiling report
- `export_report(filename)` - Export report to file
- `print_report()` - Print report to console
- `write_chrome_trace(filename)` - Export Chrome Trace JSON
- `generate_chrome_trace_json()` - Generate Chrome Trace JSON string
- `get_root_scope()` - Access root profiling scope
- `get_xspace()` - Access raw XSpace profiling data

**Example:**
```cpp
profiler_session session(opts);
session.start();
// ... profiled code ...
session.stop();
session.print_report();
```

### profiler_scope

RAII-based scope profiler that automatically tracks entry and exit.

**Usage:**
```cpp
{
    profiler_scope scope("my_operation");
    // Code is automatically profiled
} // Scope ends, timing recorded
```

**Macros:**
```cpp
QUARISMA_PROFILE_SCOPE("scope_name");  // Profile named scope
QUARISMA_PROFILE_FUNCTION();           // Profile current function
```

### profiler_report

Generates comprehensive profiling reports in multiple formats.

**Report Contents:**
- Timing statistics (min, max, mean, std dev, percentiles)
- Memory statistics (current, peak, total allocations)
- Hierarchical scope information
- Thread-specific data
- Statistical analysis results

**Methods:**
- `generate_console_report()` - Human-readable console output
- `generate_json_report()` - JSON format
- `generate_csv_report()` - CSV format
- `generate_xml_report()` - XML format
- `export_to_file(filename, format)` - Export to file
- `print_detailed_report()` - Print to console via logging

### profiler_session_builder

Fluent builder interface for creating profiler sessions.

**Builder Methods:**
- `with_timing(bool)` - Enable timing measurements
- `with_memory_tracking(bool)` - Enable memory tracking
- `with_hierarchical_profiling(bool)` - Enable scope hierarchy
- `with_statistical_analysis(bool)` - Enable statistical analysis
- `with_thread_safety(bool)` - Enable thread-safe operations
- `with_output_format(format)` - Set output format
- `with_output_file(path)` - Set output file path
- `with_max_samples(count)` - Set max statistical samples
- `with_percentiles(bool)` - Enable percentile calculations
- `with_peak_memory_tracking(bool)` - Track peak memory
- `with_memory_deltas(bool)` - Track memory changes
- `with_thread_pool_size(size)` - Set thread pool size
- `build()` - Build and return session

---

## API Reference

### Profiling Macros

```cpp
// Profile current scope
QUARISMA_PROFILE_SCOPE("scope_name");

// Profile current function
QUARISMA_PROFILE_FUNCTION();
```

### Configuration Options

```cpp
struct profiler_options {
    // Timing
    bool enable_timing_ = true;

    // Memory tracking
    bool enable_memory_tracking_ = false;
    bool track_peak_memory_ = true;
    bool track_memory_deltas_ = true;

    // Hierarchical profiling
    bool enable_hierarchical_profiling_ = false;

    // Statistical analysis
    bool enable_statistical_analysis_ = false;
    bool calculate_percentiles_ = true;
    size_t max_samples_ = 1000;

    // Thread safety
    bool enable_thread_safety_ = true;
    size_t thread_pool_size_ = std::thread::hardware_concurrency();

    // Output format
    enum class output_format_enum {
        CONSOLE,    // Human-readable console output
        FILE,       // Plain text file output
        JSON,       // JSON format
        CSV,        // CSV format
        STRUCTURED  // XML format
    };
    output_format_enum output_format_ = output_format_enum::CONSOLE;
    std::string output_file_path_;
};
```

### Quarisma-Compatible Profiler API

Quarisma provides a Quarisma-compatible profiler API for seamless integration:

```cpp
#include "profiler/profiler_api.h"

using namespace quarisma::profiler_impl;

// Configure profiler
ProfilerConfig config;
config.activities = {ActivityType::CPU};
config.output_file = "trace.json";
config.record_shapes = false;
config.profile_memory = false;
config.with_stack = false;
config.verbose = false;

// Get singleton instance
auto& profiler = ProfilerSession::instance();

// Start profiling
profiler.start(config);

// Profile code with RAII guards
{
    ProfilerGuard guard(config);
    // Code is profiled
} // Automatically stops

// Or use RecordFunction
{
    RecordFunction record("my_function");
    // Function is recorded
}

// Or use ScopedActivity
{
    ScopedActivity activity("matrix_multiply");
    // Activity is recorded
}

// Stop and export
profiler.stop();
profiler.export_trace(config.output_file);
```

**Activity Types:**
```cpp
enum class ActivityType {
    CPU,           // CPU operations
    CUDA,          // NVIDIA CUDA operations
    XPU,           // Intel XPU operations
    MTIA,          // Meta Training and Inference Accelerator
    PrivateUse1    // Custom backend
};
```

**Profiler States:**
```cpp
enum class ProfilerState {
    Disabled,      // Profiler is disabled
    Ready,         // Profiler is ready to start
    Running,       // Profiler is actively recording
    Stopped        // Profiler has stopped
};
```

---

## Usage Examples

### Example 1: Basic Profiling

```cpp
#include "profiler/session/profiler.h"

void basic_profiling_example() {
    // Create session
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_output_format(profiler_options::output_format_enum::JSON)
        .build();

    session->start();

    // Profile operations
    {
        QUARISMA_PROFILE_SCOPE("data_processing");
        std::vector<int> data(1000000);
        std::iota(data.begin(), data.end(), 0);
    }

    session->stop();
    session->print_report();
    session->export_report("basic_profile.json");
}
```

### Example 2: Hierarchical Profiling

```cpp
void hierarchical_profiling_example() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_hierarchical_profiling(true)
        .build();

    session->start();

    {
        QUARISMA_PROFILE_SCOPE("level_1");

        {
            QUARISMA_PROFILE_SCOPE("level_2_a");
            // Work...
        }

        {
            QUARISMA_PROFILE_SCOPE("level_2_b");

            {
                QUARISMA_PROFILE_SCOPE("level_3");
                // Nested work...
            }
        }
    }

    session->stop();
    session->print_report();  // Shows hierarchical tree
}
```

### Example 3: Memory Profiling

```cpp
void memory_profiling_example() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_peak_memory_tracking(true)
        .with_memory_deltas(true)
        .build();

    session->start();

    {
        QUARISMA_PROFILE_SCOPE("memory_intensive_operation");

        // Allocate large buffer
        std::vector<double> large_buffer(10000000);

        // Fill with data
        std::fill(large_buffer.begin(), large_buffer.end(), 3.14159);
    }

    session->stop();

    auto report = session->generate_report();
    std::cout << report->generate_memory_section();
}
```

### Example 4: Chrome Trace Export

```cpp
void chrome_trace_example() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_hierarchical_profiling(true)
        .build();

    session->start();

    {
        QUARISMA_PROFILE_SCOPE("main_operation");

        {
            QUARISMA_PROFILE_SCOPE("sub_operation_1");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        {
            QUARISMA_PROFILE_SCOPE("sub_operation_2");
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }

    session->stop();

    // Export Chrome Trace format
    session->write_chrome_trace("trace.json");

    std::cout << "Open trace.json in chrome://tracing or https://ui.perfetto.dev/" << std::endl;
}
```

---

## Output Formats and Visualization

### Viewing Chrome Traces

1. **Chrome DevTools** (`chrome://tracing`):
   - Open Chrome browser
   - Navigate to `chrome://tracing`
   - Click "Load" and select your `trace.json` file
   - Use mouse to zoom, pan, and explore timeline

2. **Perfetto UI** (https://ui.perfetto.dev/):
   - Visit https://ui.perfetto.dev/
   - Drag and drop your `trace.json` file
   - More advanced features than Chrome DevTools
   - Better performance for large traces

3. **Features Available**:
   - Timeline visualization with zoom/pan
   - Process and thread separation
   - Event duration bars with colors
   - Nested scope visualization
   - Search and filter capabilities
   - Statistics and aggregations
   - Export screenshots and data

---

## Intel ITT API Integration

Quarisma integrates with Intel's Instrumentation and Tracing Technology (ITT) API for use with Intel VTune Profiler.

### Building with ITT Support

```bash
cd Quarisma
mkdir build && cd build
cmake -DQUARISMA_ENABLE_PROFILER=ON \
      -DUSE_ITT=ON ..
cmake --build .
```

### Using ITT with Quarisma

```cpp
#include "profiler/session/profiler.h"
#include "profiler/itt_wrapper.h"

void itt_profiling_example() {
    // Initialize ITT
    quarisma::profiler_impl::itt_init();
    bool itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);

    // Start Quarisma profiler
    auto session = profiler_session_builder()
        .with_timing(true)
        .build();

    session->start();

    // Profile with both ITT and Quarisma
    {
        if (itt_available) {
            quarisma::profiler_impl::itt_range_push("my_operation");
        }
        QUARISMA_PROFILE_SCOPE("my_operation");

        // Your code here

        if (itt_available) {
            quarisma::profiler_impl::itt_range_pop();
        }
    }

    session->stop();
    session->write_chrome_trace("trace.json");
}
```

### ITT API Functions

```cpp
// Initialize ITT
void itt_init();

// Get ITT domain
__itt_domain* itt_get_domain();

// Push/pop ranges
void itt_range_push(const char* name);
void itt_range_pop();

// Mark events
void itt_mark_event(const char* name);
```

---

## Quarisma Kineto Integration

Quarisma integrates with Quarisma's Kineto profiler for comprehensive CPU and GPU profiling.

### Building with Kineto Support

```bash
cd Quarisma
mkdir build && cd build
cmake -DQUARISMA_ENABLE_PROFILER=ON \
      -DUSE_KINETO=ON ..
cmake --build .
```

### Using Kineto with Quarisma

```cpp
#include "profiler/session/profiler.h"
#include "profiler/kineto_shim.h"

void kineto_profiling_example() {
    // Initialize Kineto
    quarisma::profiler_impl::kineto_init(false, true);

    // Prepare trace
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);
    activities.insert(libkineto::ActivityType::CUDA_RUNTIME);

    quarisma::profiler_impl::kineto_prepare_trace(activities);
    quarisma::profiler_impl::kineto_start_trace();

    // Start Quarisma profiler
    auto session = profiler_session_builder()
        .with_timing(true)
        .build();

    session->start();

    {
        QUARISMA_PROFILE_SCOPE("kineto_workload");
        // Your code here
    }

    // Stop both profilers
    session->stop();

    std::unique_ptr<libkineto::ActivityTraceInterface> trace(
        static_cast<libkineto::ActivityTraceInterface*>(
            quarisma::profiler_impl::kineto_stop_trace()));

    // Export Quarisma trace
    session->write_chrome_trace("quarisma_trace.json");

    // Export Kineto trace
    if (trace) {
        trace->save("kineto_trace.json");
    }
}
```

---

## Best Practices

### 1. Minimize Profiling Overhead

```cpp
// ✓ Good: Profile coarse-grained operations
{
    QUARISMA_PROFILE_SCOPE("matrix_multiply");
    result = multiply_large_matrices(a, b);
}

// ✗ Bad: Profile fine-grained operations
for (int i = 0; i < 1000000; ++i) {
    QUARISMA_PROFILE_SCOPE("single_iteration");  // Too much overhead!
    // ...
}
```

### 2. Use Hierarchical Profiling

```cpp
// ✓ Good: Hierarchical structure
{
    QUARISMA_PROFILE_SCOPE("data_pipeline");

    {
        QUARISMA_PROFILE_SCOPE("load");
        load_data();
    }

    {
        QUARISMA_PROFILE_SCOPE("process");
        process_data();
    }

    {
        QUARISMA_PROFILE_SCOPE("save");
        save_data();
    }
}
```

### 3. Enable Features Selectively

```cpp
// For development: Full profiling
auto dev_session = profiler_session_builder()
    .with_timing(true)
    .with_memory_tracking(true)
    .with_hierarchical_profiling(true)
    .with_statistical_analysis(true)
    .build();

// For production: Minimal profiling
auto prod_session = profiler_session_builder()
    .with_timing(true)
    .with_hierarchical_profiling(false)
    .with_statistical_analysis(false)
    .build();
```

### 4. Use Appropriate Output Formats

```cpp
// For debugging: Console output
session->print_report();

// For analysis: JSON export
session->export_report("profile.json");

// For visualization: Chrome Trace
session->write_chrome_trace("trace.json");

// For spreadsheets: CSV export
auto report = session->generate_report();
report->export_csv_report("profile.csv");
```

### 5. Profile in Release Builds

```bash
# Build in Release mode for accurate profiling
cmake -DCMAKE_BUILD_TYPE=Release \
      -DQUARISMA_ENABLE_PROFILER=ON ..
```

---

## Troubleshooting

### Issue: No profiling data captured

**Solution:**
```cpp
// Ensure profiler is started before profiling
session->start();  // Must call this!

{
    QUARISMA_PROFILE_SCOPE("my_operation");
    // ...
}

session->stop();  // Must call this!
```

### Issue: Chrome Trace file is empty

**Solution:**
```cpp
// Ensure hierarchical profiling is enabled
auto session = profiler_session_builder()
    .with_timing(true)
    .with_hierarchical_profiling(true)  // Required for Chrome Trace!
    .build();
```

### Issue: High profiling overhead

**Solution:**
```cpp
// Reduce profiling granularity
// Profile larger scopes, not individual operations

// Disable expensive features
auto session = profiler_session_builder()
    .with_timing(true)
    .with_memory_tracking(false)  // Disable if not needed
    .with_statistical_analysis(false)  // Disable if not needed
    .build();
```

### Issue: Memory tracking not working

**Solution:**
```cpp
// Ensure memory tracking is enabled
auto session = profiler_session_builder()
    .with_timing(true)
    .with_memory_tracking(true)  // Must enable!
    .with_peak_memory_tracking(true)
    .build();
```

### Issue: Thread information missing

**Solution:**
```cpp
// Enable thread safety
auto session = profiler_session_builder()
    .with_timing(true)
    .with_thread_safety(true)  // Required for multi-threaded profiling
    .build();

// Use report builder to include thread info
auto report = profiler_report_builder(*session)
    .include_thread_info(true)
    .build();
```

---

## Summary

The Quarisma Profiler System provides comprehensive performance analysis capabilities with:

- **Multiple output formats**: Console, JSON, CSV, XML, Chrome Trace
- **Flexible display options**: Print to console, export to files, visualize in browsers
- **Rich profiling data**: Timing, memory, hierarchical scopes, statistical analysis
- **Integration options**: Intel ITT API, Quarisma Kineto
- **Low overhead**: Suitable for production use
- **Easy to use**: Simple macros and builder patterns

For more information, see:
- `Library/Core/profiler/` - Source code
- `Library/Core/profiler/README.md` - Implementation details
- `Examples/Profiling/` - Complete examples
- `Library/Core/Testing/Cxx/TestEnhancedProfiler.cpp` - Test cases

