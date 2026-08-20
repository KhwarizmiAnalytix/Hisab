# Profiler Session Summary — 2026-08-19

Point-in-time handoff for picking up this work in a **new session** with no
memory of this conversation. For living/ongoing state see
`Docs/profiler/PROFILER_MULTI_BACKEND_PLAN.md` (architecture + history,
updated throughout this session) and `Docs/profiler/PROFILER_DEPENDENCIES.md`
(per-backend dependency reference, written this session). This file is a
snapshot, not maintained going forward — don't edit it in a future session,
write a new dated one instead if another full handoff is needed.

**Nothing in this session is committed.** `git status` shows ~61 modified/
added/deleted files, all uncommitted, on branch `memory_optimisation`. Some of
the diff (marked below) predates this session — it was already staged when
this session started and this session verified/built on top of it rather than
authoring it from scratch.

---

## 1. What happened, in order

### A. Verified and fixed carried-over work (§2b/§5.2 of the plan doc)
A prior session had left an uncommitted ITT/NVTX `#if 0` audit in the working
tree (§2b of the plan doc: two real bugs found — ITT's missing
`enableProfiler`/`disableProfiler` API and an fmt link failure). This session:
- Rebuilt and retested all three backends (Kineto/ITT/Native) × both build
  systems (CMake/Bazel) to confirm that carried-over work was solid. It was.
- Root-caused and fixed `KinetoIntegration.thread_local_participation_
  requires_opt_in`, a test that had been permanently `GTEST_SKIP()`ing. Root
  cause: the test's `RECORD_USER_SCOPE` wasn't wrapped in its own block, so
  the guard destructed *after* `disableProfilerInChildThread()` had already
  popped the thread's profiler state — a **test bug**, not a library bug.
  Fixed in `Testing/Cxx/TestRecordFunctionIntegration.cpp`. Documented as
  plan-doc §2c. A **second, related bug was found but not fixed**:
  `ProfilerStateBase::handle_` isn't per-thread, so overlapping main+child
  thread opt-in can clobber callback bookkeeping — tracked as plan-doc §5.3,
  still open.

### B. Phase 2 — real `RECORD_FUNCTION` instrumentation
Implemented the plan's highest-priority open item: wired real compute/task/
allocation boundaries into the (now-verified-working) profiler engine.
- **New**: `Library/Profiler/common/instrumentation.h/.cpp` — a
  backend-agnostic shim. `RECORD_USER_SCOPE` is a real call under Kineto/ITT
  and a true no-op under the Native backend (which doesn't compile
  `bespoke/common/` at all), so consuming libraries don't need their own
  `PROFILER_HAS_*` guards. Also added `profiler::report_memory_usage()`,
  mirroring PyTorch's `c10::reportMemoryUsageToProfiler`.
- **Vectorization**: `tensor<T>::operator=`/constructors (the single
  dispatch point for every expression assignment) — 4 of 5 overloads
  instrumented; the 5th (`operator=(T2) noexcept`) deliberately left
  uninstrumented, see bug fix below.
- **Parallel**: `parallel_thread_pool.cpp`'s `run_job()` — the actual
  task-execution boundary (not `do_job()`'s enqueue, which is far
  higher-frequency/lower-value).
- **Memory**: `metal_caching_allocator.mm`'s `allocate()`/`deallocate()` —
  the one GPU backend actually buildable/testable in this sandbox (macOS, no
  CUDA/HIP toolkit). **CUDA/HIP allocators are NOT instrumented** — needs a
  CUDA/HIP-equipped machine, tracked as an open item.
- **Build wiring**: all three libraries gained an optional
  `<LIB>_ENABLE_PROFILER`/`<LIB>_HAS_PROFILER`-style CMake gate (mirrors
  Memory's existing `MEMORY_ENABLE_LOGGING` pattern) plus Bazel `deps`/
  `defines`. Required moving `Profiler` from last to second (right after
  `Logging`) in the root `CMakeLists.txt`'s `_quarisma_lib_order`, since the
  new `if(TARGET Profiler::Profiler)` gates only succeed if Profiler's
  `add_subdirectory` already ran.
- **Prerequisite fix**: removed a vestigial `//Library/Core:core_lib` Bazel
  dependency from `Library/Profiler/BUILD.bazel` — nothing in Profiler
  actually used it, and left in place it would have created a
  `Memory → Profiler → Core → Memory` cycle once Memory took its new
  Profiler dependency.

### C. Bugs found and fixed via a 5-way parallel `/code-review`
1. **`tensor::operator=(T2) noexcept` + `RECORD_USER_SCOPE`** —
   `RecordFunction` construction can throw (`std::bad_alloc`); an exception
   escaping a `noexcept` function calls `std::terminate()`. Fixed by simply
   not instrumenting that one overload.
2. **`metal_caching_allocator::deallocate()`'s profiler report was wrong on
   three counts** — used the caller-supplied (often-stale/zero) `size`
   instead of the real freed size, hardcoded `device_index=0` instead of the
   real device, and reported events even for `ptr == nullptr`. Fixed by
   computing the reported delta from a real before/after `impl_->stats()`
   snapshot via one shared `report_alloc_delta()` helper.
   **Known follow-up (not fixed)**: an independent later review found this
   before/after-snapshot approach has its own race — concurrent
   allocate/deallocate calls on the *same* allocator instance can corrupt the
   reported delta, since the two `stats()` calls aren't atomic with the
   allocation itself. Also flagged: the `stats()` calls pay for a full
   locked block-scan (`inactive_split_bytes`) that's never even read by the
   reporting path — real, currently-unaddressed efficiency waste.
3. **`KinetoEvent::activityType()`'s non-Kineto fallback returned `0`**,
   colliding with the real, common `libkineto::ActivityType::CPU_OP` (also
   `0`). Now returns a named `constexpr kActivityTypeUnavailable = 27`
   (mirrors libkineto's own `ActivityType::ENUM_COUNT` sentinel).

### D. Full removal of PyTorch-inherited dead code from `bespoke/`
User request: "clean the profiler from files/functions not related to my
work, mainly inherited from pytorch." Two rounds:

**Round 1 — TensorImpl specifically.** Deleted `common/TensorImpl.h`'s
~3500-line dead PyTorch tensor-impl port (kept the tiny stub `Storage`/
`TensorImpl`/`Tensor` structs everything else needed — then in round 2,
deleted those too once nothing referenced them), `bespoke/common/ivalue.h`
(entirely `#if 0`'d, 1800 lines), `bespoke/kineto/profiler_python.{h,cpp}`
(entirely `#if 0`'d). Simplified `util.cpp`/`nvtx_observer.cpp`/
`execution_trace_observer.cpp`/`collection.{h,cpp}`/`data_flow.h` down to
their already-working "XSigma has no tensor type" fallback bodies, dropping
the dead real-PyTorch-tensor-code branches. Retyped `TensorImplAddress`
(`data_flow.h`) from `strong::type<const profiler::TensorImpl*, ...>` to
`strong::type<const void*, ...>` — it was already only used as an opaque,
never-dereferenced identity key, so this is a pure type-erasure with zero
behavior change, and let `TensorImpl.h` be deleted outright.

**Round 2 — everything else PyTorch-inherited-and-irrelevant.** A fresh
survey found every remaining `#if 0` block across `bespoke/` and every
whole-file/subsystem with zero live callers. Removed:
- **Whole files/subsystems** (~1336 lines): `bespoke/itt/itt.h` (dead Python
  bindings decl, never included anywhere), `bespoke/common/
  combined_traceback.{h,cpp}` (entirely `#if 0`'d JIT/Python frame capture),
  `bespoke/common/standalone/execution_trace_observer.{h,cpp}` (1028 lines,
  confirmed never invoked from anywhere in the tree — a self-contained,
  always-compiled-but-dead PyTorch execution-trace-export feature).
- **Every remaining `#if 0` in `bespoke/`** (now zero total): Vulkan shader
  materialization, JIT call-stack/module-hierarchy capture, TF32 precision
  query, PyCall/PyCCall `toString()`/`Result::name()` arms, PythonGC
  draining, TorchScript `FunctionSchema` name/args/overload lookups, NCCL
  metadata real-impl, dead `LOG(INFO)` calls in `kineto_shim.cpp`/
  `kineto_client_interface.cpp`, Python nn.Module/optim.Optimizer tracking
  in `data_flow.cpp`.
- **Cascading orphans removed as a consequence**: `collection.h`'s
  `PyModuleSelf`/`PyModuleCls`/`NNModuleInfo`/`OptimizerInfo`/
  `PyExtraFieldsBase`/`ExtraFields<PyCall>`/`ExtraFields<PyCCall>`/
  `PyFrameState`; `util.{h,cpp}`'s `prepareCallstack`/`callstackStr`/
  `FileLineFunc`/`jit::StackEntry`, `format_list`,
  `checkFunctionInputsForLogging`/`checkFunctionOutputsForLogging`; every
  `USE_DISTRIBUTED`/NCCL-metadata code path (confirmed **never defined** by
  any XSigma CMakeLists.txt/BUILD.bazel anywhere in the repo — Library/
  Parallel is a local thread-pool library, not distributed/multi-node).
- **Applied one cheap fix from the follow-up review**: hoisted the
  activity-type sentinel (item C.3 above) into a named constant.
- **Deliberately left untouched** (per the survey's own caution — either
  load-bearing or a separate design call, not requested):
  `bespoke/common/unwind/` (a real, valuable ~3300-line native ELF/DWARF
  stack unwinder — currently orphaned since its only caller,
  `combined_traceback.cpp`, was just deleted, but genuinely reusable
  infrastructure for a future real `with_stack=True` implementation, not
  PyTorch cruft); `python_tracer.{h,cpp}`'s no-op registration plumbing
  (working as designed, just has nothing plugged into it);
  `Vulkan`/`PythonGC` `EventType` enum values (kept in the enum, excluded
  from the live variant — touching the enum itself is riskier/deeper than a
  `#if 0` removal); `kineto_client_interface.cpp`'s on-demand-profiling
  static registration (genuinely load-bearing — lets an external controller
  process trigger a trace via `libkineto::api().registerClient()` at
  library-load time).

Verified after every batch (rebuilt+retested ~10 times through this round),
then ran an 8-way parallel `/code-review`. **Zero issues found in the
dead-code removal itself** — every finding traced to either confirming the
removal was safe, or to pre-existing issues in code from earlier in this
session (listed under §4 below).

### E. Documentation written this session
- `Docs/profiler/PROFILER_MULTI_BACKEND_PLAN.md` — updated throughout (§2c,
  §5.3, §7 reordering, §8 new "Phase 2 implementation" section covering B/C
  above in full detail). This is the **primary living doc** — read it first
  in a new session.
- `Docs/profiler/PROFILER_DEPENDENCIES.md` — new. Per-backend (Kineto/ITT/
  NVTX/Native) dependency reference table + file:line citations, GPU support
  specifics (CUDA-only in Profiler itself, two tracing paths), and a
  rigorously-verified (not assumed) cross-language section: **zero Python or
  C bindings exist anywhere in this repo today** — no pybind11, no
  `extern "C"` public API, everything is C++-only. Includes a short "what
  would be needed" note for future Python/C interop, explicitly not a
  roadmap commitment.
- This file.

---

## 2. Verification status (all as of end of session)

| Config | CMake | Bazel |
|---|---|---|
| Kineto (default) | ✅ 7/7 suites, 10/10 Profiler tests | ✅ 212 passing, 8 expected GPU skips, 0 failing |
| ITT | ✅ 21/21 Profiler tests | ✅ 21/21 |
| Native | ✅ 321/321 Profiler tests | ✅ 321/321 |
| Metal GPU config | ✅ 7/7 suites incl. 19/19 Metal allocator tests | ✅ 19/19 |

`lintrunner`/clang-tidy clean on every touched file, except one **pre-existing,
unrelated** clang-tidy error (`collection.cpp`'s `rfind` vs `starts_with` at
an untouched line — confirmed via `git diff` to predate this session) and one
**pre-existing, unrelated** directory-local clang-tidy config gap
(`bespoke/kineto/.clang-tidy` disables all `bugprone-*` with no
`InheritParentConfig`, so lintrunner reports "no checks enabled" for that
whole directory — a known repo config issue, not something this session
caused or needs to fix).

One flaky test observed twice, confirmed unrelated to any session change
(passes on rerun, pure timing sensitivity): `ParallelCxxTests` (thread-pool
startup timing) and `ZabrVsNn.nn_is_faster_than_optimizer` (Models,
performance-comparison assertion).

---

## 3. Open items for the next session

Ordered roughly by how the plan doc's §7 currently ranks them, plus items
this session's reviews surfaced:

1. **CUDA/HIP memory instrumentation** — `cuda_caching_allocator.cpp` needs
   the same `report_memory_usage()` wiring Metal got, but requires a
   CUDA/HIP-equipped machine to compile-test (this sandbox has neither).
2. **`ProfilerStateBase::handle_` not per-thread** (§5.3) — small, same
   debugging pattern as the thread-local-opt-in fix, found but not fixed.
3. **Metal allocator race condition** (found in this session's follow-up
   review, not fixed) — `allocate()`/`deallocate()`'s before/after
   `stats()` snapshot isn't atomic with the allocation itself; concurrent
   calls on the same allocator instance can corrupt the reported delta.
   Also: the `stats()` calls pay for a scan whose result is never used.
4. **`hotspot_report.cpp`** (built earlier this session, several
   independent-review findings never acted on): duplicates duration-
   formatting/tree-rendering/top-N-ranking logic already in
   `native/session/profiler_report.cpp`; unbounded recursion in
   `accumulate()`/`render_tree()` (stack-overflow risk on deep call trees —
   plausible now that `run_job()` wraps every Parallel task in
   `RECORD_USER_SCOPE`); redundant duplicate self/total-time computation
   between the two tree walks.
5. **`cuda.cpp`'s `cudaCheck()`** — the failure-reporting statement
   (`PROFILER_CHECK(false, ...)`) is still commented out even though the
   file is now live under `PROFILER_HAS_CUDA=1`, so a failing CUDA call is
   silently swallowed instead of surfaced.
6. **Dropped test coverage, no replacement**: `RecordDebugHandles.Basic`
   (debug-handle propagation through `RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_
   AND_INPUTS`/`reportBackendEventToActiveKinetoProfiler`) was deleted along
   with `TestKinetoProfiler.cpp` in the *original* (pre-this-session) test
   cleanup — the API it tested is still live, has zero coverage now.
7. **`TestHotspotReport.cpp` silently compiles empty under ITT-only
   builds** — it's listed in the ITT Bazel/CMake test set but its body is
   gated `#if PROFILER_HAS_KINETO` only (hardcodes `ProfilerState::KINETO`,
   not a generic config). Considered fixing this session, backed off because
   verifying `enableProfiler`'s behavior with `ProfilerState::KINETO` under
   an ITT-only build (no crash?) wasn't done — would need the same
   dual-config treatment `TestRecordFunctionIntegration.cpp` already has.
8. **Phase 0 (CMake un-gating)** — replace the mutually-exclusive
   `PROFILER_BACKEND` selector with independent `PROFILER_ENABLE_*` toggles
   so Kineto/ITT/Native could build together. Not started; blocks Phase 3/4.
9. **Judgment calls flagged, not decided**: wiring `bespoke/common/unwind/`
   into a real `with_stack=True` implementation (currently orphaned but
   valuable); whether to keep or fully remove the `python_tracer.{h,cpp}`
   no-op plumbing and the `Vulkan`/`PythonGC` `EventType` enum values.
10. **Validate the CUDA path generally** (§2a) on real CUDA hardware — the
    fallback-event code in `cuda.cpp` was reasoned through carefully but
    never compile-tested; this is the same underlying constraint as item 1.

---

## 4. Things to know before touching this code again

- **Nothing is committed.** Confirm with the user before committing —
  CLAUDE.md's workflow only commits on explicit request.
- **Kineto's `bespoke/common/` is NOT compiled under the Native backend.**
  This is why `common/instrumentation.h` exists — any code that wants
  `RECORD_FUNCTION`/`RECORD_USER_SCOPE` to be safely callable regardless of
  which `PROFILER_BACKEND` is selected must go through that shim, not
  include `bespoke/common/record_function.h` directly.
  `bespoke/common/orchestration/observer.h`'s `MemoryReportingInfoBase` has
  the same constraint — that's why `report_memory_usage()` is in the same
  shim. The dependency chain is **Vectorization/Parallel/Memory → Profiler
  (optional, `TARGET Profiler::Profiler`-gated) → nothing else in this
  project**, and Profiler must build before those three in
  `_quarisma_lib_order`.
- **`RECORD_USER_SCOPE`/`RECORD_FUNCTION_WITH_SCOPE` declare a local
  variable directly in the caller's scope** (not wrapped in their own
  `{ }`) — this is exactly what caused §2c's thread-local-opt-in bug. Any
  new call site must be wrapped in an explicit block if the guard's
  destructor timing matters relative to other statements in the same scope.
- **`bespoke/common/collection.cpp`'s `TorchOpStorage::materialize()`,
  `InputOutputEncoder::isSupportedScalarList`, and `getIValueGenerator`
  went from permanent no-ops to real, live implementations this session**
  (§2 of the plan doc, the very first fix). Any new bug in this reactivated
  logic is now exercised for the first time at runtime — flagged as the
  single highest-risk semantic change of the whole session by one reviewer,
  even though it introduced no compile break and all tests pass.
- **This sandbox has no CUDA/HIP toolkit** (macOS/arm64) — every CUDA/HIP
  code path in this session's work was reasoned through carefully but is
  compile-untested. Treat it accordingly; don't claim it "works," only that
  it's "implemented and inert-when-absent, verify on real hardware."
- **`Library/Profiler` no longer has any TensorImpl/Tensor/IValue-real-impl
  code, any Python bindings, any JIT/TorchScript code, or any distributed/
  NCCL code.** If you're tempted to "restore" something from git history
  because it looks like a PyTorch feature is missing, check this file and
  the plan doc's §8 first — it was very likely already confirmed dead and
  removed on purpose, not accidentally lost.
