# Quarisma profiler documentation

All profiler-focused Markdown (and related notes) for this repository lives under **`Docs/profiler/`**.

## Start here

| Document | Description |
|----------|-------------|
| [profiler.md](profiler.md) | Main user guide (APIs, backends, CMake/Bazel, examples) |
| [Library_Profiler.md](Library_Profiler.md) | Profiler **library** target: linking, native backend, quick start |
| [Profiling_Examples.md](Profiling_Examples.md) | **Examples/Profiling/** — build, run, visualize traces |
| [PROFILER_USAGE_GUIDE.md](PROFILER_USAGE_GUIDE.md) | Usage-focused guide and workflows |
| [CORE_AND_PROFILER_MACROS.md](CORE_AND_PROFILER_MACROS.md) | Compile-time macros and feature flags |

## Kineto

| Document | Description |
|----------|-------------|
| [KINETO_INDEX.md](KINETO_INDEX.md) | Index into Kineto-related docs |
| [KINETO_README.md](KINETO_README.md) | Kineto overview |
| [KINETO_PROFILER_GUIDE.md](KINETO_PROFILER_GUIDE.md) | Detailed component guide |
| [KINETO_QUICK_REFERENCE.md](KINETO_QUICK_REFERENCE.md) | Quick reference |
| [KINETO_IMPLEMENTATION_DETAILS.md](KINETO_IMPLEMENTATION_DETAILS.md) | Implementation notes |
| [KINETO_COMPLETE_REVIEW.md](KINETO_COMPLETE_REVIEW.md) | Review / audit |
| [KINETO_ARCHITECTURE_SUMMARY.md](KINETO_ARCHITECTURE_SUMMARY.md) | Architecture summary |

## Investigations, refactoring, and history

Investigation summaries, PyTorch-alignment notes, code examples, and build metadata fixes (for example `PROFILER_BUILD_RESULTS.md`, `PROFILER_METADATA_FIX.md`, `PROFILER_CHANGES_VISUAL.md`) are in this same folder; see filenames prefixed with `PROFILER_` and `PYTORCH_PROFILER_`.

## Text diagrams

- [PROFILER_ARCHITECTURE_DIAGRAM.txt](PROFILER_ARCHITECTURE_DIAGRAM.txt)
- [PROFILER_EXECUTIVE_SUMMARY.txt](PROFILER_EXECUTIVE_SUMMARY.txt)

## Legacy path

The former top-level **`Docs/profiler.md`** duplicate of the main guide was removed; use **[profiler.md](profiler.md)** instead.
