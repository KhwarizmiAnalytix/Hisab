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

/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

#include "native/utils/time_utils.h"

#include <chrono>
#include <thread>

namespace quarisma::profiler
{

int64_t get_current_time_nanos()
{
    // Use steady_clock to guarantee monotonic progression for duration math
    auto now      = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

void sleep_for_nanos(int64_t ns)
{
    if (ns <= 0)
    {
        return;
    }
    std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
}

void spin_for_nanos(int64_t ns)
{
    if (ns <= 0)
    {
        return;
    }
    int64_t const deadline = get_current_time_nanos() + ns;
    while (get_current_time_nanos() < deadline)
    {
        // Busy-wait (spin)
    }
}

}  // namespace quarisma::profiler
