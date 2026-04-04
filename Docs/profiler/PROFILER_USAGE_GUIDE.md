# Quarisma Profiler - Enhanced Metadata Usage Guide

## Overview

The Quarisma profiler now captures comprehensive metadata for all profiled operations. This guide shows how to enable and use the enhanced profiling capabilities.

---

## Quick Start

### Basic Profiling with Enhanced Metadata

```cpp
#include "profiler/kineto/profiler_kineto.h"

// Configure profiler with enhanced metadata
quarisma::profiler_impl::impl::ProfilerConfig config(
    quarisma::profiler_impl::impl::ProfilerState::KINETO
);

// Enable input shape reporting
config.report_input_shapes = true;

// Enable verbose metadata (includes module hierarchy, call stacks)
config.experimental_config.verbose = true;

// Enable activities (CPU, CUDA, etc.)
std::set<quarisma::profiler_impl::impl::ActivityType> activities = {
    quarisma::profiler_impl::impl::ActivityType::CPU,
    quarisma::profiler_impl::impl::ActivityType::CUDA
};

// Start profiling
quarisma::profiler_impl::impl::enableProfiler(
    config,
    activities,
    {} // scopes - empty means all scopes
);

// ... Your code to profile ...

// Stop profiling and get results
auto profiler_result = quarisma::profiler_impl::impl::disableProfiler();

// Save trace to file (Chrome Trace format)
profiler_result->save("trace.json");
```

---

## Advanced Configuration

### 1. Enable Performance Counter Profiling

```cpp
config.experimental_config.performance_events = {
    "cache_misses",
    "cache_references",
    "instructions",
    "cycles",
    "branch_misses"
};
```

**Output in trace:**
```json
{
  "name": "aten::matmul",
  "cache_misses": "12345",
  "instructions": "987654",
  "cycles": "456789"
}
```

### 2. Enable Concrete Input Values

For detailed debugging, capture actual input values (WARNING: can be verbose):

```cpp
// Enable globally
quarisma::profiler_impl::impl::set_record_concrete_inputs_enabled(true);

// Now profiler will capture:
// - Actual tensor values (not just shapes)
// - Input strides
// - Detailed shape information
```

**Output in trace:**
```json
{
  "name": "aten::add",
  "Input Dims": "[[2, 3], [2, 3]]",
  "Input Strides": "[[3, 1], [3, 1]]",
  "Concrete Inputs": "[tensor([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]]), ...]"
}
```

### 3. Enable CUDA Sync Events

Track CUDA synchronization for multi-stream analysis:

```cpp
config.experimental_config.enable_cuda_sync_events = true;
```

### 4. Profile All Threads (On-Demand Mode)

Capture events from all threads, not just the main thread:

```cpp
config.experimental_config.profile_all_threads = true;
```

---

## Metadata Reference

### Metadata Captured for Each TorchOp Event

| Metadata Key | Description | Example Value |
|--------------|-------------|---------------|
| **Basic Info** | | |
| `name` | Operation name | `"aten::conv2d"` |
| `Record function id` | Unique operation ID | `"12345"` |
| **Module & Stack** | | |
| `Module Hierarchy` | Module path | `"model.encoder.layer1.conv1"` |
| `Call stack` | Call stack trace | `"forward;encode;conv_block"` |
| **Input Information** | | |
| `Input Dims` | Input tensor shapes | `"[[128, 256, 14, 14], [256, 256, 3, 3]]"` |
| `Input type` | Input data types | `"['Float', 'Float']"` |
| `Input Strides` | Tensor strides (if enabled) | `"[[50176, 196, 14, 1], ...]"` |
| `Concrete Inputs` | Actual values (if enabled) | `"[tensor([[...]]), ...]"` |
| **Keyword Arguments** | | |
| `stride` | Conv stride | `"[1, 1]"` |
| `padding` | Conv padding | `"[1, 1]"` |
| `groups` | Conv groups | `"1"` |
| (any kwarg) | Custom kwargs | Various |
| **Training Correlation** | | |
| `Fwd thread id` | Forward pass thread | `"140735268359168"` |
| `Sequence number` | Training step | `"42"` |
| **Performance** | | |
| (perf counters) | Hardware counters | Various |

---

## Example Use Cases

### Use Case 1: Debug Shape Mismatches

```cpp
// Enable shape tracking
config.report_input_shapes = true;

// Profile problematic operation
enableProfiler(config, {ActivityType::CPU}, {});

// ... code that crashes due to shape mismatch ...

auto result = disableProfiler();
result->save("debug_shapes.json");

// Open in Chrome Trace Viewer (chrome://tracing)
// Search for the operation that crashes
// Check "Input Dims" metadata to see actual shapes
```

### Use Case 2: Optimize Memory Layout

```cpp
// Enable concrete inputs to see strides
set_record_concrete_inputs_enabled(true);
config.report_input_shapes = true;

// Profile model forward pass
// ...

// Analyze trace to find:
// - Non-contiguous tensors (slow memory access)
// - Unnecessary transposes
// - Inefficient memory layouts
```

### Use Case 3: Correlate Forward and Backward Passes

```cpp
// The profiler automatically tracks sequence numbers
config.report_input_shapes = true;

// During training:
enableProfiler(config, {ActivityType::CPU, ActivityType::CUDA}, {});

// ... training loop ...

auto result = disableProfiler();
result->save("training_trace.json");

// In Chrome Trace Viewer:
// - Find a backward op
// - Check its "Sequence number" and "Fwd thread id"
// - Search for forward op with same sequence number
// - Verify forward/backward pairing is correct
```

### Use Case 4: Profile with Custom Metadata

```cpp
// Add custom metadata to operations
RECORD_FUNCTION_WITH_SCOPE(
    quarisma::RecordScope::USER_SCOPE,
    "my_custom_operation",
    inputs
);

// The operation will appear in trace with:
// - Full call stack
// - Module hierarchy (if in a module)
// - Input shapes
// - All configured metadata
```

---

## Analyzing Profiler Output

### Chrome Trace Viewer (Recommended)

1. Navigate to `chrome://tracing` in Chrome browser
2. Click "Load" and select your `trace.json` file
3. Use keyboard shortcuts:
   - `W` - Zoom in
   - `S` - Zoom out
   - `A` - Pan left
   - `D` - Pan right
   - Click on event to see metadata

### Programmatic Analysis

```cpp
auto result = disableProfiler();

// Access events programmatically
const auto& events = result->events();

for (const auto& event : events)
{
    std::cout << "Event: " << event.name() << "\n";
    std::cout << "  Duration: " << event.duration_us() << " us\n";
    std::cout << "  Thread: " << event.thread_id() << "\n";

    // Access metadata
    // (Implementation depends on KinetoEvent API)
}
```

---

## Performance Considerations

### Overhead by Feature

| Feature | Typical Overhead | When to Use |
|---------|------------------|-------------|
| Basic profiling | 1-3% | Always (production-safe) |
| Input shapes | 2-5% | Development, debugging |
| Module hierarchy | 1-2% | Development |
| Concrete inputs | 10-50% | Debugging only (heavy!) |
| Performance counters | 5-15% | Performance analysis |
| All features | 15-60% | Detailed debugging |

### Recommendations

**Production:**
```cpp
config.report_input_shapes = false;
config.experimental_config.verbose = false;
set_record_concrete_inputs_enabled(false);
config.experimental_config.performance_events.clear();
```

**Development:**
```cpp
config.report_input_shapes = true;
config.experimental_config.verbose = true;
set_record_concrete_inputs_enabled(false);  // Usually not needed
```

**Debugging:**
```cpp
config.report_input_shapes = true;
config.experimental_config.verbose = true;
set_record_concrete_inputs_enabled(true);  // When needed
config.experimental_config.performance_events = {...};  // Optional
```

---

## Troubleshooting

### Issue: No metadata in trace

**Solution:** Ensure you enable the features:
```cpp
config.report_input_shapes = true;
config.experimental_config.verbose = true;
```

### Issue: Trace file is huge

**Solution:** Disable concrete inputs:
```cpp
set_record_concrete_inputs_enabled(false);
```

### Issue: Missing module hierarchy

**Solution:** Ensure your operations are wrapped in modules and verbose mode is enabled:
```cpp
config.experimental_config.verbose = true;
```

### Issue: Performance degradation

**Solution:** Reduce metadata collection:
```cpp
// Minimal profiling
config.report_input_shapes = false;
config.experimental_config.verbose = false;
config.experimental_config.performance_events.clear();
```

---

## API Reference

### Key Functions

```cpp
// Enable profiler
void enableProfiler(
    const ProfilerConfig& config,
    const std::set<ActivityType>& activities,
    const std::unordered_set<RecordScope>& scopes = {}
);

// Disable profiler and get results
std::unique_ptr<ProfilerResult> disableProfiler();

// Enable/disable concrete input recording
void set_record_concrete_inputs_enabled(bool enable);
bool get_record_concrete_inputs_enabled();

// Manual scope recording
RECORD_FUNCTION(fn, inputs, ...);
RECORD_USER_SCOPE(name);
RECORD_USER_SCOPE_WITH_INPUTS(name, inputs);
```

### Configuration Struct

```cpp
struct ProfilerConfig
{
    ProfilerState      state;                 // KINETO, CPU, CUDA, etc.
    bool               report_input_shapes;   // Enable input shape metadata
    bool               profile_memory;        // Enable memory profiling
    bool               with_stack;            // Enable stack traces
    bool               with_flops;            // Enable FLOP counting
    bool               with_modules;          // Enable module tracking
    ExperimentalConfig experimental_config;   // Advanced options
    std::string        trace_id;              // Trace identifier
};

struct ExperimentalConfig
{
    std::vector<std::string> profiler_metrics;           // CUPTI metrics
    bool                     profiler_measure_per_kernel; // Per-kernel metrics
    bool                     verbose;                     // Verbose metadata
    std::vector<std::string> performance_events;         // Perf events
    bool                     enable_cuda_sync_events;    // CUDA sync tracking
    bool                     profile_all_threads;        // Multi-thread mode
    // ... more options
};
```

---

## Examples

See the following examples in the codebase:
- Basic profiling: `Examples/profiler_basic.cpp` (if exists)
- Advanced profiling: `Examples/profiler_advanced.cpp` (if exists)
- Test cases: `Tests/profiler_test.cpp` (if exists)

---

## Further Reading

- [Profiler design document](profiler.md)
- [Kineto Integration](Library/Core/profiler/kineto/README.md)
- [Chrome Trace Format Specification](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview)

---

**Last Updated:** 2025-11-30
**Quarisma Version:** Current (with enhanced metadata)
