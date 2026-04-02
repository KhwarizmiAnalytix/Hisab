# Quarisma Profiler Investigation Summary

## Executive Summary

The Quarisma profiler infrastructure has **significant gaps** in the C++ API layer needed for code instrumentation. While the callback/observer framework (`RecordFunction`) exists, the **high-level instrumentation API is incomplete**, making it difficult for users to instrument code for profiling.

---

## Current State: What Exists

### ✅ Core Infrastructure (Present)
1. **RecordFunction** (`Library/Core/profiler/common/record_function.h/cpp`)
   - Low-level callback mechanism for profiling events
   - Supports start/end callbacks with observer contexts
   - Thread-local and global callback registration
   - Sampling support with geometric distribution

2. **Profiler Session** (`Library/Core/profiler/native/session/profiler.h`)
   - Main profiling session management
   - Hierarchical scope tracking
   - Memory tracking integration
   - Statistical analysis
   - Multiple output formats (JSON, CSV, XML, Console)

3. **Profiler Scope** (`Library/Core/profiler/native/session/profiler.h`)
   - RAII-based scope profiling
   - Automatic timing and memory tracking
   - Macros: `PROFILER_PROFILE_SCOPE()`, `PROFILER_PROFILE_FUNCTION()`

4. **Observer/Orchestration** (`Library/Core/profiler/common/orchestration/observer.h`)
   - `ProfilerConfig` - Configuration structure
   - `ProfilerState` enum - Profiler modes (CPU, CUDA, KINETO, ITT, etc.)
   - `ActivityType` enum - Activity types to profile
   - `ProfilerStateBase` - Base class for profiler implementations

5. **Backend Profilers**
   - Kineto integration (`Library/Core/profiler/kineto/`)
   - ITT integration (`Library/Core/profiler/itt/`)
   - NVTX support (`Library/Core/profiler/base/nvtx_observer.h`)

---

## Missing Components: What's Needed

### ❌ 1. High-Level Instrumentation API
**Purpose**: User-friendly API to instrument code without dealing with callbacks

**Missing**:
- `record_function()` wrapper function - Simple API to create profiling scopes
- `profiler_guard` class - RAII guard for profiling regions
- Convenience functions for common profiling patterns
- Integration with `RecordFunction` callback system

**Impact**: Users must use low-level `RecordFunction` directly or rely on macros

---

### ❌ 2. Callback Registration Helpers
**Purpose**: Simplify registering profiling callbacks

**Missing**:
- Helper functions to register callbacks with profilers
- Callback factory functions for common observer types
- Integration between `profiler_session` and `RecordFunction` callbacks
- Thread-local callback management utilities

**Impact**: Profiler session doesn't automatically hook into `RecordFunction` events

---

### ❌ 3. Event Collection & Recording
**Purpose**: Capture profiling events from callbacks

**Missing**:
- `profiler_event` class - Represents a single profiling event
- `event_recorder` class - Collects events from callbacks
- Integration with `RecordFunction` to capture function entry/exit
- Event filtering and aggregation logic

**Impact**: Events from `RecordFunction` callbacks aren't captured by profiler session

---

### ❌ 4. Profiler Integration Layer
**Purpose**: Connect profiler session with callback system

**Missing**:
- Automatic callback registration when session starts
- Callback cleanup when session stops
- Thread-safe event collection from callbacks
- Synchronization between session state and callback state

**Impact**: Profiler session and `RecordFunction` operate independently

---

### ❌ 5. Function Schema Support
**Purpose**: Capture function metadata (names, arguments, types)

**Missing**:
- `FunctionSchema` implementation (currently stub)
- `OperatorName` implementation (currently stub)
- Schema extraction from function descriptors
- Argument/return type capture

**Impact**: Limited metadata available in profiling events

---

## How `record_function` Should Work

### Current Flow (Incomplete)
```cpp
// User creates RecordFunction manually
quarisma::RecordFunction guard(quarisma::RecordScope::FUNCTION);
guard.before("my_function");
// ... code ...
guard.end();  // Destructor calls end callbacks
```

### Desired Flow (Missing)
```cpp
// User calls high-level API
auto event = quarisma::record_function("my_function");
// ... code ...
// Event automatically recorded when scope exits
```

### What's Missing
1. **Wrapper function** that creates `RecordFunction` with proper callbacks
2. **Callback registration** that connects to profiler session
3. **Event capture** from callback start/end
4. **Integration** with profiler session's event collection

---

## Dependencies & Relationships

```
profiler_session (session management)
    ↓
    ├─→ profiler_scope (RAII scopes) ✅
    ├─→ memory_tracker ✅
    ├─→ statistical_analyzer ✅
    └─→ [MISSING] callback registration
            ↓
            └─→ RecordFunction (callback system) ✅
                    ↓
                    ├─→ [MISSING] event_recorder
                    ├─→ [MISSING] profiler_event
                    └─→ [MISSING] callback_helpers
```

---

## Recommended Implementation Order

1. **Create `profiler_event` class** - Represents captured events
2. **Create `event_recorder`** - Collects events from callbacks
3. **Create callback helpers** - Register callbacks with profiler
4. **Implement `record_function()` API** - High-level instrumentation
5. **Add integration layer** - Connect session to callbacks
6. **Implement function schema** - Capture metadata

---

## Files to Create/Modify

- `Library/Core/profiler/common/profiler_event.h/cpp` (NEW)
- `Library/Core/profiler/common/event_recorder.h/cpp` (NEW)
- `Library/Core/profiler/common/callback_helpers.h/cpp` (NEW)
- `Library/Core/profiler/common/record_function_api.h/cpp` (NEW)
- `Library/Core/profiler/native/session/profiler.h` (MODIFY - add callback integration)
- `Library/Core/profiler/common/record_function.h` (MODIFY - add schema support)

---

## Conclusion

The profiler has strong **backend infrastructure** but lacks the **C++ API layer** for practical code instrumentation. The gap is between low-level `RecordFunction` callbacks and high-level `profiler_session` usage. Implementing the missing components will enable seamless profiling instrumentation.

