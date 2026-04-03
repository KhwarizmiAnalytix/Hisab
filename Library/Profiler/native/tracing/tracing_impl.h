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

#include "native/cpu/threadpool_listener_state.h"

namespace quarisma::tracing
{

inline bool event_collector::is_enabled()
{
    return quarisma::profiler_impl::threadpool_listener::IsEnabled();
}

}  // namespace quarisma::tracing

// Stub tracing macros for portability.
#define PROFILER_TRACELITERAL(a) \
    do                           \
    {                            \
    } while (0)
#define PROFILER_TRACESTRING(s) \
    do                          \
    {                           \
    } while (0)
#define PROFILER_TRACEPRINTF(format, ...) \
    do                                    \
    {                                     \
    } while (0)
