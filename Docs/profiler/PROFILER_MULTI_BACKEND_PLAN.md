# XSigma Profiler: Multi-Backend Architecture Plan

Status: living plan. Written 2026-08-18. Supersedes ad-hoc profiler notes for the
purpose of "make the XSigma profiler behave like PyTorch's, with a TF/XLA-style
XPlane backend as a peer, not a replacement."

## 1. Where things stood before this session

`Library/Profiler` contains **three overlapping profiler designs** that were
never made to work together:

| Design | Location | Model | Build gating |
|---|---|---|---|
| Kineto engine | `bespoke/` | Line-for-line port of `torch/csrc/profiler` — `RecordFunction`, `collection.cpp`, `kineto_shim`, `profiler_kineto`, `unwind/`, `nvtx_observer`, `itt_observer`, `execution_trace_observer`, `python_tracer`. Ships PyTorch's own README verbatim. | Compiles when `PROFILER_BACKEND=KINETO` (the default). |
| XPlane/XSpace engine | `native/` | Port of TensorFlow/XLA's profiler — `xplane_builder`, `xplane_schema`, `xplane_visitor`, `host_tracer`, `traceme`, plus a hand-written `profiler_session`/`profiler_scope` RAII wrapper in `native/session/profiler.h`. | Compiles only when `PROFILER_BACKEND=NATIVE`. |
| A third, undocumented API | referenced only from a dead test | `profiler_session::instance()` singleton + `profiler_guard`, headers `profiler_api.h`/`profiler_guard.h` that don't exist anywhere in the repo. | Never — permanently disabled by an undefined macro. |

`PROFILER_BACKEND` (`KINETO` \| `NATIVE` \| `ITT`) was a single CMake selector:
build with Kineto **or** Native **or** ITT, never together
(`Library/Profiler/CMakeLists.txt`). This is the root architectural problem:
TensorFlow's XPlane format and PyTorch's Kineto/RecordFunction engine were
each fully vendored, but wired as mutually-exclusive alternatives instead of
peers.

### 1.1 What was actually dead

- `TestXSigmaProfiler.cpp` — included nonexistent headers (`profiler_api.h`,
  `profiler_guard.h`), permanently compiled out by an undefined
  `PROFILER_HAS_PROFILER` macro. **Deleted.**
- `TestKinetoProfiler.cpp` — 100% wrapped in `#if 0`, with its own comment
  admitting it tested a `profiler::kineto_profiler` API that was never
  implemented; the real API (`enableProfiler`/`disableProfiler`/
  `prepareProfiler`) had zero coverage as a result. **Deleted**; real coverage
  now lives in `TestRecordFunctionIntegration.cpp`.
- `TestProfilerBackendIntegration.cpp` — its five real Kineto integration
  tests were commented out, and it could not link under the default Kineto
  backend anyway: it unconditionally used `native/session/profiler.h`'s
  `profiler_session`/`profiler_scope`, which only exist under the Native
  backend. Confirmed by reproduction (see §2). **Deleted and split**: the
  Kineto/ITT content moved to `TestRecordFunctionIntegration.cpp` (uncommented
  and fixed); the two `ProfilerChromeTrace` tests were exact duplicates of
  coverage already in `TestProfilerChromeTraceHierarchical.cpp` and were
  dropped.
- `TestKinetoShim.cpp` — still `#if PROFILER_HAS_KINETO && 0`, i.e.
  permanently disabled by construction. **Not yet fixed** (see §5).
- `bespoke/common/ivalue.h` hardcodes `PROFILER_XXX_DISABLE_TENSOR 1` — this
  one is **intentional and correct**, not a bug: XSigma has no tensor type,
  so PyTorch's `IValue::isTensor()`/`toTensor()`/`isTensorList()` branches are
  legitimately stripped. Do not "fix" this.

## 2. Root cause of "broken RecordFunction" (fixed this session)

Reproduced directly rather than guessed at: after fixing the link issue above
so the Kineto integration tests could actually run, `RECORD_USER_SCOPE`
guards fired correctly (state transitions, callback registration all
succeeded) but **zero CPU events ever reached the output** —
`ProfilerResult::events()` and `event_tree()` were always empty.

Traced to `ThreadLocalSubqueue::TorchOpStorage::materialize()` in
`bespoke/common/collection.cpp` — the function that converts every captured
`RECORD_FUNCTION`/`RECORD_USER_SCOPE` event into a `Result` object. Its entire
body was `#if 0`'d out, with all parameters commented to `/*unused*/`; it did
nothing but clear the input buffers and return. Every event
`RecordFunction::begin_op`/`end_op` captured was silently discarded before it
could ever become a `Result`.

The disabling comment claimed `ExtraFields constructor expects
std::vector<op_input_t> but getters return std::nullopt` — tracked to
`InputOutputEncoder::getIValueGenerator()`, which had a **second, separately
`#if 0`'d real implementation** and a stub fallback whose lambda returned a
bare `std::nullopt` (type `std::nullopt_t`) instead of the
`std::vector<op_input_t>` the `ExtraFields<EventType::TorchOp>` constructor
requires — a genuine type mismatch, but one caused by the real implementation
being disabled, not by any actual dependency on unavailable types. The real
`getIValueGenerator()` body only touches the profiler's own lightweight
`TensorMetadata`/`RawTensorMetadata` structs and `profiler::irange` (both
already compiled in via `common/irange.h`) — it does **not** call any of the
`IValue::isTensor()`/`toTensor()` methods that `PROFILER_XXX_DISABLE_TENSOR`
legitimately strips, so there was no real reason for it to be disabled.

### Fix applied

1. `InputOutputEncoder::getIValueGenerator()` — removed the `#if 0` and the
   broken `std::nullopt` stub; kept the one real implementation.
2. `TorchOpStorage::materialize()` — removed both inner `#if 0` blocks
   (the autograd sequence-number plumbing loop, and the main per-event
   `ExtraFields<TorchOp>` construction loop), restored the real parameter
   names, and appended a defensive `op_events_.size() > 0` guard before the
   backward-pass-matching loop (previously untested with real data, would
   underflow on an empty queue).
3. Two latent syntax bugs this exposed (a dangling `<< "..."` left over from
   a commented-out `LOG(WARNING)` call, and two similar dangling string
   literals) — fixed as plain comments.

### Verification

Before: the five real Kineto integration tests were permanently commented
out. After: rebuilt under both CMake and Bazel, four of five now pass for
real (not skipped) with genuine `KinetoEvent` data — correct durations,
correct parent/child timing for nested scopes, correct enable/disable state
transitions. Confirmed with a standalone scratch program showing real
captured timings (see §4 for that same run repurposed as the hotspot demo).

One test remains a legitimate `GTEST_SKIP()`, not a false pass:
`KinetoIntegration.thread_local_participation_requires_opt_in` — a detached
worker thread correctly produces no events (opt-in required, matching
PyTorch's own thread-local semantics), but the attached
(`enableProfilerInChildThread()`-opted-in) worker thread's event doesn't
currently surface either. This is a narrower, separate bug in the
multi-threaded opt-in path, not the core materialization bug fixed above.
**Follow-up**, tracked in §5. *(Update: root-caused and fixed in a later
session — see §2c. It turned out to be a test bug, not a library bug.)*

## 2a. CUDA profiling, using PyTorch's own technique (fixed this session)

PyTorch's CUDA profiling has two layers: (1) full activity tracing via Kineto
+ CUPTI (kernel/memcpy/memset activities, correlated to the launching CPU op
by correlation ID), and (2) a lightweight fallback — `ProfilerState::
KINETO_GPU_FALLBACK` — that brackets a `RecordFunction` scope with a pair of
`cudaEvent_t` timestamps when full CUPTI tracing isn't available. XSigma's
`bespoke/` tree already has both layers *structurally* ported faithfully:
`ActivityType::CUDA`, `ProfilerState::KINETO_GPU_FALLBACK`, and the
`cudaStubs()->record(...)` call sites in `profiler_kineto.cpp` (§`onFunctionExit`,
`disableProfiler`) all already exist and are wired correctly. What was
missing was the same disease as §2: **the entire CUDA stub implementation
(`bespoke/base/cuda.cpp`) was wrapped in `#if 0`**, so `cudaStubs()` always
returned the disabled/no-op default and `RegisterCUDAMethods`'s static
registrar never ran.

Two things were broken here, not one:

1. The disabled body called a `profiler::cuda::` namespace
   (`GetDevice`, `getCurrentCUDAStream`, `OptionalCUDAGuard`, `device_count`)
   and included `<profiler/cuda/CUDAGuard.h>` /
   `<profiler/util/ApproximateClock.h>` — **none of which exist anywhere in
   the repository.** These were stale paths from an incomplete port (PyTorch's
   original file uses `at::cuda::*`, which XSigma's tensor-free Profiler
   library correctly has no equivalent of and should not depend on).
2. Even if the body had compiled, **no build ever defined `PROFILER_HAS_CUDA`**
   — `profiler_kineto.h`'s existing `#if PROFILER_HAS_CUDA` guard (used by
   `hasCUDA()`) was checking an always-undefined macro, i.e. always `0`, in
   every configuration including `MEMORY_GPU_BACKEND=cuda` builds.

### Fix applied

- `bespoke/base/cuda.cpp`: replaced the `profiler::cuda::` calls with direct
  CUDA Runtime API calls (`cudaGetDevice`/`cudaSetDevice`/`cudaGetDeviceCount`),
  and a small local `ScopedCUDADeviceGuard` RAII helper for `onEachDevice()`
  (previously `profiler::cuda::OptionalCUDAGuard`). Fixed the two nonexistent
  include paths (`<cuda_runtime_api.h>` for the runtime API,
  `common/approximate_clock.h` for `profiler::getTime()`, which already
  existed and was already used correctly elsewhere in this same file).
  Regated the whole file on `#if PROFILER_HAS_CUDA` instead of `#if 0`.
- `Library/Profiler/CMakeLists.txt`: added the actual `PROFILER_HAS_CUDA`
  wiring, next to the existing (working) CUPTI gate: when
  `MEMORY_GPU_BACKEND=cuda`, `find_package(CUDAToolkit)` and set
  `PROFILER_HAS_CUDA=1` + link `CUDA::cudart` (and `CUDA::nvToolsExt` if
  present) only if the toolkit is actually found; otherwise `0`. This is the
  same modern-CMake `FindCUDAToolkit` mechanism, and does not require
  `enable_language(CUDA)` or turning the Profiler target into a CUDA-language
  target — it just adds the CUDA runtime headers/lib to a plain C++ TU,
  exactly like linking any other C library.

### One deliberate simplification vs. PyTorch

PyTorch's `at::cuda::getCurrentCUDAStream()` tracks a real per-thread
stream-pool concept so that GPU-event fallback timing brackets the actual
stream work was launched on. XSigma has no cross-library "current stream"
abstraction to hook into (checked: nothing outside Profiler exposes one, and
`Library/Memory/gpu/cuda_caching_allocator.h` uses `cudaStream_t` as a raw
type without a pool). The fallback stub therefore records on the default
per-thread stream (`nullptr`/legacy stream 0) rather than a pooled stream —
correct for single-stream workloads, potentially imprecise for GPU work
explicitly launched on a non-default stream. Documented inline in
`ScopedCUDADeviceGuard`'s comment. Revisit if/when XSigma grows a real
stream-pool abstraction.

### Important scope note: this could not be compile-tested against real CUDA

This development sandbox has no CUDA toolkit (macOS/arm64). The default,
non-CUDA build (`MEMORY_GPU_BACKEND=none`, the project default) was rebuilt
and retested end-to-end after this change — confirmed
`PROFILER_HAS_CUDA=0` on the compile command line and a clean, fully green
build under both CMake and Bazel, so this change is verified **inert** for
everyone not opting into CUDA. The `PROFILER_HAS_CUDA=1` code path itself
(the actual CUDA Runtime API calls, `find_package(CUDAToolkit)` resolution)
is **not** verified to compile — it needs a `MEMORY_GPU_BACKEND=cuda` build
on a machine with the CUDA Toolkit installed. Do that before relying on it.

### A larger, pre-existing gap noticed in passing

`Library/Memory/CMakeLists.txt` references a `PROJECT_CUDA_LIBRARIES`
variable (`if(MEMORY_GPU_BACKEND STREQUAL "cuda" AND DEFINED
PROJECT_CUDA_LIBRARIES)`) that **is never defined anywhere in the repo** —
that branch is dead today in exactly the same way `PROFILER_HAS_CUDA` was.
CUDA support in XSigma's CMake build appears to be scaffolded project-wide
but not finished, not something specific to Profiler. Out of scope for this
plan (Profiler shouldn't own Memory's CUDA wiring), but worth knowing before
assuming `MEMORY_GPU_BACKEND=cuda` "just works" anywhere else in the tree.

## 2b. Full audit of `bespoke/kineto`, `bespoke/itt`, and NVTX (this session)

Following up on §2's discovery that a single `#if 0` could silently disable a
whole capability, every `#if 0` block across `bespoke/kineto/`, `bespoke/itt/`,
`bespoke/base/` (NVTX), and their shared `bespoke/common/` dependencies
(`collection.cpp`, `record_function.cpp`, `util.cpp`, `data_flow.cpp`) was
individually triaged: for each one, does the disabling comment's claimed
reason ("X not available in profiler-only build") actually hold, or is it
stale/wrong the way §2's was?

**Result: two more real bugs found and fixed; everything else audited is a
legitimate, correctly-degraded gap.**

### Real bugs fixed

1. **ITT backend had no `enableProfiler`/`disableProfiler` API at all.**
   `profiler_kineto.cpp` — the shared orchestration layer that dispatches to
   Kineto, ITT, NVTX, or PRIVATEUSE1 based on `ProfilerConfig::state` (its ITT
   branch is a two-line, `#if PROFILER_HAS_ITT`-guarded call to
   `pushITTCallbacks()`, entirely independent of libkineto) — was excluded
   from both `CMakeLists.txt`'s and `BUILD.bazel`'s ITT-only source lists.
   Confirmed by reproduction: building the ITT backend with a test that calls
   `profiler::autograd::profiler_impl::enableProfiler(...)` failed with
   `no member named 'autograd' in namespace 'profiler'` — the header wasn't
   even reachable, and the symbols weren't compiled in at all. Fixed by adding
   `profiler_kineto.{h,cpp}` (and the inert-without-Kineto
   `kineto_client_interface.{h,cpp}`) to both build systems' ITT source lists.
2. **Two small gaps this exposed, once ITT could actually reach that code:**
   - `KinetoEvent::activityType()` unconditionally forwarded to
     `Result::kinetoType()`, which only exists `#if PROFILER_HAS_KINETO`
     (compile error under ITT-only). Added an `#else` returning a default —
     no real Kineto activity ever reaches this path outside a Kineto session,
     so a default is correct, not a functional loss.
   - `disableProfiler()`'s post-session result construction explicitly
     handled `NVTX`/`PRIVATEUSE1` (returning an empty `ProfilerResult`) but
     not `ITT`, silently returning `nullptr` instead — inconsistent with its
     siblings. Added `ITT` to that branch.
3. **ITT backend failed to link: `fmt::v12::vformat`/`report_error`/etc.
   undefined.** Root cause: fmt only gets built as a real, compiled library
   (not header-only) as a *side effect* of Kineto's own vendored build
   (`FMT_SOURCE_DIR` reuse inside `add_third_party_library(kineto ...)`).
   Without Kineto, nothing compiles fmt's non-header-only symbols, yet
   `collection.cpp`, `execution_trace_observer.cpp`, `util.cpp`, and
   `profiler_kineto.cpp` all use `fmt::format` unconditionally. Confirmed this
   is independent of whether `Core` (which sets up its own, separately
   header-only, `Fmt::fmt`) is present in the build — the raw
   `"${_tpdir}/fmt/include"` path Profiler adds bypasses target-based
   propagation of Core's `FMT_HEADER_ONLY` compile definition regardless.
   Fixed narrowly and safely, scoped to Profiler's own target only (no shared
   CMake cache state touched, so it can't affect Core/Logging/Kineto's
   separate fmt setups): `target_compile_definitions(Profiler PRIVATE
   FMT_HEADER_ONLY=1)` when ITT is enabled without Kineto.
   Bazel never had this problem — `@fmt//:fmt` is unconditionally linked
   there regardless of backend.

None of this was reachable before this session because **nobody had ever
actually built and tested the ITT or Native backends** — the default
`config.build.test` invocation always builds Kineto. Building each backend
required the actual `--profiler.itt` / `--profiler.native` flags (undocumented
in the `xsigma-build` skill at the time; the dotted-token form
`profiler.itt` chained into the main token string does *not* work — it must
be its own `--profiler.itt` argument, per `Scripts/setup.py`'s `parse_args`).

### Audited and confirmed legitimate (no bug, no fix)

Every other `#if 0` found — `bespoke/common/collection.cpp` (Vulkan shader
event materialization, `PyCall`/`PyCCall`/`PythonGC` string formatting, JIT
call-stack/module-hierarchy capture, TF32 precision query),
`bespoke/common/record_function.cpp` (TorchScript `FunctionSchema`
name/arguments/returns/overload lookups), `bespoke/common/util.cpp` (JIT
call-stack formatting, tensor-shape introspection, `IValue::operator<<` —
which is declared but genuinely never *defined* anywhere in this codebase,
so enabling its callers would trade a compile error for a link error),
`bespoke/common/data_flow.cpp` (Python `nn.Module`/`optim.Optimizer` tensor
tracking), `bespoke/base/nvtx_observer.cpp` (tensor producer/consumer op-id
tracking), `bespoke/kineto/kineto_client_interface.cpp` and `kineto_shim.cpp`
(`LOG(INFO)` calls — no logging backend is wired into Profiler's minimal
dependency set), and `bespoke/itt/itt.h` (Python bindings, `initIttBindings`)
— all correctly depend on something genuinely absent (Tensor support,
disabled project-wide via `PROFILER_XXX_DISABLE_TENSOR=1` in `ivalue.h`;
TorchScript JIT; the Python tracer/bindings subsystem; Vulkan), and every one
of them already has a safe, working fallback (`#else` stub, or is simply
never populated with data) rather than silently discarding real captured
data the way §2's bug did. Left untouched.

### Verification: all three backends, both build systems

| Backend | CMake | Bazel |
|---|---|---|
| KINETO (default) | 9 passing, 1 legitimate skip (§2, thread-local opt-in) | same |
| ITT | 21 passing, 0 skipped | 21 passing, 0 skipped |
| NATIVE | 321 passing, 0 skipped | 321 passing, 0 skipped |

Invocation used (the `--profiler.X` flag is separate from the main dotted
token chain):
```
python3 setup.py config.build.test.native.ninja --project.profiler --profiler.itt
python3 setup.py config.build.test.native.ninja --project.profiler --profiler.native
bazel test //Library/Profiler/Testing/Cxx:ProfilerCxxTests --define=profiler_enable_itt=true
bazel test //Library/Profiler/Testing/Cxx:ProfilerCxxTests --define=profiler_type=native
```

## 2c. Thread-local opt-in path (fixed this session)

Followed up on §5 item 2: `KinetoIntegration.thread_local_participation_requires_opt_in`
was landing on the `events.empty()` `GTEST_SKIP()` path instead of actually
asserting `found_attached`/`found_detached`. Root-caused rather than assumed a
library bug — the attached-thread lambda in
`Testing/Cxx/TestRecordFunctionIntegration.cpp` called

```cpp
enableProfilerInChildThread();
RECORD_USER_SCOPE(attached_scope_name);   // no enclosing block
busy_wait_for(std::chrono::milliseconds(1));
disableProfilerInChildThread();
```

`RECORD_USER_SCOPE` expands to a bare local `profiler::RecordFunction guard(...)`
with no block of its own (`bespoke/common/record_function.h`), so `guard`'s
destructor — which records the end timestamp — only fires at the end of the
lambda, i.e. **after** `disableProfilerInChildThread()` had already popped this
thread's `KinetoThreadLocalState` off `thread_local_debug_info`. By the time
the destructor ran, `KinetoThreadLocalState::get(false)` returned `nullptr`,
`onFunctionExit`'s early-return left `event_->end_time_` at its
`numeric_limits<time_t>::min()` sentinel, `build_tree()` in `collection.cpp`
therefore never matched a `pop_event` for it (per its own comment: *"We use
min time to indicate the lack of a termination event ... we don't push to
`end_events_`"*), `Result::finished_` stayed `false`, and
`materializeOpEvents()` silently drops any event with `finished_ == false`.
Every other test in the same file already wraps its `RECORD_USER_SCOPE` in an
explicit `{ }` block for exactly this reason; this one didn't.

### Fix applied

`Testing/Cxx/TestRecordFunctionIntegration.cpp` — wrapped the attached
thread's `RECORD_USER_SCOPE`/`busy_wait_for` pair in its own block so the
guard destructs, and the event is marked finished, before
`disableProfilerInChildThread()` runs. Matches the pattern already used by
every other test in the file. Verified: CMake and Bazel both now report
`KinetoIntegration.thread_local_participation_requires_opt_in` as a genuine
pass (5x `--gtest_repeat` locally, no flake), not a skip — 10/10 tests
passing, 0 skipped, under both build systems.

### A second, separate bug found in the same investigation (not fixed)

`ProfilerStateBase::handle_` (`bespoke/common/orchestration/observer.h`/`.cpp`)
is a single non-thread-local field on the *shared* `KinetoThreadLocalState`
object, but `pushProfilingCallbacks`/`setCallbackHandle` is called once per
opted-in thread (main thread on `enableProfiler()`, each child thread on
`enableProfilerInChildThread()`). A child thread's opt-in clobbers the main
thread's `handle_`, tripping `SOFT_ASSERT(false, "ProfilerStateBase already
has a registered callback...")` and leaking the main thread's original
callback registration when the session is later disabled. Did not fire loudly
enough to fail the test above (the assert only logs), which is how it stayed
latent. Generic to `ProfilerStateBase`, so it would affect ITT/NVTX too if
either grows its own child-thread opt-in entry point (neither currently has
one — only Kineto declares `enableProfilerInChildThread` today) — the correct
fix is in `ProfilerStateBase` itself (e.g. a `flat_hash_map<tid, handle>`
guarded the same way `collection.cpp`'s `sub_queues_` already is), not a
Kineto-only patch. Tracked as a new follow-up in §5.

## 3. New test coverage

- `TestRecordFunctionIntegration.cpp` (new) — replaces the deleted files.
  Real, uncommented Kineto + ITT integration tests against the actual
  `enableProfiler`/`disableProfiler`/`prepareProfiler` API. Has no dependency
  on `native/session/profiler.h`, so it links and runs under the default
  Kineto backend without requiring the Native/XPlane subsystem to be compiled
  in.
- `TestHotspotReport.cpp` (new) — covers the hotspot/call-tree feature below.

Both CMake (`Testing/Cxx/CMakeLists.txt`) and Bazel
(`Testing/Cxx/BUILD.bazel`) test-file lists were updated in lockstep; both
build systems were built and tested clean (`ctest` and `bazel test`).

## 4. VTune-style hotspot / call-tree view (new)

PyTorch's own profiler has no top-down/bottom-up call-tree view built in — it
leaves that to Chrome-trace viewers or Kineto's own analysis tools. XSigma
now has a small first-party one, since RECORD_FUNCTION already builds an
in-memory, parent-linked call tree (`Result::parent_`/`children_` in
`collection.h`) with real start/end timestamps — everything a VTune-style
"Top-down Tree" / "Bottom-up Hotspots" view needs is already collected; it
just wasn't being surfaced.

New component: `bespoke/kineto/hotspot_report.{h,cpp}`,
`profiler::autograd::profiler_impl::hotspot_report`.

```cpp
auto result = profiler::autograd::profiler_impl::disableProfiler();
profiler::autograd::profiler_impl::hotspot_report report(*result);

std::cout << report.top_down_tree();        // indented call tree, self/total time, % of parent
std::cout << report.bottom_up_hotspots();    // functions merged across call sites, sorted by self time
report.call_stack_for("gemm_kernel");        // root -> ... -> leaf path, like VTune's Call Stack pane
```

Sample output (from a live run, `matmul` calling `gemm_kernel` and
`bias_add`):

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

`gemm_kernel` correctly surfaces as the top hotspot by self time despite
`matmul` having the larger total (inclusive) time — the same self-vs-total
distinction VTune's Bottom-up view is built around.

### What this is, precisely

This is **instrumentation-based**, not sampling-based: nodes come from
`RECORD_FUNCTION`/`RECORD_USER_SCOPE` scopes, not from periodic PC sampling
the way VTune's Hotspots analysis works. That means:

- Self/total time is exact for whatever call sites were annotated — no
  statistical noise, but also no visibility into code that wasn't annotated.
- The "call stack" a hotspot reports is the RecordFunction parent chain
  (named scopes), not a symbolized native instruction-pointer stack.
- This is intentionally the same tradeoff PyTorch's own `key_averages()`
  makes, extended with an explicit tree/self-time view PyTorch's C++ side
  doesn't provide out of the box.

### What would make it closer to real VTune / PyTorch's `with_stack=True`

`bespoke/common/unwind/` (a real, working fast unwinder, same one PyTorch
uses for `with_stack=True`) is already wired into `collection.cpp` when
`config.with_stack` is set — it captures raw `std::vector<void*>` return
addresses. A follow-up could feed those through `fast_symbolizer.h` and merge
symbolized native frames into `hotspot_report`'s call stacks, giving a true
PC-level breakdown for code that *isn't* explicitly annotated with
`RECORD_FUNCTION` — the same thing VTune gives you without any
instrumentation at all. Scoped out of this pass because it requires wiring a
symbolizer end-to-end and is independently useful/testable; tracked in §5.

## 5. Known follow-ups (not fixed this session)

Ordered by how much they block real usage:

1. **`RECORD_FUNCTION` is not called anywhere in XSigma's own compute code.**
   The engine now genuinely works (§2), but `Library/Vectorization`,
   `Library/Memory`, `Library/Parallel`, `Library/Core` never call it — a
   profiling session today captures whatever the caller manually wraps in
   `RECORD_USER_SCOPE`, and nothing else. This is the highest-value next
   step: instrument real kernel/allocator/task boundaries (see §6, Phase 2).
2. ~~Thread-local opt-in path~~ — **done, see §2c.** Root cause was a test bug
   (an unblocked `RECORD_USER_SCOPE` whose guard destructed after
   `disableProfilerInChildThread()` had already popped the thread's state),
   not a library bug; fixed and verified passing under both build systems.
3. **`ProfilerStateBase::handle_` is not per-thread** (found during §2c's
   investigation, not yet fixed) — a child thread's
   `enableProfilerInChildThread()` clobbers the main thread's callback-handle
   bookkeeping on the shared `KinetoThreadLocalState`, tripping a
   `SOFT_ASSERT` and leaking the main thread's original callback removal.
   Latent today (no test currently exercises overlapping main+child opt-in
   long enough to observe the leak), but real. Fix belongs in
   `ProfilerStateBase` (`bespoke/common/orchestration/observer.h`/`.cpp`) so
   it benefits Kineto/ITT/NVTX uniformly, e.g. keyed per-thread the same way
   `collection.cpp`'s `sub_queues_` already is.
4. **`TestKinetoShim.cpp`** — still `#if PROFILER_HAS_KINETO && 0` in its
   entirety (15 tests, zero running). Same "disabled and forgotten" pattern
   as the files fixed this session; not yet triaged for whether the shim API
   it tests still matches current `kineto_shim.h`.
5. **Native stack symbolization for `hotspot_report`** — see §4.
6. **Memory events aren't wired to the real allocators.**
   `startMemoryProfile()`/`stopMemoryProfile()`/`exportMemoryProfile()` exist
   in `profiler_kineto.h` but nothing in `Library/Memory`'s caching
   allocators reports into them; allocation events instead go to the
   unrelated `Library/Memory/profiler/unified_memory_stats.cpp`.
7. ~~Other `#if 0` blocks in `collection.cpp`~~ — **done, see §2b.** Full audit
   completed: every remaining `#if 0` across `bespoke/kineto`, `bespoke/itt`,
   `bespoke/base` (NVTX), and shared `bespoke/common` files was individually
   triaged. Two more real bugs found and fixed (ITT's missing orchestration
   API and its fmt link failure); everything else confirmed to be a
   legitimate, safely-degraded gap (Tensor/JIT/Python/Vulkan support that's
   genuinely absent, each with a working fallback).

## 6. Target architecture: one instrumentation point, pluggable backends

The pieces for a genuine multi-backend profiler already exist; they were
just never connected. TensorFlow's real plugin ABI is sitting unused in
`native/core/`:

- `profiler_interface` (`native/core/profiler_interface.h`) — `start()` /
  `stop()` / `collect_data(XSpace*)`. Exact match to TF's
  `tsl::profiler::ProfilerInterface`.
- `profiler_collection` (`native/core/profiler_collection.h`) — multiplexes N
  `profiler_interface` instances into one `XSpace`. TF's own design for
  combining a host tracer with a GPU tracer.
- `profiler_factory` / `register_profiler_factory` / `create_profilers`
  (`native/core/profiler_factory.h`) — pluggable tracer registry.

And PyTorch's own observer pattern is **already** proof that "one
instrumentation call site, N backends listening" works in this codebase:
`bespoke/itt/itt_observer.cpp` and `bespoke/base/nvtx_observer.cpp` are
already `RecordFunction`-callback-based observers, ported verbatim from
`torch/csrc/profiler/standalone/`.

```
   kernel/op code
        |
   RECORD_FUNCTION(...)              <- single instrumentation point (now verified working, §2)
        |
   RecordFunction callback dispatch  <- already multi-subscriber
        |
   +----+-------------+-------------+-------------+
   |    |             |             |             |
 Kineto  ITT          NVTX      XPlane-host    (future: XPlane-device via CUPTI)
 tracer  tracer       tracer     tracer
   |    |             |             |
   +----+-------------+-------------+
        each wrapped as a profiler_interface
                  |
          profiler_collection         <- existing TF-style multiplexer, currently unused
                  |
             merged x_space
                  |
     +------------+------------------+
 chrome_trace   hotspot_report      raw XSpace
   (.json)      (top-down/bottom-up) (TensorBoard-style consumers)
```

### Phase 0 — un-gate the build

Replace the mutually-exclusive `PROFILER_BACKEND` (`KINETO`\|`NATIVE`\|`ITT`)
selector with independent toggles: `PROFILER_ENABLE_KINETO`,
`PROFILER_ENABLE_ITT`, `PROFILER_ENABLE_XPLANE` (rename `NATIVE`→`XPLANE`),
each defaulting ON where its third-party dependency is available. Drop the
compile-time `#error` in `common/profiler_export.h` that currently enforces
"at most one." Always compile `common/`, `native/core/`,
`native/exporters/xplane/`, `native/tracing/` — no backend-specific
dependency, needed regardless of which tracers are active.

*Partially unblocked this session*: the specific link failure this gating
caused for RecordFunction integration tests was worked around by splitting
`TestProfilerBackendIntegration.cpp` so its Kineto content no longer needs
`native/session/profiler.h` to be compiled in. The underlying CMake
mutual-exclusivity itself is untouched — still the right Phase 0 for a real
multi-backend build.

### Phase 1 — dead code

Done for the RecordFunction-adjacent files (§1.1). `TestKinetoShim.cpp` still
open (§5.3).

### Phase 2 — instrument real call sites with `RECORD_FUNCTION`

The highest-value remaining phase (§5.1). Wrap kernel entry points in
`Library/Vectorization` (op identifier, shapes/dtype via
`RECORD_FUNCTION_WITH_INPUTS_OUTPUTS`), allocation/deallocation in
`Library/Memory`'s caching allocators (mirroring PyTorch's
`emplace_allocation_event`, feeding `startMemoryProfile()` for real), and
task submission in `Library/Parallel`'s thread pool (wiring the already-built
but unused `native/cpu/threadpool_listener.cpp`). Keep it at boundaries, not
inner loops — `RecordFunction`'s callback-empty fast path already keeps cost
near zero when no profiler is active.

### Phase 3 — make each backend a `profiler_interface`

- New: `native/core/kineto_tracer.{h,cpp}` — `start()`/`stop()` call
  `enableProfiler`/`disableProfiler`; `collect_data(XSpace*)` converts the
  returned events into `XPlane` rows (new `kineto_to_xplane.cpp` converter —
  the one genuinely new conversion logic needed).
- New: `native/core/itt_tracer.{h,cpp}` / `nvtx_tracer.{h,cpp}` — thin
  `profiler_interface` wrappers around the existing push-callback functions;
  `collect_data` is a no-op (VTune/Nsight consume ranges out-of-process).
- Existing, unchanged: `native/cpu/host_tracer.cpp` already implements
  `profiler_interface` correctly over `TraceMeRecorder`.
- Register all via `register_profiler_factory`, gated by which
  `PROFILER_ENABLE_*` toggles are compiled in.

### Phase 4 — one user-facing API

Replace the remaining fragmented session concepts with one builder-pattern
class matching this repo's convention:

```cpp
class profiler_session { /* start(), stop(), collected XSpace, exporters */ };
class profiler_session_builder {
    profiler_session_builder& with_activities(std::set<activity_enum>);  // CPU, CUDA, XPLANE_HOST, ITT, NVTX
    profiler_session_builder& with_record_shapes(bool = true);
    profiler_session_builder& with_stack(bool = true);
    profiler_session_builder& with_memory(bool = true);
    profiler_session_builder& with_schedule(size_t wait, size_t warmup, size_t active, size_t repeat = 0);
    std::unique_ptr<profiler_session> build();
};
```

`with_activities` selects which registered factories run →
`create_profilers(options)` → `profiler_collection` → the existing
`profiler_controller` state machine → merged `x_space`. Exporters off that
one `XSpace`: `export_chrome_trace()` (reuse `chrome_trace_exporter.cpp`,
already exists), `hotspot_report` (§4, already built), and raw `XSpace`
access for TensorBoard-style consumers.

## 7. Suggested order for future sessions

1. ~~Phase 2 (instrument real call sites)~~ — **done for Vectorization/Parallel/
   Memory-Metal this session, see §8.** CUDA/HIP allocator instrumentation still
   open (§8's "not done").
2. ~~§5.2 (thread-local opt-in path)~~ — **done, see §2c.**
3. §5.3 (`ProfilerStateBase::handle_` not per-thread) — small, same
   debugging pattern as §2/§2c, found but not fixed while root-causing §2c.
4. Phase 0 (CMake un-gating) — needed before Phase 3/4 can be built for real,
   but not blocking Phase 2.
5. Phase 3 → Phase 4, in order — each is mostly wiring of already-correct
   vendored code into the shared `profiler_interface` ABI.
6. §4's native-stack symbolization, once there's real instrumented code
   (Phase 2) to point it at.
7. Validate §2a's CUDA path on an actual CUDA-equipped machine
   (`MEMORY_GPU_BACKEND=cuda`) — it's implemented and reasoned through
   carefully, but could not be compile-tested in this environment. §8's new
   CUDA/HIP memory-instrumentation follow-up should be validated at the same
   time, on the same machine.
8. §8's four deferred code-review findings (KinetoEvent numeric-vs-real
   activityType type unification, `step_info` ordering under multi-thread
   `adjust_profiler_step`, two dropped test cases, the dormant
   `AppendOnlyList` bounds-check) — independent small cleanups, no urgency
   ordering relative to each other.

## 8. Phase 2 implementation: real call-site instrumentation (this session)

Implemented the top item from the previous session's ordering: wired
`RECORD_USER_SCOPE`/`profiler::report_memory_usage` into real compute/task/
allocation boundaries in three consuming libraries, closing §5 item 1 for
everything except CUDA/HIP.

### New: a backend-agnostic instrumentation shim

`RECORD_FUNCTION`/`RECORD_USER_SCOPE` (`bespoke/common/record_function.h`) and
`MemoryReportingInfoBase::reportMemoryUsage` (`bespoke/common/orchestration/
observer.h`) only exist when Profiler is built with `PROFILER_ENABLE_KINETO` or
`PROFILER_ENABLE_ITT` — under `PROFILER_BACKEND=NATIVE`, `bespoke/common/` is
excluded from the library entirely (confirmed via `Library/Profiler/
CMakeLists.txt`'s source-glob gating). A consuming library calling those
macros/APIs directly would fail to link under the Native backend. New
**`Library/Profiler/common/instrumentation.{h,cpp}`** (in `common/`, always
compiled regardless of backend) closes that gap:

- `#if PROFILER_HAS_KINETO || PROFILER_HAS_ITT`: includes the real
  `record_function.h`, so `RECORD_USER_SCOPE` is the genuine macro.
- Otherwise: defines `RECORD_USER_SCOPE(fn)` as a true no-op
  (`do { (void)(fn); } while (0)`), so callers don't need their own
  `PROFILER_HAS_*` guards around the macro itself.
- `profiler::report_memory_usage(ptr, alloc_size, total_allocated,
  total_reserved, device_type, device_index)` — new free function, mirrors
  PyTorch's `c10::reportMemoryUsageToProfiler`. Looks up the active
  `ProfilerStateBase` and forwards to its `reportMemoryUsage()` override when a
  session has `profile_memory` enabled; a real no-op (early return) otherwise,
  including under the Native backend where the whole call compiles to nothing.

### Vectorization

`Library/Vectorization/terminals/tensor.h` — `RECORD_USER_SCOPE` added to the
4 non-`noexcept` `tensor<T>::operator=`/constructor overloads that funnel every
expression assignment through `expressions_evaluator::run` (the single
dispatch point identified by this session's kernel-entry-point survey). The
5th overload, `operator=(T2 value) noexcept` (scalar fill), is **deliberately
not instrumented** — see "Fixed during review" below.

Build wiring: `_vec_has_profiler` (CMake, `if(TARGET Profiler::Profiler)`,
mirrors the existing `_vec_has_memory`/`_vec_has_logging` pattern) →
`VECTORIZATION_HAS_PROFILER` compile definition; `//Library/Profiler:Profiler`
added to both Bazel `deps` lists (there are two — `vectorization_lib`'s
internal target and its select()-suffixed sibling — both needed updating).

### Parallel

`Library/Parallel/std_thread/parallel_thread_pool.cpp` —
`RECORD_USER_SCOPE("parallel::thread_pool::run_job")` wraps the `function()`
call inside `run_job()`'s existing `try`/`catch` (the actual task-execution
boundary, not the enqueue/`do_job()` boundary — enqueue is far higher
frequency and lower value per the plan's "boundaries, not inner loops"
guidance). The already-built-but-unused `native/cpu/threadpool_listener.cpp`
mentioned in the original Phase 2 sketch was **not** wired up: it depends on
`native/tracing/tracing.h`, which — like `bespoke/common/` — is excluded from
the library entirely outside `PROFILER_BACKEND=NATIVE` (the inverse gating
problem from Vectorization's). Using it unconditionally would break the
default Kineto build; using the new `RECORD_USER_SCOPE` shim instead gives
task-boundary visibility under every backend today. Revisit
`threadpool_listener` once Phase 0 makes `native/` backend-independent.

Build wiring: `PARALLEL_HAS_PROFILER` (CMake `if(TARGET Profiler::Profiler)`;
Bazel: unconditional `//Library/Profiler:Profiler` dep + unconditional
`PARALLEL_HAS_PROFILER=1` define, matching Bazel's existing unconditional-dep
style for Parallel's other libs, as opposed to CMake's graceful-degrade style).

### Memory

New `profiler::report_memory_usage()` (above) wired into
`Library/Memory/gpu/metal/metal_caching_allocator.mm`'s `allocate()`/
`deallocate()` — the one GPU backend actually buildable/testable in this
session's sandbox (macOS, no CUDA/HIP toolkit). **CUDA/HIP
(`cuda_caching_allocator.cpp`) are not yet instrumented** — same reasoning as
§2a: can't be compile-tested here, and after this session's review (below)
found real correctness bugs in the *tested* Metal path from writing it too
quickly, doing the same for CUDA/HIP blind (no compiler, no test run) was
judged too risky to be worth it this session. Do CUDA/HIP next, on a
CUDA/HIP-equipped machine, informed by the bugs found and fixed here.

Build wiring: `MEMORY_ENABLE_PROFILER` option + `MEMORY_HAS_PROFILER_BUILD`
gate (CMake, mirrors the existing `MEMORY_ENABLE_LOGGING` pattern exactly,
default ON, graceful no-op when Profiler isn't present — e.g. still OFF in
`--project.memory`'s intentionally-minimal single-library mode); Bazel: two
separate `deps` lists needed the dependency (`memory_lib` *and*
`memory_metal_objcxx`, a second target that deliberately avoids depending on
`memory_lib` to sidestep an unrelated cycle — easy to miss, caught by actually
building `--define=memory_enable_metal=true` rather than assuming the first
`deps` edit covered both).

**A real, unrelated Bazel-only bug fixed as a prerequisite**: Profiler's
`BUILD.bazel` unconditionally depended on `//Library/Core:core_lib`, but
nothing in Profiler's source (checked by grep and by a Core-less build)
actually uses anything from Core, and CMake's `CMakeLists.txt` never linked
Core either. Left in place, it would have created a
`Memory -> Profiler -> Core -> Memory` cycle the moment Memory took its new
Profiler dependency (`Core` already depends on `Memory` in both build
systems). Removed; verified `//Library/Profiler:Profiler` and its full test
suite still build clean without it.

**CMake library-order change**: `Profiler` moved from last to second (right
after `Logging`) in the root `CMakeLists.txt`'s `_quarisma_lib_order`/
`add_subdirectory` sequence, since Memory/Vectorization/Parallel's new
`if(TARGET Profiler::Profiler)` gates only succeed if Profiler's
`add_subdirectory` already ran. Profiler has zero CMake dependency on any
other Quarisma library (confirmed: only third-party `Fmt`/`Kineto`/`Itt`/
`CUDA` in `PROFILER_DEPENDENCY_LIBS`), so this is safe in every direction.

### Fixed during review

A parallel multi-angle `/code-review` pass (5 independent reviewers) on the
full diff found two real bugs in this session's new code, fixed immediately:

1. **`tensor::operator=(T2) noexcept` + `RECORD_USER_SCOPE`** — `RecordFunction`
   construction/dispatch is not `noexcept` (e.g. `std::vector::resize`/
   `std::string` construction in `record_function.cpp` can throw
   `std::bad_alloc`); an exception escaping a `noexcept` function calls
   `std::terminate()`. The file's own comment on a neighboring constructor
   already documents this exact class of hazard for allocation failures — this
   session's own new instrumentation reintroduced it on the scalar-fill
   overload. Fixed by simply not instrumenting that one overload (documented
   inline); the other 4 non-`noexcept` overloads are correctly instrumented
   and unaffected.
2. **`metal_caching_allocator::deallocate()`'s profiler report was wrong on
   three counts** — used the caller-supplied `size` parameter directly, but
   `Impl::deallocate()` ignores that parameter entirely (its own signature
   names it `/*size*/`) and looks up the real freed size internally, and at
   least one real call path (`Library/Memory/allocator.h:194`,
   `allocator<T>::free()`) always passes a literal `0`; hardcoded
   `device_index=0` instead of the real `impl_->device()` `allocate()` already
   used (dormant today only because `Impl`'s constructor currently enforces
   `device == 0`); and reported a deallocation event even when `ptr == nullptr`
   (`Impl::deallocate` treats null as a no-op, but the profiler call didn't
   check). Fixed by computing the reported delta from a real
   before/after `impl_->stats()` snapshot instead of trusting the caller's
   `size`, sharing one small `report_alloc_delta()` helper between `allocate()`
   and `deallocate()` (also resolves the size/device-index duplication the
   same reviewers flagged as a simplification opportunity), and adding the
   missing `ptr != nullptr` guard. Re-verified: all 19
   `MetalCachingAllocator`/`MetalBufferAllocator` tests still pass under both
   build systems after the fix, including the double-free/foreign-pointer
   edge cases.
3. **`KinetoEvent::activityType()`'s non-Kineto fallback returned `0`**,
   colliding with the real, common `libkineto::ActivityType::CPU_OP` (also
   `0`) — flagged independently by 3 of the 5 reviewers. Not new to this
   session (introduced in the earlier §2b work), but cheap and unambiguous to
   fix while in the area: now returns `27`, mirroring libkineto's own
   `ActivityType::ENUM_COUNT` sentinel (documented in `ActivityType.h` as
   "not used for any profiling logic" — i.e. already the library's own
   designated out-of-band value).

### Found by review, deferred (not this session's new code, pre-existing from §2b)

The same review pass surfaced four more findings in the already-staged §2b
work (predating this session's Phase 2 changes) — real, but each needs either
deeper investigation or carries more risk than the size of the fix justifies
to take on blind at the end of an already-large session. Left as follow-ups:

- `step_info`'s ordering assumption in `TorchOpStorage::materialize()`
  (`collection.cpp`) — populated per-subqueue in `flat_hash_map` iteration
  order (not chronological), but its consumer's `adjust_profiler_step` walk
  assumes global time order. Only reachable with `"ProfilerStep#"`
  annotations from more than one thread; single-threaded step annotation
  (the common case) is unaffected.
- Two test cases dropped during the earlier `TestKinetoProfiler.cpp`/
  `TestProfilerBackendIntegration.cpp` cleanup that the plan doc's §1.1
  described as "100% dead"/"exact duplicates" but weren't: `RecordDebugHandles
  .Basic` (real, live coverage of `RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_AND_INPUTS`
  /`debugHandle()`) and `write_chrome_trace_rejects_empty_path` (an
  empty-path-rejection case not actually covered by
  `TestProfilerChromeTraceHierarchical.cpp`). Both APIs are still shipped;
  neither currently has any test coverage.
- `AppendOnlyList::Iterator`'s bounds check (`containers.h`) is commented out;
  the newly-live `getIValueGenerator()`'s scalar-tag path dereferences without
  an `exhausted()` guard the tensor-tag path has. Currently unreachable (the
  live `InputOutputEncoder::push()` stub keeps `tags_`/`ivalues_` in lockstep
  today) — latent, not exploitable yet.
