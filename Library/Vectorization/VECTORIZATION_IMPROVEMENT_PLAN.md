# Vectorization library — implementation review and improvement plan

This document reviews the current `Library/Vectorization` design and proposes a phased plan to improve **runtime efficiency**, **code readability**, and **debuggability**, without prescribing a full rewrite.

---

## 1. Architecture snapshot

The module is a classic **expression-template + SIMD backend** stack:

| Layer | Role | Main artifacts |
|--------|------|----------------|
| Build | Single ISA selection at compile time (`VECTORIZATION_TYPE`: `no`, `sse`, `avx`, `avx2`, `avx512`); optional SVML; optional Memory/Logging | `CMakeLists.txt`, `Cmake/utils.cmake` |
| Backends | Intrinsics specializations per scalar type | `backend/sse/*`, `backend/avx/*`, `backend/avx512/*` (`simd.h`, SVML hooks) |
| Packet | Aggregates multiple SIMD lanes (width `N`); load/store/math via macros and `simd<value_t>` | `common/packet.h` (~1.5k+ lines with heavy macros) |
| Expressions | Lazy trees: unary/binary/ternary + functors; evaluation dispatches on `vectorize` bool | `expressions/expression_interface.h`, `expression_interface_loader.h`, `expressions_builder.h`, `expressions_evaluator.h` |
| Terminals | Concrete storage and views | `terminals/vector.h`, `matrix.h`, `tensor.h`, `*.hxx` |
| Integration | Force-inline / vectorcall / asserts / logging | `common/vectorization_macros.h`, `common/intrin.h` |

**Evaluation path (vectorized):** `expressions_evaluator::run` peels a SIMD-aligned main loop, uses `expression_loader<E, true>::evaluate`, then `packet<value_t>::storeu`; remainder uses scalar `expression_loader<E, false>`.

**Include discipline:** `expressions.h` defines `__VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__` before pulling the graph; `expressions_evaluator.h` errors if included without that guard — intentional but easy to trip.

---

## 2. What works well

- **Clear separation** between ISA-specific `simd<T>` and portable expression logic.
- **One-hot CMake flags** (`VECTORIZATION_HAS_*`) keep preprocessor branches predictable.
- **PUBLIC SIMD compile options** on the `Vectorization` target propagate correct ISA to consumers.
- **Optional SVML** with scalar fallbacks (e.g. `normal_cdf`) avoids hard dependency on Intel math.
- **Prefetch** in `expression_loader` for base expressions is a thoughtful micro-optimization.
- **Tests** exercise SIMD ops and expressions (`TestSimd.cpp`, `TestVector.cpp`, matrix/tensor tests).

---

## 3. Detailed review

### 3.1 Efficiency

**Strengths**

- Peeling + aligned SIMD loop in `expressions_evaluator` is the right shape for throughput.
- Out-parameters on `simd` operations (`ret` parameters) match intrinsics style and avoid extra temporaries in hot paths.

**Gaps / opportunities**

1. **No runtime ISA dispatch** — A binary is tied to one `VECTORIZATION_TYPE`. For distribution on heterogeneous CPUs, you would need multiple compiled variants + resolver (or a separate “baseline + dynamic” path). This is a product/build decision, not a bug.
2. **Duplication in `expressions_evaluator`** — `run(E const&, T&)` and `run(E&&, T&)` are nearly identical; the same pattern appears in `scatter`/`fill`-style helpers. Merging via a small private implementation helper reduces drift and bug risk.
3. **`packet.h` macro unrolling** — `LOAD` / `STORE` / `FUNCTION_*` macros expand large linear sequences for `N ∈ {2,4,8,16,32}`. Compilers usually optimize this, but **compile time** and **debugger single-step cost** grow with TU size. Consider generated headers or `if constexpr` + indexed access where proven equivalent.
4. **`VECTORIZATION_VECTORIZED` scalar path** — When `VECTORIZATION_TYPE` is `no`, behavior is correct; CI should regularly build **`no`** to catch scalar-only regressions (see testing plan).
5. **CMake global flag mutation** — Non-sanitizer builds append SIMD flags to `CMAKE_CXX_FLAGS` / `CMAKE_C_FLAGS`. That affects **all** targets in the directory tree, not only `Vectorization`. Prefer **target-only** `target_compile_options` for ISA flags if other targets must stay portable.

### 3.2 Readability

**Strengths**

- Expression classes (`unary_expression`, `binary_expression`, …) are straightforward CRTP-like evaluators.
- Backend `simd` specializations read like thin wrappers over intrinsics.

**Pain points**

1. **`packet.h` macro surface** — Hundreds of lines of `LOAD`/`STORE`/`FUNCTION_*` obscure the actual data layout (`array_simd_t`, `N`). New contributors need a “map” comment or a short internal doc at the top of the file.
2. **Circular / heavy includes** — `vectorization_type_traits.h` includes `packet.h` early; understanding dependency order matters for refactors.
3. **Naming / comments** — Minor typos (“opeartor”, “expresions”) in user-visible `static_assert` messages reduce polish and searchability.
4. **Guarded include of `expressions_evaluator.h`** — The intentional `#error`-style guard is correct but non-idiomatic; a single umbrella header (`expressions.h`) is fine if **documented** in a one-line “how to include” note on the library README or module comment.

### 3.3 Debuggability

**Strengths**

- **Debug vs release ABI of SIMD helpers:** `VECTORIZATION_SIMD_RETURN_TYPE` uses `static void` + no `always_inline` / `vectorcall` when `NDEBUG` is off, which improves **stepping** and **symbol visibility** in debug builds.
- **`VECTORIZATION_CHECK`** integrates with Logging when available, with `assert` fallback.

**Pain points**

1. **Deep templates** — Expression types are long `remove_cvref_t` chains; compiler errors can be noisy. Mitigations: thin type aliases, `static_assert` with custom messages including `EXPR` names where possible, or a debug-only `trace_expression_v<E>` metafunction printing `typeid(E).name()` (demangled where available).
2. **No structured “eval trace”** — For wrong results, it is hard to know which sub-expression or index failed. A **test-only** or **verbose** mode that compares lane-wise SIMD vs scalar for a range of indices would shorten triage.
3. **Optimized TUs** — In release, inlined SIMD makes breakpoints less useful; document the pattern: “use Debug build or disable LTO for this TU when investigating.”
4. **Natvis** — MSVC attaches `PRETORIAN.natvis` when present; ensure packet/vector types have visualizers if teams rely on Visual Studio.

---

## 4. CMake / build hygiene (quick wins)

| Item | Note |
|------|------|
| `CMAKE_C_FLAGS` assignment | Lines that set `CMAKE_C_FLAGS` from `CMAKE_CXX_FLAGS` are suspicious for C-only targets; verify intent or scope flags to CXX-only. |
| Test discovery | `file(GLOB_RECURSE test_sources ...)` avoids listing tests explicitly; prefer explicit lists or `gtest_discover_tests` patterns if you need deterministic IDE/CI ordering. |
| `VECTORIZATION_ENABLE_BENCHMARK` | Benchmark registered as CTest with short min time — good; document that benchmarks are not stability gates. |

---

## 5. Improvement plan (phased)

### Phase A — Low risk, high clarity (1–2 weeks)

1. **Document the include graph** — At top of `expressions.h` or a short `ARCHITECTURE.md` in this folder: mandatory include order, role of `expression_loader`, and “always include `expressions.h`, never `expressions_evaluator.h` alone.”
2. **Fix user-visible typos** — `static_assert` and comments in `expression_interface.h` (“expressions”, “operator”).
3. **Deduplicate `expressions_evaluator::run`** — One `impl_run` accepting forwarding reference or two thin wrappers calling shared static lambda/template.
4. **Scoped SIMD flags audit** — Confirm whether global `CMAKE_CXX_FLAGS` mutation is still required; migrate to target properties where safe.
5. **Add CI matrix rows** — At minimum: `VECTORIZATION_TYPE=no` and default `avx2` (or project default); optionally `sse` on one older runner.

### Phase B — Testability and debug tooling (2–4 weeks)

1. **Golden / differential tests** — For representative expressions, assert **bit-exact or tight ULP** match between `expression_loader<E, true>` and `expression_loader<E, false>` on the same data (guarded for `VECTORIZATION_VECTORIZED`).
2. **Reduced test fixtures** — Small fixed seeds + minimal sizes to reproduce failures without random noise; keep property-based tests separate.
3. **Optional debug metaprogramming** — `VECTORIZATION_DEBUG_EXPR_TYPES` that prints demangled type names in failed checks (Logging or `stderr`).
4. **Debugger visualizers** — Natvis (MSVC) and lldb pretty-printers for `vector`/`matrix`/packet storage pointers and logical dimensions.

### Phase C — Structural readability (4–8 weeks, incremental)

1. **Split `packet.h`** — Public API + `packet_detail` / `packet_load_store.inl` or generated `packet_gen.inc` to keep the “conceptual” header under ~300 lines.
2. **Reduce macro surface** — Replace unrolled macros with `constexpr` loops + `std::index_sequence` where benchmarks show no regression; keep one macro-backed path per platform if needed for MSVC.
3. **Traits include DAG** — Break `vectorization_type_traits.h` ↔ `packet.h` cycle if possible (forward declarations, split `is_packet`).

### Phase D — Performance hardening (optional / product-driven)

1. **Micro-benchmarks per kernel** — Expand `Benchmark_simd.cpp` into named families (load/store, fma, reductions, expression trees).
2. **Runtime dispatch (if needed)** — Separate static libraries or object files per ISA + resolver in a thin facade target; only if product needs single binary across CPUs.
3. **Alignment API** — Document and enforce alignment expectations for `store` vs `storeu` on terminals; consider `alignas(VECTORIZATION_ALIGNMENT)` assertions in debug.

---

## 6. Success criteria

- **Efficiency:** No regression on existing benchmarks; optional improvement on compile time after `packet` refactor; SIMD flags not leaking to unrelated targets.
- **Readability:** New contributor can follow eval flow from `expressions.h` → `expression_loader` → `packet` → `simd` in one reading session.
- **Debuggability:** Failing test names a **small expression + index + scalar vs vector delta**; Debug builds step through non-inlined SIMD wrappers; CI covers scalar (`no`) path.

---

## 7. Out of scope (unless requirements change)

- Rewriting expression templates as a pure library (e.g. Blaze/Eigen-style) — high cost, different API.
- GPU path — `VECTORIZATION_CUDA_FUNCTION_TYPE` appears in places but full CUDA/HIP parity is a separate program of work.

---

## 8. Summary

The Vectorization module is **coherent and performance-oriented**: compile-time ISA selection, thin intrinsics layer, and expression templates with an explicit vector/scalar eval split. The main leverage for improvement is **shrinking and documenting `packet.h`**, **deduplicating evaluator code**, **widening CI across `VECTORIZATION_TYPE`**, and **adding differential tests and debugger-friendly tooling** so correctness issues are found quickly without sacrificing release performance.
