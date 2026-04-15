# Core and Profiler Macro Inventory

This document lists the macros defined by the `Core` and `Profiler` projects under `Library/Core` and `Library/Profiler`, plus the compile definitions injected into those targets by the build system.

Scope notes:

- Includes project-local `#define` names found in source and header files.
- Includes target compile definitions from CMake and Bazel.
- Excludes compiler and platform builtins such as `__LINE__`, `__GNUC__`, `__clang__`, and similar predefined macros.

Primary source files used for this inventory:

- `Library/Core/common/macros.h`
- `Library/Core/logging/logger.h`
- `Library/Core/util/exception.h`
- `Library/Core/util/registry.h`
- `Library/Profiler/common/profiler_macros.h`
- `Library/Profiler/common/profiler_export.h`
- `Library/Profiler/bespoke/common/record_function.h`
- `Library/Core/CMakeLists.txt`
- `Library/Profiler/CMakeLists.txt`
- `Cmake/flags/compile_definitions.cmake`
- `bazel/quarisma.bzl`

## Core

`151` unique project-local macro names were found under `Library/Core`:

```text
ASSERT_ANY_THROW
ASSERT_EQ
ASSERT_FALSE
ASSERT_NE
ASSERT_TRUE
CUDA_CHECK_RETURN_FALSE
CUDA_CHECK_RETURN_NULL
END_LOG_TO_FILE
END_LOG_TO_FILE_NAME
END_TEST
EXPECT_EQ
EXPECT_FALSE
EXPECT_GE
EXPECT_GT
EXPECT_LE
EXPECT_LT
EXPECT_NE
EXPECT_NEAR
EXPECT_TRUE
HIP_CHECK_RETURN_FALSE
HIP_CHECK_RETURN_NULL
LOG_TO_FILE_NAME
MACRO_CORE_PRINTF_FORMAT
MACRO_CORE_TYPE_ID_NAME
MULTI_THREADER_H
NOMINMAX
NO_THREAD_SAFETY_ANALYSIS
OPENMP_PARALLEL_TOOLS_IMPL_H
PARALLEL_H
PARALLEL_MAX_BACKENDS_NB
PARALLEL_THREAD_POOL_H
PARALLEL_TOOLS_API_H
PARALLEL_TOOLS_H
PARALLEL_TOOLS_IMPL_H
QUARISMALOG_ANONYMOUS_VARIABLE
QUARISMALOG_CONCAT
QUARISMALOG_CONCAT_IMPL
QUARISMATEST
QUARISMATEST_CALL
QUARISMATEST_VOID
QUARISMA_ALIGN
QUARISMA_ANONYMOUS_VARIABLE
QUARISMA_API
QUARISMA_ASM_COMMENT
QUARISMA_CHECK
QUARISMA_CHECK_ALL_FINITE
QUARISMA_CHECK_ALL_POSITIVE
QUARISMA_CHECK_DEBUG
QUARISMA_CHECK_STRICTLY_DECREASING
QUARISMA_CHECK_STRICTLY_INCREASING
QUARISMA_CHECK_STRICTLY_ORDERED
QUARISMA_COLD
QUARISMA_COMPRESSION_TYPE_SNAPPY
QUARISMA_CONCATENATE
QUARISMA_CONCATENATE_IMPL
QUARISMA_CONST_INIT
QUARISMA_CUDA_DEVICE
QUARISMA_CUDA_FUNCTION_TYPE
QUARISMA_CUDA_HOST
QUARISMA_DECLARE_FUNCTION_REGISTRY
QUARISMA_DECLARE_REGISTRY
QUARISMA_DECLARE_SHARED_REGISTRY
QUARISMA_DECLARE_TYPED_REGISTRY
QUARISMA_DEFINE_FUNCTION_REGISTRY
QUARISMA_DEFINE_REGISTRY
QUARISMA_DEFINE_SHARED_REGISTRY
QUARISMA_DEFINE_TYPED_REGISTRY
QUARISMA_DELETE_CLASS
QUARISMA_DELETE_COPY
QUARISMA_DELETE_COPY_AND_MOVE
QUARISMA_EXCLUSIVE_LOCKS_REQUIRED
QUARISMA_FORCE_INLINE
QUARISMA_FORMAT_STRING_TYPE
QUARISMA_FUNCTION_ATTRIBUTE
QUARISMA_FUNCTION_CONSTEXPR
QUARISMA_GPU_ALLOCATE_TRACKED
QUARISMA_GPU_DEALLOCATE_TRACKED
QUARISMA_GUARDED_BY
QUARISMA_HAS_CXA_DEMANGLE
QUARISMA_HAS_THREE_WAY_COMPARISON
QUARISMA_HAVE_ATTRIBUTE
QUARISMA_HAVE_CPP_ATTRIBUTE
QUARISMA_HIDDEN
QUARISMA_IMPORT
QUARISMA_LIFETIME_BOUND
QUARISMA_LIKELY
QUARISMA_LOCKS_EXCLUDED
QUARISMA_LOG
QUARISMA_LOG_DEBUG
QUARISMA_LOG_END_SCOPE
QUARISMA_LOG_ERROR
QUARISMA_LOG_FATAL
QUARISMA_LOG_IF
QUARISMA_LOG_INFO
QUARISMA_LOG_INFO_DEBUG
QUARISMA_LOG_INFO_DEBUG_BFC
QUARISMA_LOG_SCOPE_FUNCTION
QUARISMA_LOG_START_SCOPE
QUARISMA_LOG_WARNING
QUARISMA_MAX_THREADS
QUARISMA_NODISCARD
QUARISMA_NOINLINE
QUARISMA_NORETURN
QUARISMA_NOT_IMPLEMENTED
QUARISMA_NO_SANITIZE_MEMORY
QUARISMA_NO_THREAD_SAFETY_ANALYSIS
QUARISMA_NUMA_ENABLED
QUARISMA_PRINTF_ATTRIBUTE
QUARISMA_PRINTF_LIKE
QUARISMA_RECORD_GPU_ACCESS
QUARISMA_REGISTER_CLASS
QUARISMA_REGISTER_CREATOR
QUARISMA_REGISTER_FUNCTION
QUARISMA_REGISTER_TYPED_CLASS
QUARISMA_REGISTER_TYPED_CREATOR
QUARISMA_RESTRICT
QUARISMA_SIMD_RETURN_TYPE
QUARISMA_STRINGIZE
QUARISMA_STRINGIZE_IMPL
QUARISMA_TEST_H
QUARISMA_THROW
QUARISMA_THROW_IMPL
QUARISMA_TRACK_GPU_ALLOCATION
QUARISMA_TRACK_GPU_DEALLOCATION
QUARISMA_UID
QUARISMA_UNLIKELY
QUARISMA_UNUSED
QUARISMA_USED
QUARISMA_VECTORCALL
QUARISMA_VECTORIZED
QUARISMA_VISIBILITY
QUARISMA_VISIBILITY_ENUM
QUARISMA_VLOG_IF
QUARISMA_VLOG_SCOPE_FUNCTION
QUARISMA_VLOG_START_SCOPE
QUARISMA_WARN_ONCE
SKA_NOINLINE
START_LOG_TO_FILE
START_LOG_TO_FILE_NAME
STDTHREAD_PARALLEL_TOOLS_IMPL_H
SUPPORTS_BACKTRACE
TBB_PARALLEL_TOOLS_IMPL_H
THREADED_CALLBACK_QUEUE_H
THREADED_TASK_QUEUE_H
__DEFINED_DARWIN_TYPES
__TBB_NO_IMPLICIT_LINKAGE
__quarisma_configure_h__
__quarisma_export_h__
alignas
quarisma_int
quarisma_long
```

## Profiler

`92` unique project-local macro names were found under `Library/Profiler`:

```text
ATTRIBUTE
AT_ALIGNEDCHARARRAY_TEMPLATE_ALIGNMENT
COUNT_TAG
DEFINE_CASE
DEFINE_TAG
ELF_CHECK
ENABLE_GLOBAL_OBSERVER
FORWARD_FROM_RESULT
IS_PYTHON_3_12
ITT_H
LOG_INFO
NOMINMAX
PRINT_INST
PRINT_LINE_TABLE
PROFILER_ANONYMOUS_VARIABLE
PROFILER_API
PROFILER_ASSERT_ONLY_METHOD_OPERATORS
PROFILER_CLANG_MAJOR_VERSION
PROFILER_CONCATENATE
PROFILER_CONST_INIT
PROFILER_CUDA_CHECK
PROFILER_CUDA_VERSION
PROFILER_CUDA_VERSION_MAJOR
PROFILER_FORALL_TAGS
PROFILER_GCC_VERSION
PROFILER_GCC_VERSION_MINOR
PROFILER_GUARDED_BY
PROFILER_HAVE_ATTRIBUTE
PROFILER_HAVE_CPP_ATTRIBUTE
PROFILER_HIDDEN
PROFILER_ID
PROFILER_IMPORT
PROFILER_LIFETIME_BOUND
PROFILER_LIKELY
PROFILER_LOG_ERROR
PROFILER_NODISCARD
PROFILER_NVCC
PROFILER_PROFILER_CPU_ANNOTATION_STACK_H_
PROFILER_PROFILER_CPU_HOST_TRACER_UTILS_H_
PROFILER_PROFILER_EXPORTERS_CHROME_TRACE_EXPORTER_H_
PROFILER_PROFILER_UTILS_FORMAT_UTILS_H_
PROFILER_PROFILER_UTILS_MATH_UTILS_H_
PROFILER_PROFILER_UTILS_PARSE_ANNOTATION_H_
PROFILER_PROFILER_UTILS_PROFILER_STRINGS_H_
PROFILER_PROFILER_UTILS_TIMESPAN_H_
PROFILER_PROFILER_UTILS_TIME_UTILS_H_
PROFILER_PROFILER_UTILS_TRACE_UTILS_H_
PROFILER_PROFILE_BLOCK
PROFILER_PROFILE_FUNCTION
PROFILER_PROFILE_SCOPE
PROFILER_RDTSC
PROFILER_TRACELITERAL
PROFILER_TRACEPRINTF
PROFILER_TRACESTRING
PROFILER_TSL_PLATFORM_ENV_TIME_H_
PROFILER_TSL_PROFILER_LIB_PROFILER_INTERFACE_H_
PROFILER_UID
PROFILER_UNLIKELY
PROFILER_UNUSED
PROFILER_VISIBILITY
PROFILER_VISIBILITY_ENUM
PROFILER_XXX_DISABLE_TENSOR
PY_MONITORING_EVENT_CALL
RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_AND_INPUTS
RECORD_FUNCTION
RECORD_FUNCTION_WITH_INPUTS_OUTPUTS
RECORD_FUNCTION_WITH_SCOPE
RECORD_FUNCTION_WITH_SCOPE_INPUTS_OUTPUTS
RECORD_OUTPUTS
RECORD_TORCHSCRIPT_FUNCTION
RECORD_USER_SCOPE
RECORD_USER_SCOPE_WITH_INPUTS
RECORD_USER_SCOPE_WITH_KWARGS_ONLY
RECORD_WITH_SCOPE_DEBUG_HANDLE_AND_INPUTS
REGISTER_DEFAULT
REGISTER_PRIVATEUSE1_OBSERVER
ROLLBEAR_STRONG_TYPE_HPP_INCLUDED
SOFT_ASSERT
STRONG_HAS_FMT_FORMAT
STRONG_HAS_STD_FORMAT
TENSORIMPL_MAYBE_VIRTUAL
TRUTH_TABLE_ENTRY
TYPED_ATTR
TYPED_ATTR_WITH_DEFAULT
UNWIND_CHECK
UNWIND_WARN
WIN32_LEAN_AND_MEAN
XLA_BACKENDS_PROFILER_CPU_HOST_TRACER_H_
XLA_BACKENDS_PROFILER_CPU_METADATA_UTILS_H_
XLA_BACKENDS_PROFILER_CPU_PYTHON_TRACER_H_
__profiler_export_h__
debug_info
```

## Build-Time Compile Definitions

The following macros are injected by the build system for `Core`, `Profiler`, or their tests.

### Direct target definitions

```text
Core:
QUARISMA_SHARED_DEFINE
QUARISMA_BUILDING_DLL
QUARISMA_STATIC_DEFINE

Profiler:
PROFILER_SHARED_DEFINE
PROFILER_BUILDING_DLL
PROFILER_STATIC_DEFINE

Tests:
QUARISMA_GOOGLE_TEST
PROFILER_GOOGLE_TEST
USE_GTEST
_VARIADIC_MAX=10
```

### Shared feature definitions from CMake and Bazel

```text
PROFILER_ENABLE_ITT
PROFILER_ENABLE_KINETO
PROFILER_ENABLE_NATIVE_PROFILER
PROFILER_HAS_KINETO
PROFILER_HAS_NATIVE
QUARISMA_AVX
QUARISMA_AVX2
QUARISMA_AVX512
QUARISMA_COMPRESSION_TYPE_SNAPPY
QUARISMA_ENABLE_ALLOCATION_STATS
QUARISMA_ENABLE_COMPRESSION
MEMOY_ENABLE_CUDA
CORE_ENABLE_ENZYME
QUARISMA_ENABLE_EXPERIMENTAL
LOGGING_ENABLE_GLOG
QUARISMA_ENABLE_GTEST
QUARISMA_ENABLE_HIP
LOGGING_ENABLE_LOGURU
QUARISMA_ENABLE_MAGICENUM
MEMORY_ENABLE_MIMALLOC
QUARISMA_ENABLE_MKL
QUARISMA_ENABLE_NATIVE
QUARISMA_ENABLE_NUMA
QUARISMA_ENABLE_OPENMP
QUARISMA_ENABLE_ROCM
QUARISMA_ENABLE_SVML
QUARISMA_ENABLE_TBB
QUARISMA_GPU_ALLOC_ASYNC
QUARISMA_GPU_ALLOC_POOL_ASYNC
QUARISMA_GPU_ALLOC_SYNC
QUARISMA_HAS_COMPRESSION
QUARISMA_HAS_CUDA
QUARISMA_HAS_ENZYME
QUARISMA_HAS_EXCEPTION_PTR
QUARISMA_HAS_EXPERIMENTAL
QUARISMA_HAS_GLOG
QUARISMA_HAS_GTEST
QUARISMA_HAS_HIP
QUARISMA_HAS_LOGURU
QUARISMA_HAS_MAGICENUM
QUARISMA_HAS_MIMALLOC
QUARISMA_HAS_MKL
QUARISMA_HAS_NATIVE
QUARISMA_HAS_NATIVE_PROFILER
QUARISMA_HAS_NUMA
QUARISMA_HAS_OPENMP
QUARISMA_HAS_PTHREADS
QUARISMA_HAS_ROCM
QUARISMA_HAS_SVML
QUARISMA_HAS_TBB
QUARISMA_HAS_WIN32_THREADS
QUARISMA_LU_PIVOTING
QUARISMA_MAX_THREADS
QUARISMA_SOBOL_1111
QUARISMA_SSE
QUARISMA_USE_PTHREADS
QUARISMA_USE_WIN32_THREADS
```

## Conditional Compilation Macros

These macros appear in `#if` / `#ifdef` / `#ifndef` directives to enable or disable code sections at compile time. Only project-defined or build-system-injected macros are listed here; compiler and platform builtins (`_WIN32`, `__GNUC__`, `__cplusplus`, etc.) are excluded per the scope note above.

### Backend Feature Flags

Set by CMake based on available libraries at configure time.

> **Mutual exclusivity:** `PROFILER_HAS_KINETO`, `PROFILER_HAS_ITT`, and `PROFILER_HAS_NATIVE` are mutually exclusive — at most one may equal `1` in any given build. Enabling more than one is a CMake configure-time error (`compile_definitions.cmake`) and also a compile-time `#error` in `common/profiler_export.h`.

| Macro | Key Files | Purpose |
|---|---|---|
| `PROFILER_HAS_KINETO` | `bespoke/kineto/kineto_shim.*`, `bespoke/common/collection.cpp`, `bespoke/kineto/profiler_kineto.cpp` | Enables the entire Kineto GPU activity tracing backend. All shim functions compile as no-ops when off. Used in ~30 `#if` guards across the shim layer. |
| `PROFILER_HAS_ITT` | `bespoke/itt/itt_wrapper.*`, `bespoke/kineto/profiler_kineto.cpp`, test files | Enables Intel VTune ITT range annotations. When off, all `ITTWrapper` calls compile away. |
| `PROFILER_HAS_NATIVE` | All native test translation units | Gates the full native profiler subsystem (XPlane, stats, exporters, traceme). Every test file under `Testing/Cxx/` opens with `#if PROFILER_HAS_NATIVE`. |
| `PROFILER_HAS_PROFILER` | `Testing/Cxx/TestXSigmaProfiler.cpp` | Top-level availability flag — the profiler exists at all in this build. |
| `QUARISMA_HAS_CUDA` | `bespoke/kineto/profiler_kineto.h` | Activates CUDA-specific event types and NVML memory queries inside the Kineto profiler wrapper. |
| `KINETO_HAS_HCCL_PROFILER` | `bespoke/kineto/kineto_shim.cpp` | Enables AMD HCCL (collective communications) profiling hooks inside Kineto. Only meaningful on ROCm builds with HCCL. |

### Target / Build Variant Flags

Control which code paths are included for a given deployment target.

| Macro | Key Files | Purpose |
|---|---|---|
| `PROFILER_MOBILE` | `bespoke/kineto/kineto_shim.h`, `bespoke/common/collection.cpp`, `common/approximate_clock.h` | Mobile build — disables heavy features and switches Kineto linkage to the edge (`EDGE_PROFILER_USE_KINETO`) variant. Also used alongside `PROFILER_IOS` to select the iOS clock path. |
| `PROFILER_IOS` | `common/approximate_clock.h` | iOS-specific clock path (combined check: `PROFILER_IOS && PROFILER_MOBILE`). Falls through to `std::chrono` when RDTSC is unavailable. |
| `PROFILER_USE_ROCM` | `bespoke/common/collection.cpp`, `bespoke/common/util.cpp` | Switches GPU-side activity collection from CUDA HIP to AMD ROCm HIP APIs. |
| `PROFILER_CUDA_USE_NVTX3` | `bespoke/base/cuda.cpp` | Selects the NVTX3 header-only API over NVTX2 for NVIDIA range annotations. NVTX3 is the preferred path on modern CUDA toolkits. |
| `ROCM_ON_WINDOWS` | `bespoke/base/cuda.cpp` | ROCm on Windows exposes a different NVTX-equivalent include path; this skips the standard NVTX header. |
| `BUILD_LITE_INTERPRETER` | `bespoke/common/collection.cpp` | Stripped-down interpreter build; excludes full record-function collection paths that depend on the complete op dispatch table. |
| `FBCODE_CAFFE2` | `bespoke/common/unwind/unwind.cpp` | Selects the Meta-internal fbcode stack unwinder over the open-source libunwind path. Also gates internal DWARF helpers in `unwind_fb.cpp`. |
| `EDGE_PROFILER_USE_KINETO` | `bespoke/kineto/kineto_client_interface.cpp`, `bespoke/kineto/kineto_shim.h` | Edge/embedded Kineto shim — uses a different activity client initialisation path from full Kineto (e.g., on Apple devices). |

### Runtime Behavior Flags

Opt-in or opt-out of specific runtime subsystems.

| Macro | Key Files | Purpose |
|---|---|---|
| `PROFILER_PREFER_CUSTOM_THREAD_LOCAL_STORAGE` | `bespoke/common/record_function.cpp` | Replaces `thread_local` with a custom TLS implementation on platforms where TLS access overhead is measurable in hot paths. |
| `PROFILER_DISABLE_TENSORIMPL_EXTENSIBILITY` | `common/TensorImpl.h` | Removes virtual dispatch hooks on `TensorImpl` (two sites: class body and inline impl). Used in embedded/mobile builds to reduce vtable size and prevent unwanted subclassing. |
| `PROFILER_XXX_DISABLE_TENSOR` | `bespoke/common/ivalue.h` | Strips all tensor-related branches from `IValue` (~10 sites). Used in lightweight non-ML profiling contexts where tensor support is unnecessary. |
| `PROFILER_RDTSC` | `common/approximate_clock.h` | Signals that the RDTSC x86 cycle-counter instruction is available; enables the fast cycle-counter clock path inside `ApproximateClock`. Defined in the same file when x86 is detected. |
| `ENABLE_GLOBAL_OBSERVER` | `bespoke/kineto/kineto_client_interface.cpp` | Activates a global activity observer that intercepts all ops system-wide (off by default; opt-in for system-level tracing). |
| `USE_DISTRIBUTED` | `bespoke/common/util.h`, `bespoke/common/util.cpp`, `bespoke/common/standalone/execution_trace_observer.cpp` | Adds distributed training metadata (NCCL ranks, collective op IDs) to profiler events. Requires the distributed comms library to be linked. |
| `IS_MOBILE_PLATFORM` | `native/tracing/traceme.h` (×11 guards) | Disables the full `TraceMe` RAII machinery on mobile targets — every trace entry/exit method becomes a no-op to eliminate tracing overhead entirely. |
| `IS_PYTHON_3_12` | `bespoke/kineto/profiler_python.cpp` | Derived from Python headers; selects the new `sys.monitoring` tracing API (Python ≥ 3.12) over the legacy `PyEval_SetProfile` path. Also has a variant check for Python ≥ 3.13 compat shims. |

### Dead Code Blocks (`#if 0`)

Appears in ~30+ locations across `collection.cpp`, `kineto_shim.cpp`, `ivalue.h`, `util.cpp`, `execution_trace_observer.cpp`, `record_function.cpp`, `itt.h`, and several test files.

These blocks are permanently disabled — they contain old implementations, discarded alternative approaches, or work-in-progress code retained for reference. The pattern `#if PROFILER_HAS_KINETO && 0` seen in `TestProfilerHeavyFunction.cpp` and `TestKinetoShim.cpp` is a deliberate technique to disable a specific sub-block while keeping the surrounding Kineto feature guard intact.

---

## Notes

- Some names above are include guards or file-local helper macros rather than public API macros.
- Some macros may be conditionally defined multiple ways depending on platform or compiler; this inventory records the macro names, not every conditional expansion.
- `NOMINMAX` appears in both projects because several Windows-facing translation units define it locally before including Windows headers.
