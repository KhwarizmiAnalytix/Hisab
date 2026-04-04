# Quarisma profiling examples

Runnable C++ sources and `CMakeLists.txt` live in [`Examples/Profiling/`](../../Examples/Profiling/). This page describes how to build, run, and interpret them.

## Overview

Quarisma provides three complementary profiling systems:

1. **Quarisma Native Profiler** - Hierarchical CPU profiling with Chrome Trace JSON export
2. **Kineto Profiler** - Quarisma profiling library for GPU-related CPU operations
3. **ITT Profiler** - Intel Instrumentation and Tracing Technology for VTune integration

Each profiler can be used individually or combined for comprehensive performance analysis.

## Examples

### `example_profiling_basic.cpp`

A complete, runnable example demonstrating all three profiling systems with realistic computational workloads.

**Key Features:**
- Quarisma native profiler with hierarchical scope tracking
- Kineto profiler integration (when available)
- ITT profiler integration for VTune (when available)
- Graceful degradation when profilers are unavailable
- Matrix multiplication and sorting workloads
- Chrome Trace JSON export for visualization

**What You'll Learn:**
- How to configure and start profiler sessions
- How to instrument code with `QUARISMA_PROFILE_SCOPE()` macros
- How to combine multiple profilers for comprehensive analysis
- How to export and visualize profiling data
- Best practices for profiling instrumentation

## Building the Examples

### Using Quarisma Build Scripts

```bash
# Navigate to Scripts directory
cd Scripts

# Build with Ninja + Clang
python setup.py config.build.examples.ninja.clang

# Or build with Visual Studio 2022 (Windows)
python setup.py config.build.examples.vs22
```

### Using CMake Directly

```bash
# Create build directory
mkdir build && cd build

# Configure with examples enabled
cmake -DQUARISMA_ENABLE_EXAMPLES=ON ..

# Build
cmake --build .

# Or build specific example
cmake --build . --target example_profiling_basic
```

### Build Output

The compiled examples will be located in:
```
build_ninja/bin/example_profiling_basic
```

## Running the Examples

### Basic Execution

```bash
# Navigate to build directory
cd build_ninja

# Run the profiling example
./bin/example_profiling_basic
```

### Expected Output

```
============================================
Quarisma Profiling Examples
============================================

=== Example 1: Quarisma Native Profiler ===
✓ Quarisma profiler started
  Matrix multiplication completed (100x100)
  Sorting completed (10000 elements)
✓ Quarisma profiler stopped
✓ Trace saved to: quarisma_native_profile.json

Visualization:
  1. Chrome DevTools: chrome://tracing
  2. Perfetto UI: https://ui.perfetto.dev

=== Example 2: Kineto Profiler ===
✓ Kineto profiler initialized
✓ Combined profiling started (Kineto + Quarisma)
  Workload completed
✓ Combined profiling stopped
✓ Quarisma trace saved to: kineto_quarisma_trace.json
✓ Kineto trace saved to: kineto_only_trace.json

=== Example 3: ITT Profiler ===
✗ ITT not available (VTune not installed)
  Falling back to Quarisma profiler only
✓ Profiling started
  Workload completed
✓ Profiling stopped
✓ Quarisma trace saved to: itt_quarisma_trace.json

============================================
All examples completed!
============================================
```

### Output Files

After running the example, you'll find the following JSON trace files in the build directory:

| File | Size | Description |
|------|------|-------------|
| `quarisma_native_profile.json` | ~2-4 MB | Quarisma native profiler trace (Example 1) |
| `kineto_quarisma_trace.json` | ~2-4 MB | Quarisma trace from Kineto example (Example 2) |
| `kineto_only_trace.json` | ~1 KB | Kineto-only trace (GPU-related CPU ops) |
| `itt_quarisma_trace.json` | ~2-4 MB | Quarisma trace from ITT example (Example 3) |

## Visualizing Profiling Results

### Chrome DevTools (chrome://tracing)

**Best for:** Quick visualization, hierarchical drill-down, event details

**Steps:**
1. Open Chrome browser
2. Navigate to `chrome://tracing`
3. Click **"Load"** button
4. Select a JSON trace file (e.g., `quarisma_native_profile.json`)
5. Explore the timeline:
   - **W/S** - Zoom in/out
   - **A/D** - Pan left/right
   - **Click events** - View details and nested scopes

**What to Look For:**
- Hierarchical scope nesting (parent/child relationships)
- Event durations (how long each operation took)
- Thread information (which thread executed each operation)
- Nested operations (drill down into `matrix_multiply` → `matrix_multiply_computation`)

### Perfetto UI (https://ui.perfetto.dev)

**Best for:** Advanced analysis, SQL queries, custom visualizations

**Steps:**
1. Visit [https://ui.perfetto.dev](https://ui.perfetto.dev)
2. Click **"Open trace file"**
3. Select a JSON trace file
4. Explore the interactive timeline with advanced features:
   - SQL queries on trace data
   - Custom track grouping
   - Detailed statistics
   - Export to various formats

**Advanced Features:**
- Query trace events with SQL
- Analyze performance bottlenecks
- Compare multiple traces
- Generate custom reports

### Intel VTune Profiler (ITT Annotations)

**Best for:** Deep CPU analysis, hardware performance counters, ITT annotations

**Prerequisites:**
- Intel VTune Profiler installed
- ITT profiler available (`QUARISMA_HAS_ITT=1`)

**Steps:**
1. Run the example under VTune:
   ```bash
   vtune -collect hotspots -app ./bin/example_profiling_basic
   ```

2. View results in VTune GUI:
   ```bash
   vtune-gui
   ```

3. Look for ITT annotations in the timeline:
   - `itt_workload`
   - `matrix_computation`
   - Custom task regions

**What to Look For:**
- ITT task annotations in the timeline
- CPU hotspots and bottlenecks
- Hardware performance counters
- Call stacks and source code correlation

## Understanding the Profiling Data

### Hierarchical Scope Structure

The Quarisma profiler captures hierarchical scopes. For example, in the matrix multiplication example:

```
matrix_operations (parent scope)
├─ generate_matrix (nested scope)
├─ generate_matrix (nested scope)
└─ matrix_multiply (nested scope)
   └─ matrix_multiply_computation (deeply nested scope)
```

### Event Properties

Each profiling event in the Chrome Trace JSON contains:

| Property | Description | Example |
|----------|-------------|---------|
| `name` | Scope name | `"matrix_multiply"` |
| `ph` | Event phase | `"X"` (complete event) |
| `ts` | Timestamp (microseconds) | `1234567890` |
| `dur` | Duration (microseconds) | `5432` |
| `pid` | Process ID | `12345` |
| `tid` | Thread ID | `67890` |

### Performance Metrics

When analyzing the traces, look for:

1. **Hotspots** - Operations with the longest duration
2. **Nesting depth** - How many levels of scopes are nested
3. **Thread utilization** - Which threads are active and when
4. **Idle time** - Gaps between operations
5. **Overhead** - Profiling overhead (typically ~100ns per scope)

## Code Examples

### Example 1: Quarisma Native Profiler

```cpp
#include "profiler/session/profiler.h"

void my_function() {
    // Configure profiler
    profiler_options opts;
    opts.enable_timing_ = true;
    opts.output_format_ = profiler_options::output_format_enum::JSON;

    // Start profiling
    profiler_session session(opts);
    session.start();

    // Instrument code
    {
        QUARISMA_PROFILE_SCOPE("my_operation");
        // ... your code ...
    }

    // Stop and export
    session.stop();
    session.write_chrome_trace("my_profile.json");
}
```

### Example 2: Kineto Profiler (Combined with Quarisma)

```cpp
#include "profiler/session/profiler.h"
#include "profiler/kineto_shim.h"

void my_function() {
    // Initialize Kineto
    quarisma::profiler_impl::kineto_init(false, true);
    
    std::set<libkineto::ActivityType> activities;
    activities.insert(libkineto::ActivityType::CPU_OP);
    quarisma::profiler_impl::kineto_prepare_trace(activities);
    quarisma::profiler_impl::kineto_start_trace();

    // Start Quarisma profiler
    profiler_options opts;
    opts.enable_timing_ = true;
    opts.output_format_ = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    // Instrument code
    {
        QUARISMA_PROFILE_SCOPE("my_operation");
        // ... your code ...
    }

    // Stop both profilers
    session.stop();
    auto kineto_trace = quarisma::profiler_impl::kineto_stop_trace();

    // Export traces
    session.write_chrome_trace("quarisma_trace.json");
    if (kineto_trace) {
        static_cast<libkineto::ActivityTraceInterface*>(kineto_trace)->save("kineto_trace.json");
    }
}
```

### Example 3: ITT Profiler (Combined with Quarisma)

```cpp
#include "profiler/session/profiler.h"
#include "profiler/itt_wrapper.h"

void my_function() {
    // Initialize ITT
    quarisma::profiler_impl::itt_init();
    bool const itt_available = (quarisma::profiler_impl::itt_get_domain() != nullptr);

    // Start Quarisma profiler
    profiler_options opts;
    opts.enable_timing_ = true;
    opts.output_format_ = profiler_options::output_format_enum::JSON;

    profiler_session session(opts);
    session.start();

    // Instrument with both ITT and Quarisma
    {
        if (itt_available) {
            quarisma::profiler_impl::itt_range_push("my_operation");
        }
        QUARISMA_PROFILE_SCOPE("my_operation");

        // ... your code ...

        if (itt_available) {
            quarisma::profiler_impl::itt_range_pop();
        }
    }

    // Stop and export
    session.stop();
    session.write_chrome_trace("itt_trace.json");
}
```

## Best Practices

### 1. Scope Naming
- Use descriptive names: `"matrix_multiply"` not `"func1"`
- Include iteration info: `"process_batch_" + std::to_string(i)`
- Keep names consistent across profilers

### 2. Granularity
- Profile at multiple levels (coarse and fine-grained)
- Avoid profiling trivial operations (< 1 microsecond)
- Balance detail vs. overhead (~100ns per scope)

### 3. Graceful Degradation
- Always check profiler availability before use
- Provide fallback to Quarisma profiler when Kineto/ITT unavailable
- Use conditional compilation for optional profilers

### 4. Output Management
- Use descriptive filenames: `"matrix_multiply_profile.json"`
- Include timestamps for multiple runs
- Clean up old trace files to save disk space

### 5. Performance
- Disable profiling in production builds
- Use `profiler_session` RAII for automatic cleanup
- Minimize string allocations in hot paths

## Troubleshooting

### Issue: Empty or small JSON file (< 1 KB)

**Solution:**
- Ensure profiler session is started and stopped correctly
- Check that `QUARISMA_PROFILE_SCOPE` macros are used
- Verify `session.write_chrome_trace()` is called

### Issue: Kineto trace has no events

**Solution:**
- Kineto's `CPU_OP` captures GPU-related operations only
- Use Quarisma profiler for general CPU profiling
- Check that Kineto is properly initialized

### Issue: ITT annotations not visible in VTune

**Solution:**
- Ensure VTune is installed and app is run under VTune
- Check that `itt_get_domain()` returns non-null
- Verify ITT is enabled (`QUARISMA_HAS_ITT=1`)

### Issue: Chrome DevTools shows "Invalid trace format"

**Solution:**
- Ensure JSON file is complete (`session.stop()` called)
- Check file size is > 0 bytes
- Validate JSON syntax: `python3 -m json.tool < trace.json`

## Additional resources

- **Quarisma profiler docs**: [Docs/profiler/README.md](README.md) (index), [Library_Profiler.md](Library_Profiler.md), [profiler.md](profiler.md)
- **Chrome Trace Event Format**: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
- **Perfetto UI**: https://ui.perfetto.dev
- **Intel VTune Profiler**: https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/vtune-profiler.html
- **Quarisma Kineto**: https://github.com/pytorch/kineto

## License

Copyright © 2024 Quarisma Development Team. All rights reserved.

