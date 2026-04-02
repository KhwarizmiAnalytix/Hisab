# Quarisma Profiler Instrumentation Review

This document captures the current issues, root causes, and recommended remediation plan for the Quarisma profiling stack (Kineto, ITT, NVTX, PRIVATEUSE1, and native profiler components).

---

## 1. Identified Issues

| ID | Area | File / Location | Description |
|----|------|-----------------|-------------|
 | 1 | Profiler state management | `Library/Core/profiler/kineto/profiler_kineto.cpp:637-905` | `profiler_state_info_ptr` is a single shared pointer accessed from multiple threads (main, child threads, dynamic toggles) with no synchronization. Stale or null data is dereferenced inside `toggleTorchOpCollectionDynamic`, `enableProfilerInChildThread`, and other flows, leading to races and crashes. |
 | 2 | Observer exports | `Library/Core/profiler/itt/itt_observer.h`, `Library/Core/profiler/base/nvtx_observer.h` | `pushITTCallbacks`/`pushNVTXCallbacks` lack `PROFILER_API`/`PROFILER_VISIBILITY`, so the functions are not exported from the Core DLL and cannot be linked by downstream modules, causing unresolved externals in shared builds. |
| 3 | Global string linkage | `Library/Core/profiler/common/record_function.cpp:21` | `kParamCommsCallName` is defined with `extern const std::string` despite the header declaring `extern PROFILER_API const std::string`. This produces mismatched import/export semantics and ODR issues on Windows. |
| 4 | Feature-flag mismatch | Tests vs. implementation | ITT availability is guarded by `PROFILER_HAS_ITT` in implementation, but tests use `PROFILER_HAS_ITTAPI`. Since `configure.h` never defines the latter, ITT tests are silently skipped even when ITT is enabled, hiding regressions. |
| 5 | Coverage gaps | Multiple | No automated tests exercise `toggleTorchOpCollectionDynamic`, backend event reporting, NVTX/PRIVATEUSE1 observers, or Chrome-trace failure modes. Kineto tests still fail (`RecordDebugHandles.Basic`), and there is little validation of error paths. |
| 6 | Legacy dependencies | `Library/Core/profiler/common/*` | Numerous TODOs reference missing Quarisma equivalents for Quarisma/TensorImpl, MTIA, logging, etc. Portions of the profiler rely on unimplemented helpers or `#if 0` blocks, so certain instrumentation paths cannot function fully. |

---

## 2. Root Cause Summary

1. Profiler state handling was ported from PyTorch assuming a single profiler session and lacks proper synchronization or lifecycle management in Quarisma’s DLL context.
2. Export macros were omitted when splitting the profiler into shared libraries, leaving observer entry points hidden.
3. The `kParamCommsCallName` definition predates the introduction of `PROFILER_API`, so the header/source pair now disagree on linkage.
4. Build-system macros diverged (`PROFILER_ENABLE_ITT` option vs. `PROFILER_HAS_ITT` compile definition); tests introduced their own `PROFILER_HAS_ITTAPI` guard that is never set.
5. New instrumentation additions did not receive dedicated regression tests, so broad areas remain unverified and the stated 98% coverage goal is unmet.
6. Porting from upstream sources stalled before Quarisma-specific abstractions were provided, leaving TODOs and disabled code that block full functionality.

---

## 3. Remediation Plan (Priority Order)

### 3.1 Stabilize Profiler State Handling
- Refactor `profiler_state_info_ptr` into a synchronized structure (e.g., mutex + optional) or embed the state/scopes directly in `KinetoThreadLocalState`.
- Guard every dereference (`toggleTorchOpCollectionDynamic`, child-thread helpers) with null checks and fallbacks.
- Ensure NVTX/ITT/PRIVATEUSE1 sessions clear or reinitialize the stored scopes so subsequent CPU sessions do not reuse stale data.

### 3.2 Export Observer Entry Points
- Annotate `pushITTCallbacks`, `pushNVTXCallbacks`, and any similar helpers with `PROFILER_API`/`PROFILER_VISIBILITY`.
- Verify the corresponding `.cpp` files are linked into the Core DLL and add a linker regression test (simple TU that references the symbols).

### 3.3 Fix Global Constant Linkage
- Change the definition of `kParamCommsCallName` in `record_function.cpp` to `const PROFILER_API std::string kParamCommsCallName = …;`.
- Audit other profiler globals declared with `PROFILER_API` to ensure their definitions match (no stray `extern` keywords).

### 3.4 Unify ITT Feature Flags
- Standardize on `PROFILER_HAS_ITT` (generated from `PROFILER_ENABLE_ITT`) across the entire tree.
- Update profiler tests (`TestProfilerBackendIntegration.cpp`, `TestITTWrapper.cpp`, etc.) to use the same macro so ITT coverage runs when enabled.
- Document the mapping in `common/configure.h` or developer docs to prevent future divergence.

### 3.5 Expand Instrumentation Test Coverage
- Add NVTX and PRIVATEUSE1 observer integration tests similar to the new Kineto/ITT cases.
- Create tests hitting `toggleTorchOpCollectionDynamic`, backend event reporting, Chrome-trace failure paths, and `enableProfilerWithEventPostProcess`.
- Fix the existing `RecordDebugHandles.Basic` failure to unblock Kineto regression suites.

### 3.6 Address Legacy TODOs / Missing Dependencies
- For each “Missing Quarisma dependency” comment, either implement the required helper (e.g., tensor metadata extraction) or remove the dead code paths.
- Replace `#if 0` blocks around logging/MTIA calls with Quarisma equivalents or feature checks so the code does not silently skip essential behavior.

---

## 4. Estimated Effort

| Work Item | Est. Duration |
|-----------|---------------|
| Profiler state refactor | 2–3 days |
| Export fixes + linkage verification | 0.5 day |
| `kParamCommsCallName` + global audit | 0.5 day |
| ITT macro unification | 0.5 day |
| Expanded test coverage | 2–3 days (depends on CI availability for Kineto/ITT) |
| Legacy dependency cleanup | Follow-up effort (multi-day) once core fixes land |

---

## 5. Architectural Considerations

- Evaluate moving away from global singletons toward per-session objects passed explicitly to child threads. This would simplify state management and make on-demand profiling safer.
- Consider consolidating the multiple profiler backends (Kineto, NVTX, ITT, PRIVATEUSE1) behind a common interface so feature-specific code cannot leak into unrelated builds.
- Revisit error-handling strategy: many profiler code paths still throw exceptions; Quarisma coding standards prefer explicit error propagation without exceptions.

---

## 6. Next Steps

1. Implement profiler state guard fixes and export macro corrections.
2. Apply ITT macro cleanup and fix `kParamCommsCallName`.
3. Add the missing tests and unblock existing Kineto suites.
4. Schedule the larger dependency cleanup once stability and coverage goals are met.

Please coordinate the above tasks with the build/release schedule to ensure profiling regressions are caught early. 
