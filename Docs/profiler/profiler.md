# XSigma Profiler

User guide, API, backends, examples, and architecture for
[`Library/Profiler`](../../Library/Profiler/). Link `Quarisma::Profiler`
(CMake) or `//Library/Profiler:Profiler` (Bazel). Include root is
`Library/Profiler`.

C++ only — there are no Python or C bindings in this repository.

## Table of contents

1. [Overview](#overview)
2. [Backends and build](#backends-and-build)
3. [Architecture](#architecture)
4. [Instrumentation](#instrumentation)
5. [Native session API](#native-session-api)
6. [Kineto](#kineto)
7. [ITT and NVTX](#itt-and-nvtx)
8. [GPU / CUDA](#gpu--cuda)
9. [Runnable examples](#runnable-examples)
10. [Output formats](#output-formats)
11. [Hotspot report](#hotspot-report)
12. [Console table columns](#console-table-columns)
13. [Macros and feature flags](#macros-and-feature-flags)
14. [Dependencies](#dependencies)
15. [Best practices](#best-practices)
16. [Troubleshooting](#troubleshooting)
17. [Open follow-ups](#open-follow-ups)

---

## Overview

XSigma’s profiler is a modular performance-analysis library:

| Capability | Notes |
|---|---|
| Timing | Nanosecond precision (`std::chrono::high_resolution_clock` / approximate clock) |
| Memory | Allocation/deallocation tracking and peak usage |
| Hierarchy | Nested scopes with parent/child timing |
| Threads | Per-thread collection; lock-free hot path where possible |
| Statistics | Min / max / mean / stddev / percentiles (25, 50, 75, 90, 95, 99) |
| Export | Console, JSON, CSV, XML, Chrome Trace, XPlane |
| Overhead | Designed for ~100 ns per scope, ~1 KB per active scope |
| ITT | Intel VTune range annotations (ITT backend) |
| Kineto | PyTorch-style CPU (+ optional CUDA/CUPTI) activity tracing |

The native (XPlane/TraceMe) pipeline is **always compiled** — it is not a
backend choice. `PROFILER_BACKEND` selects the *instrumentation* backend
layered on top of it, and Kineto/ITT remain mutually exclusive with each
other (only one instrumentation backend at a time). NVTX is not a fourth
backend — it is a `ProfilerState` inside the Kineto/ITT orchestration layer.

| Backend | CMake | Macro | Sources | Third-party |
|---|---|---|---|---|
| Native / XPlane (always on) | n/a — not selectable | `PROFILER_HAS_NATIVE` (always `1`) | `native/` (+ `common/`) | none |
| Kineto (default instrumentation) | `PROFILER_BACKEND=KINETO` | `PROFILER_HAS_KINETO` | `bespoke/` | `ThirdParty/kineto` |
| ITT | `PROFILER_BACKEND=ITT` | `PROFILER_HAS_ITT` | `bespoke/` (minus Python/JIT-only) | `ThirdParty/ittapi` |
| NVTX | not independently selectable | n/a | `bespoke/base/nvtx_observer.cpp` | CUDA `nvToolsExt` (optional) |

Enabling both `PROFILER_HAS_KINETO` and `PROFILER_HAS_ITT` at once is a
configure-time error and a compile-time `#error` in
`common/profiler_export.h`; `PROFILER_HAS_NATIVE` is excluded from that
check since it's unconditional. [Phase 0](#target-architecture) of the
multi-backend plan — dropping the old native/Kineto/ITT three-way
mutual-exclusion so native compiles alongside whichever instrumentation
backend is selected — is done.

---

## Backends and build

Use [`Scripts/setup.py`](../../Scripts/setup.py) — do not invoke CMake/ninja
directly. The `--profiler.X` flag is a **separate argument**, not a dotted
token in the main chain (`profiler.itt` chained into `config.build.test…`
does not work).

### Native (always compiled)

`native/` plus shared `common/` are always compiled into `Profiler`,
regardless of which instrumentation backend (below) is selected — it is not
something you opt into.

- **Header:** `#include "native/session/profiler.h"`
- **Namespace:** `profiler` (`profiler_session_builder`, `profiler_session`,
  `profiler_scope`). Scope macros: `PROFILER_PROFILE_SCOPE`,
  `PROFILER_PROFILE_FUNCTION`, `PROFILER_PROFILE_BLOCK`

`--profiler.native` / `profiler_native` are accepted as no-ops (with a
warning) for backward compatibility with older invocations — they used to
select the native-only backend; there is nothing left to select.

### Kineto (default)

Sets `PROFILER_HAS_KINETO=1`, links libkineto, compiles `bespoke/kineto` and
related sources.

```bash
cd Scripts
python3 setup.py config.build.test.native.ninja --project.profiler
```

- **CMake:** `-DPROFILER_BACKEND=KINETO` (default)
- **Bazel:** default (no extra config), or `profiler_enable_kineto=true` with
  `profiler_type=kineto`

### ITT

```bash
cd Scripts
python3 setup.py config.build.test.native.ninja --project.profiler --profiler.itt
```

- **CMake:** `-DPROFILER_BACKEND=ITT`
- **Bazel:** `--config=itt` (`profiler_type=itt` + `profiler_enable_itt=true`)

### CMake options (library)

| Variable | Default | Values / notes |
|---|---|---|
| `PROFILER_BACKEND` | `KINETO` | `KINETO`, `ITT` — sets `PROFILER_ENABLE_KINETO`/`PROFILER_ENABLE_ITT`. `PROFILER_ENABLE_NATIVE_PROFILER` is always `ON`, independent of this. |
| `PROFILER_ENABLE_TESTING` | ON | Tests |
| `PROFILER_ENABLE_EXAMPLES` | OFF | `Examples/Profiling` |
| `PROFILER_ENABLE_GTEST` | ON | GoogleTest |
| `PROFILER_ENABLE_BENCHMARK` | ON | Google Benchmark |
| `PROFILER_CXX_STANDARD` | 20 | 11–23 |

Root also exposes `PROFILER_INCLUDE_GATE_ONLY` to gate Kineto before
`add_subdirectory`.

### Bazel

Starlark: [`bazel/profiler.bzl`](../../bazel/profiler.bzl). `native/**` is
globbed in unconditionally in `BUILD.bazel` (not part of any `select`).
Select order for the instrumentation backend: **ITT** → default **Kineto**.

| Mode | Config | Result |
|---|---|---|
| Kineto (default) | *(none)* | `PROFILER_HAS_KINETO=1`, `PROFILER_HAS_NATIVE=1` |
| ITT | `--config=itt` | `PROFILER_HAS_ITT=1`, `PROFILER_HAS_NATIVE=1` |

LTO, coverage, sanitizers, linker/cache, spell, Valgrind are **CMake only**.

### Tests

`ProfilerCxxTests` always includes the native-pipeline tests now (they are
no longer conditionally excluded). Shared TUs that reference `bespoke/*`
must guard includes with `PROFILER_HAS_KINETO` or `PROFILER_HAS_ITT` so
Bazel's include graph matches CMake.

---

## Architecture

```
Application
  RECORD_USER_SCOPE / PROFILER_PROFILE_SCOPE / TraceMe
        │
        ├─ Native session (always compiled; PROFILER_HAS_NATIVE=1)
        │     profiler_session → profiler_scope → host_tracer / TraceMeRecorder
        │     → XPlane → (Chrome Trace | scope_tree_builder reconstruction → report)
        │
        └─ Bespoke orchestration (PROFILER_HAS_KINETO or PROFILER_HAS_ITT, one active)
              RecordFunction callbacks
                    ├─ Kineto  (RecordQueue → Result → ProfilerResult)
                    ├─ ITT     (VTune ranges; empty ProfilerResult)
                    └─ NVTX    (Nsight ranges; empty ProfilerResult)
```

The native session and the Kineto/ITT orchestration layer now build and run
side by side (native is not an alternative to them, it's always present).
`profiler_scope` no longer tracks its own hierarchy tree: it emplaces a real
`traceme` for its lifetime, so `PROFILER_PROFILE_SCOPE` rides the same
lock-free, thread-local `traceme_recorder` path `host_tracer` reads from.
`profiler_session::build_scope_tree()` lazily reconstructs a hierarchy view
over the collected XSpace (nesting events by interval containment) rather
than maintaining a live, mutex-protected tree — hierarchy is a *derived
view*, computed once per collection and cached, matching how the TF/XLA
profiler treats it. `generate_chrome_trace_json()`/`write_chrome_trace()`
delegate to `chrome_trace_exporter` (the same exporter Kineto/ITT XSpace
data would use) instead of a separate hand-rolled JSON writer.

### Native tree (`native/`)

| Directory | Role |
|---|---|
| `native/core/` | Plugin ABI: `profiler_interface`, `profiler_collection`, `profiler_factory`, `profiler_controller`, `profiler_lock` |
| `native/cpu/` | `host_tracer` over `TraceMeRecorder`, `annotation_stack`, stub Python tracer |
| `native/tracing/` | `traceme` / `TraceMeRecorder` (independent of `RECORD_FUNCTION`) |
| `native/exporters/xplane/` | TensorBoard XPlane schema (ported; no TF runtime) |
| `native/exporters/` | `chrome_trace_exporter` |
| `native/session/` | Public RAII API: session, scope, builder, report |
| `native/analysis/` | Stats aggregation |
| `native/memory/` | Session-local alloc annotation (not `Library/Memory`) |

`host_tracer_factory.cpp` registers `host_tracer` at static-init.

### Bespoke tree (`bespoke/`)

Shared orchestration for Kineto, ITT, and NVTX: `ProfilerConfig`,
`ProfilerState` (`KINETO` / `ITT` / `NVTX` / `KINETO_GPU_FALLBACK` /
`KINETO_ONDEMAND`), `ActivityType`, `MemoryReportingInfoBase`.

Kineto call chain:

1. `RECORD_USER_SCOPE` / `RECORD_FUNCTION` (`bespoke/common/record_function.h`)
2. `RecordFunctionCallback` registered by `enableProfiler`
3. `onFunctionEnter` → `ThreadLocalSubqueue::begin_op`; exit stamps end time
   and pops the correlation ID
4. `ProfilerStateBase` TLS stack; `KinetoThreadLocalState` holds a `RecordQueue`
5. `RecordQueue` / `ThreadLocalSubqueue` — per-thread lists; `getRecords()`
   materializes `Result` objects and merges libkineto GPU/runtime activities
6. `Result` + `ExtraFields<EventType::*>` (`TorchOp`, `Allocation`,
   `OutOfMemory`, `Backend`, `Kineto`)
7. Public view: `KinetoEvent` / `ProfilerResult` in
   `profiler::autograd::profiler_impl`
8. Optional: `hotspot_report` over `ProfilerResult::event_tree()`

### Two instrumentation mechanisms (not yet unified)

| Mechanism | Backend | Header |
|---|---|---|
| `PROFILER_PROFILE_SCOPE` / `TraceMe` | Native | `native/session/profiler.h`, `native/tracing/traceme.h` |
| `RECORD_FUNCTION` / `RECORD_USER_SCOPE` | Kineto / ITT | `common/instrumentation.h` (shim) |

Unifying them — wrapping each backend as a `profiler_interface` fed by one
call site — is Phases 3–4 of the [target architecture](#target-architecture).

### Target architecture

```
kernel / op / allocator / thread-pool code
        │
RECORD_FUNCTION / RECORD_USER_SCOPE     ← single instrumentation point
        │
RecordFunction callback dispatch        ← already multi-subscriber
        ├─ Kineto tracer
        ├─ ITT tracer
        ├─ NVTX tracer
        └─ XPlane host tracer (future: device via CUPTI)
              each wrapped as profiler_interface
                    │
              profiler_collection (TF-style multiplexer; exists, unused)
                    │
              merged x_space
                    ├─ chrome_trace (.json)
                    ├─ hotspot_report
                    └─ raw XSpace
```

| Phase | Intent | Status |
|---|---|---|
| 0 | Independent `PROFILER_ENABLE_*` toggles; drop mutual-exclusion `#error` | Done — native always compiles; Kineto/ITT stay mutually exclusive with each other only |
| 1 | Remove dead tests / unused APIs | Mostly done; `TestKinetoShim.cpp` still `#if 0` |
| 2 | Instrument real call sites via `common/instrumentation.h` | Vectorization, Parallel, Memory-Metal done; CUDA/HIP allocators not |
| 3 | Wrap each backend as `profiler_interface` | Not done |
| 4 | One user-facing `profiler_session_builder` selecting activities | Not done |

---

## Instrumentation

Other libraries should include **`common/instrumentation.h`**, not
`bespoke/common/record_function.h` directly. Under Kineto/ITT the macros are
real; under Native they are no-ops, so call sites need no `PROFILER_HAS_*`
guards.

```cpp
#include "common/instrumentation.h"

void gemm(...) {
    RECORD_USER_SCOPE("vectorization::gemm");
    // ...
}

// Allocators (signed size: negative = deallocation)
profiler::report_memory_usage(
    ptr, /*alloc_size*/ static_cast<int64_t>(nbytes),
    total_allocated, total_reserved,
    /*device_type*/ static_cast<int16_t>(profiler::device_enum::cpu),
    /*device_index*/ -1);
```

### Already instrumented

| Library | Site |
|---|---|
| Vectorization | Non-`noexcept` `tensor<T>` assign/construct overloads that funnel through `expressions_evaluator::run`. Scalar-fill `operator=(T2) noexcept` is **not** instrumented (`RecordFunction` is not `noexcept`). |
| Parallel | `parallel::thread_pool::run_job` — the execute boundary, not enqueue |
| Memory | Metal caching allocator `allocate` / `deallocate` via `report_memory_usage` |

CMake/Bazel: `VECTORIZATION_HAS_PROFILER`, `PARALLEL_HAS_PROFILER`,
`MEMORY_ENABLE_PROFILER` / `MEMORY_HAS_PROFILER`. CUDA/HIP caching allocators
are not yet wired.

### RecordFunction macros (Kineto / ITT)

Defined in `bespoke/common/record_function.h` (pulled in by the shim):

```cpp
RECORD_FUNCTION(fn, ...)
RECORD_USER_SCOPE(fn)
RECORD_FUNCTION_WITH_SCOPE(scope, fn)
RECORD_FUNCTION_WITH_INPUTS_OUTPUTS(...)
RECORD_USER_SCOPE_WITH_INPUTS(...)
RECORD_USER_SCOPE_WITH_KWARGS_ONLY(...)
RECORD_WITH_SCOPE_DEBUG_HANDLE_AND_INPUTS(...)
RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_AND_INPUTS(...)
RECORD_OUTPUTS(...)
RECORD_TORCHSCRIPT_FUNCTION(...)   // no-op / unused without TorchScript
```

`RECORD_USER_SCOPE` expands to a local `profiler::RecordFunction` guard. The
end timestamp is recorded in the destructor — wrap the work in `{ }` so the
guard destructs **before** `disableProfiler()` / `disableProfilerInChildThread()`.

```cpp
enableProfilerInChildThread();
{
    RECORD_USER_SCOPE("worker");
    do_work();
}  // guard ends here
disableProfilerInChildThread();
```

### `report_memory_usage`

Mirrors PyTorch’s `c10::reportMemoryUsageToProfiler`. No-op when no session
is active, `profile_memory` was not requested, or the Native backend is
built. Device type is `profiler::device_enum` (CPU, CUDA, HIP, PrivateUse1).

---

## Native session API

Always available (`PROFILER_HAS_NATIVE` is unconditionally `1`). Link `Quarisma::Profiler`.

### Quick start

```cpp
#include "native/session/profiler.h"

using namespace profiler;

int main() {
    auto session = profiler_session_builder()
        .with_timing(true)
        .with_memory_tracking(true)
        .with_hierarchical_profiling(true)
        .with_statistical_analysis(true)
        .with_output_format(profiler_options::output_format_enum::JSON)
        .build();

    if (!session) {
        return 1;
    }
    session->start();

    {
        PROFILER_PROFILE_FUNCTION();
        {
            PROFILER_PROFILE_SCOPE("nested_operation");
            std::vector<int> data(1000, 42);
        }
    }

    session->stop();
    session->print_report();
    session->export_report("profile_results.json");
    return 0;
}
```

### Builder methods

| Method | Purpose |
|---|---|
| `with_timing(bool)` | Timing measurements |
| `with_memory_tracking(bool)` | Allocation tracking |
| `with_hierarchical_profiling(bool)` | Nested scopes |
| `with_statistical_analysis(bool)` | Stats aggregation |
| `with_thread_safety(bool)` | Thread-safe collection |
| `with_output_format(...)` | JSON, CSV, XML, console |
| `with_output_file(string)` | Default export path |
| `with_max_samples(size_t)` | Cap per series |
| `with_percentiles(bool)` | Percentile stats |
| `with_peak_memory_tracking(bool)` | Peak usage |
| `with_memory_deltas(bool)` | Deltas between points |
| `with_thread_pool_size(size_t)` | Worker-pool hint |
| `build()` | Returns `std::unique_ptr<profiler_session>` |

### Session methods

`start()` / `stop()` / `is_active()`, `create_scope(name)`,
`generate_report()` / `export_report(filename)` / `print_report()`,
`write_chrome_trace(path)` where implemented.

### Scope macros

```cpp
PROFILER_PROFILE_SCOPE("scope_name");
PROFILER_PROFILE_FUNCTION();
PROFILER_PROFILE_BLOCK("block_name") {
    // ...
}
```

Prefer these over manual start/stop. They expand to a `profiler_scope` RAII
object.

### TraceMe (Native tracing layer)

`TraceMe` / `traceme` in `native/tracing/` can coexist with session scopes:

```cpp
{
    traceme trace("traceme_scope");
    PROFILER_PROFILE_SCOPE("native_scope");
}
```

On mobile (`IS_MOBILE_PLATFORM`), `TraceMe` methods compile to no-ops.

### Memory tracker / statistical analyzer

```cpp
memory_tracker tracker;
tracker.start_tracking();
void* ptr = malloc(1024);
tracker.track_allocation(ptr, 1024, "custom_allocation");
tracker.track_deallocation(ptr);
free(ptr);
auto mem = tracker.get_current_stats();
tracker.stop_tracking();

statistical_analyzer analyzer;
analyzer.start_analysis();
analyzer.add_timing_sample("my_function", duration_ms);
auto stats = analyzer.calculate_timing_stats("my_function");
analyzer.stop_analysis();
```

### Multi-threaded session

```cpp
auto session = profiler_session_builder()
    .with_thread_safety(true)
    .build();
session->start();

std::vector<std::thread> threads;
for (int i = 0; i < 4; ++i) {
    threads.emplace_back([i]() {
        PROFILER_PROFILE_SCOPE("thread_" + std::to_string(i));
    });
}
for (auto& t : threads) {
    t.join();
}
session->stop();
```

Unit tests: `Library/Profiler/Testing/Cxx/` (for example
`TestEnhancedProfiler.cpp`, `TestProfilerChromeTraceHierarchical.cpp`).

---

## Kineto

Default backend. Public header: `bespoke/kineto/profiler_kineto.h`.
Namespace: `profiler::autograd::profiler_impl` (historical PyTorch name,
kept so existing call sites compile). XSigma does **not** link libtorch.

### Enable / disable

```cpp
#include "bespoke/kineto/profiler_kineto.h"
#include "common/instrumentation.h"

using namespace profiler::autograd::profiler_impl;
using profiler::profiler_impl::impl::ActivityType;
using profiler::profiler_impl::impl::ProfilerConfig;
using profiler::profiler_impl::impl::ProfilerState;

ProfilerConfig config(ProfilerState::KINETO);
config.profile_memory = true;

std::set<ActivityType> activities{ActivityType::CPU};
enableProfiler(config, activities);
{
    RECORD_USER_SCOPE("matmul");
    // work
}
auto result = disableProfiler();
result->save("trace.json");
```

| Function | Role |
|---|---|
| `prepareProfiler(config, activities)` | Prepare libkineto before start |
| `enableProfiler(config, activities, scopes = {})` | Push TLS state, register callbacks, start trace |
| `enableProfilerWithEventPostProcess(..., cb, scopes)` | Same, plus lazy event enrichment |
| `disableProfiler()` | Stop, materialize `KinetoEvent`s, return `ProfilerResult` |
| `toggleCollectionDynamic(enable, activities)` | Pause/resume collection |
| `enableProfilerInChildThread()` / `disableProfilerInChildThread()` | Opt a worker into the main session |
| `isProfilerEnabledInMainThread()` | Query |
| `startMemoryProfile()` / `stopMemoryProfile()` / `exportMemoryProfile(path)` | Memory-profile export (allocators must call `report_memory_usage`) |
| `reportBackendEventToActiveKinetoProfiler(...)` | Inject a backend span |

Child threads do **not** participate unless they call
`enableProfilerInChildThread()`.

### Config

```cpp
struct ProfilerConfig {
    ProfilerState state;       // KINETO, KINETO_GPU_FALLBACK, KINETO_ONDEMAND, NVTX, ITT
    bool report_input_shapes;
    bool profile_memory;
    bool with_stack;           // captures raw return addresses via bespoke/common/unwind/
    bool with_flops;
    bool with_modules;
    ExperimentalConfig experimental_config;
    std::string trace_id;
};
```

`ActivityType`: `CPU`, `CUDA`, `XPU`, `HPU`, `MTIA`, `PrivateUse1`.

`RecordScope` (filter which `RecordFunction`s fire): `FUNCTION`,
`BACKWARD_FUNCTION`, `CUSTOM_CLASS`, `USER_SCOPE`, `STATIC_RUNTIME_OP`,
`STATIC_RUNTIME_MODEL`. TorchScript / lite-interpreter scopes were removed.

### Results

```cpp
struct ProfilerResult {
    uint64_t trace_start_ns() const;
    const std::vector<KinetoEvent>& events() const;
    const std::vector<experimental_event_t>& event_tree() const;
    void save(const std::string& path);  // Chrome Trace JSON (destructive on the libkineto trace)
};
```

`KinetoEvent` exposes name, start/end/duration ns, device type/index,
correlation IDs, optional shapes/stack/module hierarchy, FLOPs, extra
metadata, CUDA elapsed µs (when CUDA events were recorded).

### Clock conversion and correlation IDs

Events are stamped with an approximate clock and converted to Unix time via
`ApproximateClockToUnixTimeConverter` so traces align with the system
timeline. GPU kernels are linked to the launching CPU op by correlation ID
(`pushCorrelationId` / `popCorrelationId` in `kineto_shim`).

### libkineto surface

XSigma never includes `libkineto.h` from a public header. The only TU that
talks to libkineto is `bespoke/kineto/kineto_shim.cpp` (plus
`kineto_client_interface.cpp` for on-demand). Used types:
`libkineto::api()`, `ActivityProfilerInterface`, `ActivityType`,
`CpuTraceBuffer`, `GenericTraceActivity`, `ActivityTraceInterface`,
`ITraceActivity`, `ClientInterface`, `processId()` / `systemThreadId()`,
`timeSinceEpoch` / `get_time_converter()`.

Activity types XSigma maps but does not itself generate (they appear only if
libkineto/CUPTI produces them): `GPU_MEMCPY`, `GPU_MEMSET`,
`CONCURRENT_KERNEL`, `CUDA_RUNTIME`, `CUDA_DRIVER`, `CUDA_SYNC`,
`PYTHON_FUNCTION`, `PRIVATEUSE1_*`, `XPU_*`, `MTIA_*`, `HPU_OP`,
`COLLECTIVE_COMM`, `OVERHEAD`.

### PyTorch types removed

This port is not a PyTorch runtime. Deleted or emptied: `IValue`,
`FunctionSchema` / `OperatorHandle` / `OperatorName`, `InputOutputEncoder`
input/output/kwargs overloads, `EventType::Vulkan`, NCCL meta helpers,
`ProfilerConfig::{to,from}IValue`, TorchScript/lite `RecordScope`s.

Kept as XSigma-owned names (no libtorch): `EventType::TorchOp` (a
`RECORD_FUNCTION` CPU span), Python event kinds + `python_tracer` plug-in
(default no-op), `profiler::autograd::profiler_impl`, empty
`TensorMetadata` stubs, `PRIVATEUSE1` observer, `kPythonTracerPlaneName`.

`bespoke/common/ivalue.h` hardcodes `PROFILER_XXX_DISABLE_TENSOR 1` —
intentional; XSigma has no tensor type.

### Python tracer plug-in

`bespoke/common/orchestration/python_tracer.{h,cpp}`:
`PythonTracerBase` / `registerTracer` / `registerMemoryTracer`, default
`NoOpPythonTracer`. Native’s `python_tracer_factory.cpp` registers a stub
`profiler_interface` when `python_tracer_level > 0`. Full CPython
`PyEval_SetProfile` bindings were removed; a future tracer plugs in via
`registerTracer` without changing the collection engine.

### Thread safety

Per-thread `RecordQueue` subqueues: recording itself is unsynchronized.
Synchronized points: `ProfilerStateBase::push` / `pop`, callback
registration. Global mode shares the main thread’s state via `shared_ptr`.

See also [known `handle_` bug](#open-follow-ups).

---

## ITT and NVTX

### ITT (VTune)

Depends on vendored `ThirdParty/ittapi` (`Cmake/packages/FindITT.cmake`
always points at that copy). Shares `bespoke/base/` and `bespoke/common/`
with Kineto. When ITT is built **without** Kineto, CMake hand-enumerates
orchestration files (`profiler_kineto.{h,cpp}`, `kineto_shim.cpp`,
`kineto_client_interface.{h,cpp}`) and sets `FMT_HEADER_ONLY=1` on the
Profiler target (fmt’s compiled symbols otherwise come from libkineto’s
build). New files added under `bespoke/kineto/` that ITT also needs must be
added to that list.

```cpp
#include "bespoke/itt/itt_wrapper.h"

profiler::profiler_impl::itt_init();
if (profiler::profiler_impl::itt_get_domain() != nullptr) {
    profiler::profiler_impl::itt_range_push("my_operation");
    // work
    profiler::profiler_impl::itt_range_pop();
    profiler::profiler_impl::itt_mark("checkpoint");
}
```

Same `enableProfiler` / `disableProfiler` entry point as Kineto, with
`ProfilerState::ITT`. Dispatch: `pushITTCallbacks` → `ITTThreadLocalState`
→ `itt_wrapper.cpp` (`__itt_task_begin` / `__itt_task_end`, domain
`"XSigma"`). ITT does **not** use `RecordQueue` / `KinetoEvent`;
`disableProfiler()` returns an empty `ProfilerResult`. Ranges go to VTune.

```bash
vtune -collect hotspots -app ./bin/example_profiling_basic
vtune-gui
```

### NVTX (Nsight)

`ProfilerState::NVTX`, available whenever Kineto or ITT compiled
`bespoke/base/`. `pushNVTXCallbacks` → `enterNVTX` →
`cudaStubs()->rangePush` (`nvtxRangePushA` / `nvtxRangePop` when
`PROFILER_HAS_CUDA`). Tensor producer/consumer correlation is a no-op
(XSigma has no tensor type).

---

## GPU / CUDA

Profiler **activity tracing** is CUDA-only. There is no HIP or Metal
activity-tracing backend under `Library/Profiler`. That is distinct from
`Library/Memory` GPU **allocators**, which can report into the profiler via
`report_memory_usage` (Metal is wired; CUDA/HIP allocators are not).

Two CUDA paths, both gated by `PROFILER_HAS_CUDA` (set when
`MEMORY_GPU_BACKEND=cuda` and `find_package(CUDAToolkit)` succeeds):

1. **CUPTI** — device-side kernel/memcpy/memset timing, correlated to the
   launching CPU op. Requires Kineto built with `LIBKINETO_NOCUPTI=OFF`.
2. **`KINETO_GPU_FALLBACK`** — CPU-side `cudaEvent_t` pair in
   `bespoke/base/cuda.cpp`, using the CUDA Runtime API directly. Records on
   the default per-thread stream (XSigma has no stream-pool abstraction).

`PROFILER_HAS_CUDA=0` is verified on non-CUDA hosts. The
`PROFILER_HAS_CUDA=1` path needs a CUDA Toolkit machine before it is relied
on.

---

## Runnable examples

Sources: [`Examples/Profiling/`](../../Examples/Profiling/)
(`example_profiling_basic.cpp`).

```bash
cd Scripts
python3 setup.py config.build.examples.ninja.clang
# Windows: python3 setup.py config.build.examples.vs22
```

Binary: `build_ninja/bin/example_profiling_basic`. Typical traces:

| File | Description |
|---|---|
| `quarisma_native_profile.json` | Native hierarchical Chrome Trace |
| `kineto_quarisma_trace.json` / `kineto_only_trace.json` | Combined / Kineto-only |
| `itt_quarisma_trace.json` | Native trace from the ITT example |

The example degrades gracefully when Kineto or ITT is unavailable.

### Visualize Chrome Trace

- **chrome://tracing** — Load the JSON; W/S zoom, A/D pan, click for details.
- **[Perfetto UI](https://ui.perfetto.dev)** — SQL queries, custom tracks.

Chrome Trace JSON uses **nanoseconds** and sets `"displayTimeUnit": "ns"`
(hierarchical and XPlane-derived traces).

Event fields: `name`, `ph` (`X` = complete), `ts`, `dur`, `pid`, `tid`.

Look for hotspots (longest duration), nesting depth, thread utilization,
idle gaps, and profiler overhead (~100 ns/scope).

---

## Output formats

### JSON (session report)

```json
{
  "profiling_data": {
    "total_duration_ns": 1000000,
    "scopes": [
      {
        "name": "main_scope",
        "duration_ns": 1000000,
        "memory_allocated": 1024000,
        "memory_freed": 512000,
        "children": []
      }
    ]
  }
}
```

### CSV

```
scope_name,duration_ns,memory_allocated,memory_freed,thread_id
main_scope,1000000,1024000,512000,1
```

### Chrome Trace

```json
{
  "traceEvents": [
    {"name": "process_name", "ph": "M", "pid": 1, "args": {"name": "Host"}},
    {"name": "main_scope", "ph": "X", "pid": 1, "tid": 100, "ts": 1000, "dur": 1000000}
  ],
  "displayTimeUnit": "ns"
}
```

### XPlane

Structured planes / lines / events used internally by the Native backend
(`native/exporters/xplane/`). Same representation TensorBoard’s profiler
plugin consumes.

### Console

Hierarchical tree plus memory and timing statistics (min/max/mean/stddev
and percentiles).

---

## Hotspot report

Kineto-only. `bespoke/kineto/hotspot_report.h` —
`profiler::autograd::profiler_impl::hotspot_report`.

Instrumentation-based (not PC sampling): nodes are `RECORD_FUNCTION` /
`RECORD_USER_SCOPE` scopes. Self/total time is exact for annotated sites;
the “call stack” is the RecordFunction parent chain, not a symbolized
native stack. `config.with_stack` already captures raw `void*` addresses
via `bespoke/common/unwind/`; feeding those through the symbolizer into
this report is an [open follow-up](#open-follow-ups).

```cpp
auto result = profiler::autograd::profiler_impl::disableProfiler();
profiler::autograd::profiler_impl::hotspot_report report(*result);

std::cout << report.top_down_tree();
std::cout << report.bottom_up_hotspots();
std::cout << report.call_stack_for("gemm_kernel");
std::cout << report.table();  // PyTorch key_averages().table() columns
std::cout << report.table("self_xpu_time_total", /*row_limit=*/10);
```

```
== Top-down tree ==
[100.0%] matmul  total=5.00ms self=1.00ms
  [60.0%] gemm_kernel  total=3.00ms self=3.00ms
  [20.0%] bias_add  total=1.00ms self=1.00ms

== Bottom-up hotspots ==
self time      total time     calls  name
3.00ms         3.00ms         1      gemm_kernel
1.00ms         5.00ms         1      matmul
1.00ms         1.00ms         1      bias_add
```

Tests: `TestHotspotReport.cpp`, `TestRecordFunctionIntegration.cpp`.

---

## Console table columns

`hotspot_report::table(sort_by, row_limit)` prints the same breakdown as
PyTorch `prof.key_averages().table()` / the [Intel Kineto profiler
table](https://intel.github.io/intel-extension-for-pytorch/xpu/2.3.110+xpu/tutorials/features/profiler_kineto.html).

| Column | Formula | Format |
|---|---|---|
| Name | Aggregated scope / op / kernel name | Left, min width 20 |
| Self CPU % | `(self_cpu / sum(self_cpu)) * 100` | `XX.XX%`, width 12 |
| Self CPU | CPU time excluding same-device children | us/ms/s, 3 decimals |
| CPU total % | `(cpu_total / sum(self_cpu)) * 100` | `XX.XX%` |
| CPU total | Inclusive CPU time | us/ms/s |
| CPU time avg | `cpu_total / call_count` | us/ms/s |
| Self CUDA / % / total / avg | Same for CUDA/HIP; omitted when all zeros | Optional |
| Self XPU / % / total / avg | Same for XPU; omitted when all zeros | Optional |
| # of Calls | Aggregated invocation count | Integer |

Footer: `Self CPU time total:` (and CUDA/XPU totals when those columns are
shown). `sort_by` accepts `self_cpu_time_total`, `cpu_time_total`,
`cpu_time_avg`, `self_cuda_time_total`, `self_xpu_time_total`, `count`, and
the matching `*_total` / `*_avg` keys. Empty `sort_by` keeps self-CPU
descending order. `row_limit` 0 prints every row.

---

## Macros and feature flags

Public / build macros that actually gate profiler behavior. Include guards
and file-local helpers are omitted.

### User-facing

| Macro | Header | Role |
|---|---|---|
| `PROFILER_PROFILE_SCOPE` / `_FUNCTION` / `_BLOCK` | `native/session/profiler.h` | Native RAII scopes |
| `RECORD_FUNCTION` / `RECORD_USER_SCOPE` / … | `common/instrumentation.h` | Kineto/ITT scopes (no-op under Native) |
| `PROFILER_API` / `PROFILER_VISIBILITY` / `PROFILER_UNUSED` | `common/profiler_export.h`, `common/profiler_macros.h` | Export / unused params |
| `PROFILER_LIKELY` / `PROFILER_UNLIKELY` / `PROFILER_NODISCARD` | `common/profiler_macros.h` | Attributes |

### Backend flags

`PROFILER_HAS_KINETO` and `PROFILER_HAS_ITT` are mutually exclusive with
each other (`#error` in `common/profiler_export.h` if both are `1`).
`PROFILER_HAS_NATIVE` is independent of that and always `1`.

| Macro | Purpose |
|---|---|
| `PROFILER_HAS_KINETO` | Kineto GPU/CPU activity tracing; shim is no-op when off |
| `PROFILER_HAS_ITT` | Intel VTune ITT ranges |
| `PROFILER_HAS_NATIVE` | Native XPlane / session / TraceMe — always `1` |
| `PROFILER_HAS_CUDA` | CUDA Runtime + optional CUPTI/NVTX in Profiler |
| `PROFILER_HAS_INSTRUMENTATION` | 1 when Kineto or ITT; set by `instrumentation.h` |
| `QUARISMA_HAS_CUDA` | CUDA event types / NVML queries inside the Kineto wrapper |
| `KINETO_HAS_HCCL_PROFILER` | AMD HCCL hooks (ROCm + HCCL only) |

CMake also injects `PROFILER_ENABLE_KINETO` / `PROFILER_ENABLE_ITT` (one of
the two) and `PROFILER_ENABLE_NATIVE_PROFILER` (always `ON`), plus the usual
`PROFILER_SHARED_DEFINE` / `PROFILER_BUILDING_DLL` / `PROFILER_STATIC_DEFINE`.
Tests define `PROFILER_GOOGLE_TEST`.

### Other compile-time switches

| Macro | Purpose |
|---|---|
| `PROFILER_MOBILE` / `PROFILER_IOS` | Reduced features; iOS clock path |
| `PROFILER_USE_ROCM` | ROCm HIP activity collection |
| `PROFILER_CUDA_USE_NVTX3` | NVTX3 header-only API |
| `PROFILER_PREFER_CUSTOM_THREAD_LOCAL_STORAGE` | Custom TLS on hot paths |
| `PROFILER_XXX_DISABLE_TENSOR` | Strip tensor branches from `IValue` (always 1) |
| `PROFILER_RDTSC` | x86 cycle-counter clock |
| `ENABLE_GLOBAL_OBSERVER` | System-wide activity observer |
| `USE_DISTRIBUTED` | NCCL rank / collective metadata |
| `EDGE_PROFILER_USE_KINETO` | Embedded Kineto client path |
| `IS_PYTHON_3_12` | `sys.monitoring` vs `PyEval_SetProfile` (bindings removed) |

`#if 0` blocks remain in `collection.cpp`, `kineto_shim.cpp`, `ivalue.h`,
and similar — they are **intentionally** disabled (TorchScript, Vulkan,
tensor producer/consumer, Python bindings). Do not re-enable without the
missing types. `#if PROFILER_HAS_KINETO && 0` disables a sub-block while
keeping the surrounding Kineto guard.

---

## Dependencies

| Backend | Links |
|---|---|
| Kineto | `Kineto::kineto` (`ThirdParty/kineto`), fmt (via libkineto), optional `CUDA::cudart` + `CUDA::nvToolsExt` |
| ITT | vendored ittapi; fmt header-only on this branch; borrowed Kineto orchestration TUs |
| Native | C++ standard library only |

Profiler does not depend on `Library/Core`. Consuming libraries (Memory,
Vectorization, Parallel) may depend on Profiler.

Cross-language: no pybind11, no `extern "C"` public ABI. Adding Python
would need a vendored binding library, a `PROFILER_ENABLE_PYTHON` toggle, a
shim around `enableProfiler` / `profiler_session` / `hotspot_report`, and a
real extension module — none of that is scaffolded.

---

## Best practices

1. **RAII scopes** — `PROFILER_PROFILE_SCOPE` or `RECORD_USER_SCOPE` in a
   `{ }` block; never disable the session before the guard destructs.
2. **Instrument boundaries**, not inner loops. Empty-callback `RecordFunction`
   is cheap; named scopes in a tight kernel are not.
3. **Short names** — reduce string overhead on the hot path.
4. **Enable only what you need** — timing vs memory vs stats vs CUDA.
5. **Check availability** — `PROFILER_HAS_*` and `itt_get_domain()`.
6. **Export traces** — Chrome Trace / Perfetto for timelines; hotspot report
   for self vs total; JSON/CSV for scripts.
7. **Thread safety** — Native: `.with_thread_safety(true)`. Kineto: opt in
   workers with `enableProfilerInChildThread()`.
8. **Do not instrument `noexcept` functions** with `RECORD_USER_SCOPE`
   (`RecordFunction` can allocate).

---

## Troubleshooting

| Symptom | What to check |
|---|---|
| Empty `ProfilerResult` / no CPU events | `RECORD_USER_SCOPE` must destruct before `disableProfiler()`. Nested `{ }` around the scope. |
| Child thread missing from the trace | Call `enableProfilerInChildThread()` on that thread. |
| High overhead | Fewer / coarser scopes; disable memory/stats/CUDA; avoid per-iteration names. |
| Empty or &lt; 1 KB JSON | Session `start`/`stop`; macros actually compiled in; `write_chrome_trace` / `save` called. |
| Kineto trace has no events | `ActivityType::CPU` captures ops that went through `RecordFunction`. Use Native scopes for ad-hoc CPU regions, or wrap them in `RECORD_USER_SCOPE`. |
| ITT ranges missing in VTune | Run under VTune; `itt_get_domain()` non-null; `PROFILER_HAS_ITT=1`. |
| Chrome “Invalid trace format” | `stop()` before export; `python3 -m json.tool < trace.json`. |
| Native headers fail to compile under Kineto | `native/session/profiler.h` is not in the Kineto build. Use `common/instrumentation.h` from other libraries. |
| ITT-only link errors on fmt | ITT-without-Kineto must compile Profiler with `FMT_HEADER_ONLY=1` (CMake already does this). |
| Missing metadata (shapes, stacks) | `ProfilerConfig::report_input_shapes` / `with_stack`; tensor shapes stay empty without a tensor type. |

Platforms: Windows, Linux, macOS.

---

## Open follow-ups

1. **`ProfilerStateBase::handle_` is not per-thread** — a child’s
   `enableProfilerInChildThread()` can clobber the main thread’s callback
   handle (`SOFT_ASSERT`, leaked unregistration). Fix in
   `bespoke/common/orchestration/observer.*` (per-tid map, like
   `sub_queues_`).
2. **`TestKinetoShim.cpp`** still `#if PROFILER_HAS_KINETO && 0`.
3. **CUDA/HIP allocator instrumentation** — same `report_memory_usage`
   pattern as Metal, on a CUDA/HIP machine; validate `PROFILER_HAS_CUDA=1`.
4. **Phase 0** — independent backend toggles, then Phase 3/4
   (`profiler_interface` wrappers + one session API).
5. **Symbolized stacks in `hotspot_report`** — `unwind/` + `fast_symbolizer.h`.
6. **`startMemoryProfile` path** vs Memory’s own
   `unified_memory_stats.cpp` — not the same pipeline.
7. Pre-existing: `step_info` order under multi-thread `ProfilerStep#`;
   dropped tests `RecordDebugHandles.Basic` and
   `write_chrome_trace_rejects_empty_path`; dormant `AppendOnlyList`
   bounds-check.

Historical session notes, investigation checklists, and duplicate Kineto /
PyTorch-table write-ups were merged into this file and removed.

---

## See also

- Implementation: [`Library/Profiler/README.md`](../../Library/Profiler/README.md)
- Examples: [`Examples/Profiling/`](../../Examples/Profiling/)
- [Chrome Trace Event Format](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU)
- [Perfetto UI](https://ui.perfetto.dev)
- [Intel ITT API](https://github.com/intel/ittapi)
- [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html)
- [Kineto](https://github.com/pytorch/kineto)
