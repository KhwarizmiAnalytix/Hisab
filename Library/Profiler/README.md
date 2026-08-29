# Profiler (`Library/Profiler`)

Profiler **implementation**: native CPU profiler, **Kineto** (PyTorch-style) integration, **ITT** (VTune), Chrome trace export, and tests.

## Documentation

User guide: **[`Docs/profiler/profiler.md`](../../Docs/profiler/profiler.md)**.

Native end-to-end (session → TraceMe → XSpace → Chrome / report /
`stats_calculator` / native **hotspot_report**) and Kineto end-to-end
(`RECORD_*` → correlation ids → `ProfilerResult` → `save()` / Kineto
hotspot, including GPU CUPTI): see
[profiler.md](../../Docs/profiler/profiler.md).

## Tests (`Testing/Cxx/`)

Keep CMake `TestFiles` and Bazel `_PROFILER_COMMON_TESTS` in sync:

- `TestProfilerBackendFunction.cpp` / `TestProfilerBackendMemory.cpp` — Kineto, ITT, NVTX
- `TestProfilerBackendMetadata.cpp` — Kineto event metadata / extra fields
- `TestProfilerBackendGpuFallback.cpp` / `TestProfilerBackendMetal.cpp` — CUDA/HIP event fallback and Metal stub
- `TestProfilerBackendOutput.cpp` / `TestProfilerChromeTraceHierarchical.cpp` — native reports and Chrome
- `TestProfilerXPlanePipeline.cpp` / `TestProfilerThreadpoolTracing.cpp` — XSpace / tracing e2e
- `TestProfilerNativeHotspot.cpp` / `TestHotspotReport.cpp` — native vs Kineto hotspots
- `TestProfilerHeavyFunction.cpp` — native + Kineto + ITT stress
- `TestProfilerGpuTracer.cpp` — TF GpuTracer `/device:GPU:N` (collector + device probe)

## Layout

- `CMakeLists.txt` — `PROFILER_BACKEND`, `PROFILER_ENABLE_*`.
- `BUILD.bazel` — `//Library/Profiler:Profiler` and tests.
- `native/` — always-on native profiler pipeline (traceme/xplane/host_tracer/gpu_tracer/profiler_session).
- `bespoke/kineto/`, `bespoke/itt/` — instrumentation backends layered on top of `native/`.
- `Testing/Cxx/` — backend and integration tests.

---

## CMake options

### Backend (single control point)

`native/` (traceme/xplane/host_tracer/profiler_session) is always compiled — it is not a backend
choice. `PROFILER_BACKEND` only selects the *instrumentation* backend layered alongside it.

| CMake variable | Default | Values |
|----------------|---------|--------|
| `PROFILER_BACKEND` | `KINETO` | `KINETO`, `ITT` — sets `PROFILER_ENABLE_KINETO` / `PROFILER_ENABLE_ITT` (`PROFILER_ENABLE_NATIVE_PROFILER` is always `ON`) |

### Feature and toolchain

| CMake variable | Default | Summary |
|----------------|---------|---------|
| `PROFILER_LTO_MODE` | `off` (or `auto` when LTO is requested) | LTO: `off` / `thin` / `full` / `ipo` / `auto` |
| `PROFILER_ENABLE_COVERAGE` | OFF | Coverage |
| `PROFILER_ENABLE_TESTING` | ON | Tests |
| `PROFILER_ENABLE_EXAMPLES` | OFF | Examples under `Examples/Profiling` |
| `PROFILER_ENABLE_GTEST` | ON | GoogleTest |
| `PROFILER_ENABLE_BENCHMARK` | ON | Google Benchmark |
| `PROFILER_ENABLE_ICECC` / `PROFILER_ENABLE_CACHE` / `PROFILER_ENABLE_CLANGTIDY` / … | see `CMakeLists.txt` | Tooling |

### `CACHE STRING`

| CMake variable | Default | Notes |
|----------------|---------|-------|
| `PROFILER_CXX_STANDARD` | 20 | `11`–`23` |
| `PROFILER_SANITIZER_TYPE` | address | if sanitizer ON |
| `PROFILER_LINKER_CHOICE` | default | linker |
| `PROFILER_CACHE_BACKEND` | none | compiler cache |

Early-only gate: `PROFILER_INCLUDE_GATE_ONLY` (root uses to gate Kineto before `add_subdirectory`).

---

## Bazel flags

Starlark: [`bazel/profiler.bzl`](../../bazel/profiler.bzl). `select` order: **ITT** → default **Kineto**.
`native/**` is globbed in unconditionally in `BUILD.bazel` — it is not part of either `select`.

### Backend (`profiler_enable_*` defines)

Typical invocations (see `.bazelrc`):

| Mode | Defines / config | `PROFILER_HAS_*` result |
|------|-------------------|-------------------------|
| Kineto (default) | *(none)* or `profiler_enable_kineto=true` — `build:kineto` | `PROFILER_HAS_KINETO=1` |
| ITT | `profiler_enable_itt=true` — `build:itt` | `PROFILER_HAS_ITT=1` |

`//bazel:enable_itt` matches `profiler_enable_itt=true`. The native
traceme/xplane pipeline is always compiled (no `HAS_*` gate), independent of
which arm above is selected.

### Other

| Mechanism | Effect |
|-----------|--------|
| `profiler_enable_benchmark` | Default ON in `.bazelrc` (CMake parity) |
| `enable_gtest` | Project-wide |

### CMake-only

`PROFILER_CXX_STANDARD` → `c++20` in `profiler.bzl`. LTO, coverage, sanitizers, linker/cache, spell, Valgrind — **CMake only**.
