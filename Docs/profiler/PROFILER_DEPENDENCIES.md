# XSigma Profiler: Per-Backend Dependencies & Cross-Language Support

Status: living reference. Written 2026-08-19. Companion to
`Docs/profiler/PROFILER_MULTI_BACKEND_PLAN.md` (the architecture/history doc) —
this file is a flat dependency reference, not a narrative.

## 1. Backend overview

`Library/Profiler` ships **four independently-gated "activity sources"** that
share instrumentation (`RECORD_FUNCTION`/`RECORD_USER_SCOPE`) but differ in
what they link and what tree they compile from:

| Backend | CMake flag | Compile-time macro | Source tree | Third-party dep |
|---|---|---|---|---|
| Kineto | `PROFILER_ENABLE_KINETO` (default ON) | `PROFILER_HAS_KINETO` | `bespoke/` | `ThirdParty/kineto` (vendored) |
| ITT | `PROFILER_ENABLE_ITT` | `PROFILER_HAS_ITT` | `bespoke/` (minus Python/JIT-only bits) | `ThirdParty/ittapi` (vendored) |
| NVTX | *(not independently selectable — a `ProfilerState` inside Kineto/ITT)* | n/a | `bespoke/base/nvtx_observer.cpp` | CUDA Toolkit's `nvToolsExt` (optional) |
| Native/XPlane | `PROFILER_ENABLE_NATIVE_PROFILER` | `PROFILER_HAS_NATIVE` | `native/` | none (standalone) |

`PROFILER_BACKEND` (`KINETO`\|`ITT`\|`NATIVE`) is still the single CMake
selector today — the three are mutually exclusive at build time (Phase 0 of
the multi-backend plan would lift this; not done yet). NVTX rides along
inside whichever of Kineto/ITT is active, since it's implemented as another
`ProfilerState` value in the shared orchestration layer, not a separate
source tree.

---

## 2. Kineto backend

**Enables**: full CPU + GPU activity tracing, the same engine PyTorch itself
uses (`torch.profiler`).

**Depends on**:
- `ThirdParty/kineto` (Meta's libkineto), vendored as a git submodule. CMake
  target `Kineto::kineto`, built via `add_third_party_library(kineto ...)`
  (`Library/Profiler/CMakeLists.txt:239-260`). Bazel: `local_repository(name
  = "kineto", path = "ThirdParty/kineto")` (`WORKSPACE.bazel:149-150`), linked
  unconditionally in `Library/Profiler/BUILD.bazel`'s default `deps`.
- `ThirdParty/fmt` (also vendored) — libkineto links it as a compiled
  library as a side effect of its own build (`FMT_SOURCE_DIR` reuse,
  `CMakeLists.txt:236`).
- **Optional GPU tracing** — CUPTI, via the CUDA Toolkit. Gated by
  `MEMORY_GPU_BACKEND=cuda` (a *Memory*-owned variable Profiler reads
  directly — a known cross-library coupling, see
  `PROFILER_MULTI_BACKEND_PLAN.md` §2a). When set, libkineto is built with
  `LIBKINETO_NOCUPTI=OFF` (`CMakeLists.txt:198-202`) and Profiler separately
  runs its own `find_package(CUDAToolkit QUIET)`
  (`CMakeLists.txt:209-227`) to set `PROFILER_HAS_CUDA` and link
  `CUDA::cudart` (+ `CUDA::nvToolsExt` if present). This is the flag that
  gates `bespoke/base/cuda.cpp` (see §5).

**Compiles**: `bespoke/base/*`, `bespoke/kineto/*`, `bespoke/common/*` (minus
`bespoke/common/unwind/*.cpp`, explicitly excluded —
`CMakeLists.txt:341-343`) — the CMake glob at `CMakeLists.txt:326-345`.

**Public API** (`bespoke/kineto/profiler_kineto.h`):
```
enableProfiler(config, activities, scopes)
disableProfiler() -> std::unique_ptr<ProfilerResult>
prepareProfiler(config, activities)
enableProfilerInChildThread() / disableProfilerInChildThread()
startMemoryProfile() / stopMemoryProfile() / exportMemoryProfile(path)
```
Plus `bespoke/kineto/hotspot_report.h` (this session's VTune-style
top-down/bottom-up view over the same `ProfilerResult`) and
`native/exporters/chrome_trace_exporter.*`-equivalent JSON export.

---

## 3. ITT backend

**Enables**: Intel VTune Amplifier integration (task/range markers VTune's
UI understands directly).

**Depends on**:
- `ThirdParty/ittapi` (Intel's ITT API), vendored. CMake: `find_package(ITT)`
  (`CMakeLists.txt:293`) resolves via `Cmake/packages/FindITT.cmake`, which
  points `ITT_ROOT` at `${PROJECT_SOURCE_DIR}/ThirdParty/ittapi`
  (`FindITT.cmake:27-28`) — i.e. it *looks* like a system `find_package`
  call but always resolves to the vendored copy, never a system install.
  Bazel: `local_repository(name = "ittapi", path = "ThirdParty/ittapi")`
  (`WORKSPACE.bazel:161-162`).
- Shares `bespoke/base/` and `bespoke/common/` with Kineto (that's why
  those directories aren't named `bespoke/kineto-only/`) — when ITT is
  enabled *without* Kineto, the CMakeLists hand-enumerates the
  Kineto-authored files ITT's orchestration layer still needs
  (`profiler_kineto.{h,cpp}`, `kineto_shim.cpp`,
  `kineto_client_interface.{h,cpp}`) rather than reusing Kineto's glob
  (`CMakeLists.txt:363-397` — a known fragility, flagged in this session's
  review as something that silently misses new backend-agnostic files added
  to `bespoke/kineto/` until manually added to this list too).
- Without Kineto compiled in, nothing builds fmt's non-header-only symbols
  (normally a side effect of libkineto's own build), so this branch also
  sets `FMT_HEADER_ONLY=1` scoped to the Profiler target
  (`CMakeLists.txt:405-424`).

**Compiles**: `bespoke/itt/*` + the borrowed Kineto-orchestration files above.

**Public API**: `bespoke/itt/itt_wrapper.h` — `itt_init()`,
`itt_range_push(name)`/`itt_range_pop()`, `itt_mark(name)`,
`itt_get_domain()`. Also reachable through the *same*
`enableProfiler`/`disableProfiler` calls as Kineto (§2), passing
`ProfilerState::ITT` — the orchestration layer is genuinely shared, not
duplicated.

---

## 4. NVTX

**Enables**: NVIDIA Nsight Systems/Compute range markers (`nvtxRangePush`
equivalent).

**Not a separate backend** — it's `ProfilerState::NVTX`, one more value the
shared `enableProfiler`/`disableProfiler` orchestration in
`bespoke/kineto/profiler_kineto.cpp` dispatches on
(`profiler_kineto.cpp:699,872-874,972`), routing to
`bespoke/base/nvtx_observer.cpp`'s `pushNVTXCallbacks()`. Available whenever
Kineto or ITT is built (both compile `bespoke/base/`).

**Depends on**: the CUDA Toolkit's `nvToolsExt` library, linked as
`CUDA::nvToolsExt` — same optional gate as Kineto's CUPTI support
(`CMakeLists.txt:215-217`; only linked `if(TARGET CUDA::nvToolsExt)`, i.e.
only when the CUDA Toolkit is present at all, independent of whether CUPTI
itself is available).

**Tensor/producer-consumer tracking**: NVTX's tensor-address-based
producer/consumer correlation feature (`getInputTensorOpIds`,
mirroring PyTorch's own NVTX tensor-provenance tracking) was simplified this
session to a real no-op — XSigma has no tensor type to correlate by address,
so it always returns an empty producer-op list (correct behavior, not a
regression: see `Docs/profiler/PROFILER_MULTI_BACKEND_PLAN.md`'s
Phase-2/PyTorch-cleanup entries).

---

## 5. GPU support specifics (CUDA only)

Profiler's own GPU activity tracing is **CUDA-only** — there is no HIP or
Metal file anywhere under `Library/Profiler` (confirmed: `find
Library/Profiler -iname "*hip*"` / `"*metal*"` both return nothing). This is
distinct from `Library/Memory`, which *does* have its own HIP and Metal
**allocator** backends (`Library/Memory/gpu/`) — those report allocation
events *into* the profiler via `profiler::report_memory_usage()`
(`Library/Profiler/common/instrumentation.h`, wired this session), but they
are not a GPU *activity-tracing* backend the way Kineto/CUPTI is.

Two independent CUDA tracing paths exist, both gated by
`PROFILER_HAS_CUDA` (§2):

1. **Full CUPTI activity tracing** — real device-side kernel/memcpy/memset
   timing, correlated to the launching CPU op. Requires
   `MEMORY_GPU_BACKEND=cuda` *and* a found CUDA Toolkit *and* Kineto built
   with `LIBKINETO_NOCUPTI=OFF`.
2. **`KINETO_GPU_FALLBACK`** — a lighter CPU-side bracket using a pair of
   `cudaEvent_t` timestamps, implemented in `bespoke/base/cuda.cpp` (fixed
   from a dead `#if 0` state earlier this session — see
   `PROFILER_MULTI_BACKEND_PLAN.md` §2a). Uses the CUDA **Runtime API**
   directly (`cudaGetDevice`/`cudaSetDevice`/`cudaGetDeviceCount`), not
   libkineto/CUPTI, so it works even when full CUPTI tracing isn't linked.

**Not compile-tested in this development environment** (macOS/arm64, no CUDA
Toolkit) — verified inert (`PROFILER_HAS_CUDA=0`, clean build) on this
machine, but the `PROFILER_HAS_CUDA=1` code path itself needs validation on
a CUDA-equipped machine before being relied on (open item, tracked in the
plan doc).

---

## 6. Native / XPlane backend

**Enables**: a standalone, dependency-free CPU profiler modeled on
TensorFlow/XLA's XPlane trace format — the same underlying representation
TensorBoard's profiler plugin consumes.

**Depends on**: nothing outside the C++ standard library — no vendored
third-party code, confirmed by `CMakeLists.txt:348-357`'s comment ("standalone,
no external dependencies") and its glob (`native/*.h`, `native/*.cpp`, no
`add_third_party_library` call in this branch).

**Subtrees**:

| Directory | Role |
|---|---|
| `native/core/` | The plugin ABI: `profiler_interface` (start/stop/collect_data), `profiler_collection` (multiplexes N interfaces into one XSpace), `profiler_factory` (pluggable tracer registry), `profiler_controller`, `profiler_lock`. |
| `native/cpu/` | `host_tracer` (implements `profiler_interface` over `TraceMeRecorder`), `annotation_stack`, `metadata_collector`. |
| `native/tracing/` | `traceme`/`traceme_encode` (the lightweight instrumentation macro for this backend — a *different* mechanism from `RECORD_FUNCTION`), `traceme_recorder`. |
| `native/exporters/xplane/` | The XSpace/XPlane schema, builder, visitor, and utils — TensorFlow's own trace format, ported. |
| `native/exporters/` | `chrome_trace_exporter` — JSON export, same output format Kineto's exporter produces, independently implemented for this backend. |
| `native/session/` | The public-facing API: `profiler_session`, `profiler_scope`, `profiler_session_builder`, `profiler_report`, `memory_tracker`, `statistical_analyzer`. |
| `native/analysis/` | `stats_calculator`, `statistical_analyzer` — post-processing/aggregation over collected stats. |
| `native/memory/` | `memory_tracker`, `scoped_memory_debug_annotation` — TensorFlow-style memory annotation, separate from `Library/Memory`. |
| `native/platform/`, `native/utils/` | OS/time/env-var shims and format/parse helpers the above layers use. |

**Public API** (`native/session/profiler.h`):
```cpp
profiler::profiler_session session(profiler::profiler_options{...});
session.start();
{
    auto scope = session.create_scope("my_region");
    // ... work ...
}
session.stop();
session.write_chrome_trace("trace.json");
auto report = session.generate_report();
```
Builder pattern via `profiler_session_builder`, matching this project's own
`_builder` convention (see root `CLAUDE.md`).

**Independent of RECORD_FUNCTION**: this backend's own `traceme`/`TraceMe`
macros are its instrumentation point today, not `RECORD_FUNCTION`/
`RECORD_USER_SCOPE` — the two instrumentation mechanisms are not yet unified
(that unification is Phase 3/4 of the multi-backend plan: wrapping
`host_tracer` as one more `profiler_interface` fed by the same
`RECORD_FUNCTION` call site Kineto/ITT/NVTX already share).

---

## 7. Cross-language support: Python / C / C++

**Short answer: C++ only. No Python or C bindings exist anywhere in this
repository today.**

Verified, not assumed:

- **No pybind11 anywhere.** `ThirdParty/` has no `pybind11` submodule; no
  `pybind11_add_module`/`PYBIND11_MODULE`/`Python_add_library` call exists in
  any `CMakeLists.txt`, `WORKSPACE.bazel`, `MODULE.bazel`, or `.bzl` file in
  the tree. `Scripts/setup.py` is XSigma's own **build driver** (invokes
  CMake/ninja), not a Python package/extension build — it produces no `.so`
  importable from Python.
- **No C API surface.** The only `extern "C"` blocks in `Library/Profiler`
  are internal linkage-convention workarounds, not a public API:
  `bespoke/kineto/profiler_kineto.cpp:37-49` (a weak-symbol OpenMP
  workaround copied verbatim from libkineto, unrelated to language interop)
  and `bespoke/common/unwind/unwind.cpp:62-63,568` (assembly-adjacent
  stack-unwinding entry points, internal to the unwinder, not exported for
  external callers). Every public header (`profiler_kineto.h`,
  `hotspot_report.h`, `instrumentation.h`, `native/session/profiler.h`) uses
  C++-only constructs throughout — namespaces, classes, templates,
  `std::string`/`std::vector`/`std::optional` by value, references — none of
  it is C-ABI-compatible or callable from plain C without a wrapper layer.
- **Python tracer/bindings were real in upstream PyTorch, but were dead code
  in this port and have been deleted.** `bespoke/kineto/profiler_python.{h,cpp}`
  (PyTorch's Python-frame stack tracer) and `bespoke/itt/itt.h`'s
  `initIttBindings(PyObject*)` declaration were both entirely `#if 0`'d
  (never compiled) before this session and were removed outright as part of
  today's PyTorch-dead-code cleanup — see
  `PROFILER_MULTI_BACKEND_PLAN.md`. `bespoke/common/orchestration/
  python_tracer.{h,cpp}` still exists as a **no-op registration shim**
  (`PythonTracerBase::make()` always returns a `NoOpPythonTracer` — nothing
  ever calls `registerTracer`), kept because `collection.cpp` still threads
  through it structurally; it is not a binding layer, just a hook with
  nothing plugged into it.
- **No other XSigma library has Python/C bindings either** — this isn't a
  Profiler-specific gap. The whole project (`Library/Core`, `Library/Memory`,
  `Library/Vectorization`, `Library/Parallel`, `Library/Models`) is a
  pure-C++ library with no cross-language interop layer.

### What adding real Python interop would need

Not currently planned or scaffolded, but for reference — the shape of it,
based on what PyTorch's own (now-deleted-here) `torch/csrc/profiler/python/`
layer looked like:

1. A pybind11 (or nanobind) dependency — vendor it under `ThirdParty/`
   following the existing pattern (`FindITT.cmake`-style resolution, or a
   Bazel `local_repository`), plus a `PROFILER_ENABLE_PYTHON`-style CMake
   toggle mirroring `PROFILER_ENABLE_KINETO`/`_ITT`.
2. A thin `extern "C"`-or-pybind11-`class_<>` shim layer exposing
   `enableProfiler`/`disableProfiler`/`ProfilerResult` (Kineto),
   `profiler_session`/`profiler_scope` (Native), and `hotspot_report` —
   translating `std::string`/`std::vector` returns into Python-native types
   at the boundary, the way `profiler_python.cpp`'s deleted
   `unpackTensorMap`/`toTensorMetadata` used to for PyTorch's tensor types
   (XSigma has none, so that specific translation wouldn't be needed).
3. A `PYBIND11_MODULE` entry point built as a proper Python extension
   (`.so`/`.pyd`) via a new CMake/Bazel target, plus a `setup.py`
   (Python-packaging, not to be confused with `Scripts/setup.py`,
   XSigma's existing build driver) or `scikit-build-core` config to make it
   `pip install`-able.
4. If a stable **C** ABI is also wanted (e.g. for other languages, not just
   Python): a separate `extern "C"` header exposing opaque handles
   (`profiler_session_t*`) and plain-function entry points, since the
   current public headers are not C-compatible as written.

None of this exists today; this section is a factual "what's needed",
not a roadmap commitment.
