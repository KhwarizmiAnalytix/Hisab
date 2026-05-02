# Quarisma — Project CMake Flags Reference

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

### Vectorization

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `VECTORIZATION_TYPE` | `avx2` | [CMakeLists.txt](../CMakeLists.txt) | SIMD target. One of: `no`, `sse`, `avx`, `avx2`, `avx512`. Gates which `PROJECT_SSE/AVX/AVX2/AVX512` local flag is set by [utils.cmake](tools/utils.cmake). |

### Features / optional libraries

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `ENABLE_GTEST` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Pull in Google Test. |
| `CORE_ENABLE_MAGICENUM` | `ON` | [CMakeLists.txt](../CMakeLists.txt) | Pull in `magic_enum` for static enum reflection. |
| `CORE_ENABLE_EXPERIMENTAL` | `OFF` | [tools/experimental.cmake](tools/experimental.cmake) | Enable features under active development. Sets `PROJECT_EXPERIMENTAL_FOUND=TRUE` when `ON`. |
| `ENABLE_BENCHMARK` | `OFF` | [CMakeLists.txt](../CMakeLists.txt) | Pull in Google Benchmark. |
| `CORE_ENABLE_STATIC_MKL` | `OFF` | [tools/mkl.cmake](tools/mkl.cmake) | Prefer static MKL linkage on Unix. Has no effect when `CORE_ENABLE_MKL=OFF`. |
| `CORE_ENABLE_COMPRESSION` | `OFF` | [tools/compression.cmake](tools/compression.cmake) | Enable data compression support. |
| `CORE_COMPRESSION_TYPE` | `none` | [tools/compression.cmake](tools/compression.cmake) | Compression backend. One of: `none`, `snappy`. |

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
| `PROJECT_LINKER_CHOICE` | `default` | [tools/linker.cmake](tools/linker.cmake) | Preferred linker. One of: `default`, `lld`, `mold`, `gold`, `lld-link`. Auto-detected when set to `default`. |

### Code quality / analysis

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_ENABLE_SANITIZER` | `OFF` | [tools/sanitize.cmake](tools/sanitize.cmake) | Enable a compiler sanitizer. Clang/GCC only. Forces `MEMORY_ENABLE_MIMALLOC=OFF` and `PROJECT_ENABLE_LTO=OFF`. |
| `PROJECT_SANITIZER_TYPE` | `address` | [tools/sanitize.cmake](tools/sanitize.cmake) | Which sanitizer to use. One of: `address`, `undefined`, `thread`, `memory`, `leak`. Only active when `PROJECT_ENABLE_SANITIZER=ON`. |

### WebAssembly (Emscripten targets only)

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_WEBASSEMBLY_THREADS` | *(unset)* | [flags/platform.cmake](flags/platform.cmake) | Enable `-pthread` and shared memory for WASM builds. Pass via `-D` when targeting Emscripten. |
| `PROJECT_WEBASSEMBLY_64_BIT` | *(unset)* | [flags/platform.cmake](flags/platform.cmake) | Enable `-sMEMORY64=1` for WASM64. Pass via `-D` when targeting Emscripten. |

### Platform / toolchain overrides

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_LLVM_INSTALL_PREFIX` | *(auto)* | [flags/platform.cmake](flags/platform.cmake) | Override the LLVM installation root used to set macOS linker rpath. Falls back to `/opt/homebrew/opt/llvm`. |
| `USE_NATIVE_ARCH` | *(unset)* | [tools/utils.cmake](tools/utils.cmake) | Add `-march=native` to compiler flags if the compiler supports it. |

### Algorithmic / numeric options

| Flag | Default | Declared in | Description |
|---|---|---|---|
| `PROJECT_SOBOL_1111` | *(unset)* | [flags/compile_definitions.cmake](flags/compile_definitions.cmake) | Enable Sobol 1111 sequence variant. Emits `PROJECT_SOBOL_1111=1` compile definition when set. |
| `PROJECT_LU_PIVOTING` | *(unset)* | [flags/compile_definitions.cmake](flags/compile_definitions.cmake) | Enable LU pivoting in linear algebra routines. Emits `PROJECT_LU_PIVOTING=1` compile definition when set. |

---

## Local flags — computed at configure time

These are set with plain `set()` (no CACHE). They are derived from detection probes or from the global flags above. They **cannot** be overridden via `-D` and are **not** persisted in `CMakeCache.txt`.

### Compiler / exception detection

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_HAS_EXCEPTION_PTR` | [tools/utils.cmake](tools/utils.cmake) | Set to `1` when the compiler can compile `std::exception_ptr` under `-std=c++20`. Consumed by [flags/compile_definitions.cmake](flags/compile_definitions.cmake) → emits `PROJECT_HAS_EXCEPTION_PTR=1\|0`. |
| `PROJECT_EXCEPTION_PTR_SUPPORTED` | [tools/utils.cmake](tools/utils.cmake) | Intermediate `check_cxx_source_compiles` result. Do not use directly; read `PROJECT_HAS_EXCEPTION_PTR` instead. |
| `PROJECT_IS_NUMA_AVAILABLE` | [tools/utils.cmake](tools/utils.cmake) | Set when `<numa.h>` / `<numaif.h>` compile successfully. Resets `MEMORY_ENABLE_NUMA=OFF` when absent. |

### Vectorization detection

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_COMPILER_SUPPORTS_SSE_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler accepts SSE intrinsics. |
| `PROJECT_COMPILER_SUPPORTS_AVX_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler accepts AVX intrinsics. |
| `PROJECT_COMPILER_SUPPORTS_AVX2_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler accepts AVX2 intrinsics. |
| `PROJECT_COMPILER_SUPPORTS_AVX512_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler accepts AVX512F/DQ/VL intrinsics. |
| `PROJECT_COMPILER_SUPPORTS_FMA_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler accepts FMA intrinsics. Extends `VECTORIZATION_COMPILER_FLAGS` when true. |
| `PROJECT_COMPILER_SUPPORTS_SVML_EXTENSIONS` | [tools/utils.cmake](tools/utils.cmake) | Raw probe result: compiler has native SVML (`_mm256_exp_ps` etc.). When absent and vectorization is active, `VECTORIZATION_ENABLE_SVML` is forced to `ON` (see [Library/Vectorization/Cmake/utils.cmake](../Library/Vectorization/Cmake/utils.cmake)). |
| `PROJECT_SSE` | [tools/utils.cmake](tools/utils.cmake) | Set to `1` when `VECTORIZATION_TYPE=sse` and SSE is supported. Consumed by [flags/compile_definitions.cmake](flags/compile_definitions.cmake). |
| `PROJECT_AVX` | [tools/utils.cmake](tools/utils.cmake) | Set to `1` when `VECTORIZATION_TYPE=avx` and AVX is supported. |
| `PROJECT_AVX2` | [tools/utils.cmake](tools/utils.cmake) | Set to `1` when `VECTORIZATION_TYPE=avx2` and AVX2 is supported. |
| `PROJECT_AVX512` | [tools/utils.cmake](tools/utils.cmake) | Set to `1` when `VECTORIZATION_TYPE=avx512` and AVX512 is supported. |
| `VECTORIZATION` | [tools/utils.cmake](tools/utils.cmake) | `ON` when any of `PROJECT_SSE/AVX/AVX2/AVX512` is set. Used as a guard for SVML detection. |
| `VECTORIZATION_COMPILER_FLAGS` | [tools/utils.cmake](tools/utils.cmake) | The actual compiler flags for the active SIMD level (e.g. `-mavx2 -mf16c -mfma`). Applied to `CMAKE_C/CXX_FLAGS` in [flags/platform.cmake](flags/platform.cmake). |

### Assembled compile-definition lists

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_COMPILE_DEFINITIONS` | [flags/compile_definitions.cmake](flags/compile_definitions.cmake) | List of `NAME=0\|1` defines assembled from all `PROJECT_*` hardware / compiler detections. Consumed directly by targets that opt in. |
| `PROJECT_DEPENDENCY_COMPILE_DEFINITIONS` | [flags/compile_definitions.cmake](flags/compile_definitions.cmake) | Copy of `PROJECT_COMPILE_DEFINITIONS`. Seeded into every module's `{MODULE}_DEPENDENCY_COMPILE_DEFINITIONS` list in [CMakeLists.txt](../CMakeLists.txt). |

### Sanitizer / compiler-cache resolved values

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_SANITIZER` | [tools/sanitize.cmake](tools/sanitize.cmake) | Resolved copy of `PROJECT_SANITIZER_TYPE` stored as a CACHE STRING, used to build the `-fsanitize=` flag. |
| `QUARISMA_CCACHE_PROGRAM` | [tools/cache.cmake](tools/cache.cmake) | Path to the found `ccache` executable. Lazily populated on first `quarisma_target_apply_cache()` call that uses `ccache`. |
| `QUARISMA_SCCACHE_PROGRAM` | [tools/cache.cmake](tools/cache.cmake) | Path to the found `sccache` executable. Lazily populated on first `quarisma_target_apply_cache()` call that uses `sccache`. |
| `QUARISMA_BUILDCACHE_PROGRAM` | [tools/cache.cmake](tools/cache.cmake) | Path to the found `buildcache` executable. Lazily populated on first `quarisma_target_apply_cache()` call that uses `buildcache`. |

### Experimental feature marker

| Flag | Set in | Description |
|---|---|---|
| `PROJECT_EXPERIMENTAL_FOUND` | [tools/experimental.cmake](tools/experimental.cmake) | `CACHE BOOL TRUE` — set only when `CORE_ENABLE_EXPERIMENTAL=ON`. Signals to downstream code that experimental APIs are available. |

---

## Flag data-flow summary

```
CMakeLists.txt  ─── VECTORIZATION_TYPE ──►  utils.cmake
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
