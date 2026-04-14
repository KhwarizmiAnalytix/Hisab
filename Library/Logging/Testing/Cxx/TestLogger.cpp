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

#include <iostream>
#include <string>
#include <utility>

#include "CoreTest.h"
#include "logger.h"

namespace
{
QUARISMA_UNUSED void log_handler(void* user_data, const quarisma::logger::Message& message)
{
    auto* lines = reinterpret_cast<std::string*>(user_data);
    (*lines) += "\n";
    (*lines) += message.message;
}
}  // namespace

QUARISMATEST(Logger, test)
{
    int    arg     = 0;
    char** arg_str = nullptr;

    quarisma::logger::Init(arg, arg_str);
    quarisma::logger::Init();

    quarisma::logger::ConvertToVerbosity(-100);
    quarisma::logger::ConvertToVerbosity(+100);
    quarisma::logger::ConvertToVerbosity("OFF");
    quarisma::logger::ConvertToVerbosity("ERROR");
    quarisma::logger::ConvertToVerbosity("WARNING");
    quarisma::logger::ConvertToVerbosity("INFO");
    quarisma::logger::ConvertToVerbosity("MAX");
    quarisma::logger::ConvertToVerbosity("NAN");

    QUARISMA_UNUSED auto v1 = quarisma::logger::ConvertToVerbosity(1);
    QUARISMA_UNUSED auto v2 = quarisma::logger::ConvertToVerbosity("TRACE");

    std::string lines;
    QUARISMA_LOG(
        INFO,
        "changing verbosity to {}",
        static_cast<int>(quarisma::logger_verbosity_enum::VERBOSITY_TRACE));

    quarisma::logger::AddCallback(
        "sonnet-grabber", log_handler, &lines, quarisma::logger_verbosity_enum::VERBOSITY_INFO);

    quarisma::logger::SetStderrVerbosity(quarisma::logger_verbosity_enum::VERBOSITY_TRACE);

    QUARISMA_LOG_SCOPE_FUNCTION(INFO);
    {
        // Note: Formatted scope logging is not supported in fmt-style API
        // Using simple scope start/end instead
        QUARISMA_LOG_START_SCOPE(INFO, "Sonnet 18");
        const auto* whom = "thee";
        QUARISMA_LOG(INFO, "Shall I compare {} to a summer's day?", whom);

        const auto* what0 = "lovely";
        const auto* what1 = "temperate";
        QUARISMA_LOG(INFO, "Thou art more {} and more {}:", what0, what1);

        const auto* month = "May";
        QUARISMA_LOG_IF(INFO, true, "Rough winds do shake the darling buds of {},", month);
        QUARISMA_LOG_IF(INFO, true, "And {}'s lease hath all too short a date;", "summers");
        QUARISMA_LOG_END_SCOPE("Sonnet 18");
    }

    std::cerr << "--------------------------------------------" << std::endl
              << lines << std::endl
              << std::endl
              << "--------------------------------------------" << std::endl;

    QUARISMA_LOG_WARNING("testing generic warning -- should only show up in the log");

    // remove callback since the user-data becomes invalid out of this function.
    quarisma::logger::RemoveCallback("sonnet-grabber");

    // test out explicit scope start and end markers.
    {
        QUARISMA_LOG_START_SCOPE(INFO, "scope-0");
    }
    QUARISMA_LOG_START_SCOPE(INFO, "scope-1");
    QUARISMA_LOG_INFO("some text");
    QUARISMA_LOG_END_SCOPE("scope-1");
    {
        QUARISMA_LOG_END_SCOPE("scope-0");
    }

    quarisma::logger::SetInternalVerbosityLevel(v2);

    quarisma::logger::SetThreadName("ttq::worker");
    QUARISMA_UNUSED auto th = quarisma::logger::GetThreadName();

    quarisma::logger::LogScopeRAII obj;

    quarisma::logger::LogScopeRAII obj1 = std::move(obj);

    END_TEST();
}
