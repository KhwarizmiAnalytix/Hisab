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

/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

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
#ifndef PROFILER_TSL_PLATFORM_ENV_TIME_H_
#define PROFILER_TSL_PLATFORM_ENV_TIME_H_

#include <cstdint>  // for uint64_t

#include "common/profiler_export.h"  // for PROFILER_API

namespace quarisma
{

/// \brief An interface used by the tsl implementation to
/// access timer related operations.
class env_time
{
public:
    static constexpr uint64_t k_micros_to_picos_   = 1000ULL * 1000ULL;
    static constexpr uint64_t k_micros_to_nanos_   = 1000ULL;
    static constexpr uint64_t k_millis_to_micros_  = 1000ULL;
    static constexpr uint64_t k_millis_to_nanos_   = 1000ULL * 1000ULL;
    static constexpr uint64_t k_nanos_to_picos_    = 1000ULL;
    static constexpr uint64_t k_seconds_to_millis_ = 1000ULL;
    static constexpr uint64_t k_seconds_to_micros_ = 1000ULL * 1000ULL;
    static constexpr uint64_t k_seconds_to_nanos_  = 1000ULL * 1000ULL * 1000ULL;

    env_time()          = default;
    virtual ~env_time() = default;

    /// \brief Returns the number of nano-seconds since the Unix epoch.
    PROFILER_API static uint64_t now_nanos();

    /// \brief Returns the number of micro-seconds since the Unix epoch.
    static uint64_t now_micros() { return now_nanos() / k_micros_to_nanos_; }

    /// \brief Returns the number of seconds since the Unix epoch.
    static uint64_t now_seconds() { return now_nanos() / k_seconds_to_nanos_; }

    /// \brief A version of now_nanos() that may be overridden by a subclass.
    virtual uint64_t get_overridable_now_nanos() const { return now_nanos(); }

    /// \brief A version of now_micros() that may be overridden by a subclass.
    virtual uint64_t get_overridable_now_micros() const
    {
        return get_overridable_now_nanos() / k_micros_to_nanos_;
    }

    /// \brief A version of now_seconds() that may be overridden by a subclass.
    virtual uint64_t get_overridable_now_seconds() const
    {
        return get_overridable_now_nanos() / k_seconds_to_nanos_;
    }
};

}  // namespace quarisma

#endif  // PROFILER_TSL_PLATFORM_ENV_TIME_H_
