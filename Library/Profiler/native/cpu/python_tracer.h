/*
 * Quarisma: High-Performance Quantitative Library
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

/* Copyright 2022 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef XLA_BACKENDS_PROFILER_CPU_PYTHON_TRACER_H_
#define XLA_BACKENDS_PROFILER_CPU_PYTHON_TRACER_H_
#if 0
#include <memory>

#include "native/core/profiler_interface.h"

namespace quarisma
{
namespace profiler_impl
{

struct python_tracer_options
{
    // Whether to enable python function calls tracing.
    // NOTE: Runtime overhead ensues if enabled.
    bool enable_trace_python_function = false;

    // Whether to enable python traceme instrumentation.
    bool enable_python_traceme = true;

    // Whether profiling stops within an atexit handler.
    bool end_to_end_mode = false;
};

std::unique_ptr<profiler_interface> create_python_tracer(
    const python_tracer_options& options);

}  // namespace profiler_impl
}  // namespace quarisma

#endif  // 0 (python tracer stub disabled)
#endif  // XLA_BACKENDS_PROFILER_CPU_PYTHON_TRACER_H_
