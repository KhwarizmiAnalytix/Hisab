# GPU Implementation Redesign & Plan

> **Stale for Memory (August 2026).** HIP shares the CUDA caching-allocator TU;
> Metal has a matching segment cache. Ownership is `data_ptr` + `data_view`.
> Do not treat HIP as “raw hipMalloc” or reintroduce the deleted device
> manager / pool / tracking layers. Current Memory status:
> [`Docs/memory_design.md`](../Docs/memory_design.md) §10.
>
> **Stale for Vectorization (August 2026).** Metal expression fusion landed
> (one JIT MSL kernel per assignment). Remaining work is launch-contract
> unification, not unfused lowering. Current status:
> [`Docs/vectorization_backends.md`](../Docs/vectorization_backends.md).

Proposed architecture and implementation plan for CUDA, HIP, and Metal in XSigma.

> **Goal (historical):** Move from compile-time, CUDA-centric GPU support to a
> runtime-polymorphic backend architecture that gives CUDA, HIP, and Metal the
> same allocator, stream, and expression evaluation capabilities.

Older baseline defects: `Docs/CUDA_HIP_REMEDIATION_PLAN.md` (also superseded
for Memory).

---

## 1. What is wrong with the current design

| Problem | Impact |
|---|---|
| `memory::allocator<T>` is broken for HIP | Uses raw `cudaMalloc`/`cudaMemcpy` guarded by `MEMORY_HAS_CUDA`; HIP tensors throw at runtime |
| `cuda_caching_allocator` is CUDA-only | Throws on non-CUDA; no HIP caching allocator |
| Streams/transfers are CUDA-only and use detached `std::thread`s | High latency, race conditions, no HIP/Metal stream support |
| Metal expression eval (historical) lowered to fixed kernels + temps | **Done:** fused JIT MSL. Dispatch is still synchronous (`waitUntilCompleted`) |
| Device manager, memory pool, resource tracker are CUDA-only | No HIP runtime support in Memory library |
| All GPU tests are CUDA-named; no active GPU CI | CUDA-only validation; HIP/Metal untested in CI |

Key files implicated:

- `Library/Memory/allocator.h` — unified allocator, broken HIP path
- `Library/Memory/helper/memory_allocator.cpp` — correct low-level CUDA/HIP dispatch (bypassed by the above)
- `Library/Memory/gpu/cuda_caching_allocator.{h,cpp}` — CUDA-only caching allocator
- `Library/Memory/gpu/gpu_device_manager.cpp`, `gpu_memory_transfer.cpp`, `gpu_memory_pool.cpp` — CUDA-only
- `Library/Vectorization/expressions/expressions_evaluator_metal.h` — fused MSL emit + `run_metal`
- `Library/Vectorization/backend/gpu/metal/metal_dispatch.mm` — fused dispatch; still synchronous (`waitUntilCompleted`)

---

## 2. Design principles

### 2.1 Backend polymorphism at runtime

Replace compile-time backend selection in library code with a runtime backend interface.
Each backend (CUDA, HIP, Metal) implements the same interface; the active backend is
chosen at runtime from available devices.

- **Current:** Compile-time `MEMORY_GPU_BACKEND` picks one backend per build.
- **Proposed:** `memory::gpu::backend` interface with `cuda_backend`, `hip_backend`,
  `metal_backend` implementations.

### 2.2 Single unified allocator

One `memory::allocator<T>` that delegates to the active backend's memory allocator.
No raw `cudaMalloc`/`cudaFree` in the unified layer; all backend-specific calls live
in the backend implementation.

- **Current:** `allocator.h` uses raw `cudaMalloc`/`cudaFree` and is broken for HIP.
- **Proposed:** `allocator.h` calls `backend::allocate/free/copy`; backend handles
  CUDA/HIP/Metal specifics.

### 2.3 Backend-agnostic caching allocator

A generic caching allocator templated on the backend, using backend-specific block
handles and stream types. CUDA and HIP share the same block-pool logic; Metal uses its
own shared-storage model.

- **Current:** `cuda_caching_allocator` is CUDA-only and throws on HIP.
- **Proposed:** `gpu::caching_allocator<Backend>` with stream-aware deferred reclamation
  for CUDA/HIP.

### 2.4 Unified stream/event abstraction

A single `gpu_stream`/`gpu_event` type that wraps `cudaStream_t` / `hipStream_t` /
`id<MTLCommandQueue>`. Async transfers use native callbacks instead of detached
`std::thread`s.

- **Current:** Only `cuda_stream_impl` exists; transfers spawn detached `std::thread`s.
- **Proposed:** `gpu_stream` and `gpu_event` abstract CUDA/HIP/Metal with native async
  callbacks.

### 2.5 Metal fusion strategy

**Landed.** Metal walks the expression tree, JIT-compiles one MSL kernel
(cached by source), and dispatches it. Named kernels remain for fill,
`reduce_sum`, and tests. Open Vectorization work is aligning `run_metal` with
`run_gpu` (stream, pointer signature, reductions, errors) — see
[`Docs/vectorization_backends.md`](../Docs/vectorization_backends.md).

### 2.6 Testing and CI discipline

Backend-agnostic test files that run on any available GPU, plus CI jobs for CUDA and HIP
on Linux and CUDA on Windows.

- **Current:** All GPU tests are CUDA-named; no active GPU CI.
- **Proposed:** `TestGpu*.cpp` parameterized by `device_enum`; Linux CUDA + HIP CI,
  Windows CUDA CI.

---

## 3. Target architecture

| Layer | Components | Responsibility | Backend awareness |
|---|---|---|---|
| User API | `memory::allocator<T>`, `tensor<T>`, `expressions_evaluator` | Backend-agnostic allocation, tensor storage, expression evaluation | None — works with any active backend |
| Backend interface | `memory::gpu::backend`, `gpu_stream`, `gpu_event` | Virtual interface: allocate, free, copy, create_stream, record_event, sync, get_device_info | Runtime polymorphic; one active backend instance per device |
| Backend implementations | `cuda_backend`, `hip_backend`, `metal_backend` | CUDA/HIP/Metal runtime calls, kernel launch, stream/event management | Fully backend-specific; compiled conditionally, selected at runtime |
| Caching allocator | `gpu::caching_allocator<Backend>` | Block pooling, stream-aware deferred reclamation, fragmentation handling | Templated on backend traits (block handle, stream, event) |
| Kernel / compute | CUDA/HIP: expression kernels; Metal: runtime-generated MSL | Fused expression evaluation on GPU | CUDA/HIP share templates; Metal generates MSL source at runtime |

---

## 4. Implementation plan

### Phase 1 — Backend interface + unified allocator fix (1–2 weeks)

**Goal:** Make `memory::allocator<T>` work for all three backends and establish the
runtime backend interface.

Tasks:

- [ ] Define `memory::gpu::backend` abstract interface (allocate, free, copy,
      create_stream, get_device_info, etc.).
- [ ] Implement `cuda_backend`, `hip_backend`, `metal_backend` as concrete classes.
- [ ] Rewrite `memory::allocator<T>` to delegate GPU paths to the backend interface
      (remove raw `cudaMalloc`/`cudaFree`).
- [ ] Move the existing `helper/memory_allocator.cpp` logic into
      `cuda_backend`/`hip_backend`.
- [ ] Update `tensor<T>` and Vectorization tests to use the unified allocator.
- [ ] Add `TestBackendInterface.cpp` with backend-agnostic allocation/copy tests.

Deliverables:

- `memory::allocator<T>` works for CUDA, HIP, and Metal
- `tensor<T>` can allocate on HIP without throwing
- No raw CUDA calls in the unified allocator

**Risk:** Low — mostly refactoring existing correct helper code behind a new interface.

### Phase 2 — Backend-agnostic caching allocator + streams (2–3 weeks)

**Goal:** Provide a single caching allocator that works for CUDA and HIP, and a unified
stream/event abstraction.

Tasks:

- [ ] Create `gpu::caching_allocator<Backend>` templated on backend traits.
- [ ] Migrate `cuda_caching_allocator` logic into the generic template (stream-aware
      blocks, deferred reclamation, best-fit).
- [ ] Implement `hip_stream_impl` and `hip_event_impl` using the new
      `gpu_stream`/`gpu_event` interface.
- [ ] Implement Metal stream/event wrappers around `MTLCommandQueue`/`MTLEvent`.
- [ ] Replace detached-`std::thread` transfers with backend-native async copy + callback.
- [ ] Update `gpu_memory_pool`, `gpu_memory_transfer`, `gpu_resource_tracker` to use the
      backend interface.
- [ ] Add `TestGpuCachingAllocator.cpp` and `TestGpuStreams.cpp` running on both CUDA
      and HIP.

Deliverables:

- Caching allocator works on CUDA and HIP
- No `std::thread`-per-transfer
- Streams/events are backend-agnostic

**Risk:** Medium — touches core allocator logic; needs careful stream-safety review.

### Phase 3 — Metal fusion and op expansion (2–4 weeks)

**Goal:** Bring Metal to feature parity for the expression-tree subset and add kernel
fusion.

Tasks:

- [ ] Design a runtime MSL kernel generator: given an expression-tree type, emit a
      single fused MSL kernel.
- [ ] Implement a kernel cache keyed by expression type hash (avoid recompiling MSL).
- [ ] Expand the Metal kernel set: comparisons, min/max, multi-block `reduce_sum`,
      gather/scatter, more math ops.
- [ ] Add async Metal dispatch using `MTLCommandBuffer` completion handlers instead of
      `waitUntilCompleted`.
- [ ] Support Metal double via software emulation (if needed) or document float-only
      clearly.
- [ ] Add `TestMetalFusion.cpp` and `TestMetalOps.cpp`.

Deliverables:

- Metal expression trees fuse into single kernels
- Async Metal dispatch available
- Metal op coverage matches the Vectorization starter set

**Risk:** Medium–High — MSL codegen is new ground; float-only limitation remains a
hardware constraint.

### Phase 4 — Testing, CI, and cleanup (1–2 weeks)

**Goal:** Regression-protect the new design and remove legacy CUDA-only paths.

Tasks:

- [ ] Parameterize all GPU tests by `device_enum` so they run on CUDA, HIP, and Metal
      when available.
- [ ] Re-enable Linux CUDA CI job and add Linux HIP CI job (ROCm container).
- [ ] Add Windows CUDA CI job if runner capacity allows.
- [ ] Remove or deprecate `cuda_caching_allocator.h/cpp`, `gpu_device_manager.cpp`, and
      other CUDA-only legacy files.
- [ ] Update `Docs/CUDA_HIP_REMEDIATION_PLAN.md` to reflect the new architecture.
- [ ] Add backend-selection documentation and examples.

Deliverables:

- GPU tests run on all three backends
- Active CI for CUDA and HIP
- Legacy CUDA-only code removed or deprecated

**Risk:** Low — mostly test and CI work.

---

## 5. Migration map

| Legacy / current | New design | Phase |
|---|---|---|
| `memory::allocator<T>` raw `cudaMalloc`/`cudaFree` | `memory::gpu::backend::allocate/free` via runtime backend | Phase 1 |
| `helper/memory_allocator.cpp` direct CUDA/HIP branches | Moved into `cuda_backend` / `hip_backend` implementations | Phase 1 |
| `cuda_caching_allocator` (CUDA-only) | `gpu::caching_allocator<cuda_backend>` / `<hip_backend>` | Phase 2 |
| `cuda_stream_impl` only | `gpu_stream` interface + cuda/hip/metal implementations | Phase 2 |
| Detached `std::thread` transfers | Backend-native async copy with callbacks | Phase 2 |
| Fixed Metal kernels + temp-buffer dispatch | Runtime-generated fused MSL kernels | Phase 3 |
| CUDA-named tests only | `TestGpu*.cpp` parameterized by `device_enum` | Phase 4 |

---

## 6. Key code sketches

### Backend interface

```cpp
namespace memory::gpu {

class backend {
public:
    virtual ~backend() = default;

    virtual device_enum type() const = 0;
    virtual bool available() const = 0;

    virtual void* allocate(size_t bytes, int device, void* stream) = 0;
    virtual void free(void* ptr, size_t bytes, int device, void* stream) = 0;
    virtual void copy(const void* src, void* dst, size_t bytes,
                      device_enum src_type, device_enum dst_type,
                      void* stream) = 0;

    virtual std::unique_ptr<gpu_stream> create_stream(int device, int priority) = 0;
    virtual std::unique_ptr<gpu_event> create_event(int device) = 0;

    virtual gpu_device_info get_device_info(int device) const = 0;
};

backend& active_backend();  // runtime-selected

}  // namespace memory::gpu
```

### Unified allocator delegation

```cpp
template <class T, std::size_t alignment>
struct allocator {
    static pointer allocate(size_type n, device_enum type, int device_index = 0) {
        if (type == device_enum::CPU) {
            return static_cast<pointer>(cpu_allocate(n * sizeof(T), alignment));
        }
        // CUDA, HIP, or Metal: delegate to the active backend
        void* ptr = memory::gpu::active_backend().allocate(
            n * sizeof(T), device_index, nullptr);
        if (!ptr) throw std::bad_alloc();
        return static_cast<pointer>(ptr);
    }

    static void free(pointer& ptr, device_enum type, int device_index = 0) {
        if (type == device_enum::CPU) {
            cpu_free(ptr);
        } else {
            memory::gpu::active_backend().free(ptr, 0, device_index, nullptr);
        }
        ptr = nullptr;
    }
};
```

### Backend-agnostic caching allocator

```cpp
template <typename Backend>
class caching_allocator {
    using block_handle = typename Backend::block_handle;
    using stream_type  = typename Backend::stream_type;

public:
    void* allocate(size_t size, stream_type stream);
    void deallocate(void* ptr, size_t size, stream_type stream);
    void empty_cache();

private:
    struct Block {
        void* ptr;
        size_t size;
        stream_type last_stream;
        typename Backend::event_type event;
        bool in_use;
    };

    std::mutex mutex_;
    std::unordered_map<void*, std::unique_ptr<Block>> blocks_;
    std::multimap<size_t, Block*> free_blocks_;
    std::vector<Block*> deferred_blocks_;
};

// Instantiations
using cuda_caching_allocator = caching_allocator<cuda_backend>;
using hip_caching_allocator  = caching_allocator<hip_backend>;
```

### Metal fusion (sketch)

```cpp
// Given an expression tree type E, generate one fused MSL kernel.
template <typename E>
std::string generate_metal_kernel() {
    // Walk the expression tree type at compile time and emit MSL source:
    //   out[tid] = expression_loader<E>::evaluate(inputs, tid);
    // Cache the compiled pipeline by typeid(E).hash_code().
}

template <typename E, typename T>
void run_metal_fused(E const& expr, T* out, size_t n) {
    static auto pso = get_or_compile_pso<E>();
    dispatch_fused_kernel(pso, expr, out, n);
}
```

---

## 7. Success criteria

### Functional

- `tensor<T>` allocates and computes on CUDA, HIP, and Metal without throwing.
- Caching allocator works on CUDA and HIP.
- Metal expression trees fuse into single kernels.

### Performance

- No detached `std::thread` per transfer.
- Caching allocator hit rate > 90% for repeated patterns.
- Metal dispatch latency comparable to CUDA/HIP for same ops.

### Quality

- GPU tests run on all three backends in CI.
- No raw backend-specific calls in unified API layers.
- Documentation updated for backend selection.

---

## 8. Guardrails while implementing

- **Never modify anything under `ThirdParty/`** — use ADL shims / force-included headers
  / CMake-level guards instead (see `Library/Vectorization/Testing/Cxx/cuda_fmt_int128_fix.h`).
- **Use `MEMORY_HAS_CUDA` / `MEMORY_HAS_HIP`** (and per-module `VECTORIZATION_HAS_*`)
  for GPU-conditional code — never `PROJECT_HAS_*`, which is defined nowhere and
  silently compiles code out (see commit `f15cf987`).
- **`try`/`catch` stays at the CUDA/HIP runtime call boundary only** — translate to the
  project's own error/result type immediately; don't propagate exceptions across layers.
- **HIP is Unix-only** by policy; don't add Windows HIP workarounds.
- **Build/test via `Scripts/setup.py`** — do not invoke CMake/ninja/ctest directly.
