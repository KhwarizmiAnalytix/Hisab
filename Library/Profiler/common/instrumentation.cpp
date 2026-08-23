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

#include "common/instrumentation.h"

#if PROFILER_HAS_KINETO || PROFILER_HAS_ITT
#include "bespoke/common/orchestration/observer.h"
#endif

namespace profiler
{

void report_memory_usage(
    void*   ptr,
    int64_t alloc_size,
    size_t  total_allocated,
    size_t  total_reserved,
    int16_t device_type,
    int16_t device_index)
{
#if PROFILER_HAS_KINETO || PROFILER_HAS_ITT
    auto* state = profiler_impl::impl::ProfilerStateBase::get();
    if (state == nullptr || !state->memoryProfilingEnabled())
    {
        return;
    }

    device_option device{};
    device.index_ = device_index;
    device.type_  = static_cast<device_enum>(device_type);
    state->reportMemoryUsage(ptr, alloc_size, total_allocated, total_reserved, device);
#else
    (void)ptr;
    (void)alloc_size;
    (void)total_allocated;
    (void)total_reserved;
    (void)device_type;
    (void)device_index;
#endif
}

bool memory_profiling_active()
{
#if PROFILER_HAS_KINETO || PROFILER_HAS_ITT
    auto* state = profiler_impl::impl::ProfilerStateBase::get();
    return state != nullptr && state->memoryProfilingEnabled();
#else
    return false;
#endif
}

}  // namespace profiler
