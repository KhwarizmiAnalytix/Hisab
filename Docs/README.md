# Docs

This directory contains maintained project documentation. Historical one-off
analysis, completion reports, and stale status snapshots have been removed to
keep this tree useful as a reference.

## Current Documents

- [PROJECT_DEPENDENCIES.md](PROJECT_DEPENDENCIES.md) - Library dependency graph and optional links.
- [PROJECT_FLAGS.md](PROJECT_FLAGS.md) - Project CMake cache flags.
- [flags.md](flags.md) - `Cmake/flags` module reference.
- [BAZEL_USER_GUIDE.md](BAZEL_USER_GUIDE.md) - Bazel build usage, configs, and known gaps.
- [CUDA_HIP_REMEDIATION_PLAN.md](CUDA_HIP_REMEDIATION_PLAN.md) - Older CUDA/HIP remediation notes (several Memory items are done; current Memory status is `memory_design.md` §10).
- [memory_design.md](memory_design.md) - Memory library design: CPU/GPU paths, `data_ptr`/`data_view`, caching-allocator client API, done vs still open.
- [vectorization_backends.md](vectorization_backends.md) - Current CPU / CUDA / HIP / Metal evaluator contracts. Fusion is done; launch signatures, streams, and reductions still differ. SIMD ISA chooser: [readme/vectorization.md](readme/vectorization.md).
- [profiler/profiler.md](profiler/profiler.md) - Profiler user and architecture guide.

## README-Backed Guides

The `readme/` subtree holds extended guides linked from the root
[README.md](../README.md), including setup, build configuration, vectorization,
sanitizers, coverage, static analysis, caching, logging, and coding standards.
