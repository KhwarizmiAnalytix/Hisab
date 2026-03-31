# Kineto Profiler - Complete Review & Analysis

## Executive Summary

The **Kineto Profiler** is Quarisma's high-performance profiling system built on top of Quarisma's libkineto library. It captures detailed execution traces of CPU and GPU operations, enabling comprehensive performance analysis and optimization.

This document provides a complete review of the Kineto profiler architecture, entry points, classes, and functions required to run the profiler.

---

## What is Kineto?

**Kineto** (from Quarisma) is a production-grade profiling library that:
- Captures CPU and GPU execution traces
- Records function entry/exit events with precise timing
- Collects metadata (tensor shapes, memory allocations, stack traces)
- Exports traces in Chrome Trace JSON format
- Supports multi-threaded profiling
- Provides correlation between CPU and GPU events

---

## Architecture Overview

### Three-Layer Design

```
┌─────────────────────────────────────────┐
│  User API Layer                         │
│  enableProfiler() / disableProfiler()   │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  State Management Layer                 │
│  KinetoThreadLocalState                 │
│  RecordQueue (lock-free event buffer)   │
│  ProfilerConfig                         │
└──────────────┬──────────────────────────┘
               │
┌──────────────▼──────────────────────────┐
│  libkineto Integration Layer            │
│  Kineto Shim (kineto_shim.h)            │
│  ActivityTraceWrapper                   │
│  libkineto::ActivityProfiler            │
└─────────────────────────────────────────┘
```

---

## Entry Points (Main Functions)

### 1. **enableProfiler()** - Start Profiling
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:834`

```cpp
void enableProfiler(
    const quarisma::profiler_impl::impl::ProfilerConfig& config,
    const std::set<quarisma::profiler_impl::impl::ActivityType>& activities,
    const std::unordered_set<quarisma::RecordScope>& scopes = {});
```

**What it does:**
1. Validates no profiler already running
2. Creates `KinetoThreadLocalState` for current thread
3. Initializes `RecordQueue` for event collection
4. Registers `onFunctionEnter()` and `onFunctionExit()` callbacks
5. Initializes libkineto trace collection
6. Stores state for child thread access

**Parameters:**
- `config`: Profiler settings (state, memory tracking, stack capture, etc.)
- `activities`: What to profile (CPU, CUDA, XPU, etc.)
- `scopes`: Which function types to capture (FUNCTION, BACKWARD_FUNCTION, USER_SCOPE, etc.)

---

### 2. **disableProfiler()** - Stop Profiling
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:915`

```cpp
std::unique_ptr<ProfilerResult> disableProfiler();
```

**What it does:**
1. Removes registered callbacks
2. Stops libkineto trace collection
3. Finalizes `ActivityTraceWrapper`
4. Converts `RecordQueue` events to `KinetoEvent` objects
5. Builds event hierarchy
6. Returns `ProfilerResult` with all collected data

**Returns:** `ProfilerResult` containing:
- All recorded events as `KinetoEvent` objects
- Event hierarchy tree
- Trace start timestamp
- libkineto trace wrapper

---

### 3. **Thread Support Functions**

```cpp
// Check if main thread is profiling
bool isProfilerEnabledInMainThread();

// Enable profiling in child thread
void enableProfilerInChildThread();

// Disable profiling in child thread
void disableProfilerInChildThread();
```

---

## Core Classes

### **KinetoThreadLocalState**
**Location:** `profiler_kineto.cpp:390`

Manages thread-local profiling state:
- Inherits from `ProfilerStateBase`
- Holds `RecordQueue` for event collection
- Stores `KinetoEvent` vector
- Maintains clock converter (approximate → Unix time)
- Optional post-processing callback

**Key Methods:**
- `get(bool global)` - Retrieve current thread's state
- `reportMemoryUsage()` - Record memory allocations
- `reportVulkanEventToProfiler()` - Record GPU events

---

### **ProfilerConfig**
**Location:** `observer.h:140`

Configuration structure:
```cpp
struct ProfilerConfig {
    ProfilerState state;              // KINETO, KINETO_GPU_FALLBACK, etc.
    bool report_input_shapes;         // Capture tensor shapes
    bool profile_memory;              // Track memory allocations
    bool with_stack;                  // Capture call stacks
    bool with_flops;                  // Calculate FLOPs
    bool with_modules;                // Track module hierarchy
    ExperimentalConfig experimental_config;
    std::string trace_id;
};
```

---

### **KinetoEvent**
**Location:** `profiler_kineto.h:28`

Represents a single profiled event:
```cpp
struct KinetoEvent {
    std::string name();                    // Operation name
    uint64_t startNs();                    // Start time (ns)
    uint64_t endNs();                      // End time (ns)
    uint64_t durationNs();                 // Duration (ns)
    quarisma::device_enum deviceType();      // CPU, CUDA, etc.
    int deviceIndex();                     // Device ID
    uint64_t correlationId();              // Links CPU↔GPU
    
    // Optional metadata
    bool hasShapes();
    const quarisma::array_ref<std::vector<int64_t>> shapes();
    bool hasStack();
    const quarisma::array_ref<std::string> stack();
    std::string backend();
};
```

---

### **ProfilerResult**
**Location:** `profiler_kineto.h:92`

Final output from profiling:
```cpp
struct ProfilerResult {
    uint64_t trace_start_ns() const;
    const std::vector<KinetoEvent>& events() const;
    const std::vector<experimental_event_t>& event_tree() const;
    void save(const std::string& path);
};
```

---

## Configuration Enums

### **ProfilerState**
```cpp
enum class ProfilerState {
    Disabled = 0,
    CPU,                          // CPU-only profiling
    CUDA,                         // CPU + CUDA events
    NVTX,                         // NVIDIA markers only
    ITT,                          // Intel VTune markers
    PRIVATEUSE1,                  // Custom backend markers
    KINETO,                       // Full libkineto
    KINETO_GPU_FALLBACK,          // Fallback when CUPTI unavailable
    KINETO_PRIVATEUSE1_FALLBACK,
    KINETO_ONDEMAND               // On-demand profiling
};
```

### **ActivityType**
```cpp
enum class ActivityType {
    CPU = 0,           // CPU operations
    XPU,               // XPU kernels
    CUDA,              // CUDA kernels
    HPU,               // HPU kernels
    MTIA,              // MTIA kernels
    PrivateUse1        // Custom backend
};
```

### **RecordScope**
```cpp
enum class RecordScope : uint8_t {
    FUNCTION = 0,              // Quarisma/Quarisma ops
    BACKWARD_FUNCTION,         // Autograd nodes
    TORCHSCRIPT_FUNCTION,      // TorchScript functions
    CUSTOM_CLASS,              // Torchbind classes
    USER_SCOPE,                // User-defined regions
    LITE_INTERPRETER,          // Lite interpreter
    STATIC_RUNTIME_OP,         // Static runtime ops
    STATIC_RUNTIME_MODEL       // Static runtime models
};
```

---

## Event Recording Flow

### During Profiling

```
Function Execution
    ↓
RecordFunction detects function call
    ↓
onFunctionEnter() callback triggered
    ↓
recordQueue.begin_op(function)
    ↓
Event start recorded with:
  - Function name
  - Thread ID
  - Timestamp
  - Input shapes (if enabled)
    ↓
Function body executes
    ↓
onFunctionExit() callback triggered
    ↓
recordQueue.end_op()
    ↓
Event end recorded with:
  - End timestamp
  - Duration
  - Output metadata
    ↓
Event stored in KinetoThreadLocalState
```

---

## libkineto Integration

### Kineto Shim Layer
**Location:** `kineto_shim.h`

Abstraction over libkineto:
- `prepareTrace()` - Initialize trace collection
- `startTrace()` - Begin recording
- `stopTrace()` - End recording, return `ActivityTraceWrapper`
- `pushCorrelationId()` - Link CPU and GPU events
- `recordThreadInfo()` - Register thread metadata

### Key libkineto Classes
- `libkineto::ActivityProfiler` - Main profiling interface
- `libkineto::CpuTraceBuffer` - Stores CPU events
- `libkineto::ActivityTraceInterface` - Unified trace interface
- `libkineto::GenericTraceActivity` - Individual activity record

---

## Output Format

### Chrome Trace JSON
```json
{
  "traceEvents": [
    {
      "name": "aten::add",
      "ph": "X",
      "ts": 1234567890,
      "dur": 1000,
      "pid": 12345,
      "tid": 67890,
      "args": {
        "shapes": "[[1, 2, 3]]",
        "backend": "CPU"
      }
    }
  ]
}
```

**Viewable in:**
- Chrome DevTools (chrome://tracing)
- Perfetto (ui.perfetto.dev)
- Quarisma TensorBoard plugin

---

## Key Features

✅ **CPU Profiling** - All CPU operations  
✅ **GPU Profiling** - CUDA, XPU, HPU support  
✅ **Memory Tracking** - Allocation/deallocation events  
✅ **Stack Traces** - Optional call stack capture  
✅ **Tensor Metadata** - Shapes, dtypes, concrete inputs  
✅ **Module Hierarchy** - Quarisma module structure  
✅ **Correlation IDs** - Link CPU and GPU events  
✅ **Thread-Safe** - Per-thread and global modes  
✅ **Extensible** - Custom backend support (PrivateUse1)  
✅ **Post-Processing** - Optional event enrichment callbacks

---

## Thread Safety

- **Thread-Local State:** Each thread maintains independent profiler state
- **Global vs. Per-Thread:** Profiler can run globally (all threads) or per-thread
- **No Synchronization Needed:** Event recording is lock-free
- **Child Thread Support:** Threads can join/leave profiling dynamically

---

## File Organization

```
Library/Core/profiler/pytroch_profiler/
├── profiler_kineto.h          # Main API
├── profiler_kineto.cpp        # Implementation
├── observer.h                 # Config classes
├── observer.cpp               # Config implementation
├── record_function.h          # RecordFunction interface
├── record_function.cpp        # RecordFunction implementation
├── kineto_shim.h              # libkineto wrapper
├── kineto_shim.cpp            # libkineto wrapper impl
├── collection.h               # Event collection
├── collection.cpp             # Event collection impl
├── events.h                   # Event structures
├── containers.h               # Container types
├── api.h                      # API aliases
└── kineto_client_interface.h  # Client interface
```

---

## Documentation Suite

Complete documentation available in `Docs/`:

1. **KINETO_README.md** - Overview and quick start
2. **KINETO_ARCHITECTURE_SUMMARY.md** - High-level architecture
3. **KINETO_PROFILER_GUIDE.md** - Detailed component guide
4. **KINETO_QUICK_REFERENCE.md** - API quick reference
5. **KINETO_IMPLEMENTATION_DETAILS.md** - Technical deep dive
6. **KINETO_INDEX.md** - Navigation and index

---

## Quick Start Example

```cpp
// 1. Configure
quarisma::profiler_impl::impl::ProfilerConfig config(
    quarisma::profiler_impl::impl::ProfilerState::KINETO,
    true,   // report_input_shapes
    true,   // profile_memory
    true,   // with_stack
    false,  // with_flops
    false   // with_modules
);

// 2. Set activities
std::set<quarisma::profiler_impl::impl::ActivityType> activities{
    quarisma::profiler_impl::impl::ActivityType::CPU
};

// 3. Start
quarisma::autograd::profiler_impl::enableProfiler(config, activities);

// 4. Run code
// ... your code to profile ...

// 5. Stop and save
auto result = quarisma::autograd::profiler_impl::disableProfiler();
result->save("profile_trace.json");

// 6. Analyze
for (const auto& event : result->events()) {
    std::cout << event.name() << ": " 
              << event.durationNs() << " ns\n";
}
```

---

## Summary

The Kineto Profiler is a sophisticated, production-grade profiling system that:

1. **Captures Events** via callback interception (onFunctionEnter/Exit)
2. **Stores Events** in lock-free RecordQueue for minimal overhead
3. **Manages State** via thread-local KinetoThreadLocalState
4. **Integrates with libkineto** for trace collection and export
5. **Supports Multi-Threading** with global and per-thread modes
6. **Exports Results** in Chrome Trace JSON format
7. **Provides Rich Metadata** including shapes, stacks, and correlations

The architecture is clean, modular, and extensible, making it suitable for both simple profiling tasks and complex performance analysis scenarios.

---

**For detailed information, see the complete documentation suite in `Docs/KINETO_*.md`**

