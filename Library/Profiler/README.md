# Profiler (`Library/Profiler`)

Profiler **implementation**: native CPU profiler, **Kineto** (PyTorch-style) integration, **ITT** (VTune), Chrome trace export, and tests.

## Documentation

User guides and macro reference: **[`Docs/profiler/`](../../Docs/profiler/README.md)** — e.g. [`Library_Profiler.md`](../../Docs/profiler/Library_Profiler.md), [`profiler.md`](../../Docs/profiler/profiler.md).

## Layout

- `CMakeLists.txt` — `PROFILER_BACKEND`, `PROFILER_ENABLE_*`.
- `BUILD.bazel` — `//Library/Profiler:Profiler` and tests.
- `native/`, `bespoke/kineto/`, `bespoke/itt/` — backends.
- `Testing/Cxx/` — backend and integration tests.

---

## CMake options

### Backend (single control point)

| CMake variable | Default | Values |
|----------------|---------|--------|
| `PROFILER_BACKEND` | `KINETO` | `KINETO`, `NATIVE`, `ITT` — sets `PROFILER_ENABLE_KINETO` / `PROFILER_ENABLE_ITT` / `PROFILER_ENABLE_NATIVE_PROFILER` |

### Feature and toolchain

| CMake variable | Default | Summary |
|----------------|---------|---------|
| `PROFILER_ENABLE_LTO` | OFF | LTO |
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

Starlark: [`bazel/profiler.bzl`](../../bazel/profiler.bzl). `select` order: **native** → **ITT** → default **Kineto**.

### Backend (`profiler_type` + helpers)

Typical invocations (see `.bazelrc`):

| Mode | Defines / config | `PROFILER_HAS_*` result |
|------|-------------------|-------------------------|
| Kineto (default) | *(none)* or `profiler_enable_kineto=true` with `profiler_type=kineto` | `PROFILER_HAS_KINETO=1` |
| Native | `profiler_type=native` — `build:native_profiler` | `PROFILER_HAS_NATIVE=1` |
| ITT | `profiler_type=itt` and `profiler_enable_itt=true` — `build:itt` | `PROFILER_HAS_ITT=1` |

`//bazel:enable_native_profiler` matches `profiler_type=native`.  
`//bazel:enable_itt` matches `profiler_enable_itt=true` (use together with `profiler_type=itt` as in `build:itt`).

### Other

| Mechanism | Effect |
|-----------|--------|
| `profiler_enable_benchmark` | Default ON in `.bazelrc` (CMake parity) |
| `enable_gtest` | Project-wide |

### CMake-only

`PROFILER_CXX_STANDARD` → `c++20` in `profiler.bzl`. LTO, coverage, sanitizers, linker/cache, spell, Valgrind — **CMake only**.
