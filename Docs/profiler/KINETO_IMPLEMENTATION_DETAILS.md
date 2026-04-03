# Kineto Profiler Implementation Details

## Internal Architecture

### State Stack Management

Kineto uses a **stack-based state management** system:

```cpp
// Global thread-local storage
thread_local std::shared_ptr<ProfilerStateBase> profiler_state;

// Push state when enabling
ProfilerStateBase::push(std::make_shared<KinetoThreadLocalState>(config, activities));

// Pop state when disabling
auto state = ProfilerStateBase::pop();
```

**Why Stack-Based?**
- Supports nested profiling sessions
- Automatic cleanup on scope exit
- Thread-safe per-thread storage
- Allows profiler state queries

---

## Event Collection Mechanism

### RecordQueue Architecture

```
RecordQueue (Thread-Safe)
    ↓
Subqueue (Per-Thread)
    ↓
Event Buffer (Lock-Free)
    ↓
KinetoEvent Objects
```

**Key Properties:**
- **Lock-Free Design:** Minimal contention during recording
- **Per-Thread Subqueues:** Each thread has independent buffer
- **Automatic Flushing:** Events transferred to libkineto periodically
- **Memory Efficient:** Reuses buffers, minimal allocations

### Event Recording Flow

```cpp
// 1. Function entry
onFunctionEnter(RecordFunction fn)
    ↓
recordQueue.getSubqueue()->begin_op(fn)
    ↓
Allocate event slot
Store: name, thread_id, timestamp, input_shapes

// 2. Function execution
// ... user code ...

// 3. Function exit
onFunctionExit(ObserverContext ctx)
    ↓
recordQueue.getSubqueue()->end_op()
    ↓
Store: end_timestamp, output_metadata
Mark event complete
```

---

## Clock Conversion

### Approximate Clock to Unix Time

```cpp
struct KinetoThreadLocalState {
    quarisma::ApproximateClockToUnixTimeConverter clockConverter;
};
```

**Why Approximate Clock?**
- **Performance:** Faster than system clock calls
- **Accuracy:** Synchronized with Unix time periodically
- **Consistency:** All events use same clock reference

**Conversion Process:**
1. Record approximate clock value at profiler start
2. Convert to Unix time using calibration
3. Apply offset to all subsequent events
4. Ensures events align with system timeline

---

## libkineto Integration Points

### Trace Lifecycle

```
enableProfiler()
    ↓
libkineto::api().activityProfiler().prepareTrace(activities)
    ↓
libkineto::api().activityProfiler().startTrace()
    ↓
[Profiling Active - Events Collected]
    ↓
disableProfiler()
    ↓
libkineto::api().activityProfiler().stopTrace()
    ↓
Returns libkineto::ActivityTraceInterface
    ↓
Wrapped in ActivityTraceWrapper
    ↓
Returned to user
```

### Correlation IDs

**Purpose:** Link CPU operations to GPU kernels

```cpp
// When launching GPU kernel
pushCorrelationId(correlation_id);
// ... GPU kernel launch ...
popCorrelationId();

// In trace:
// CPU Event: correlation_id = 12345
// GPU Event: correlation_id = 12345
// → Events are linked
```

---

## Callback Registration

### Profiling Callbacks

```cpp
template <bool global>
void pushProfilingCallbacks(const std::unordered_set<RecordScope>& scopes)
{
    // Register callbacks for each scope
    for (auto scope : scopes) {
        at::addThreadLocalCallback(
            at::RecordFunctionCallback(
                onFunctionEnter<global>,
                onFunctionExit<global>
            ).scopes(scope)
        );
    }
}
```

**Callback Lifecycle:**
1. `enableProfiler()` registers callbacks
2. Callbacks intercept all `RecordFunction` calls
3. Events recorded in `RecordQueue`
4. `disableProfiler()` removes callbacks
5. Callbacks no longer invoked

---

## Memory Management

### Event Storage

```cpp
struct KinetoThreadLocalState {
    std::vector<KinetoEvent> kinetoEvents;           // ~100 bytes each
    std::vector<experimental_event_t> eventTree;     // Hierarchical
    RecordQueue recordQueue;                         // Lock-free buffer
};
```

**Memory Optimization:**
- **Lazy Allocation:** Events allocated on-demand
- **Vector Reuse:** Pre-allocated capacity
- **Shared Pointers:** Efficient event tree sharing
- **Move Semantics:** Avoid copies during finalization

### Trace Finalization

```cpp
std::unique_ptr<ProfilerResult> disableProfiler()
{
    // 1. Stop collection
    auto trace = libkineto::api().activityProfiler().stopTrace();
    
    // 2. Convert events
    auto kineto_events = convertToKinetoEvents(recordQueue);
    
    // 3. Build hierarchy
    auto event_tree = buildEventTree(kineto_events);
    
    // 4. Create result (move semantics)
    return std::make_unique<ProfilerResult>(
        trace_start_ns,
        std::move(kineto_events),
        std::move(trace),
        std::move(event_tree)
    );
}
```

---

## Thread Safety Guarantees

### Per-Thread Isolation

```cpp
// Each thread has independent state
thread_local std::shared_ptr<ProfilerStateBase> profiler_state;

// No synchronization needed for:
// - Event recording
// - RecordQueue operations
// - Clock conversion
```

### Global Profiling Mode

```cpp
// When global=true:
// - Main thread profiler state shared with child threads
// - Child threads access via shared_ptr
// - Minimal synchronization (only state access)
```

### Synchronization Points

```cpp
// Only synchronized operations:
1. ProfilerStateBase::push() - Mutex protected
2. ProfilerStateBase::pop() - Mutex protected
3. Callback registration - Atomic operations
4. profiler_state_info_ptr access - Atomic pointer
```

---

## Post-Processing Callbacks

### Event Enrichment

```cpp
using post_process_t = std::function<void(
    int64_t debug_handle,
    std::vector<std::string>& jit_stack,
    std::vector<std::string>& jit_modules
)>;

void enableProfilerWithEventPostProcess(
    const ProfilerConfig& config,
    const std::set<ActivityType>& activities,
    post_process_t&& cb,
    const std::unordered_set<RecordScope>& scopes = {});
```

**Use Cases:**
- Populate stack traces from debug handles
- Resolve module hierarchy
- Add custom metadata
- Correlate with external data

**Execution:**
```cpp
// After profiling stops, for each event:
post_process_cb(
    event.debug_handle,
    event.jit_stack,
    event.jit_modules
);
```

---

## Error Handling

### No Exceptions Policy

All error handling uses return values:

```cpp
// ✅ Correct
if (!enableProfiler(...)) {
    // Handle error
    return false;
}

// ❌ Incorrect (not used)
try {
    enableProfiler(...);
} catch (...) {
    // ...
}
```

### Validation Checks

```cpp
// enableProfiler validates:
PROFILER_CHECK(
    KinetoThreadLocalState::get(config.global()) == nullptr,
    "Profiler is already enabled");

PROFILER_CHECK(!activities.empty(), "No activities specified");

PROFILER_CHECK(
    has_cpu || !config.global(),
    "Ondemand profiling must enable CPU tracing");
```

---

## Performance Considerations

### Overhead Minimization

1. **Lock-Free RecordQueue:** Minimal contention
2. **Approximate Clock:** Faster than system calls
3. **Lazy Event Conversion:** Only on disableProfiler()
4. **Per-Thread State:** No global synchronization
5. **Callback Filtering:** Only record specified scopes

### Profiling Overhead

- **CPU Profiling:** ~5-10% overhead
- **Memory Profiling:** +5-15% overhead
- **Stack Capture:** +10-20% overhead
- **GPU Profiling:** Minimal (GPU-side collection)

### Optimization Tips

```cpp
// Minimize overhead:
ProfilerConfig config(
    ProfilerState::KINETO,
    false,  // Don't capture shapes
    false,  // Don't profile memory
    false,  // Don't capture stacks
    false,  // Don't calculate FLOPs
    false   // Don't track modules
);

// Only enable what you need
std::set<ActivityType> activities{ActivityType::CPU};
```

---

## Debugging and Diagnostics

### Profiler State Queries

```cpp
// Check if profiler is active
bool enabled = quarisma::profiler_impl::impl::profilerEnabled();

// Get current profiler type
auto type = quarisma::profiler_impl::impl::profilerType();

// Get current configuration
auto config = quarisma::profiler_impl::impl::getProfilerConfig();
```

### Logging

```cpp
// Enable verbose logging
ExperimentalConfig exp_config;
exp_config.verbose = true;

ProfilerConfig config(
    ProfilerState::KINETO,
    false, false, false, false, false,
    exp_config  // Verbose logging enabled
);
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "Profiler already enabled" | Nested enableProfiler() | Call disableProfiler() first |
| No GPU events | CUPTI not available | Use KINETO_GPU_FALLBACK |
| Missing events | Wrong RecordScope | Check scopes parameter |
| High overhead | Too many options enabled | Disable unnecessary features |
| Memory bloat | Long profiling session | Increase buffer size or profile shorter duration |

---

## Extension Points

### Custom Backends (PrivateUse1)

```cpp
// Register custom backend observer
quarisma::profiler_impl::impl::pushPRIVATEUSE1CallbacksStub(config, scopes);

// Custom backend can:
// - Record custom events
// - Emit markers (NVTX, ITT)
// - Integrate with external tools
```

### Event Post-Processing

```cpp
// Enrich events after collection
enableProfilerWithEventPostProcess(
    config,
    activities,
    [](int64_t handle, auto& stack, auto& modules) {
        // Custom enrichment logic
        stack.push_back("custom_frame");
    }
);
```

---

## Testing Considerations

### Unit Test Patterns

```cpp
QUARISMATEST(profiler_test, basic_profiling) {
    ProfilerConfig config(ProfilerState::KINETO);
    std::set<ActivityType> activities{ActivityType::CPU};
    
    enableProfiler(config, activities);
    // ... code to profile ...
    auto result = disableProfiler();
    
    EXPECT_TRUE(result);
    EXPECT_GT(result->events().size(), 0);
}
```

### Coverage Requirements

- **Minimum 98% code coverage** for profiler code
- Test all ProfilerState options
- Test all ActivityType combinations
- Test thread-local and global modes
- Test error conditions

---

## Related Components

- **RecordFunction:** Function interception mechanism
- **RecordQueue:** Event collection buffer
- **ActivityTraceWrapper:** libkineto trace wrapper
- **ApproximateClock:** High-performance clock
- **ITT/NVTX Observers:** Alternative profiling backends

