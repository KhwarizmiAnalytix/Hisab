# Memory Library Design

Scope: `Library/Memory` after the allocator consolidation (August 2026). This
document describes the design **as it exists now** — two allocation paths and
their supporting infrastructure — the rationale behind the consolidation, and
the known limitations that fall out of it.

---

## 1. Design goals

1. **Simple and fast CPU allocation.** The CPU hot path is a direct call into
   the best available system allocator (mimalloc by default). No virtual
   dispatch, no pooling layer, no mutex, no bookkeeping on the hot path.
2. **Cached GPU allocation.** CUDA allocations go through a PyTorch-style
   caching allocator so tensor alloc/free churn amortizes into cached segment
   reuse instead of `cudaMalloc`/`cudaFree` round-trips.
3. **One ownership story.** Client code holds memory through
   `memory::data_ptr<T, clone>` (RAII) and allocates through
   `memory::allocator<T>` (static, policy-free). There is no allocator
   interface to implement, select, or register.
4. **Compile-time backend selection.** CPU allocator backend, GPU backend
   (none/CUDA/HIP/Metal), NUMA, memkind, and TBB are all CMake/Bazel-time
   decisions expressed as `MEMORY_HAS_*` macros. No runtime plugin machinery.

The consolidation deliberately **removed** the previous machinery — the
`Allocator` virtual interface, `process_state` singleton, `allocator_cpu`,
BFC / pool / retry / tracking backends, `gpu_memory_*` helper layers, and the
visualization dashboard — because mimalloc already provides per-thread
caching for CPU (making the pool/BFC layers pure overhead on the default
path), and a single CUDA caching allocator covers the GPU use case. Do not
reintroduce those layers without a measured need (see
`Library/Memory/CLAUDE.md`).

---

## 2. Architecture at a glance

```
client code (Vectorization tensors, expressions, …)
        │
        ▼
memory::data_ptr<T, clone>            common/data_ptr.h    — RAII owner
        │  allocates/frees/copies via
        ▼
memory::allocator<T, alignment>       allocator.h          — static dispatch by device_enum
        │
        ├─ device_enum::CPU ────► memory::cpu::memory_allocator
        │                          helper/memory_allocator.{h,cpp}
        │                          mimalloc │ TBB │ Android memalign │ MSVC │ POSIX
        │
        ├─ device_enum::CUDA ───► memory::gpu::caching_allocator_for_device(i)
        │   (compiled when          gpu/cuda_caching_allocator.{h,cpp}
        │    MEMORY_HAS_CUDA)        one shared instance per device
        │
        └─ device_enum::METAL ──► memory::metal::allocate/deallocate
            (compiled when           gpu/metal/metal_buffer_allocator.{h,mm}
             MEMORY_HAS_METAL)       MTLResourceStorageModeShared buffers
```

### File map

| Path | Role |
|---|---|
| `allocator.h` / `allocator.cpp` | `allocator<T>` static dispatch (allocate/free/copy + alignment helpers) |
| `common/data_ptr.{h,cpp}` | RAII owning/non-owning pointer used by Vectorization |
| `common/device.{h,cpp}` | `device_enum` (CPU/CUDA/HIP/PrivateUse1/METAL), `device_option` |
| `common/memory_macros.h` | `MEMORY_ALIGNMENT` (64, 16 on `MEMORY_MOBILE`), force-inline/likely/export helpers |
| `common/memory_containers.h` | `memory_map`/`memory_set` aliases (flat-hash optional) |
| `common/numa.{h,cpp}` | `NUMAMove` / `GetCurrentNUMANode` (Linux, `MEMORY_HAS_NUMA`) |
| `helper/memory_allocator.{h,cpp}` | Raw CPU allocation backend — the entire CPU implementation |
| `gpu/cuda_caching_allocator.{h,cpp}` | CUDA segment cache + per-device registry |
| `gpu/metal/metal_buffer_allocator.{h,mm}` | Metal shared-storage buffers |
| `profiler/unified_memory_stats.{h,cpp}` | `unified_cache_stats` — the only statistics surface (caching allocator metrics) |
| `util/memory_exception.h` | `MEMORY_CHECK`/`MEMORY_LOG_*` macros |

---

## 3. CPU path — `cpu::memory_allocator`

The whole CPU implementation is ~200 lines in `helper/memory_allocator.cpp`.
`allocator<T>::allocate(n, CPU)` is one force-inline hop into:

```cpp
void* allocate(std::size_t nbytes,
               std::size_t alignment = default_alignment(),   // MEMORY_ALIGNMENT
               init_policy_enum init = UNINITIALIZED);
void  free(void* ptr, std::size_t nbytes = 0) noexcept;
std::size_t usable_size(const void* ptr) noexcept;
```

### Backend selection (compile time, first match wins)

| Order | Guard | allocate | free | usable_size |
|---|---|---|---|---|
| 1 | `MEMORY_HAS_MIMALLOC` (default ON) | `mi_aligned_alloc` | `mi_free` | `mi_usable_size` |
| 2 | `MEMORY_HAS_TBB` | `scalable_aligned_malloc` | `scalable_aligned_free` | `scalable_msize` |
| 3 | `__ANDROID__` | `memalign` | `free` | `malloc_usable_size` |
| 4 | MSVC / MinGW | `_aligned_malloc` | `_aligned_free` | **0** (needs original alignment) |
| 5 | Apple | `posix_memalign`/`malloc` | `free` | `malloc_size` |
| 6 | other POSIX | `posix_memalign`/`malloc` | `free` | `malloc_usable_size` |

`usable_size` returns the backend-reported block size (≥ requested), or 0
where the backend cannot report (MSVC) — callers must treat 0 as "unknown",
not "empty".

### Behavior notes

- **Validation**: `MEMORY_CHECK` rejects `nbytes == 0`; debug builds also
  assert power-of-two alignment ≥ `sizeof(void*)`.
- **NUMA** (`MEMORY_HAS_NUMA`, Linux only): after allocation, `NUMAMove(ptr,
  nbytes, GetCurrentNUMANode())` applies first-touch policy. Free needs no
  NUMA handling.
- **Init policies**: `UNINITIALIZED` (fastest), `ZERO` (`memset 0` — also
  exposed as `allocate_zero`), `PATTERN` (`memset 0xCC`, debug builds only).
- **Escape hatches**: `allocate_tbb`/`free_tbb` and `allocate_mi`/`free_mi`
  call a specific backend directly (return nullptr when that backend is not
  compiled in) — used by the CPU benchmark to compare backends.
- **Threading**: everything is delegated to the backend. mimalloc gives
  per-thread free lists, so the multithreaded hot path is effectively
  contention-free without any project-level locking.

### mimalloc statistics (opt-in)

mimalloc release builds compile statistics **out** (`MI_STAT=0` in
`ThirdParty/mimalloc`); `MEMORY_ENABLE_MIMALLOC_STATS=ON` (setup.py
`--mimalloc_stats` — a `--` flag, not a dotted token, since `_` is a token
delimiter there) rebuilds the vendored `mimalloc-static` with `MI_STAT=1` and
sets `MEMORY_HAS_MIMALLOC_STATS=1`. Two consumption routes:

- **Env vars**: `MIMALLOC_SHOW_STATS=1` dumps full stats at process exit;
  `MIMALLOC_VERBOSE=1` prints init messages.
- **API** (`helper/memory_allocator.h`): `has_stats()` (compile-time
  availability), `stats_print()` (`mi_stats_merge` + dump to stderr),
  `process_info(process_memory_info&)` (RSS / commit / page faults).

Because mimalloc is linked with `MI_OVERRIDE=OFF`, the stats cover only
allocations that went through this path — not the process's `malloc`.

### What was deliberately dropped

CPU-side allocation statistics (`allocator_cpu`'s counters), the visitor /
NUMA-selector hooks in `process_state`, and the never-populated generic stats
structs (`unified_resource_stats`, `atomic_timing_stats`,
`comprehensive_memory_stats`, `memory_fragmentation_metrics`) are gone — the
CPU hot path carries no XSigma-level counters (mimalloc's own opt-in
statistics are the exception; see above). `usable_size` remains for
per-block tooling; `unified_cache_stats` (CUDA caching allocator) is the only
built-in metrics surface.

---

## 4. CUDA path — `cuda_caching_allocator`

A behavioral port of PyTorch's `c10/cuda/CUDACachingAllocator`, in
`gpu/cuda_caching_allocator.{h,cpp}` (~900 lines, PIMPL). CUDA-only; the TU
compiles to a throwing stub on non-CUDA builds so the header surface is always
available.

### Size classes and segments

One `cudaMalloc` per **segment**; many blocks per segment.

| Request (after rounding) | Segment size |
|---|---|
| < 512 B → rounded up to 512 B multiples | — |
| ≤ 1 MiB ("small") | 2 MiB |
| 1–10 MiB | 20 MiB |
| ≥ 10 MiB | rounded up to 2 MiB multiples |

Two free pools: `small_blocks_` (≤ 1 MiB requests) and `large_blocks_`.
A request is only ever served from its own pool.

### Block lifecycle

- **Reuse lookup**: pools are ordered sets keyed by `(stream, size,
  registration_counter, ptr)`. Lookup is `lower_bound(size, stream)` →
  smallest sufficient block **on the same stream**; equal-size segments
  recycle FIFO (oldest `registration_counter` first). Blocks are never reused
  on a different stream than the allocation stream.
- **Split**: an oversized cached block is split when the remainder is ≥ 512 B
  (small pool) or > 1 MiB (large pool). The remainder goes back to its pool.
  Blocks carry intrusive `prev`/`next` links within their segment.
- **Coalesce**: on free, a block merges with `prev`/`next` only if the
  neighbor is free, has no pending events, and no recorded cross-stream uses.
- **Release to driver**: only *whole* (never-split) segments can be
  `cudaFree`d — split remainders share a segment with live neighbors.

### Cross-stream safety

- `record_stream(ptr, stream)` (or a deallocate stream hint) marks a live
  block as used on another stream (uses on the allocation stream are ignored).
- On free, a block with recorded uses is not pooled immediately: one CUDA
  event per recorded stream is queued (`insert_events_locked`), and the block
  is reclaimed lazily by `process_events_locked()` — called at the top of
  every `allocate`/`deallocate` — once all events complete. Event objects are
  pooled (`event_pool_`) to avoid `cudaEventCreate` churn.
- Per-stream event queues are drained independently to avoid head-of-line
  blocking.
- `insert_events_locked` is exception-safe: if event recording fails midway,
  blocks with zero recorded events return to their pool instead of being
  orphaned (this is one of the sanctioned `try/catch` boundaries around the
  CUDA runtime).

### OOM and cache-pressure handling

`allocate` failure chain, mirroring upstream:

1. `get_free_block` from the pool.
2. Run registered **free-memory callbacks** (`add_free_memory_callback`; they
   run under the allocator lock, which is recursive so callbacks may
   deallocate/`empty_cache` on the same allocator); if any freed memory,
   retry the pool.
3. `cudaMalloc` a new segment (non-OOM driver errors throw immediately; the
   CUDA error state is cleared either way).
4. On `cudaErrorMemoryAllocation`: `release_cached_blocks_locked()` —
   synchronize all pending events (`num_sync_all_streams++`) and release
   every whole-segment cached block — then retry the driver **once**.
5. Still failing → `num_ooms++` and `std::bad_alloc`.

Independently, `set_max_cached_bytes(bytes)` (default: unlimited) trims the
cache largest-first among releasable whole segments after every allocate and
deallocate. `empty_cache()` force-releases everything.

### Device discipline

Every driver-facing entry point (`alloc_segment`, `insert_events`,
`process_events`, `synchronize_and_free_events`, `release_segment`,
destructor) holds a `DeviceGuard(device_)` — a RAII `cudaSetDevice` wrapper —
so the allocator is safe to use regardless of the caller's current device.
(Event polling gained its own guard in the consolidation review; events are
device-local.)

### Threading model

One `std::recursive_mutex` per allocator instance serializes all operations
(recursive because free-memory callbacks re-enter). `cudaMalloc` is called
under the lock, same as upstream. Stats counters are atomics but are only
mutated under the lock except where noted.

### Statistics — `unified_cache_stats` (`profiler/unified_memory_stats.h`)

`stats()` returns a snapshot: `cache_hits`/`cache_misses` (+ `cache_hit_rate()`),
`bytes_cached`/`peak_bytes_cached`, `cache_blocks`, `inactive_split_bytes`
(free split remainders that can't be returned to the driver),
`driver_allocations`/`driver_frees`/`cache_evictions`, `bytes_reserved`
(total segment bytes held), `bytes_allocated`, `successful_allocations/frees`,
`num_alloc_retries`, `num_ooms`, `num_sync_all_streams`.

### Per-device registry

```cpp
MEMORY_API cuda_caching_allocator& caching_allocator_for_device(int device_index);
```

Defined in the `.cpp` (mutex + `unordered_map<int, unique_ptr<...>>`, lazy
creation, device index validated against `cudaGetDeviceCount`). Living inside
the Memory library (not as an inline header function) guarantees **one**
registry process-wide even when multiple shared libraries link Memory —
allocator identity is how foreign/double frees are detected, so splitting the
registry across DSOs would break that. This registry is what
`allocator<T>`'s CUDA branch uses; `cuda_caching_allocator_template<T>` in
the header offers a type-safe per-instance wrapper for direct users.

---

## 5. Metal path — `metal_buffer_allocator`

`gpu/metal/metal_buffer_allocator.{h,mm}` — a plain C++ header (no
Objective-C types cross it) over an Objective-C++ implementation:

- `allocate(bytes)` → `newBufferWithLength:options:MTLResourceStorageModeShared`
  on the process-wide `MTLCreateSystemDefaultDevice()`, returns
  `buffer.contents`. Throws `std::bad_alloc` when no Metal device or on
  allocation failure.
- Shared storage on Apple Silicon means the returned pointer is simultaneously
  host- and GPU-addressable → `copy()` is a plain `memcpy` in every direction.
- A mutex-protected `unordered_map<void*, id<MTLBuffer>>` tracks host pointer
  → buffer so `mtl_buffer_handle(ptr)` can hand Vectorization's dispatch layer
  the `MTLBuffer` to bind as a kernel argument; `deallocate` erases the entry
  (ARC releases the buffer). `deallocate(nullptr)` is a no-op.
- No caching/pooling layer — Metal allocations are assumed infrequent relative
  to CUDA workloads.
- No fp64: `allocator<T>` selects a throwing branch for `double` on METAL at
  compile time (`if constexpr`), raising `std::invalid_argument` if reached —
  Apple GPUs have no hardware double support.

---

## 6. Ownership: `data_ptr<T, clone>`

`common/data_ptr.h` — the type Vectorization tensors actually hold.

| Aspect | Design |
|---|---|
| Owning ctor | `data_ptr(size, device)` → `allocator<T>::allocate`; `allocated_=true` |
| Wrapping ctor | `data_ptr(ptr, size, device)` / `(ptr, size, from, to)`: `clone=true` copies into owned memory; `clone=false` borrows (never freed) |
| Copy ctor/assign | Deep-copy iff `clone==true`, otherwise share the borrowed pointer |
| Move | Always transfers ownership, source nulled (`noexcept`) |
| Destructor | Frees only when `allocated_ && data_ != nullptr` |
| `aligned_` flag | Set true for owned allocations (the allocator always satisfies `MEMORY_ALIGNMENT`); computed for borrowed pointers |
| Device index | **Not stored** — CUDA allocations always use device 0 (see limitations) |
| GPU access | Trivial accessors are `__host__ __device__` under `__CUDACC__`/`__HIPCC__` (`DATA_PTR_GPU_CALLABLE`) so tensors can be passed in kernel argument structs |

There is no reference counting: ownership is unique (`clone=true`) or absent
(`clone=false`). Cross-device copies go through `data_ptr::copy(rhs)`, which
allocates on first use and then `allocator<T>::copy`s.

### Cross-device copy matrix (`allocator<T>::copy`)

| from → to | Mechanism |
|---|---|
| CPU → CPU | `memcpy` |
| CPU ↔ CUDA, CUDA ↔ CUDA | `cudaMemcpy` / `cudaMemcpyAsync(stream)` with the right `cudaMemcpyKind`; errors throw `std::runtime_error` |
| CPU ↔ METAL, METAL ↔ METAL | `memcpy` (shared storage) |
| anything else | `std::invalid_argument` |

`from_index`/`to_index` are currently ignored (single-device copy).

---

## 7. Error-handling contract

- **Allocate paths throw**: `std::bad_alloc` on exhaustion (CPU null return,
  CUDA after the flush-and-retry chain, Metal on failure),
  `std::invalid_argument` for unsupported device/type combinations.
- **Free paths**: CPU and Metal frees are non-throwing. CUDA `deallocate`
  throws `std::invalid_argument` on a foreign pointer and `std::logic_error`
  on double free — by design, since both are caller bugs. Because
  `~data_ptr()` is implicitly `noexcept`, such a bug terminates the process
  rather than corrupting silently (previously a failed `cudaFree` was
  swallowed).
- `MEMORY_CHECK` (`util/memory_exception.h`) backs the internal invariants
  with formatted messages.

---

## 8. Configuration matrix

| Knob | CMake | Bazel | Effect |
|---|---|---|---|
| GPU backend | `MEMORY_GPU_BACKEND=none\|cuda\|hip\|metal` | `//bazel:enable_cuda` / `enable_hip` | Sets `MEMORY_HAS_CUDA/HIP/METAL`; gates `gpu/*.cpp` compilation (none excludes them; `.mm` only under metal) |
| mimalloc | `MEMORY_ENABLE_MIMALLOC` (ON) | default on; `//bazel:disable_mimalloc` | Backend #1 for CPU path |
| TBB malloc | `MEMORY_ENABLE_TBB` | `memory_enable_tbb` | Backend #2 |
| NUMA | `MEMORY_ENABLE_NUMA` (Linux) | `memory_enable_numa` | First-touch in CPU allocate |
| memkind | `MEMORY_ENABLE_MEMKIND` (Linux, OFF) | `memory_enable_memkind` | Extended-memory support |
| Logging | `MEMORY_ENABLE_LOGGING` (ON) | — | Links Logging; `MEMORY_HAS_LOGGING` |

The old `MEMORY_GPU_ALLOC` strategy knob and `MEMORY_HAS_ALLOCATION_STATS`
were removed with the machinery that consumed them.

Per-permutation compile coverage: `none` and `metal` are built and tested on
macOS; `cuda` compiles the caching allocator and the registry (verified by
inspection — build on a CUDA host before relying on it); `hip` currently
compiles only the stub (see limitations).

---

## 9. Testing surface

| File | Covers |
|---|---|
| `Testing/Cxx/TestCPUMemory.cpp` | Raw CPU backend: aligned alloc/free, alignment sweep, null-free, `usable_size`, `allocate_zero` |
| `Testing/Cxx/TestCudaCachingAllocator.cpp` | Caching allocator (CUDA builds only) |
| `Testing/Cxx/TestMetalBufferAllocator.cpp` | Metal allocate/deallocate/copy/handle (Metal builds only) |
| `Testing/Cxx/BenchmarkCPUMemoryAllocators.cpp` | Backend comparison benchmark (malloc/aligned/mimalloc/TBB) plus the production path (`memory_allocator`, `allocator<T>`, `data_ptr`) |
| `Testing/Cxx/BenchmarkPyTorchComparison.cpp` | LibTorch CPU-allocation comparison (`c10` allocator and `torch::empty` vs `memory_allocator` and `data_ptr`); built only when `MEMORY_ENABLE_LIBTORCH` is ON and `find_package(Torch)` succeeds |

Test files are globbed per-backend (`TestGpu*`/`TestCuda*`/`TestHip*`/
`TestMetal*` filtered by `MEMORY_GPU_BACKEND`).

---

## 10. Known limitations

1. **CUDA-only compile verification gap.** The CUDA branch of `allocator<T>`
   and the registry were added without a local CUDA toolchain; they need one
   `--gpu_backend=cuda` build on a CUDA machine.
2. **HIP has no allocation path.** `allocator<T>`'s GPU branch compiles only
   under `MEMORY_HAS_CUDA`; HIP-only builds throw for GPU requests (the same
   gap existed before the consolidation — the caching allocator is
   CUDA-specific and no HIP port exists).
3. **Single-device `data_ptr`.** Device index is not stored; CUDA allocations
   and frees assume device 0. Freeing a non-zero-device pointer through the
   device-0 allocator throws (foreign pointer). Multi-GPU callers must use
   `allocator<T>`/`caching_allocator_for_device` directly with matching
   indices.
4. **Throwing free on caller bugs** (foreign/double free) inside a `noexcept`
   destructor terminates. Happy-path frees never throw.
5. **No CPU-side allocation statistics** anymore; the caching allocator's
   `unified_cache_stats` is the only built-in metrics surface.
6. **Metal has no caching layer** and no fp64; both are deliberate for the
   current Vectorization use.
