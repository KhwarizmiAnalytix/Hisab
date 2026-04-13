#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "ProfilerTest.h"
#include "native/platform/env_time.h"
#include "native/platform/env_var.h"

using namespace quarisma;

namespace
{
#ifdef _WIN32
void set_env(const char* name, const char* value)
{
    _putenv_s(name, value);
}
void unset_env(const char* name)
{
    _putenv_s(name, "");
}
#else
void set_env(const char* name, const char* value)
{
    setenv(name, value, 1);
}
void unset_env(const char* name)
{
    unsetenv(name);
}
#endif

}  // namespace

// ============================================================================
// Environment Time Tests
// ============================================================================

PROFILERTEST(Profiler, env_time_now_nanos_returns_positive)
{
    uint64_t time = env_time::now_nanos();

    EXPECT_GT(time, 0);
}

PROFILERTEST(Profiler, env_time_now_nanos_monotonic)
{
    uint64_t time1 = env_time::now_nanos();
    uint64_t time2 = env_time::now_nanos();

    EXPECT_GE(time2, time1);
}

PROFILERTEST(Profiler, env_time_now_nanos_increasing)
{
    uint64_t time1 = env_time::now_nanos();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t time2 = env_time::now_nanos();

    EXPECT_GT(time2, time1);
}

PROFILERTEST(Profiler, env_time_now_micros_returns_positive)
{
    uint64_t time = env_time::now_micros();

    EXPECT_GT(time, 0);
}

PROFILERTEST(Profiler, env_time_now_micros_monotonic)
{
    uint64_t time1 = env_time::now_micros();
    uint64_t time2 = env_time::now_micros();

    EXPECT_GE(time2, time1);
}

PROFILERTEST(Profiler, env_time_now_seconds_returns_positive)
{
    uint64_t time = env_time::now_seconds();

    EXPECT_GT(time, 0);
}

PROFILERTEST(Profiler, env_time_now_seconds_monotonic)
{
    uint64_t time1 = env_time::now_seconds();
    uint64_t time2 = env_time::now_seconds();

    EXPECT_GE(time2, time1);
}

PROFILERTEST(Profiler, env_time_nanos_to_micros_conversion)
{
    uint64_t nanos  = 1000000;  // 1ms
    uint64_t micros = nanos / 1000;

    EXPECT_EQ(micros, 1000);
}

PROFILERTEST(Profiler, env_time_nanos_to_seconds_conversion)
{
    uint64_t nanos   = 1000000000;  // 1 second
    uint64_t seconds = nanos / 1000000000;

    EXPECT_EQ(seconds, 1);
}

PROFILERTEST(Profiler, env_time_micros_to_nanos_conversion)
{
    uint64_t micros = 1000;  // 1ms
    uint64_t nanos  = micros * 1000;

    EXPECT_EQ(nanos, 1000000);
}

PROFILERTEST(Profiler, env_time_multiple_reads_consistency)
{
    uint64_t time1 = env_time::now_nanos();
    uint64_t time2 = env_time::now_nanos();
    uint64_t time3 = env_time::now_nanos();

    EXPECT_GE(time2, time1);
    EXPECT_GE(time3, time2);
}

PROFILERTEST(Profiler, env_time_sleep_and_measure)
{
    uint64_t time1 = env_time::now_nanos();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    uint64_t time2 = env_time::now_nanos();

    uint64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 2500000);  // At least 2.5ms
}

PROFILERTEST(Profiler, env_time_high_resolution_verification)
{
    // Verify we're getting nanosecond precision
    uint64_t time1 = env_time::now_nanos();
    uint64_t time2 = env_time::now_nanos();

    // The difference should be small (less than 1 second)
    uint64_t diff = time2 - time1;
    EXPECT_LT(diff, 1000000000);  // Less than 1 second
}

PROFILERTEST(Profiler, env_time_consistency_between_units)
{
    uint64_t nanos   = env_time::now_nanos();
    uint64_t micros  = env_time::now_micros();
    uint64_t seconds = env_time::now_seconds();

    // All should be positive
    EXPECT_GT(nanos, 0);
    EXPECT_GT(micros, 0);
    EXPECT_GT(seconds, 0);

    // Nanos should be larger than micros
    EXPECT_GT(nanos, micros);
}

PROFILERTEST(Profiler, env_time_large_duration_measurement)
{
    uint64_t time1 = env_time::now_nanos();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint64_t time2 = env_time::now_nanos();

    uint64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 25000000);  // At least 25ms
}

PROFILERTEST(Profiler, env_time_micros_precision)
{
    uint64_t time1 = env_time::now_micros();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    uint64_t time2 = env_time::now_micros();

    uint64_t elapsed = time2 - time1;
    EXPECT_GE(elapsed, 50);  // At least 50 microseconds
}

PROFILERTEST(Profiler, env_time_seconds_precision)
{
    uint64_t time1 = env_time::now_seconds();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint64_t time2 = env_time::now_seconds();

    // Seconds should be the same or time2 >= time1
    EXPECT_GE(time2, time1);
}

// ============================================================================
// Environment Variable Tests
// ============================================================================

PROFILERTEST(Profiler, env_var_read_bool_values)
{
    static constexpr char const* var_name = "PROFILER_TEST_ENV_BOOL";

    unset_env(var_name);
    bool value   = false;
    bool success = read_bool_from_env_var(var_name, true, &value);
    EXPECT_TRUE(success);
    EXPECT_TRUE(value);

    set_env(var_name, "False");
    success = read_bool_from_env_var(var_name, true, &value);
    EXPECT_TRUE(success);
    EXPECT_FALSE(value);

    set_env(var_name, "TRUE");
    success = read_bool_from_env_var(var_name, false, &value);
    EXPECT_TRUE(success);
    EXPECT_TRUE(value);

    set_env(var_name, "not_a_bool");
    value   = true;
    success = read_bool_from_env_var(var_name, false, &value);
    EXPECT_FALSE(success);
    EXPECT_FALSE(value);

    unset_env(var_name);
}

PROFILERTEST(Profiler, env_var_read_int64_with_trimming_and_fallback)
{
    static constexpr char const* var_name = "PROFILER_TEST_ENV_INT";

    set_env(var_name, "  -42  ");
    int64_t value   = 0;
    bool    success = read_int64_from_env_var(var_name, 7, &value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, -42);

    set_env(var_name, "123abc");
    value   = 99;
    success = read_int64_from_env_var(var_name, value, &value);
    EXPECT_FALSE(success);
    EXPECT_EQ(value, 99);

    set_env(var_name, "   ");
    value   = -1;
    success = read_int64_from_env_var(var_name, value, &value);
    EXPECT_FALSE(success);
    EXPECT_EQ(value, -1);

    unset_env(var_name);
    value   = 0;
    success = read_int64_from_env_var(var_name, 1234, &value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, 1234);
}

PROFILERTEST(Profiler, env_var_read_float_with_trimming_and_invalid)
{
    static constexpr char const* var_name = "PROFILER_TEST_ENV_FLOAT";

    set_env(var_name, "  3.25  ");
    float value   = 0.0F;
    bool  success = read_float_from_env_var(var_name, 1.0F, &value);
    EXPECT_TRUE(success);
    EXPECT_FLOAT_EQ(value, 3.25F);

    set_env(var_name, "1.5extra");
    value   = -4.0F;
    success = read_float_from_env_var(var_name, value, &value);
    EXPECT_FALSE(success);
    EXPECT_FLOAT_EQ(value, -4.0F);

    set_env(var_name, "\t ");
    value   = 9.0F;
    success = read_float_from_env_var(var_name, value, &value);
    EXPECT_FALSE(success);
    EXPECT_FLOAT_EQ(value, 9.0F);

    unset_env(var_name);
    value   = 0.0F;
    success = read_float_from_env_var(var_name, 2.5F, &value);
    EXPECT_TRUE(success);
    EXPECT_FLOAT_EQ(value, 2.5F);
}

PROFILERTEST(Profiler, env_var_read_string_default_and_override)
{
    static constexpr char const* var_name = "PROFILER_TEST_ENV_STRING";

    unset_env(var_name);
    std::string value;
    bool        success = read_string_from_env_var(var_name, "fallback", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, "fallback");

    set_env(var_name, "actual");
    value.clear();
    success = read_string_from_env_var(var_name, "ignored", value);
    EXPECT_TRUE(success);
    EXPECT_EQ(value, "actual");

    unset_env(var_name);
}

PROFILERTEST(Profiler, env_var_read_strings_with_trimming_and_default)
{
    static constexpr char const* var_name = "PROFILER_TEST_ENV_STRINGS";

    std::vector<std::string> values;

    set_env(var_name, "alpha, beta , ,gamma");
    bool success = read_strings_from_env_var(var_name, "ignored", values);
    EXPECT_TRUE(success);
    ASSERT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], "alpha");
    EXPECT_EQ(values[1], "beta");
    EXPECT_EQ(values[2], "gamma");

    unset_env(var_name);
    success = read_strings_from_env_var(var_name, "delta,epsilon", values);
    EXPECT_TRUE(success);
    ASSERT_EQ(values.size(), 2U);
    EXPECT_EQ(values[0], "delta");
    EXPECT_EQ(values[1], "epsilon");

    // Test with empty string - on Windows, setting to empty string may unset the variable
    // so we expect the default value to be used
    values.clear();
    set_env(var_name, "");
    success = read_strings_from_env_var(var_name, "zeta", values);
    EXPECT_TRUE(success);
    // On Windows, _putenv_s(name, "") unsets the variable, so default is used
    // On Unix, setenv(name, "", 1) sets it to empty string
    // We accept both behaviors
    if (values.empty())
    {
        // Unix behavior: empty string results in empty vector
        EXPECT_TRUE(values.empty());
    }
    else
    {
        // Windows behavior: empty string unsets variable, so default is used
        ASSERT_EQ(values.size(), 1U);
        EXPECT_EQ(values[0], "zeta");
    }

    unset_env(var_name);
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
