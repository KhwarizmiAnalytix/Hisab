# Logging (`Library/Logging`)

**Structured logging**: levels, lazy evaluation, back traces, and a pluggable backend — **Loguru** (default in CMake), **glog**, or **native** (fmt-based).

## Layout

- `CMakeLists.txt` — `LOGGING_BACKEND`, `LOGGING_ENABLE_*`.
- `BUILD.bazel` — `//Library/Logging:Logging`; backend deps from `select`.
- `logger/` — public headers and implementation.
- `Testing/Cxx/` — unit tests.

---

## CMake options

### Backend

| CMake variable | Default | Values |
|----------------|---------|--------|
| `LOGGING_BACKEND` | `LOGURU` | `NATIVE`, `LOGURU`, `GLOG` — exactly one `LOGGING_HAS_*=1` |

### Feature and toolchain

| CMake variable | Default | Summary |
|----------------|---------|---------|
| `LOGGING_ENABLE_LTO` | OFF | LTO |
| `LOGGING_ENABLE_COVERAGE` | OFF | Coverage |
| `LOGGING_ENABLE_TESTING` | ON | Tests |
| `LOGGING_ENABLE_EXAMPLES` | OFF | Examples |
| `LOGGING_ENABLE_GTEST` | ON | GoogleTest |
| `LOGGING_ENABLE_BENCHMARK` | ON | Google Benchmark |
| `LOGGING_ENABLE_ICECC` / `LOGGING_ENABLE_CACHE` / `LOGGING_ENABLE_CLANGTIDY` / `LOGGING_ENABLE_FIX` / `LOGGING_ENABLE_IWYU` / `LOGGING_ENABLE_SANITIZER` / `LOGGING_ENABLE_SPELL` / `LOGGING_ENABLE_VALGRIND` | see `CMakeLists.txt` | Tooling |

### `CACHE STRING`

| CMake variable | Default | Notes |
|----------------|---------|-------|
| `LOGGING_CXX_STANDARD` | 20 | `11`–`23` |
| `LOGGING_SANITIZER_TYPE` | address | if sanitizer ON |
| `LOGGING_LINKER_CHOICE` | default | linker selection |
| `LOGGING_CACHE_BACKEND` | none | ccache / sccache / buildcache |

---

## Bazel flags

Starlark: [`bazel/logging.bzl`](../../bazel/logging.bzl).

### Backend (`logging_backend`)

| Define | `config_setting` | `LOGGING_HAS_*` |
|--------|------------------|-----------------|
| *(unset)* | default branch | Loguru = 1 |
| `logging_backend=glog` | `//bazel:logging_glog` | Glog = 1 |
| `logging_backend=native` | `//bazel:logging_native` | Native = 1 |
| `logging_backend=loguru` | `//bazel:logging_loguru` | Explicit Loguru (same as default) |

Use `--config=logging_glog`, `logging_loguru`, or `logging_native` from `.bazelrc`.

### Other

| Mechanism | Effect |
|-----------|--------|
| `logging_enable_benchmark` | Default ON in root `.bazelrc` (CMake parity); benchmark targets live under `Testing/Cxx/BUILD.bazel` |
| `enable_gtest` | Project-wide; affects other libraries’ `HAS_GTEST` — Logging tests use gtest deps directly |

### CMake-only

`LOGGING_CXX_STANDARD` → `c++20` in `logging.bzl`. LTO, coverage, sanitizers, clang-tidy, linker/cache, spell, Valgrind, Icecream — **CMake only**.
