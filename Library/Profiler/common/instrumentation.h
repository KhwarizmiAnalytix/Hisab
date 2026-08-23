/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of Quarisma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/profiler_export.h"
#include "common/profiler_macros.h"

// RECORD_FUNCTION/RECORD_USER_SCOPE (bespoke/common/record_function.h) and
// MemoryReportingInfoBase::reportMemoryUsage (bespoke/common/orchestration/
// observer.h) exist when PROFILER_HAS_KINETO or PROFILER_HAS_ITT is 1.
// Native (traceme/xplane) always compiles alongside that backend. This
// header is what other libraries (Vectorization, Memory, Parallel, ...)
// should include: real RECORD_* under Kineto/ITT, no-op macros only if
// both HAS flags are 0 (not a supported CMake configuration).
#if PROFILER_HAS_KINETO || PROFILER_HAS_ITT
#include "bespoke/common/record_function.h"
#define PROFILER_HAS_INSTRUMENTATION 1
#else
#define PROFILER_HAS_INSTRUMENTATION 0
#define RECORD_USER_SCOPE(fn) \
    do                        \
    {                         \
        (void)(fn);           \
    } while (0)
#endif

namespace profiler
{

/**
 * @brief Reports one allocation/deallocation event to the active profiling
 * session, mirroring PyTorch's c10::reportMemoryUsageToProfiler. Safe to
 * call unconditionally from any allocator: a true no-op when no profiling
 * session is active or memory profiling was not requested.
 *
 * @param ptr Address returned by (or passed to) the allocator.
 * @param alloc_size Signed size of this allocation, negative for a
 *        deallocation, matching PyTorch's convention.
 * @param total_allocated Running total of bytes currently allocated.
 * @param total_reserved Running total of bytes reserved by the allocator's
 *        pool, including currently-unused blocks.
 * @param device_type One of profiler::device_enum's underlying values (CPU,
 *        CUDA, HIP, PrivateUse1).
 * @param device_index Device ordinal, or -1 if not applicable.
 */
PROFILER_API void report_memory_usage(
    void*   ptr,
    int64_t alloc_size,
    size_t  total_allocated,
    size_t  total_reserved,
    int16_t device_type,
    int16_t device_index);

/**
 * @brief Cheap check for whether the active session wants memory events,
 * mirroring PyTorch's c10::memoryProfilingEnabled(). report_memory_usage()
 * is already a true no-op when profiling isn't active, but that check comes
 * too late for a caller that must gather non-trivial data first (e.g. a
 * caching allocator snapshotting its stats before and after an allocate/
 * deallocate to compute the real byte delta) -- such callers should check
 * this first and skip that work entirely when it returns false.
 */
PROFILER_API bool memory_profiling_active();

}  // namespace profiler
