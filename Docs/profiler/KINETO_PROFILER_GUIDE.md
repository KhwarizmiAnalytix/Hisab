# Quarisma Kineto Profiler Architecture Guide

## Overview

The Kineto profiler is a high-performance profiling system integrated into Quarisma that captures CPU and GPU execution traces. It's built on top of **libkineto** (Quarisma's profiling library) and provides detailed performance insights for debugging and optimization.

---

## Architecture Components

### 1. **Entry Points (Main Functions)**

#### `enableProfiler()`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:834`

```cpp
void enableProfiler(
    const quarisma::profiler_impl::impl::ProfilerConfig& config,
    const std::set<quarisma::profiler_impl::impl::ActivityType>& activities,
    const std::unordered_set<quarisma::RecordScope>& scopes = {});
```

**Purpose:** Initializes and starts profiling session.

**Key Steps:**
1. Validates no profiler is already running
2. Creates `KinetoThreadLocalState` to manage thread-local profiling state
3. Registers profiling callbacks via `pushProfilingCallbacks()`
4. Initializes libkineto trace collection
5. Stores state in thread-local storage for child threads

**Parameters:**
- `config`: Profiler configuration (state, memory profiling, stack capture, etc.)
- `activities`: Set of activities to trace (CPU, CUDA, XPU, etc.)
- `scopes`: RecordFunction scopes to capture (FUNCTION, BACKWARD_FUNCTION, USER_SCOPE, etc.)

---

#### `disableProfiler()`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:915`

```cpp
std::unique_ptr<ProfilerResult> disableProfiler();
```

**Purpose:** Stops profiling and returns collected results.

**Key Steps:**
1. Pops profiler state from thread-local storage
2. Removes registered callbacks
3. Finalizes trace collection via libkineto
4. Converts collected events to `KinetoEvent` objects
5. Returns `ProfilerResult` containing events and trace

**Returns:** `ProfilerResult` with:
- `events()`: Vector of `KinetoEvent` objects
- `event_tree()`: Hierarchical event structure
- `trace_start_ns()`: Trace start timestamp

---

### 2. **Core State Management**

#### `KinetoThreadLocalState`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:390`

```cpp
struct KinetoThreadLocalState : public ProfilerStateBase {
    uint64_t startTime;
    quarisma::ApproximateClockToUnixTimeConverter clockConverter;
    quarisma::profiler_impl::impl::RecordQueue recordQueue;
    std::vector<KinetoEvent> kinetoEvents;
    std::vector<experimental_event_t> eventTree;
    post_process_t eventPostProcessCb;  // Optional post-processing
};
```

**Responsibilities:**
- Maintains thread-local profiling state
- Manages `RecordQueue` for collecting events
- Stores collected `KinetoEvent` objects
- Handles clock conversion (approximate to Unix time)

**Key Methods:**
- `get(bool global)`: Retrieves current thread's profiler state
- `reportMemoryUsage()`: Records memory allocation events
- `reportVulkanEventToProfiler()`: Records Vulkan GPU events

---

### 3. **Event Recording Callbacks**

#### `onFunctionEnter()` / `onFunctionExit()`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:534`

```cpp
template <bool use_global_state_ptr = false>
std::unique_ptr<at::ObserverContext> onFunctionEnter(const at::RecordFunction& fn);
```

**Purpose:** Intercepts function entry/exit for profiling.

**Flow:**
1. `onFunctionEnter()` is called when a recorded function starts
2. Retrieves current `KinetoThreadLocalState`
3. Calls `recordQueue.getSubqueue()->begin_op(fn)` to record operation start
4. Returns `ObserverContext` to track operation
5. `onFunctionExit()` is called when function completes
6. Records operation end time and metadata

---

### 4. **Configuration Classes**

#### `ProfilerConfig`
**Location:** `Library/Core/profiler/pytroch_profiler/observer.h:140`

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

#### `ActivityType` Enum
**Location:** `Library/Core/profiler/pytroch_profiler/observer.h:14`

```cpp
enum class ActivityType {
    CPU = 0,
    XPU,
    CUDA,
    HPU,
    MTIA,
    PrivateUse1,
    NUM_KINETO_ACTIVITIES
};
```

#### `ProfilerState` Enum
**Location:** `Library/Core/profiler/pytroch_profiler/observer.h:32`

```cpp
enum class ProfilerState {
    Disabled = 0,
    CPU,                          // CPU-only
    CUDA,                         // CPU + CUDA
    NVTX,                         // NVIDIA markers only
    ITT,                          // Intel VTune markers
    PRIVATEUSE1,                  // Custom backend markers
    KINETO,                       // Full libkineto
    KINETO_GPU_FALLBACK,          // Fallback when CUPTI unavailable
    KINETO_PRIVATEUSE1_FALLBACK,
    KINETO_ONDEMAND               // On-demand profiling
};
```

---

### 5. **Result Classes**

#### `KinetoEvent`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.h:28`

Represents a single profiled event with:
- `name()`: Operation name
- `startNs()` / `endNs()`: Timestamps in nanoseconds
- `durationNs()`: Event duration
- `deviceType()`: CPU, CUDA, etc.
- `shapes()`: Tensor shapes (if captured)
- `stack()`: Call stack (if captured)
- `correlationId()`: Links CPU and GPU events

#### `ProfilerResult`
**Location:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.h:92`

```cpp
struct ProfilerResult {
    uint64_t trace_start_ns() const;
    const std::vector<KinetoEvent>& events() const;
    const std::vector<experimental_event_t>& event_tree() const;
    void save(const std::string& path);  // Save to JSON/Chrome format
};
```

---

## Execution Flow

### Starting Profiler
```
User Code
  ↓
enableProfiler(config, activities, scopes)
  ↓
Create KinetoThreadLocalState
  ↓
Register RecordFunction callbacks
  ↓
Initialize libkineto trace
  ↓
Profiling Active
```

### During Profiling
```
Function Execution
  ↓
onFunctionEnter() → recordQueue.begin_op()
  ↓
Function Body Executes
  ↓
onFunctionExit() → recordQueue.end_op()
  ↓
Event Stored in RecordQueue
```

### Stopping Profiler
```
disableProfiler()
  ↓
Stop libkineto trace collection
  ↓
Convert RecordQueue events to KinetoEvent objects
  ↓
Finalize ActivityTraceWrapper
  ↓
Return ProfilerResult
  ↓
User can save/analyze results
```

---

## libkineto Integration

### Kineto Shim Layer
**Location:** `Library/Core/profiler/pytroch_profiler/kineto_shim.h`

Provides abstraction over libkineto:
- `prepareTrace()`: Initialize trace collection
- `startTrace()`: Begin recording
- `stopTrace()`: End recording and return `ActivityTraceWrapper`
- `pushCorrelationId()`: Link CPU and GPU events
- `recordThreadInfo()`: Register thread metadata

### Key libkineto Classes
- `libkineto::ActivityProfiler`: Main profiling interface
- `libkineto::CpuTraceBuffer`: Stores CPU events
- `libkineto::ActivityTraceInterface`: Unified trace interface
- `libkineto::GenericTraceActivity`: Individual activity record

---

## Thread Safety

- **Thread-Local State:** Each thread maintains its own `KinetoThreadLocalState`
- **Global vs. Thread-Local:** Profiler can run globally (all threads) or per-thread
- **Child Thread Support:**
  - `enableProfilerInChildThread()`: Join child thread to profiling
  - `disableProfilerInChildThread()`: Remove child thread from profiling
  - `isProfilerEnabledInMainThread()`: Check if main thread is profiling

---

## Output Formats

### Chrome Trace Format (JSON)
```json
{
  "traceEvents": [
    {
      "name": "operation_name",
      "ph": "X",
      "ts": 1234567890,
      "dur": 1000,
      "pid": 12345,
      "tid": 67890
    }
  ]
}
```

### Saved via
```cpp
ProfilerResult result = disableProfiler();
result->save("trace.json");
```

---

## Key Features

✅ **CPU Profiling:** Captures all CPU operations  
✅ **GPU Profiling:** CUDA, XPU, HPU support  
✅ **Memory Tracking:** Allocation/deallocation events  
✅ **Stack Traces:** Optional call stack capture  
✅ **Tensor Metadata:** Shapes, dtypes, concrete inputs  
✅ **Module Hierarchy:** Track Quarisma module structure  
✅ **Correlation IDs:** Link CPU and GPU events  
✅ **Thread-Safe:** Per-thread and global profiling modes  
✅ **Extensible:** Support for custom backends (PrivateUse1)

---

## Usage Example

```cpp
// Configure profiler
quarisma::profiler_impl::impl::ProfilerConfig config(
    quarisma::profiler_impl::impl::ProfilerState::KINETO,
    true,   // report_input_shapes
    true,   // profile_memory
    true,   // with_stack
    false,  // with_flops
    false   // with_modules
);

// Specify activities
std::set<quarisma::profiler_impl::impl::ActivityType> activities{
    quarisma::profiler_impl::impl::ActivityType::CPU
};

// Start profiling
quarisma::autograd::profiler_impl::enableProfiler(config, activities);

// ... code to profile ...

// Stop and get results
auto result = quarisma::autograd::profiler_impl::disableProfiler();

// Save trace
result->save("profile_trace.json");

// Access events
for (const auto& event : result->events()) {
    std::cout << event.name() << ": " 
              << event.durationNs() << " ns\n";
}
```

---

## Related Files

- **Headers:** `profiler_kineto.h`, `observer.h`, `kineto_shim.h`
- **Implementation:** `profiler_kineto.cpp`, `kineto_shim.cpp`
- **Callbacks:** `record_function.h`, `record_function.cpp`
- **Collection:** `collection.h`, `collection.cpp`
- **Events:** `events.h`, `containers.h`

