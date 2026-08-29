# XSigma — Project CMake Flags Reference

## Scope definitions

| Scope | How declared | Persisted | User-settable via `-D` |
|---|---|---|---|
| **Global** | `option()` or `set(... CACHE ...)` | `CMakeCache.txt` | Yes |
| **Local** | `set()` (no CACHE) | Never | No — computed at configure time |

---

## Global flags — user-configurable

These survive between `cmake` invocations and can be overridden with `-DFLAG=value`.

### Build output

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `BUILD_SHARED_LIBS` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Build shared libraries (`.so` / `.dll`). Drives `BUILD_SHARED_LIBS`. |
| `BUILD_TESTING` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Enable test subdirectories for all modules. Values: `ON`, `OFF`, `WANT`. |
| `PROJECT_ENABLE_EXAMPLES` | `OFF` | [CMakeLists.txt](../CMakeLists.txt) | Build programs under `Examples/`. |
| `XSIGMA_ENABLE_EXTERNAL` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Prefer system third-party libraries. Mapped onto the vendored `XSIGMA_ENABLE_EXTERNAL` option inside `ThirdParty/` (do not edit that tree). |
| `XSIGMA_LIBRARY_PROJECT` | empty | [CMakeLists.txt](../CMakeLists.txt) | If set, configure only that `Library/*` module and its CMake deps (`setup.py --project.NAME`). |

### Vectorization

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `VECTORIZATION_CPU_BACKEND` | host-dependent | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | SIMD target. One of: `no`, `sse`, `avx`, `avx2`, `avx512`, `neon`, `sve`. Gates which `PROJECT_SSE/AVX/AVX2/AVX512/NEON/SVE` local flag is set by [Library/Vectorization/Cmake/utils.cmake](../Library/Vectorization/Cmake/utils.cmake). |

### Features / optional libraries

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `ENABLE_GTEST` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Pull in Google Test. |
| `CORE_ENABLE_MAGICENUM` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Pull in `magic_enum` for static enum reflection. |
| `CORE_ENABLE_EXPERIMENTAL` | `OFF` | [Library/Core/Cmake/experimental.cmake](../Library/Core/Cmake/experimental.cmake) | Enable features under active development. Sets `PROJECT_EXPERIMENTAL_FOUND=TRUE` when `ON`. |
| `ENABLE_BENCHMARK` | `OFF` | [CMakeLists.txt](../CMakeLists.txt) | Pull in Google Benchmark. |
| `ENABLE_STATIC_MKL` | `OFF` | [Library/Core/Cmake/mkl.cmake](../Library/Core/Cmake/mkl.cmake) | Prefer static MKL linkage on Unix. Has no effect when `CORE_ENABLE_MKL=OFF`. |
| `CORE_ENABLE_COMPRESSION` | `OFF` | [Library/Core/Cmake/compression.cmake](../Library/Core/Cmake/compression.cmake) | Enable data compression support. |
| `CORE_COMPRESSION_TYPE` | `none` | [Library/Core/Cmake/compression.cmake](../Library/Core/Cmake/compression.cmake) | Compression backend. One of: `none`, `snappy`. |

### LTO (Link-Time Optimization)

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_ENABLE_LTO` | `OFF` | [CMakeLists.txt](../CMakeLists.txt) | **Aggregate read-only flag.** Set to `ON` automatically when any per-module LTO option (`LOGGING_ENABLE_LTO`, `MEMORY_ENABLE_LTO`, …) is `ON`. Drives `CMAKE_INTERPROCEDURAL_OPTIMIZATION`. Do not set directly. |

### Build speed / compiler caching

Each library module owns its cache settings. `Scripts/setup.py` fans out `cache_type` to all `*_CACHE_BACKEND` variables and `cache` to all `*_ENABLE_CACHE` variables (pass the `cache` token to disable launchers when defaults are ON).

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `LOGGING_ENABLE_CACHE` | `ON` | [Library/Logging/CMakeLists.txt](../Library/Logging/CMakeLists.txt) | Enable compiler caching for the Logging target. |
| `LOGGING_CACHE_BACKEND` | `none` | [Library/Logging/CMakeLists.txt](../Library/Logging/CMakeLists.txt) | Cache backend for Logging: `none`, `ccache`, `sccache`, `buildcache`. |
| `MEMORY_ENABLE_CACHE` | `ON` | [Library/Memory/CMakeLists.txt](../Library/Memory/CMakeLists.txt) | Enable compiler caching for the Memory target. |
| `MEMORY_CACHE_BACKEND` | `none` | [Library/Memory/CMakeLists.txt](../Library/Memory/CMakeLists.txt) | Cache backend for Memory: `none`, `ccache`, `sccache`, `buildcache`. |
| `CORE_ENABLE_CACHE` | `ON` | [Library/Core/CMakeLists.txt](../Library/Core/CMakeLists.txt) | Enable compiler caching for the Core target. |
| `CORE_CACHE_BACKEND` | `none` | [Library/Core/CMakeLists.txt](../Library/Core/CMakeLists.txt) | Cache backend for Core: `none`, `ccache`, `sccache`, `buildcache`. |
| `PARALLEL_ENABLE_CACHE` | `ON` | [Library/Parallel/CMakeLists.txt](../Library/Parallel/CMakeLists.txt) | Enable compiler caching for the Parallel target. |
| `PARALLEL_CACHE_BACKEND` | `none` | [Library/Parallel/CMakeLists.txt](../Library/Parallel/CMakeLists.txt) | Cache backend for Parallel: `none`, `ccache`, `sccache`, `buildcache`. |
| `PROFILER_ENABLE_CACHE` | `ON` | [Library/Profiler/CMakeLists.txt](../Library/Profiler/CMakeLists.txt) | Enable compiler caching for the Profiler target. |
| `PROFILER_CACHE_BACKEND` | `none` | [Library/Profiler/CMakeLists.txt](../Library/Profiler/CMakeLists.txt) | Cache backend for Profiler: `none`, `ccache`, `sccache`, `buildcache`. |
| `LOGGING_ENABLE_ICECC` | `OFF` | [Library/Logging/CMakeLists.txt](../Library/Logging/CMakeLists.txt) | Use Icecream distributed compilation for Logging. |
| `MEMORY_ENABLE_ICECC` | `OFF` | [Library/Memory/CMakeLists.txt](../Library/Memory/CMakeLists.txt) | Use Icecream distributed compilation for Memory. |
| `CORE_ENABLE_ICECC` | `OFF` | [Library/Core/CMakeLists.txt](../Library/Core/CMakeLists.txt) | Use Icecream distributed compilation for Core. |
| `PARALLEL_ENABLE_ICECC` | `OFF` | [Library/Parallel/CMakeLists.txt](../Library/Parallel/CMakeLists.txt) | Use Icecream distributed compilation for Parallel. |
| `PROFILER_ENABLE_ICECC` | `OFF` | [Library/Profiler/CMakeLists.txt](../Library/Profiler/CMakeLists.txt) | Use Icecream distributed compilation for Profiler. |
| `PROJECT_LINKER_CHOICE` | `default` | [tools/lto.cmake](../Cmake/tools/lto.cmake) | Preferred linker. One of: `default`, `lld`, `mold`, `gold`, `lld-link`. Auto-detected when set to `default`. |

### Code quality / analysis

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_ENABLE_SANITIZER` | `OFF` | [tools/sanitize.cmake](../Cmake/tools/sanitize.cmake) | Enable a compiler sanitizer. Clang/GCC only. Forces `MEMORY_ENABLE_MIMALLOC=OFF` and `PROJECT_ENABLE_LTO=OFF`. |
| `PROJECT_SANITIZER_TYPE` | `address` | [tools/sanitize.cmake](../Cmake/tools/sanitize.cmake) | Which sanitizer to use. One of: `address`, `undefined`, `thread`, `memory`, `leak`. Only active when `PROJECT_ENABLE_SANITIZER=ON`. |

### WebAssembly (Emscripten targets only)

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_WEBASSEMBLY_THREADS` | *(unset)* | [flags/platform.cmake](../Cmake/flags/platform.cmake) | Enable `-pthread` and shared memory for WASM builds. Pass via `-D` when targeting Emscripten. |
| `PROJECT_WEBASSEMBLY_64_BIT` | *(unset)* | [flags/platform.cmake](../Cmake/flags/platform.cmake) | Enable `-sMEMORY64=1` for WASM64. Pass via `-D` when targeting Emscripten. |

### Platform / toolchain overrides

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_LLVM_INSTALL_PREFIX` | *(auto)* | [flags/platform.cmake](../Cmake/flags/platform.cmake) | Override the LLVM installation root used to set macOS linker rpath. Falls back to `/opt/homebrew/opt/llvm`. |
| `USE_NATIVE_ARCH` | `OFF` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Add `-march=native` to compiler flags if the compiler supports it. |

### Algorithmic / numeric options

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_SOBOL_1111` | *(unset)* | [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake) | Enable Sobol 1111 sequence variant. Emits `PROJECT_SOBOL_1111=1` compile definition when set. |
| `PROJECT_LU_PIVOTING` | *(unset)* | [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake) | Enable LU pivoting in linear algebra routines. Emits `PROJECT_LU_PIVOTING=1` compile definition when set. |

---

## Local flags — computed at configure time

These are set with plain `set()` (no CACHE). They are derived from detection probes or from the global flags above. They **cannot** be overridden via `-D` and are **not** persisted in `CMakeCache.txt`.

### Compiler / exception detection

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_HAS_EXCEPTION_PTR` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when the compiler can compile `std::exception_ptr` under `-std=c++20`. Consumed by [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake) → emits `PROJECT_HAS_EXCEPTION_PTR=1\|0`. |

### Vectorization detection

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_SSE` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=sse` and SSE is supported. Consumed by [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake). |
| `PROJECT_AVX` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=avx` and AVX is supported. |
| `PROJECT_AVX2` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=avx2` and AVX2 is supported. |
| `PROJECT_AVX512` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=avx512` and AVX512 is supported. |
| `PROJECT_NEON` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=neon` and NEON is supported. |
| `PROJECT_SVE` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | Set to `1` when `VECTORIZATION_CPU_BACKEND=sve` and SVE is supported. |
| `VECTORIZATION` | [Library/Vectorization/Cmake/utils.cmake](../Library/Vectorization/Cmake/utils.cmake) | `ON` when an active SIMD backend is supported. Used as a guard for SVML detection. |
| `VECTORIZATION_COMPILER_FLAGS` | [Library/Vectorization/CMakeLists.txt](../Library/Vectorization/CMakeLists.txt) | The actual compiler flags for the active SIMD level (for example, `-mavx2 -mf16c -mfma`). Applied to `CMAKE_C/CXX_FLAGS` by Vectorization. |

### Assembled compile-definition lists

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_COMPILE_DEFINITIONS` | [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake) | List of `NAME=0\|1` defines assembled from all `PROJECT_*` hardware / compiler detections. Consumed directly by targets that opt in. |
| `PROJECT_DEPENDENCY_COMPILE_DEFINITIONS` | [flags/compile_definitions.cmake](../Cmake/flags/compile_definitions.cmake) | Copy of `PROJECT_COMPILE_DEFINITIONS`. Seeded into every module's `{MODULE}_DEPENDENCY_COMPILE_DEFINITIONS` list in [CMakeLists.txt](../CMakeLists.txt). |

### Sanitizer / compiler-cache resolved values

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_SANITIZER` | [tools/sanitize.cmake](../Cmake/tools/sanitize.cmake) | Resolved copy of `PROJECT_SANITIZER_TYPE` stored as a CACHE STRING, used to build the `-fsanitize=` flag. |
| `XSIGMA_CCACHE_PROGRAM` | [tools/cache.cmake](../Cmake/tools/cache.cmake) | Path to the found `ccache` executable. Lazily populated on first `xsigma_target_apply_cache()` call that uses `ccache`. |
| `XSIGMA_SCCACHE_PROGRAM` | [tools/cache.cmake](../Cmake/tools/cache.cmake) | Path to the found `sccache` executable. Lazily populated on first `xsigma_target_apply_cache()` call that uses `sccache`. |
| `XSIGMA_BUILDCACHE_PROGRAM` | [tools/cache.cmake](../Cmake/tools/cache.cmake) | Path to the found `buildcache` executable. Lazily populated on first `xsigma_target_apply_cache()` call that uses `buildcache`. |

### Experimental feature marker

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_EXPERIMENTAL_FOUND` | [Library/Core/Cmake/experimental.cmake](../Library/Core/Cmake/experimental.cmake) | `CACHE BOOL TRUE` — set only when `CORE_ENABLE_EXPERIMENTAL=ON`. Signals to downstream code that experimental APIs are available. |

---

## Flag data-flow summary

```
CMakeLists.txt  ─── VECTORIZATION_CPU_BACKEND ──►  utils.cmake
                                                      │
                                              probes SSE/AVX/AVX2/AVX512
                                                      │
                              PROJECT_SSE/AVX/AVX2/AVX512  VECTORIZATION_COMPILER_FLAGS
                                      │                          │
                                      ▼                          ▼
                          compile_definitions.cmake        platform.cmake
                          (builds PROJECT_COMPILE_         (appends to CMAKE_CXX_FLAGS)
                           DEFINITIONS list)
                                      │
                                      ▼
                          PROJECT_DEPENDENCY_COMPILE_DEFINITIONS
                                      │
                         fan-out to each module's
                         {MODULE}_DEPENDENCY_COMPILE_DEFINITIONS
```
