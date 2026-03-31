# Kineto Profiler Quick Reference

## Entry Point Functions

### `enableProfiler()` - Start Profiling
```cpp
void enableProfiler(
    const quarisma::profiler_impl::impl::ProfilerConfig& config,
    const std::set<quarisma::profiler_impl::impl::ActivityType>& activities,
    const std::unordered_set<quarisma::RecordScope>& scopes = {});
```
**File:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:834`

**What it does:**
- Creates thread-local profiler state
- Registers function entry/exit callbacks
- Initializes libkineto trace collection
- Enables event recording

**Example:**
```cpp
quarisma::profiler_impl::impl::ProfilerConfig config(
    quarisma::profiler_impl::impl::ProfilerState::KINETO);
std::set<quarisma::profiler_impl::impl::ActivityType> activities{
    quarisma::profiler_impl::impl::ActivityType::CPU};
quarisma::autograd::profiler_impl::enableProfiler(config, activities);
```

---

### `disableProfiler()` - Stop Profiling
```cpp
std::unique_ptr<ProfilerResult> disableProfiler();
```
**File:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:915`

**What it does:**
- Stops event recording
- Finalizes trace collection
- Converts events to `KinetoEvent` objects
- Returns results

**Example:**
```cpp
auto result = quarisma::autograd::profiler_impl::disableProfiler();
result->save("trace.json");
```

---

## Configuration Classes

### `ProfilerConfig` - Main Configuration
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

**Common States:**
- `ProfilerState::KINETO` - Full profiling with libkineto
- `ProfilerState::KINETO_GPU_FALLBACK` - CPU + GPU (CUPTI fallback)
- `ProfilerState::KINETO_ONDEMAND` - On-demand profiling
- `ProfilerState::NVTX` - NVIDIA markers only
- `ProfilerState::ITT` - Intel VTune markers

---

### `ActivityType` - What to Profile
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

---

### `RecordScope` - Which Functions to Capture
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

## Result Classes

### `ProfilerResult` - Profiling Output
```cpp
struct ProfilerResult {
    uint64_t trace_start_ns() const;
    const std::vector<KinetoEvent>& events() const;
    const std::vector<experimental_event_t>& event_tree() const;
    void save(const std::string& path);
};
```

**Methods:**
- `trace_start_ns()` - Get trace start time (nanoseconds)
- `events()` - Get all recorded events
- `event_tree()` - Get hierarchical event structure
- `save(path)` - Save to JSON/Chrome trace format

---

### `KinetoEvent` - Individual Event
```cpp
struct KinetoEvent {
    std::string name() const;
    uint64_t startNs() const;
    uint64_t endNs() const;
    uint64_t durationNs() const;
    quarisma::device_enum deviceType() const;
    int deviceIndex() const;
    uint64_t correlationId() const;
    
    // Optional metadata
    bool hasShapes() const;
    const quarisma::array_ref<std::vector<int64_t>> shapes() const;
    bool hasStack() const;
    const quarisma::array_ref<std::string> stack() const;
    std::string backend() const;
};
```

---

## State Management

### `KinetoThreadLocalState` - Thread-Local State
**File:** `Library/Core/profiler/pytroch_profiler/profiler_kineto.cpp:390`

```cpp
struct KinetoThreadLocalState : public ProfilerStateBase {
    uint64_t startTime;
    RecordQueue recordQueue;
    std::vector<KinetoEvent> kinetoEvents;
    std::vector<experimental_event_t> eventTree;
    
    static KinetoThreadLocalState* get(bool global);
    void reportMemoryUsage(...);
    void reportVulkanEventToProfiler(vulkan_id_t id);
};
```

**Key Methods:**
- `get(bool global)` - Get current thread's profiler state
- `reportMemoryUsage()` - Record memory allocation
- `reportVulkanEventToProfiler()` - Record GPU event

---

## Callback Functions

### `onFunctionEnter()` - Function Entry Hook
```cpp
template <bool use_global_state_ptr = false>
std::unique_ptr<at::ObserverContext> onFunctionEnter(
    const at::RecordFunction& fn);
```

**Called when:**
- A recorded function starts execution
- Captures function name, arguments, metadata

**Returns:** `ObserverContext` to track operation

---

### `onFunctionExit()` - Function Exit Hook
```cpp
void onFunctionExit(at::ObserverContext* ctx);
```

**Called when:**
- A recorded function completes
- Records end time and final metadata

---

## Thread Support Functions

### Multi-Thread Profiling
```cpp
// Check if main thread is profiling
bool isProfilerEnabledInMainThread();

// Enable profiling in child thread
void enableProfilerInChildThread();

// Disable profiling in child thread
void disableProfilerInChildThread();
```

**Use Case:** When child threads need to participate in profiling started by main thread.

---

## libkineto Integration

### Kineto Shim Functions
**File:** `Library/Core/profiler/pytroch_profiler/kineto_shim.h`

```cpp
// Prepare trace collection
void prepareTrace(
    bool cpuOnly,
    const ActivitySet& activities,
    const ExperimentalConfig& config,
    const std::string& trace_id = "");

// Start/stop trace
void startTrace();
ActivityTraceWrapper stopTrace();

// Correlation IDs (link CPU and GPU events)
void pushCorrelationId(uint64_t correlation_id);
void popCorrelationId();

// Thread metadata
void recordThreadInfo();
```

---

## Common Patterns

### Basic Profiling
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

// 4. Run code to profile
// ... your code ...

// 5. Stop and save
auto result = quarisma::autograd::profiler_impl::disableProfiler();
result->save("profile.json");
```

### Accessing Events
```cpp
auto result = quarisma::autograd::profiler_impl::disableProfiler();
for (const auto& event : result->events()) {
    std::cout << "Event: " << event.name() << "\n"
              << "  Duration: " << event.durationNs() << " ns\n"
              << "  Device: " << static_cast<int>(event.deviceType()) << "\n";
}
```

### Memory Profiling
```cpp
quarisma::profiler_impl::impl::ProfilerConfig config(
    quarisma::profiler_impl::impl::ProfilerState::KINETO,
    false,  // report_input_shapes
    true,   // profile_memory ← Enable memory tracking
    false,  // with_stack
    false,  // with_flops
    false   // with_modules
);
```

---

## File Locations

| Component | File |
|-----------|------|
| Main API | `profiler_kineto.h` |
| Implementation | `profiler_kineto.cpp` |
| Configuration | `observer.h` |
| Events | `events.h`, `containers.h` |
| Callbacks | `record_function.h` |
| Collection | `collection.h` |
| libkineto Shim | `kineto_shim.h`, `kineto_shim.cpp` |
| Client Interface | `kineto_client_interface.h` |

**Base Path:** `Library/Core/profiler/pytroch_profiler/`

---

## Output Format

### Chrome Trace JSON
```json
{
  "traceEvents": [
    {
      "name": "operation_name",
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

Viewable in:
- Chrome DevTools (chrome://tracing)
- Perfetto (ui.perfetto.dev)
- Quarisma TensorBoard plugin

---

## Key Concepts

**Correlation ID:** Links CPU operations to GPU kernels  
**RecordQueue:** Thread-safe event collection buffer  
**ActivityTraceWrapper:** Wraps libkineto trace for saving  
**Thread-Local State:** Each thread has independent profiler state  
**Global vs. Per-Thread:** Global profiling captures all threads; per-thread is isolated  
**Post-Processing:** Optional callback to enrich events after collection

