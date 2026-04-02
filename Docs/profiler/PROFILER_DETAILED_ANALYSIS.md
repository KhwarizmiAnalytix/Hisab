# Quarisma Profiler: Detailed Technical Analysis

## Architecture Overview

### Three-Layer Design

```
┌─────────────────────────────────────────────────────┐
│ Layer 1: User API (MISSING)                         │
│ - record_function("name")                           │
│ - profiler_guard                                    │
│ - PROFILER_PROFILE_SCOPE (partially implemented)      │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ Layer 2: Instrumentation (INCOMPLETE)               │
│ - RecordFunction (callback system) ✅               │
│ - Event collection (MISSING)                        │
│ - Callback registration (MISSING)                   │
│ - profiler_session integration (MISSING)            │
└─────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────┐
│ Layer 3: Backends (COMPLETE)                        │
│ - Kineto profiler ✅                                │
│ - ITT integration ✅                                │
│ - NVTX support ✅                                   │
│ - Native profiler ✅                                │
└─────────────────────────────────────────────────────┘
```

---

## RecordFunction Deep Dive

### What It Does
- **Callback mechanism** for profiling events
- **Thread-local & global** callback registration
- **Sampling support** with geometric distribution
- **Observer context** passing between start/end callbacks

### What It Doesn't Do
- **Doesn't capture events** - Callbacks run but events aren't stored
- **Doesn't integrate with profiler_session** - Operates independently
- **Doesn't provide high-level API** - Users must create RecordFunction manually
- **Doesn't manage lifecycle** - No automatic start/stop with session

### Key Classes
```cpp
RecordFunction              // Main profiling guard
RecordFunctionCallback      // Callback pair (start/end)
StepCallbacks              // Active callbacks for a scope
ObserverContext            // Context passed to callbacks
RecordFunctionTLS          // Thread-local state
```

---

## profiler_session Deep Dive

### What It Does
- **Session lifecycle** management (start/stop)
- **Hierarchical scope** tracking
- **Memory tracking** integration
- **Statistical analysis** of measurements
- **Report generation** (JSON, CSV, XML, Console)

### What It Doesn't Do
- **Doesn't hook into RecordFunction** - No callback registration
- **Doesn't capture RecordFunction events** - Separate event collection
- **Doesn't provide record_function() API** - Only PROFILER_PROFILE_SCOPE macro
- **Doesn't integrate with Kineto** - Kineto is separate system

### Key Classes
```cpp
profiler_session           // Main session
profiler_scope             // RAII scope (uses profiler_session)
profiler_scope_data        // Scope data structure
profiler_report            // Report generation
memory_tracker             // Memory statistics
statistical_analyzer       // Statistical analysis
```

---

## The Gap: Missing Integration

### Problem 1: No Event Capture from RecordFunction
- `RecordFunction` callbacks execute but events aren't stored
- `profiler_session` doesn't know about `RecordFunction` events
- No way to correlate `RecordFunction` events with session timeline

### Problem 2: No High-Level API
- Users must manually create `RecordFunction` objects
- No `record_function("name")` convenience function
- `PROFILER_PROFILE_SCOPE` macro exists but doesn't use `RecordFunction`

### Problem 3: No Callback Integration
- `profiler_session` doesn't register callbacks with `RecordFunction`
- No automatic event collection when session is active
- Profiler and callback system operate independently

### Problem 4: No Event Structure
- No `profiler_event` class to represent captured events
- No `event_recorder` to collect events from callbacks
- Events from callbacks are lost

---

## What Needs to Be Built

### 1. Event Representation
```cpp
struct profiler_event {
    std::string name;
    uint64_t start_ns;
    uint64_t end_ns;
    uint64_t thread_id;
    RecordScope scope;
    std::vector<IValue> inputs;
    std::vector<IValue> outputs;
};
```

### 2. Event Collection
```cpp
class event_recorder {
    void record_start(const RecordFunction& fn);
    void record_end(const RecordFunction& fn);
    std::vector<profiler_event> get_events();
};
```

### 3. Callback Helpers
```cpp
CallbackHandle register_profiler_callbacks(
    profiler_session* session);
void unregister_profiler_callbacks(CallbackHandle handle);
```

### 4. High-Level API
```cpp
std::unique_ptr<profiler_event> record_function(
    const std::string& name);
```

---

## Implementation Strategy

### Phase 1: Event Infrastructure
- Create `profiler_event` struct
- Create `event_recorder` class
- Add event storage to `profiler_session`

### Phase 2: Callback Integration
- Create callback helper functions
- Register callbacks when session starts
- Unregister callbacks when session stops
- Collect events in `event_recorder`

### Phase 3: High-Level API
- Create `record_function()` wrapper
- Create `profiler_guard` RAII class
- Update macros to use new API

### Phase 4: Testing & Validation
- Unit tests for event capture
- Integration tests with profiler_session
- Performance benchmarks

---

## Key Dependencies

- `RecordFunction` callback system (already exists)
- `profiler_session` lifecycle (already exists)
- Thread-local state management (already exists)
- Memory tracking (already exists)
- Statistical analysis (already exists)

---

## Success Criteria

✅ Users can call `record_function("name")` to instrument code
✅ Events are automatically captured and stored
✅ Events integrate with profiler_session timeline
✅ Reports include RecordFunction events
✅ Thread-safe event collection
✅ Minimal performance overhead

