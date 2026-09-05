# CUDA/HIP Remediation Plan — Memory + Vectorization (Windows & Unix)

> **Superseded for Memory status (August 2026).** HIP now shares the CUDA
> caching-allocator translation unit via `gpu/gpu_runtime.h`. The deleted
> `gpu_device_manager` / `gpu_memory_pool` / tracking layers are not coming
> back. Current design, client API, and done vs still-open:
> [`memory_design.md`](memory_design.md) §10.
>
> This file is kept as a historical log of the July 2026 Phase 1
> (`PROJECT_HAS_*` / HIP-on-Windows) work. Do not treat D5–D8 or WS2 as
> open Memory work.

Status (historical): Phase 1 (WS1) applied 2026-07-26. Platform policy:
**HIP is Unix (Linux) only**; Windows gets a configure-time error directing
users to CUDA. See §10 for the fix log.
Owner: TBD.

## 1. Executive summary

Two libraries expose GPU backends behind a shared `MEMORY_GPU_BACKEND` /
`VECTORIZATION_GPU_BACKEND` CMake option (`none|cuda|hip|metal`), driven from a
single `gpu_backend` token in `Scripts/setup.py` (setup.py:1176-1178). Their
actual CUDA/HIP maturity is very different:

- **Vectorization** has genuine CUDA/HIP parity: the same `.cu`/`.cpp` sources
  compile under `nvcc` and `hipcc` (`Testing/Cxx/CMakeLists.txt`), kernels are
  written once and only branch on launch syntax
  (`expressions/expressions_evaluator_gpu.h:98-119`), and SIMD scalar math is
  backend-symmetric (`backend/gpu/{float,double}/simd.h`).
- **Memory** advertises CUDA/HIP support in comments but most of the GPU
  subsystem (`cuda_caching_allocator`, `gpu_device_manager`,
  `gpu_memory_transfer`, `gpu_memory_pool`, `gpu_allocator_tracking`,
  `gpu_resource_tracker`) is CUDA-only. There are **zero** `TestHip*.cpp`
  files, and two confirmed macro-naming bugs mean the CUDA allocation-strategy
  option and the CUDA allocator tests are currently non-functional even on
  CUDA-only builds.
- **CI GPU coverage (updated 2026-09):** `cmake-gpu-backend-tests` in
  `.github/workflows/ci.yml` builds CUDA on Ubuntu (compile + ctest with
  driver stub) and Windows (build-only — no `nvcuda.dll` stub), HIP on
  Ubuntu, and Metal on macOS. Legacy `build-matrix` `cuda_enabled: ON`
  entries remain commented; the active path uses `MEMORY_GPU_BACKEND` /
  `VECTORIZATION_GPU_BACKEND`, not the obsolete `MEMORY_ENABLE_CUDA`.

This plan sequences the work to bring Memory up to Vectorization's level of
CUDA/HIP parity, fix the confirmed build-system bugs, and keep CI coverage
aligned with the current GPU backend flags.

## 2. Confirmed baseline defects

| # | Defect | Location | Impact |
|---|--------|----------|--------|
| # | Defect | Location | Impact | Status |
|---|--------|----------|--------|--------|
| D1 | `cuda.cmake`/`hip.cmake` read a `PROJECT_GPU_ALLOC` variable that is **never set anywhere in the repo** (the actual cache variable is `MEMORY_GPU_ALLOC`), and define `PROJECT_CUDA_ALLOC_*`/`PROJECT_HIP_ALLOC_*`; all consumers check `MEMORY_CUDA_ALLOC_*`/`MEMORY_HIP_ALLOC_*` | `Library/Memory/Cmake/cuda.cmake:209-221`, `hip.cmake:86-98` vs. `gpu/allocator_gpu.h:113-117,307-311`, `helper/memory_allocator.h:123-127`, `helper/memory_allocator.cpp:273,286,300,342,355,369,431,444,458,488,501,515` | `MEMORY_GPU_ALLOC=SYNC/ASYNC/POOL_ASYNC` had **two independent** reasons to silently have no effect; allocator always fell through to the `#else` default path | **FIXED** — both cuda.cmake and hip.cmake now read `MEMORY_GPU_ALLOC` and emit `MEMORY_{CUDA,HIP}_ALLOC_*` |
| D2 | Tests guard real GPU calls with `#if PROJECT_HAS_CUDA` / `#if PROJECT_HAS_HIP`, never defined anywhere (only `MEMORY_HAS_CUDA`/`MEMORY_HAS_HIP` exist) | Turned out to span **14 files**, not 2: `Library/Memory/Testing/Cxx/{TestCudaAllocator,TestCudaCachingAllocator,TestCPUMemory,TestGpuDeviceManager,TestGpuAllocatorTracking,TestGpuAllocatorFactory,TestGpuAllocatorBenchmark,TestGpuMemoryTransfer,TestGpuMemoryPool,TestGpuMemoryAlignment,TestTrackingSystemBenchmark,TestGpuResourceTracker,TestGpuMemoryWrapper}.cpp` and `Library/Core/Testing/Cxx/TestEnzymeAD.cpp` | Test bodies dead code on every backend; `TestCudaCachingAllocator.cpp`'s entire body (incl. all `MEMORYTEST` registrations) was excluded, so those tests didn't even exist as far as gtest was concerned | **FIXED** — all 14 files renamed to `MEMORY_HAS_CUDA`/`MEMORY_HAS_HIP`; verified `MEMORY_HAS_CUDA` propagates transitively from `Memory` → `Core` → `CoreCxxTests` via `PUBLIC` compile definitions, so the `TestEnzymeAD.cpp` fix is live there too |
| D3 | Third, unrelated macro spelling `XSIGMA_CUDA_ALLOC_SYNC`/`XSIGMA_HIP_ALLOC_SYNC`, plus stale `-DXSIGMA_CUDA_ALLOC=...` guidance printed to the user | `Testing/Cxx/TestGpuAllocatorBenchmark.cpp:715-720,741-743` | Diagnostic printout always showed "DEFAULT (SYNC fallback)" regardless of actual strategy; printed guidance referenced a compile define that was never how the strategy is actually selected | **FIXED** — macros renamed to `MEMORY_*_ALLOC_*`; guidance now points at `-DMEMORY_GPU_ALLOC=...` |
| D4 | `hip.cmake` unconditionally appends `--expt-extended-lambda` to `CMAKE_HIP_FLAGS` — this is an **nvcc-only** flag, not recognized by `hipcc`/Clang | `Library/Memory/Cmake/hip.cmake:110` (old) | Would break real ROCm builds; never caught because no HIP CI/tests exist | **FIXED** — flag removed with an explanatory comment |
| D5 | `cuda_caching_allocator` is explicitly CUDA-only; throws under HIP builds; no `hip_caching_allocator` exists | `Library/Memory/gpu/cuda_caching_allocator.{h,cpp}` (header comment, `Impl` stub at cpp:492-514) | No caching allocator on HIP — falls back to raw `hipMalloc`/`hipFree` via `allocator_gpu.h` with no pooling | Open — WS2, largest remaining item |
| D6 | `gpu_device_manager` only implements `initialize_cuda()`; `device_enum::HIP` used only for string formatting | `Library/Memory/gpu/gpu_device_manager.cpp:504-527` | No device enumeration/management on HIP | Open — WS2 |
| D7 | `gpu_memory_transfer`, `gpu_memory_pool`, `gpu_allocator_tracking`, `gpu_resource_tracker` are CUDA-only (`gpu_stream::create` throws for non-CUDA device types) | `Library/Memory/gpu/gpu_memory_transfer.cpp`, `gpu_memory_pool.cpp`, `gpu_allocator_tracking.cpp`, `gpu_resource_tracker.cpp` | Streams, pooling, tracking unavailable on HIP | Open — WS2 |
| D8 | No `TestHip*.cpp` files exist; generic `TestGpu*.cpp` files are internally CUDA-specific | `Library/Memory/Testing/Cxx/` | Zero runtime verification of the HIP path, ever | Open — WS4, blocked on WS2 |
| D9 | `hip.cmake` had no Windows/toolchain gate analogous to `cuda.cmake`'s MinGW/MSYS2 exclusion (`cuda.cmake:13`) | `Library/Memory/Cmake/hip.cmake`, `Library/Vectorization/CMakeLists.txt:602` | On Windows, `find_package(hip REQUIRED)`/`find_package(hip QUIET)` would either fail with a confusing generic CMake error or (if a Windows HIP SDK happened to be present) attempt a build path that had never been validated | **FIXED** — both files now `FATAL_ERROR` immediately on `WIN32` with a message pointing to CUDA as the Windows GPU backend; §8's open question is resolved as "HIP is Unix-only, hard error on Windows" |
| D10 | CI has no active CUDA jobs (all commented out) and no HIP jobs at all | `.github/workflows/ci.yml:80-200` (CUDA), entire file (HIP) | No regression protection for any GPU code path on any platform | **FIXED (2026-09)** — `cmake-gpu-backend-tests` covers CUDA Ubuntu (build+test), CUDA Windows (build-only), HIP Ubuntu, Metal macOS. Legacy commented `build-matrix` CUDA rows are superseded; flags use `MEMORY_GPU_BACKEND` / `VECTORIZATION_GPU_BACKEND` |

## 3. Scope

**In scope:** `Library/Memory` and `Library/Vectorization` — their `Cmake/*`
GPU config, C/C++ and CUDA/HIP source, unit tests, and benchmarks — on
**Windows** and **Unix (Linux)**, for the **CUDA** and **HIP** backends only.

**Out of scope:** the Metal backend (Apple Silicon has no CUDA/HIP — this is
already correctly excluded, see `ci.yml` comment "CUDA not officially
supported on Apple Silicon"), `ThirdParty/` (never modified, per
`CLAUDE.md`), and any non-GPU-backend work in either library.

## 4. Target platform/backend support matrix

| Backend | Linux | Windows | Notes |
|---|---|---|---|
| CUDA | Supported, CI-verified (target) | Supported, CI-verified (target) | `cuda.cmake` already excludes MinGW/MSYS2 (`cuda.cmake:13`); MSVC + nvcc/Clang is the supported Windows toolchain |
| HIP | Supported, CI-verified (target) | **Unsupported by design** — `cmake -DMEMORY_GPU_BACKEND=hip`/`-DVECTORIZATION_GPU_BACKEND=hip` now `FATAL_ERROR`s immediately on `WIN32` with a message pointing to CUDA | Resolved (§8, D9 fixed): minimum-effort policy — fail fast and clearly rather than attempt an unvalidated ROCm-on-Windows build |
| Metal | N/A | N/A | macOS/Apple Silicon only, unaffected by this plan |

## 5. Workstreams

### WS1 — CMake macro correctness (Memory), both platforms — ✅ DONE (see §10)
- ~~Rename `PROJECT_CUDA_ALLOC_*`/`PROJECT_HIP_ALLOC_*` → `MEMORY_CUDA_ALLOC_*`/`MEMORY_HIP_ALLOC_*` in `cuda.cmake`/`hip.cmake` (fixes D1)~~ — done; also fixed the underlying `PROJECT_GPU_ALLOC` → `MEMORY_GPU_ALLOC` variable-name bug that made D1 worse than first scoped.
- ~~Remove `--expt-extended-lambda` from `CMAKE_HIP_FLAGS` (fixes D4)~~ — done.
- ~~Add a Windows/toolchain gate to `hip.cmake` (fixes D9)~~ — done, as a hard `FATAL_ERROR` (policy: HIP is Unix-only, minimum effort on Windows is "fail clearly"); mirrored in `Library/Vectorization/CMakeLists.txt`'s HIP block too, since it has its own independent `find_package(hip)`/`enable_language(HIP)` path.
- ~~Replace `PROJECT_HAS_CUDA` with `MEMORY_HAS_CUDA` (fixes D2), and `XSIGMA_CUDA_ALLOC_SYNC`/`XSIGMA_HIP_ALLOC_SYNC` with the corrected macros (fixes D3)~~ — done; scope grew from 2 files to 14 once the same dead macro was found repo-wide (including `Library/Core/Testing/Cxx/TestEnzymeAD.cpp`).

### WS2 — HIP feature parity in Memory C/C++ code
Bring `Library/Memory/gpu/*` up to the branching pattern already used
correctly in `helper/memory_allocator.cpp` and `allocator_gpu.h`
(`#if MEMORY_HAS_CUDA … #elif MEMORY_HAS_HIP …`):
- `gpu_device_manager`: add `initialize_hip()` mirroring `initialize_cuda()` (fixes D6).
- `gpu_memory_transfer`: add a `hip_stream_impl` alongside `cuda_stream_impl`; stop throwing for HIP device types (fixes D7 partially).
- `gpu_memory_pool`, `gpu_allocator_tracking`, `gpu_resource_tracker`: add HIP branches (fixes D7).
- `cuda_caching_allocator`: **decision needed** — either (a) generalize into a backend-templated caching allocator shared by CUDA/HIP, mirroring Vectorization's single-source approach, or (b) keep it explicitly CUDA-only and document that HIP always uses the uncached `allocator_gpu` path. Recommend (a) for parity but it's the largest single piece of work in this plan — see §8.

### WS3 — Vectorization validation (no known Memory-style bugs, but unverified in CI)
- Re-run `TestGpuSimd.cu`, `TestTensorGpu.cpp`, `BenchmarkTensorGpu.cpp` under both `cuda` and `hip` backends on Linux to confirm the existing parity claims hold on current ROCm/CUDA toolkit versions (no code changes expected; this is a verification task feeding WS6 CI jobs).
- Confirm the Clang-as-CUDA-compiler test exclusion (`Testing/Cxx/CMakeLists.txt:14-26`) doesn't need a HIP-side counterpart.

### WS4 — Testing
- After WS1/WS2 land, add `TestHip*.cpp` (or generalize `TestCuda*.cpp` into backend-parameterized `TestGpu*.cpp`, matching how Vectorization reuses one source file for both backends) so the HIP path gets real runtime coverage (fixes D8).
- Add negative tests: building with `MEMORY_GPU_BACKEND=hip` should not silently compile out entire test bodies (regression guard for D2/D8).

### WS5 — Benchmarking
- Fix the macro references in `TestGpuAllocatorBenchmark.cpp` (part of WS1/D3).
- Extend `TestGpuAllocatorBenchmark.cpp` and Vectorization's `BenchmarkTensorGpu.cpp` to run under the HIP backend once WS2 lands, so caching-allocator-vs-raw-alloc performance is comparable across CUDA/HIP.
- Capture baseline numbers on both Linux and Windows CUDA before/after WS1 fixes (the allocation-strategy bug in D1 means current benchmark numbers may not reflect the intended SYNC/ASYNC/POOL_ASYNC strategy at all).

### WS6 — CI re-enablement — ✅ DONE (2026-09)
- ~~Investigate why the Windows CUDA jobs were disabled and re-enable at least one Linux + one Windows CUDA job.~~ — superseded by `cmake-gpu-backend-tests` (Ubuntu CUDA build+test with stub; Windows CUDA build-only).
- ~~Add a new Linux HIP CI job.~~ — covered by the same matrix (`backend: hip` on `ubuntu-latest`).
- ~~Windows HIP CI feasibility.~~ — policy already enforced: `FATAL_ERROR` on `WIN32` for HIP (D9); no Windows HIP CI job by design.
- Remaining hygiene (optional): delete or rewrite the commented legacy `build-matrix` `cuda_enabled: ON` blocks so they cannot be re-enabled with the obsolete `-DMEMORY_ENABLE_CUDA` flag.

## 6. Phased execution

1. **Phase 1 — Build-system correctness (WS1).** Low risk, no behavior change beyond fixing dead/mismatched macros. Unblocks accurate measurement of everything downstream. Verify locally with `Scripts/setup.py` on both platforms before touching library code.
2. **Phase 2 — Memory HIP parity (WS2).** The bulk of new code. Land device manager → memory transfer → pool/tracking/resource-tracker → caching allocator, in that dependency order.
3. **Phase 3 — Testing (WS4)**, immediately following each WS2 sub-piece rather than all at the end, so each new HIP code path gets a test before the next one is built on top of it.
4. **Phase 4 — Vectorization verification (WS3)**, can run in parallel with Phase 2/3 since no code changes are anticipated there.
5. **Phase 5 — Benchmarking (WS5)**, after Phase 2/3 land, so numbers reflect corrected allocation-strategy behavior.
6. **Phase 6 — CI re-enablement (WS6).** ✅ Done via `cmake-gpu-backend-tests`
   (see §5 WS6). Legacy commented `build-matrix` CUDA rows are optional cleanup.

## 7. Local verification matrix

Per `CLAUDE.md`, always drive builds through `Scripts/setup.py` from
`Scripts/`, never CMake/ninja/ctest directly. Minimum matrix to validate this
plan (`--project.memory` / `--project.vectorization` restrict to one library
at a time; drop the flag to build both):

```
# Linux / macOS, CUDA
python3 setup.py build.TEST.native.avx2.cuda.config --project.memory
python3 setup.py build.TEST.native.avx2.cuda.config --project.vectorization

# Linux, HIP (requires ROCm toolchain installed)
python3 setup.py build.TEST.native.avx2.hip.config --project.memory
python3 setup.py build.TEST.native.avx2.hip.config --project.vectorization

# Windows, CUDA (MSVC + CUDA Toolkit)
python3 setup.py build.TEST.native.avx2.cuda.config --project.memory
python3 setup.py build.TEST.native.avx2.cuda.config --project.vectorization

# Windows, HIP — only once WS3/§8 decision is made
python3 setup.py build.TEST.native.avx2.hip.config --project.memory

# Regression guard: no-GPU baseline must remain unaffected
python3 setup.py build.TEST.native.avx2.config
```

Run each matrix cell before and after Phase 1/2 changes to produce a
before/after diff of test pass/fail and benchmark numbers for the PR
description.

## 8. Open questions / risks

- ~~**Windows HIP viability**~~ — **Resolved 2026-07-26**: policy is minimum
  effort for HIP on Windows, full support kept Unix-only. `hip.cmake` and
  Vectorization's HIP CMake block now `FATAL_ERROR` immediately on `WIN32`
  instead of attempting `find_package(hip)`/`enable_language(HIP)`. WS2's HIP
  parity code (device manager, memory transfer/pool/tracking, caching
  allocator) only needs to build and run on Linux; no Windows+ROCm testing is
  in scope.
- **`cuda_caching_allocator` generalization (WS2)** is the largest-effort item
  in this plan. Recommend a short spike to confirm the CUDA/HIP runtime APIs
  it depends on (`cudaMalloc`/`cudaFree`/stream APIs and the block-splitting
  logic) have no CUDA-only semantics before committing to full parity.
- **CI cost**: adding a ROCm Linux CI job and re-enabling a Windows CUDA job
  both require GPU-capable or emulated runners; confirm runner availability/
  cost before Phase 6.
- Confirm with the user whether **Metal** should be pulled into the same
  macro-consistency pass (it shares the tri-state `GPU_BACKEND` option) even
  though it's out of scope for CUDA/HIP parity itself.

## 9. Definition of done

- All 10 confirmed defects (D1–D10) fixed or explicitly downgraded to a
  documented, intentional limitation with a tracking note in this file.
- `Library/Memory` and `Library/Vectorization` build and pass their test
  suites via `Scripts/setup.py` for `cuda` and `hip` backends on Linux, and
  for `cuda` on Windows (Windows `hip` per §8 decision).
- At least one CI job per (platform × backend) combination in scope is green
  and un-commented in `ci.yml`.
- Benchmark baselines captured for CUDA and HIP allocator paths on Linux, and
  CUDA on Windows.

## 10. Fix log

### 2026-07-26 — WS1 applied (D1, D2, D3, D4, D9)

Files changed:
- `Library/Memory/Cmake/cuda.cmake` — reads `MEMORY_GPU_ALLOC` (was reading
  the never-set `PROJECT_GPU_ALLOC`); emits `MEMORY_CUDA_ALLOC_{SYNC,ASYNC,POOL_ASYNC}`
  (was `PROJECT_CUDA_ALLOC_*`).
- `Library/Memory/Cmake/hip.cmake` — same `MEMORY_GPU_ALLOC` /
  `MEMORY_HIP_ALLOC_*` fix; added a `WIN32` → `FATAL_ERROR` gate at the top
  (HIP is Unix-only); removed the nvcc-only `--expt-extended-lambda` flag from
  `CMAKE_HIP_FLAGS`; dropped the now-redundant `NOT MSVC` branch around the
  `-g`/`-O3` flags since Windows is hard-excluded above it.
- `Library/Vectorization/CMakeLists.txt` — added the matching `WIN32` →
  `FATAL_ERROR` gate to its independent HIP block (it has its own
  `find_package(hip)`/`enable_language(HIP)`, separate from Memory's).
- 14 test files (13 in `Library/Memory/Testing/Cxx/`, 1 in
  `Library/Core/Testing/Cxx/TestEnzymeAD.cpp`) — `PROJECT_HAS_CUDA` →
  `MEMORY_HAS_CUDA`, `PROJECT_HAS_HIP` → `MEMORY_HAS_HIP`.
- `Library/Memory/Testing/Cxx/TestGpuAllocatorBenchmark.cpp` — additionally
  fixed the `XSIGMA_CUDA_ALLOC_SYNC`/`XSIGMA_HIP_ALLOC_SYNC` diagnostic
  macros and the `-DXSIGMA_CUDA_ALLOC=...` guidance text printed to users.

Verification performed (no CUDA/HIP hardware available in this environment,
so verification was config-level + CPU-only regression, all via
`Scripts/setup.py` per `CLAUDE.md`):
- `python3 setup.py config.hip --project.memory` on Darwin (Unix) — confirmed
  the new `WIN32` guard does **not** trigger on a non-Windows host, and the
  file proceeds to fail cleanly at `find_package(hip)` (ROCm not installed
  here) rather than a CMake syntax error.
- `python3 setup.py config.cuda --project.memory` on Darwin — confirmed
  `cuda.cmake` proceeds past the edited allocation-strategy block and fails
  cleanly at `find_package(CUDAToolkit REQUIRED)` (no CUDA toolkit here), i.e.
  no syntax regression.
- `python3 setup.py config.build.test --project.memory` (CPU-only,
  `MEMORY_GPU_BACKEND=none`) — full build + `MemoryCxxTests` pass, confirming
  the renamed macros in `TestCPUMemory.cpp`/`TestTrackingSystemBenchmark.cpp`
  don't regress the no-GPU path.
- `python3 setup.py config.build.test --project.vectorization` (CPU-only) —
  full build + `VectorizationCxxTests` pass, confirming the
  `Vectorization/CMakeLists.txt` edit doesn't regress the no-GPU path.
- Not verified (no hardware/toolchain here): an actual `MEMORY_GPU_BACKEND=cuda`
  or `=hip` build/test run on real hardware. This should be the first thing
  done on a CUDA- or ROCm-capable Linux machine before proceeding to WS2.

Remaining open items: D5–D8 (WS2, HIP feature parity in Memory's C/C++ code).
D10 / WS6 CI coverage is closed via `cmake-gpu-backend-tests` (see §2 / §5).

### 2026-09-05 — CUDA CMake hygiene (D10 doc + arch sync + flag cleanup)

- `.github/workflows/ci.yml` — replaced obsolete `-DMEMORY_ENABLE_CUDA=…`
  with `-DMEMORY_GPU_BACKEND=none|cuda` (matrix expression maps ON→cuda).
- `Library/Memory/README.md` — documents `MEMORY_GPU_BACKEND` instead of
  `MEMORY_ENABLE_CUDA` / `MEMORY_ENABLE_HIP`.
- `Library/Memory/Cmake/cuda.cmake` — dropped pre-sm_75 arch options
  (fermi…volta); `all` / forced defaults use shared
  `XSIGMA_CUDA_ARCHITECTURES_DEFAULT` (`75;80;86;89;90`).
- `Library/Vectorization/CMakeLists.txt` — inherits
  `CMAKE_CUDA_ARCHITECTURES` from Memory when set; re-caches Clang CUDA
  implicit includes after the second `enable_language(CUDA)`.
- This file — D10 / WS6 marked fixed; executive summary CI bullet updated.
