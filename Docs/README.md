# XSigma Documentation

The documents below describe the current checked-out source. The build scripts,
`.bazelrc`, and module `CMakeLists.txt` files are the implementation authority
when a new option is added.

## Build documentation

- [CMake setup guide](readme/setup.md) - `Scripts/setup.py` actions, tokens,
  and direct CMake equivalents.
- [Build configuration](readme/build/build-configuration.md) - CMake build
  types, module-scoped options, LTO, testing, and backend selection.
- [Build examples](readme/usage-examples.md) - Current commands for development,
  release, diagnostics, parallelism, and GPU builds.
- [CMake option reference](PROJECT_FLAGS.md) - Public cache variables and their
  module prefixes.
- [Bazel guide](BAZEL_USER_GUIDE.md) - Bazel `8.4.2` usage, configurations, and
  known feature gaps.

## Library documentation

- [Memory design](memory_design.md) - CPU/GPU allocation model and caching
  allocator status.
- [Vectorization backends](vectorization_backends.md) - CPU, CUDA, HIP, and
  Metal evaluator contracts.
- [Profiler guide](profiler/profiler.md) - Native, Kineto, and ITT profiling.
- [Vectorization SIMD guide](readme/vectorization.md) - CPU backend selection.
- [Project dependencies](PROJECT_DEPENDENCIES.md) - Library dependency graph.

## Supporting guides

The `readme/` directory also contains focused guides for sanitizers, coverage,
static analysis, compiler caching, logging, Valgrind, coding standards, and
cross-platform builds. Those guides use the same module-scoped CMake option
model documented in [PROJECT_FLAGS.md](PROJECT_FLAGS.md).
