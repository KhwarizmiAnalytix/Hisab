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
#ifdef _WIN32
#include <windows.h>  // for GetModuleHandleW, GetProcAddress, FILETIME

#include <chrono>   // for duration_cast, duration, nanoseconds, system...
#include <cstdint>  // for int64_t, uint64_t

#include "native/platform/env_time.h"  // for env_time

using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::chrono::system_clock;

namespace quarisma
{

namespace
{
typedef VOID(WINAPI* FnGetSystemTimePreciseAsFileTime)(LPFILETIME);  //NOLINT
}

uint64_t env_time::now_nanos()
{
    static FnGetSystemTimePreciseAsFileTime precise_time_function =
        []() -> FnGetSystemTimePreciseAsFileTime
    {
        HMODULE module = GetModuleHandleW(L"kernel32.dll");
        if (module != nullptr)
        {
            return (FnGetSystemTimePreciseAsFileTime)GetProcAddress(
                module, "GetSystemTimePreciseAsFileTime");
        }

        return nullptr;
    }();

    if (precise_time_function != nullptr)
    {
        // GetSystemTimePreciseAsFileTime function is only available in latest
        // versions of Windows, so we need to check for its existence here.
        // All std::chrono clocks on Windows proved to return values that may
        // repeat, which is not good enough for some uses.
        constexpr int64_t kUnixEpochStartTicks = 116444736000000000LL;

        // This interface needs to return system time and not just any time
        // because it is often used as an argument to TimedWait() on condition
        // variable.
        FILETIME system_time;
        precise_time_function(&system_time);

        LARGE_INTEGER li;
        li.LowPart  = system_time.dwLowDateTime;
        li.HighPart = system_time.dwHighDateTime;
        // Subtract unix epoch start
        li.QuadPart -= kUnixEpochStartTicks;

        constexpr int64_t kFtToNanoSec = 100;
        li.QuadPart *= kFtToNanoSec;
        return li.QuadPart;
    }
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace quarisma
#endif