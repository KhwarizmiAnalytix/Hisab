# Vectorization (`Library/Vectorization`)

**SIMD** library: fixed-size **packets**, expression templates, and backends for **SSE**, **AVX**, **AVX2**, and **AVX-512** (`backend/sse`, `backend/avx`, `backend/avx512`).

## Layout

- `CMakeLists.txt` — `VECTORIZATION_TYPE`, packet size, SVML, testing/benchmark/tooling.
- `BUILD.bazel` — `//Library/Vectorization:Vectorization`, `VectorizationCxxTests`, `benchmark_simd`.
- `Cmake/` — SIMD capability checks (`utils.cmake`, etc.).
- `Testing/` — generated test headers and C++ tests.

---

## CMake options

### Primary selectors

| CMake variable | Default | Summary |
|----------------|---------|---------|
| `VECTORIZATION_TYPE` | `avx2` | `no`, `sse`, `avx`, `avx2`, `avx512` — which backend tree is compiled |
| `VECTORIZATION_PACKET_SIZE` | `4` | Lane count; must match ISA / kernels (max 256) |
| `VECTORIZATION_CXX_STANDARD` | `20` | `11`, `14`, `17`, `20`, `23` |

### Feature and tooling

| CMake variable | Default | Summary |
|----------------|---------|---------|
| `VECTORIZATION_ENABLE_LTO` / `VECTORIZATION_ENABLE_COVERAGE` | OFF | LTO / coverage |
| `VECTORIZATION_ENABLE_TESTING` | ON | Test subtree |
| `VECTORIZATION_ENABLE_EXAMPLES` | OFF | Examples |
| `VECTORIZATION_ENABLE_GTEST` | ON | GoogleTest |
| `VECTORIZATION_ENABLE_BENCHMARK` | ON | `BenchmarkSimd.cpp` → `benchmark_simd` |
| `USE_NATIVE_ARCH` | OFF | `-march=native` (Clang/GCC) |
| `VECTORIZATION_ENABLE_SVML` | *(probe)* | Set in `Cmake/utils.cmake`: ON if the SVML intrinsic probe **fails** for the active SIMD flags (link ThirdParty `svml`); OFF if the compiler already provides those intrinsics |
| `VECTORIZATION_ENABLE_ICECC` / `VECTORIZATION_ENABLE_CACHE` / `VECTORIZATION_ENABLE_CLANGTIDY` / `VECTORIZATION_ENABLE_FIX` / `VECTORIZATION_ENABLE_IWYU` / `VECTORIZATION_ENABLE_SANITIZER` / `VECTORIZATION_ENABLE_SPELL` / `VECTORIZATION_ENABLE_VALGRIND` | see `CMakeLists.txt` | Tooling |

### `CACHE STRING` (tooling)

| CMake variable | Default | Notes |
|----------------|---------|-------|
| `VECTORIZATION_SANITIZER_TYPE` | address | if sanitizer enabled |
| `VECTORIZATION_LINKER_CHOICE` | default | `default`, `lld`, `mold`, `gold`, `lld-link` |
| `VECTORIZATION_CACHE_BACKEND` | none | `none`, `ccache`, `sccache`, `buildcache` |

`VECTORIZATION_HAS_MEMORY` / `VECTORIZATION_HAS_LOGGING` are **detected** from CMake targets when building inside the full tree.

---

## Bazel flags

Starlark: [`bazel/vectorization.bzl`](../../bazel/vectorization.bzl). SIMD × OS groups: [`bazel/vectorization_settings.bzl`](../../bazel/vectorization_settings.bzl).

### SIMD tier and ISA flags

| Define | Effect |
|--------|--------|
| `vectorization_type` | `no` / `sse` / `avx` / `avx2` / `avx512` — selects `//bazel:vectorization_type_*`, backend globs, and `VECTORIZATION_HAS_*` / `VECTORIZATION_VECTORIZED` |
| *(unset)* | Defaults to AVX2 behavior (`//conditions:default` in selects + `.bazelrc` sets `vectorization_type=avx2`) |

Compiler ISA flags come from `vectorization_simd_copts()` (MSVC `/arch:*` or GCC/Clang `-m*`).

### Other preprocessor defines

| Define / rule | Effect |
|---------------|--------|
| `vectorization_enable_svml` | **`true`** → force `//bazel:enable_svml` (always `VECTORIZATION_HAS_SVML=1`, `@svml//:SVML`, `svml.h`). **`false`** → `//bazel:disable_svml` (no SVML). **Unset** → autodetect via `//bazel:svml_configure.bzl` (`vectorization_svml_autodetect` repo): host compile probe per SIMD tier (same snippet as `utils.cmake`); Windows assumes third-party SVML is needed |
| `enable_gtest` | `false` → `//bazel:disable_gtest` → `VECTORIZATION_HAS_GTEST=0` on the library |
| `vectorization_enable_testing` | `false` → `//bazel:vectorization_disable_testing` — **skips** `VectorizationCxxTests` (`target_compatible_with`) |
| `vectorization_enable_benchmark` | `false` → `//bazel:vectorization_disable_benchmark` — **skips** `benchmark_simd` |

Fixed in Starlark today: `VECTORIZATION_PACKET_SIZE=4`, `VECTORIZATION_HAS_MEMORY=1`, `VECTORIZATION_HAS_LOGGING=1` (full monorepo layout).

### Library-only copts

`vectorization_library_copts()` adds MSVC `/WX` and Unix `-include cstdlib` on `vectorization_lib` only (matches CMake `PRIVATE` on the Vectorization target).

### CMake-only

`VECTORIZATION_PACKET_SIZE` and `VECTORIZATION_CXX_STANDARD` are not Bazel `--define`s (packet size and `c++20` are fixed in `vectorization.bzl`). LTO, coverage, sanitizers, cache/linker/tooling — **CMake only**. SVML autodetect is mirrored under Bazel via `WORKSPACE` + `svml_configure` (re-run when `CXX`/`CC` changes).
