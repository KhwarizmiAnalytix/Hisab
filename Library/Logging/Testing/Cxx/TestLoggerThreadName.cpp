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

// Test that quarismaLogger::GetThreadName is unaffected by concurrent accesses
// and usage of quarismaLogger::Init()

#include <atomic>
#include <string>
#include <thread>

#include "LoggingTest.h"
#include "logger/logger.h"

// Control the order of operations between the threads
std::atomic_bool wait1;
std::atomic_bool wait2;

void Thread1()
{
    const std::string threaName = "T1";
    while (!wait1.load()) {}

    logging::logger::SetThreadName(threaName);

    wait2.store(true);

    if (logging::logger::GetThreadName() != threaName)
    {
        LOGGING_LOG(ERROR, "Name mismatch !");
    }
}

void Thread2()
{
    const std::string threaName = "T2";
    logging::logger::SetThreadName(threaName);

    wait1.store(true);
    while (!wait2.load()) {}

    logging::logger::Init();

    if (logging::logger::GetThreadName() != threaName)
    {
        LOGGING_LOG(ERROR, "Name mismatch !");
    }
}

LOGGINGTEST(Logger, thread_name)
{
    LOGGING_UNUSED int    arg     = 0;
    LOGGING_UNUSED char** arg_str = nullptr;

    wait1.store(false);
    wait2.store(false);
    std::thread t1(Thread1);
    std::thread t2(Thread2);

    t1.join();
    t2.join();
    END_TEST();
}
