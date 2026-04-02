# Quarisma Profiler: Missing Components Specification

## Component 1: profiler_event

**Location**: `Library/Core/profiler/common/profiler_event.h`

**Purpose**: Represents a single profiling event captured from RecordFunction

**Required Members**:
```cpp
struct profiler_event {
    std::string name;                    // Function/scope name
    uint64_t start_ns;                   // Start timestamp (nanoseconds)
    uint64_t end_ns;                     // End timestamp (nanoseconds)
    uint64_t thread_id;                  // Thread that executed event
    RecordScope scope;                   // Scope type (FUNCTION, USER_SCOPE, etc.)
    std::vector<quarisma::IValue> inputs;  // Function inputs (if captured)
    std::vector<quarisma::IValue> outputs; // Function outputs (if captured)
    int64_t sequence_nr;                 // Sequence number for correlation
    RecordFunctionHandle handle;         // Unique event handle
};
```

**Dependencies**: RecordFunction, IValue, RecordScope

---

## Component 2: event_recorder

**Location**: `Library/Core/profiler/common/event_recorder.h`

**Purpose**: Collects profiling events from RecordFunction callbacks

**Required Methods**:
```cpp
class event_recorder {
public:
    // Called from RecordFunction start callback
    std::unique_ptr<ObserverContext> on_function_enter(
        const RecordFunction& fn);
    
    // Called from RecordFunction end callback
    void on_function_exit(
        const RecordFunction& fn, ObserverContext* ctx);
    
    // Get all recorded events
    const std::vector<profiler_event>& get_events() const;
    
    // Clear recorded events
    void clear_events();
    
    // Get event count
    size_t event_count() const;
};
```

**Dependencies**: RecordFunction, profiler_event, ObserverContext

---

## Component 3: callback_helpers

**Location**: `Library/Core/profiler/common/callback_helpers.h`

**Purpose**: Helper functions to register/unregister profiler callbacks

**Required Functions**:
```cpp
// Register callbacks with RecordFunction system
CallbackHandle register_profiler_callbacks(
    event_recorder* recorder,
    const std::unordered_set<RecordScope>& scopes = {});

// Unregister callbacks
void unregister_profiler_callbacks(CallbackHandle handle);

// Create callback pair for event recording
RecordFunctionCallback create_event_recording_callback(
    event_recorder* recorder);
```

**Dependencies**: RecordFunction, event_recorder, CallbackHandle

---

## Component 4: record_function_api

**Location**: `Library/Core/profiler/common/record_function_api.h`

**Purpose**: High-level API for code instrumentation

**Required Functions**:
```cpp
// Simple scope recording
class record_function_scope {
public:
    explicit record_function_scope(const std::string& name);
    ~record_function_scope();
    
    // Get the underlying RecordFunction
    RecordFunction& get_record_function();
};

// Convenience function
std::unique_ptr<record_function_scope> record_function(
    const std::string& name);

// Macro for easy usage
#define PROFILER_RECORD_FUNCTION(name) \
    auto _rf = quarisma::record_function(name)
```

**Dependencies**: RecordFunction, profiler_session

---

## Component 5: profiler_session Integration

**Location**: `Library/Core/profiler/native/session/profiler.h` (MODIFY)

**Changes Needed**:
```cpp
class profiler_session {
private:
    // NEW: Event recorder for RecordFunction events
    std::unique_ptr<event_recorder> event_recorder_;
    
    // NEW: Callback handle for RecordFunction integration
    CallbackHandle record_function_callback_handle_;
    
public:
    // NEW: Get event recorder
    event_recorder& get_event_recorder();
    
    // NEW: Register callbacks when session starts
    void register_record_function_callbacks();
    
    // NEW: Unregister callbacks when session stops
    void unregister_record_function_callbacks();
};
```

**Dependencies**: event_recorder, callback_helpers

---

## Component 6: FunctionSchema Implementation

**Location**: `Library/Core/profiler/common/record_function.h` (MODIFY)

**Current State**: Stub implementation
```cpp
class FunctionSchema {
};
```

**Needed**:
```cpp
class FunctionSchema {
public:
    const std::string& name() const;
    const std::string& overload_name() const;
    const std::vector<Argument>& arguments() const;
    const std::vector<Return>& returns() const;
    std::optional<OperatorName> operator_name() const;
};
```

**Dependencies**: OperatorName, Argument, Return types

---

## Component 7: OperatorName Implementation

**Location**: `Library/Core/profiler/common/record_function.h` (MODIFY)

**Current State**: Stub implementation
```cpp
class OperatorName {
};
```

**Needed**:
```cpp
class OperatorName {
public:
    const std::string& name() const;
    const std::string& overload_name() const;
};
```

---

## Integration Points

### When profiler_session::start() is called:
1. Create event_recorder
2. Register RecordFunction callbacks via callback_helpers
3. Store callback handle for later cleanup

### When profiler_session::stop() is called:
1. Unregister RecordFunction callbacks
2. Collect events from event_recorder
3. Merge events with profiler_scope events
4. Include in final report

### When user calls record_function("name"):
1. Create RecordFunction with USER_SCOPE
2. Register with active profiler_session callbacks
3. Capture start/end events
4. Return RAII guard for automatic cleanup

---

## Thread Safety Requirements

- event_recorder must be thread-safe
- callback_helpers must handle concurrent registration/unregistration
- RecordFunction callbacks are already thread-safe
- profiler_session integration must use existing locks

---

## Performance Considerations

- Minimal overhead when profiling disabled
- Event recording should use lock-free structures where possible
- Callback registration should be O(1)
- Event collection should not block profiler_session

