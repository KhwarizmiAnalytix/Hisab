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
// observer.h) only exist when PROFILER_ENABLE_KINETO or PROFILER_ENABLE_ITT
// is on -- under PROFILER_BACKEND=NATIVE, bespoke/common/ is excluded from
// the library entirely (see Library/Profiler/CMakeLists.txt). This header is
// the one thing outside libraries (Vectorization, Memory, Parallel, ...)
// should include to instrument a call site: it is a real call under
// Kineto/ITT and a true no-op under Native, so instrumented call sites don't
// need their own PROFILER_HAS_* guards.
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
 * session is active, memory profiling wasn't requested for it, or Profiler
 * was built with PROFILER_BACKEND=NATIVE.
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

}  // namespace profiler
