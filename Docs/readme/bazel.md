# Bazel build guide (Quarisma)

This document describes how Quarisma is built with **Bazel**, how that lines up with **CMake** and `Scripts/setup.py`, and what you need if you treat a **single `Library/*` tree** as its own repository.

## Prerequisites

- **Bazel 7+ / 8** via [Bazelisk](https://github.com/bazelbuild/bazelisk) (recommended) or a system `bazel` binary.
- A C++ toolchain appropriate for your OS (Clang is the default assumed by `Scripts/setup_bazel.py`; MSVC is used when you pass `vs17` / `vs19` / `vs22` / `vs26` on Windows).
- Repository root contains `MODULE.bazel`, `WORKSPACE.bazel`, `.bazelrc`, `bazel/`, `Library/`, `ThirdParty/`, and `Scripts/setup_bazel.py`.

Optional user overrides: create `.bazelrc.user` in the repo root (`try-import` is already wired from `.bazelrc`).

## Quick start

From the repository root:

```bash
# Configure summary only (no compile)
python Scripts/setup_bazel.py config.release

# Debug build + tests (default toolchain: Ninja-style + Clang via Bazel’s CC toolchain)
python Scripts/setup_bazel.py build.test

# Release, AVX2, run tests — dotted tokens match setup.py ergonomics
python Scripts/setup_bazel.py config.build.test.release.avx2

# Long flags aligned with setup.py
python Scripts/setup_bazel.py build.test.release --sanitizer.undefined
python Scripts/setup_bazel.py build.test --parallel.tbb --logging.glog
```

Full CLI reference: `python Scripts/setup_bazel.py --help`.

## How Bazel maps to CMake / `setup.py`

`Scripts/setup.py` drives **CMake** with a large `QuarismaFlags` surface (per-module fan-out of `*_ENABLE_*` options). `Scripts/setup_bazel.py` translates a **deliberately similar** token language into Bazel `--config=…` flags and occasional `--define=<cmake-aligned-key>=…` values (e.g. `core_enable_mkl`, `memory_enable_cuda`, `vectorization_type`).

### Feature parity (high level)

| Area | CMake / `setup.py` | Bazel |
|------|-------------------|--------|
| Build type | `-DCMAKE_BUILD_TYPE=` | `--config=debug\|release\|relwithdebinfo` |
| C++ standard | `cxx17` / `cxx20` / `cxx23` tokens → `*_CXX_STANDARD` | Token or `--cxxopt` from script; `.bazelrc` has `build:cxx17` etc. |
| Vectorization | `sse` / `avx` / `avx2` / `avx512` | `--config=sse` … `avx512` |
| LTO | `lto` | `--config=lto` |
| OpenMP / TBB / `PARALLEL_BACKEND` | `openmp`, `tbb`, `parallel.*` | `--define=parallel_backend=std\|openmp\|tbb` (default `std` in `.bazelrc`); `--config=openmp` / `tbb`; legacy `parallel_enable_openmp` / `parallel_enable_tbb` still honored via `selects.with_or` |
| MKL, CUDA, HIP | `mkl`, `cuda`, `hip` | `--config=mkl` / `cuda` / `hip` (+ platform toolchains for CUDA) |
| NUMA / memkind | `numa`, `memkind` | `--config=numa` / `memkind` |
| mimalloc / magic_enum | Defaults ON in CMake; tokens flip | mimalloc: **`memory_enable_mimalloc=true`** in root `.bazelrc` + link `@mimalloc` by default (`memory.bzl`); opt out `--define=memory_enable_mimalloc=false`. magic_enum: Starlark defaults; `--config=magic_enum` if needed |
| Logging backend | `native` / `loguru` / `glog`, `--logging.*` | `--config=logging_native` / `logging_loguru` / `logging_glog` |
| Profiler | `profiler.kineto` / `itt` / `native`, Xcode→native | `--config=kineto` / `itt` / `native_profiler`; Xcode defaults to native in `setup_bazel.py` |
| Sanitizers | `--sanitizer.*` CMake names | `--config=asan` / `tsan` / `ubsan` / `msan` / `lsan` or same `--sanitizer.*` long flags |
| GoogleTest | Default ON; token `gtest` **disables** | `--config=gtest` added by default; `gtest` token → `--define=enable_gtest=false` |
| Google Benchmark | Default ON (matches each module’s `*ENABLE_BENCHMARK` in CMake); token optional | Root `.bazelrc` sets `core_enable_benchmark`, `memory_enable_benchmark`, `parallel_enable_benchmark`, `logging_enable_benchmark`, `profiler_enable_benchmark`, `vectorization_enable_benchmark`, and `enable_benchmark`; `setup_bazel.py` adds `--config=benchmark` by default |
| Shared libs | Token `static` → `BUILD_SHARED_LIBS=ON` (shared) | `--define=build_shared_libs=true` |
| Enzyme | `enzyme` + LLVM plugin | `--config=enzyme` + `-fpass-plugin` from `setup_bazel.py` or `.bazelrc.user` |
| Compiler cache / linker / IWYU / clang-tidy fix / icecc / cppcheck / valgrind / spell | Per-target CMake options | **Not modeled** in Bazel; tokens warn in `setup_bazel.py` (use Bazel remote cache / CI instead) |
| `external` third-party layout | `QUARISMA_ENABLE_EXTERNAL` | **CMake-only** today |

When something is marked **CMake-only**, the Bazel graph may still build the same code paths using vendored `ThirdParty/` and `WORKSPACE.bazel` deps—the **developer workflow** flag simply has no Starlark equivalent yet.

### Token parsing parity

`setup_bazel.py` accepts:

- **Dotted argv segments** like `config.build.test.release` (same ergonomic style as `setup.py`).
- **Long flags** compatible with `setup.py`: `--sanitizer.*`, `--logging.*`, `--profiler.*`, `--parallel.*`.
- **Merged dotted pairs** inside a segment, e.g. `release.parallel.openmp` → `parallel.openmp` + `release`.

This avoids the pitfall where `--sanitizer.address` was previously split incorrectly on `.`.

## Repository layout (Bazel)

| Path | Role |
|------|------|
| `MODULE.bazel` | Bzlmod dependencies (`rules_cc`, `googletest`, `abseil-cpp`, …). |
| `WORKSPACE.bazel` | Legacy workspace roots for vendored/http third parties (needed with `build --enable_workspace` for Bazel 8). |
| `.bazelrc` | Global `--config` definitions (build types, sanitizers, feature defines). |
| `.bazelrc.user` | Optional local machine overrides (not committed). |
| `bazel/` | `BUILD.bazel` `config_setting`s + `*.bzl` helpers (`quarisma.bzl`, `core.bzl`, `memory.bzl`, …). |
| `Library/<Module>/BUILD.bazel` | Targets for Core, Logging, Memory, Parallel, Profiler. |
| `ThirdParty/` | `BUILD` glue, local `cpuinfo`, patches, and http_archive `build_file` references. |

## “Self-contained” libraries and splitting a module into its own repo

Each `Library/<Name>/BUILD.bazel` is written to pull **module policy** from `//bazel:<module>.bzl` and `//bazel:BUILD.bazel` `config_setting`s instead of hard-coding global defines. That is the right *shape* for extraction.

However, a **standalone Git repository** for e.g. only `Library/Core` still needs you to **vendor or re-declare**:

1. **`bazel/`** — at minimum `quarisma.bzl`, `core.bzl`, and the `config_setting` targets those `select()` branches depend on (or trim unused branches).
2. **Third-party labels** — today `Core` depends on `@fmt`, `@magic_enum//`, `//ThirdParty/cpuinfo`, `//Library/Logging`, `//Library/Memory`, etc. A split repo must replace those with `MODULE.bazel` deps or local `new_local_repository` paths.
3. **Root wiring** — `MODULE.bazel`, `WORKSPACE.bazel` (if you still need vendored archives), and a root `BUILD.bazel` if you want a package boundary at the repo root.
4. **`//Library/...` paths** — either keep the same directory layout (`Library/Core/...`) in the new repo or run a mechanical rename of labels and `load()` paths.

There is **no single-button “extract”**; the codebase is structured so the *build logic travels with the module* (`*.bzl`), but **dependency edges remain monorepo-wide** until you duplicate or publish the depended libraries.

**Suggested checklist for a split:**

- [ ] Copy `bazel/quarisma.bzl`, `bazel/core.bzl`, `bazel/BUILD.bazel` (subset), and any `select()` configs you actually use.
- [ ] Replace `deps = ["//Library/Logging:logging_lib", …]` with your packaging story (submodules, published artifacts, or copied trees).
- [ ] Provide `MODULE.bazel` with the same `bazel_dep` versions or pin your own.
- [ ] Re-run `bazel build //Library/Core:core_lib` (or the new equivalent label) with `--enable_workspace` if you still rely on `WORKSPACE.bazel`.

## Raw Bazel invocations (without the Python helper)

The helper is optional. Equivalent knobs:

```bash
bazel build //... \
  --config=clang \
  --config=release \
  --config=avx2 \
  --config=logging_loguru \
  --config=kineto \
  --config=gtest
```

Coverage (LLVM cov map is platform-sensitive; see comments in `.bazelrc`):

```bash
bazel coverage //Library/... --combined_report=lcov --config=clang --config=debug
```

## Troubleshooting

- **“Another command is running” / stuck server** — use the `batch` token in `setup_bazel.py` or run `bazel shutdown` before `--batch` invocations (see `.bazelrc` comments).
- **Enzyme** — requires Clang and a discoverable LLVM plugin; `setup_bazel.py` searches common paths or honors `ENZYME_PLUGIN_PATH` (mirrors CMake’s enzyme discovery).
- **MSVC and `/std:c++`** — global `--cxxopt=-std=c++20` is not MSVC syntax; per-target `quarisma_copts()` in `bazel/quarisma.bzl` applies `/std:c++20` on Windows.

## See also

- `Scripts/setup.py` — canonical CMake flag matrix.
- `Cmake/PROJECT_FLAGS.md` — CMake option reference.
- `Docs/readme/build/build-configuration.md` — CMake-oriented build types and standards (wording may differ slightly from Bazel defaults; trust `.bazelrc` + `bazel/*.bzl` for Bazel).
