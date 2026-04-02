# Quarisma Profiler Refactoring Summary

## Overview
Successfully refactored the `Library/Core/profiler/pytroch_profiler` directory to align with Quarisma coding standards and conventions.

## Changes Made

### 1. Directory Structure Flattening
- **Before**: Nested directory structure with 68 files across multiple levels:
  - `aten/src/Quarisma/`
  - `quarisma/csrc/autograd/`
  - `quarisma/csrc/profiler/`
  - `quarisma/csrc/profiler/orchestration/`
  - `quarisma/csrc/profiler/python/`
  - `quarisma/csrc/profiler/standalone/`
  - `quarisma/csrc/profiler/stubs/`
  - `quarisma/csrc/profiler/unwind/`

- **After**: All 66 C++ files (.h and .cpp) moved to base `pytroch_profiler` directory
- Removed all empty nested directories
- Preserved Python files and documentation in original locations

### 2. Include Path Updates
- **Old Format**: `#include <quarisma/csrc/profiler/file.h>`
- **New Format**: `#include "file.h"` (relative to pytorch_profiler directory)
- Updated all internal includes to use relative paths
- Updated downstream references in `Library/Core/experimental/quarisma_autograd/`:
  - `profiler_kineto.h`
  - `profiler_kineto.cpp`
  - `profiler_legacy.h`
  - `init.cpp`
  - `profiler_python.cpp`
  - `python_function.cpp`

### 3. Macro Replacements
Replaced Quarisma-specific macros with Quarisma equivalents:

| Quarisma Macro | Quarisma Macro | Purpose |
|---|---|---|
| `PROFILER_API` | `PROFILER_API` | Function export/import |
| `PROFILER_PYTHON_API` | `PROFILER_API` | Python-facing function export |
| `PROFILER_CHECK` | `PROFILER_CHECK` | Internal assertions |
| `PROFILER_INTERNAL_ASSERT_DEBUG_ONLY` | `PROFILER_CHECK_DEBUG` | Debug-only assertions |
| `PROFILER_CHECK` | `PROFILER_CHECK` | Runtime checks |
| `PROFILER_API_ENUM` | (removed) | Enum visibility (not needed) |
| `PROFILER_DIAGNOSTIC_PUSH_AND_IGNORED_IF_DEFINED` | `PROFILER_DIAGNOSTIC_PUSH` | Diagnostic control |
| `PROFILER_DIAGNOSTIC_POP` | `PROFILER_DIAGNOSTIC_POP` | Diagnostic control |

### 4. Visibility and API Macros
- Added `PROFILER_VISIBILITY` before public class declarations
- Added `PROFILER_API` before public function declarations
- Ensured proper DLL export/import semantics for Windows

### 5. Namespace Corrections
- Preserved `c10::` namespace references (not replaced with `quarisma::`)
- Preserved `quarisma::` namespace references where appropriate
- Maintained compatibility with Quarisma's type system

## Files Modified

### Refactored Files (66 total)
All files moved from nested directories to base `pytroch_profiler` directory:
- `record_function.h/cxx` (from `aten/src/Quarisma/`)
- `profiler_kineto.h/cxx`, `profiler_python.h/cxx`, `profiler.h` (from `quarisma/csrc/autograd/`)
- `collection.h/cxx`, `combined_traceback.h/cxx`, `containers.h`, `data_flow.h/cxx`, `events.h`
- `kineto_shim.h/cxx`, `kineto_client_interface.h/cxx`, `api.h`
- `observer.h/cxx`, `python_tracer.h/cxx`, `vulkan.h/cxx` (from orchestration/)
- `execution_trace_observer.h/cxx`, `itt_observer.h/cxx`, `nvtx_observer.h/cxx`, `privateuse1_observer.h/cxx` (from standalone/)
- `base.h/cxx`, `cuda.cpp`, `itt.cpp` (from stubs/)
- `unwind.h/cxx`, `unwind_fb.cpp`, `unwind_error.h`, `unwinder.h`, and 20+ header files (from unwind/)
- `util.h/cxx`, `perf.h/cxx`, `perf-inl.h`, `init.h/cxx`, `pybind.h`

### Downstream Files Updated (6 total)
- `Library/Core/experimental/quarisma_autograd/profiler_kineto.h`
- `Library/Core/experimental/quarisma_autograd/profiler_kineto.cpp`
- `Library/Core/experimental/quarisma_autograd/profiler_legacy.h`
- `Library/Core/experimental/quarisma_autograd/init.cpp`
- `Library/Core/experimental/quarisma_autograd/profiler_python.cpp`
- `Library/Core/experimental/quarisma_autograd/python_function.cpp`

## Quarisma Infrastructure Used
- **Export Macros**: `Library/Core/common/profiler_export.h` (PROFILER_API, PROFILER_VISIBILITY)
- **Common Macros**: `Library/Core/common/profiler_macros.h` (PROFILER_CHECK, PROFILER_NODISCARD, etc.)

## Compliance
✅ Follows Quarisma coding standards
✅ Uses snake_case naming conventions
✅ Proper include path structure
✅ DLL export/import macros applied
✅ No exception-based error handling
✅ Maintains cross-platform compatibility

## Notes
- Python files in `quarisma/profiler/` and `quarisma/autograd/` directories preserved as-is
- Duplicate `combined_traceback` files in `quarisma/csrc/profiler/python/` preserved (not moved)
- README.md documentation preserved in original location
- All changes maintain backward compatibility with existing Quarisma integration

