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

#include "LoggingTest.h"
#include "logger/logger.h"

namespace
{
LOGGING_UNUSED void log_handler(void* user_data, const logging::logger::Message& message)
{
    auto* lines = reinterpret_cast<std::string*>(user_data);
    (*lines) += "\n";
    (*lines) += message.message;
}
}  // namespace

LOGGINGTEST(Logger, test)
{
    int    arg     = 0;
    char** arg_str = nullptr;

    logging::logger::Init(arg, arg_str);
    logging::logger::Init();

    logging::logger::ConvertToVerbosity(-100);
    logging::logger::ConvertToVerbosity(+100);
    logging::logger::ConvertToVerbosity("OFF");
    logging::logger::ConvertToVerbosity("ERROR");
    logging::logger::ConvertToVerbosity("WARNING");
    logging::logger::ConvertToVerbosity("INFO");
    logging::logger::ConvertToVerbosity("MAX");
    logging::logger::ConvertToVerbosity("NAN");

    LOGGING_UNUSED auto v1 = logging::logger::ConvertToVerbosity(1);
    LOGGING_UNUSED auto v2 = logging::logger::ConvertToVerbosity("TRACE");

    std::string lines;
    LOGGING_LOG(
        INFO,
        "changing verbosity to {}",
        static_cast<int>(logging::logger_verbosity_enum::VERBOSITY_TRACE));

    logging::logger::AddCallback(
        "sonnet-grabber", log_handler, &lines, logging::logger_verbosity_enum::VERBOSITY_INFO);

    logging::logger::SetStderrVerbosity(logging::logger_verbosity_enum::VERBOSITY_TRACE);

    LOGGING_LOG_SCOPE_FUNCTION(INFO);
    {
        // Note: Formatted scope logging is not supported in fmt-style API
        // Using simple scope start/end instead
        LOGGING_LOG_START_SCOPE(INFO, "Sonnet 18");
        const auto* whom = "thee";
        LOGGING_LOG(INFO, "Shall I compare {} to a summer's day?", whom);

        const auto* what0 = "lovely";
        const auto* what1 = "temperate";
        LOGGING_LOG(INFO, "Thou art more {} and more {}:", what0, what1);

        const auto* month = "May";
        LOGGING_LOG_IF(INFO, true, "Rough winds do shake the darling buds of {},", month);
        LOGGING_LOG_IF(INFO, true, "And {}'s lease hath all too short a date;", "summers");
        LOGGING_LOG_END_SCOPE("Sonnet 18");
    }

    std::cerr << "--------------------------------------------" << std::endl
              << lines << std::endl
              << std::endl
              << "--------------------------------------------" << std::endl;

    LOGGING_LOG_WARNING("testing generic warning -- should only show up in the log");

    // remove callback since the user-data becomes invalid out of this function.
    logging::logger::RemoveCallback("sonnet-grabber");

    // test out explicit scope start and end markers.
    {
        LOGGING_LOG_START_SCOPE(INFO, "scope-0");
    }
    LOGGING_LOG_START_SCOPE(INFO, "scope-1");
    LOGGING_LOG_INFO("some text");
    LOGGING_LOG_END_SCOPE("scope-1");
    {
        LOGGING_LOG_END_SCOPE("scope-0");
    }

    logging::logger::SetInternalVerbosityLevel(v2);

    logging::logger::SetThreadName("ttq::worker");
    LOGGING_UNUSED auto th = logging::logger::GetThreadName();

    logging::logger::LogScopeRAII obj;

    logging::logger::LogScopeRAII obj1 = std::move(obj);

    END_TEST();
}
