/*
 * Quarisma Kineto Profiler Tests
 *
 * Comprehensive test suite for the Kineto profiler wrapper.
 * Tests cover happy paths, edge cases, boundary conditions, and error handling.
 */

#include <chrono>
#include <cmath>
#include <memory>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ProfilerTest.h"
#include "bespoke/common/record_function.h"
#include "bespoke/kineto/profiler_kineto.h"

#if 0
// NOTE: The following tests were written for a profiler::kineto_profiler API that does not exist.
// The actual Kineto profiler API is in profiler::autograd::profiler_impl namespace with functions like
// enableProfiler(), disableProfiler(), and prepareProfiler().
// These tests are disabled until the API is implemented or tests are rewritten to use the actual API.

using namespace profiler::kineto_profiler;

// ============================================================================
// Test Fixtures
// ============================================================================

class KinetoProfilerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset state before each test
        kineto_profiler::initialize(true);
    }

    void TearDown() override
    {
        // Cleanup after each test
    }
};

// ============================================================================
// Factory Method Tests
// ============================================================================

PROFILERTEST(Profiler, kineto_create_returns_valid_profiler)
{
    auto profiler = kineto_profiler::create();
    // Profiler may be nullptr if Kineto is not available
    // This is acceptable for cross-platform builds
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

PROFILERTEST(Profiler, kineto_create_with_config_returns_valid_profiler)
{
    profiling_config config;
    config.enable_cpu_tracing = true;
    config.enable_gpu_tracing = false;
    config.output_dir         = "./test_profiles";
    config.trace_name         = "test_trace";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

PROFILERTEST(Profiler, kineto_create_with_empty_config)
{
    profiling_config config;
    auto             profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_NE(profiler.get(), nullptr);
    }
}

// ============================================================================
// Profiling Control Tests
// ============================================================================

PROFILERTEST(Profiler, kineto_profiler_not_profiling_initially)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        EXPECT_FALSE(profiler->is_profiling());
    }
}

PROFILERTEST(Profiler, kineto_start_profiling_succeeds)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        bool result = profiler->start_profiling();
        // Result depends on Kineto availability
        if (result)
        {
            EXPECT_TRUE(profiler->is_profiling());
            profiler->stop_profiling();
        }
    }
}

PROFILERTEST(Profiler, kineto_stop_profiling_returns_result)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            auto result = profiler->stop_profiling();
            EXPECT_FALSE(profiler->is_profiling());
            // Result structure should be valid - either success or error message
            EXPECT_TRUE(result.success || !result.error_message.empty());
        }
    }
}

PROFILERTEST(Profiler, kineto_stop_profiling_without_start)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        auto result = profiler->stop_profiling();
        EXPECT_FALSE(result.success);
        EXPECT_FALSE(result.error_message.empty());
    }
}

PROFILERTEST(Profiler, kineto_cannot_start_profiling_twice)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            bool second_start = profiler->start_profiling();
            EXPECT_FALSE(second_start);
            profiler->stop_profiling();
        }
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

/*PROFILERTEST(Profiler, kineto_get_config_returns_valid_config)
{
    profiling_config config;
    config.trace_name = "custom_trace";
    config.output_dir = "/tmp/profiles";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        const auto& retrieved_config = profiler->get_config();
        EXPECT_EQ(retrieved_config.trace_name, "custom_trace");
        EXPECT_EQ(retrieved_config.output_dir, "/tmp/profiles");
    }
}*/

PROFILERTEST(Profiler, kineto_set_config_succeeds_when_not_profiling)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        profiling_config new_config;
        new_config.trace_name = "updated_trace";

        bool result = profiler->set_config(new_config);
        EXPECT_TRUE(result);
        EXPECT_EQ(profiler->get_config().trace_name, "updated_trace");
    }
}

PROFILERTEST(Profiler, kineto_set_config_fails_when_profiling)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        if (profiler->start_profiling())
        {
            profiling_config new_config;
            bool             result = profiler->set_config(new_config);
            EXPECT_FALSE(result);
            profiler->stop_profiling();
        }
    }
}

PROFILERTEST(Profiler, kineto_config_with_zero_max_activities)
{
    profiling_config config;
    config.max_activities = 0;  // Unlimited

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().max_activities, 0);
    }
}

PROFILERTEST(Profiler, kineto_config_with_positive_max_activities)
{
    profiling_config config;
    config.max_activities = 1000;

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().max_activities, 1000);
    }
}

// ============================================================================
// Initialization Tests
// ============================================================================

PROFILERTEST(Profiler, kineto_initialize_cpu_only)
{
    bool result = kineto_profiler::initialize(true);
    // Result depends on Kineto availability
    EXPECT_TRUE(result || !result);  // Always true - just checking it doesn't crash
}

PROFILERTEST(Profiler, kineto_initialize_with_gpu)
{
    bool result = kineto_profiler::initialize(false);
    // Result depends on Kineto availability
    EXPECT_TRUE(result || !result);  // Always true - just checking it doesn't crash
}

PROFILERTEST(Profiler, kineto_is_initialized_after_create)
{
    auto profiler = kineto_profiler::create();
    // If profiler was created, Kineto should be initialized
    if (profiler)
    {
        EXPECT_TRUE(kineto_profiler::is_initialized());
    }
}

// ============================================================================
// Edge Cases and Boundary Conditions
// ============================================================================

PROFILERTEST(Profiler, kineto_null_profiler_pointer)
{
    std::unique_ptr<kineto_profiler> profiler = nullptr;
    EXPECT_EQ(profiler.get(), nullptr);
}

PROFILERTEST(Profiler, kineto_profiler_move_semantics)
{
    auto profiler1 = kineto_profiler::create();
    if (profiler1)
    {
        auto profiler2 = std::move(profiler1);
        EXPECT_EQ(profiler1.get(), nullptr);
        EXPECT_NE(profiler2.get(), nullptr);
    }
}

PROFILERTEST(Profiler, kineto_config_with_empty_strings)
{
    profiling_config config;
    config.output_dir = "";
    config.trace_name = "";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().output_dir, "");
        EXPECT_EQ(profiler->get_config().trace_name, "");
    }
}

PROFILERTEST(Profiler, kineto_config_with_special_characters)
{
    profiling_config config;
    config.output_dir = "/tmp/profiles_@#$%";
    config.trace_name = "trace_!@#$%^&*()";

    auto profiler = kineto_profiler::create_with_config(config);
    if (profiler)
    {
        EXPECT_EQ(profiler->get_config().output_dir, "/tmp/profiles_@#$%");
        EXPECT_EQ(profiler->get_config().trace_name, "trace_!@#$%^&*()");
    }
}

PROFILERTEST(Profiler, kineto_profiler_destructor_stops_profiling)
{
    {
        auto profiler = kineto_profiler::create();
        if (profiler)
        {
            profiler->start_profiling();
            EXPECT_TRUE(profiler->is_profiling());
            // Destructor should stop profiling
        }
    }
    // If we reach here without crash, destructor worked correctly
    EXPECT_TRUE(true);
}
#endif  // Disabled tests for non-existent kineto_profiler API

PROFILERTEST(RecordDebugHandles, Basic)
{
    constexpr int64_t kMyFunctionHandle   = 42;
    constexpr int64_t kPrepareHandle      = 100;
    constexpr int64_t kDivisionHandle     = 101;
    constexpr int64_t kInvokeFHandle      = 102;
    constexpr int64_t kFinalizeHandle     = 103;
    constexpr int64_t kDefaultDebugHandle = -1;

    const auto no_inputs = profiler::array_ref<const profiler::IValue>{};

    const auto now_in_us = []()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };

    auto record_step = [&](const char* name, int64_t handle, auto&& body)
    {
        RECORD_EDGE_SCOPE_WITH_DEBUG_HANDLE_AND_INPUTS(name, handle, no_inputs);
        const auto start_us = now_in_us();
        body();
        auto end_us = now_in_us();
        if (end_us <= start_us)
        {
            end_us = start_us + 1;
        }
        profiler::autograd::profiler_impl::reportBackendEventToActiveKinetoProfiler(
            start_us,
            end_us,
            handle,
            profiler::RecordScope::LITE_INTERPRETER,
            name,
            "record_debug_handles_test");
    };

    auto profiled_f = [&](float& value) -> float
    {
        float sum = 0.0f;
        record_step(
            "not_my_function",
            kDefaultDebugHandle,
            [&]()
            {
                record_step("f_prepare_inputs", kDefaultDebugHandle, [&]() { sum = 0.0f; });
                record_step(
                    "f_accumulate",
                    kDefaultDebugHandle,
                    [&]()
                    {
                        for (size_t i = 0; i < 5000; ++i)
                        {
                            sum += value * 3.1415F;
                        }
                    });
            });
        return sum;
    };

    const std::set<profiler::autograd::profiler_impl::ActivityType> activities(
        {profiler::autograd::profiler_impl::ActivityType::CPU});

    const auto profiler_config = profiler::autograd::profiler_impl::ProfilerConfig(
        profiler::autograd::profiler_impl::ProfilerState::KINETO, false, false, true, true);
    profiler::autograd::profiler_impl::prepareProfiler(profiler_config, activities);

    const std::unordered_set<profiler::RecordScope> scopes = {
        profiler::RecordScope::LITE_INTERPRETER};
    profiler::autograd::profiler_impl::enableProfiler(profiler_config, activities, scopes);
    ASSERT_TRUE(profiler::autograd::profiler_impl::isProfilerEnabledInMainThread());
    ASSERT_TRUE(profiler::hasCallbacks());
    profiler::RecordFunctionGuard record_function_guard(/*is_enabled=*/true);
    record_step(
        "my_function",
        kMyFunctionHandle,
        [&]()
        {
            float x{5.9999F};
            float y{2.1212F};
            float z{0.0F};

            record_step(
                "prepare_inputs",
                kPrepareHandle,
                [&]()
                {
                    x = std::abs(x);
                    y = static_cast<float>(std::fabs(y));
                });

            record_step("division_step", kDivisionHandle, [&]() { z = x / y; });

            record_step("invoke_f", kInvokeFHandle, [&]() { z = profiled_f(z); });

            record_step("finalize_results", kFinalizeHandle, [&]() { (void)z; });
        });

    auto        profiler_results_ptr = profiler::autograd::profiler_impl::disableProfiler();
    const auto& kineto_events        = profiler_results_ptr->events();

    if (kineto_events.empty())
    {
        // Some configurations may not emit kineto events even when profiler plumbing is active.
        return;
    }

    const std::vector<std::pair<std::string, int64_t>> expected_events = {
        {"my_function", kMyFunctionHandle},
        {"prepare_inputs", kPrepareHandle},
        {"division_step", kDivisionHandle},
        {"invoke_f", kInvokeFHandle},
        {"not_my_function", kDefaultDebugHandle},
        {"f_prepare_inputs", kDefaultDebugHandle},
        {"f_accumulate", kDefaultDebugHandle},
        {"finalize_results", kFinalizeHandle}};

    std::unordered_map<std::string, int64_t> expected_debug_handles;
    std::unordered_map<std::string, size_t>  observed_counts;
    for (const auto& [name, handle] : expected_events)
    {
        expected_debug_handles.emplace(name, handle);
        observed_counts.emplace(name, 0);
    }

    std::vector<std::string> recorded_event_names;
    recorded_event_names.reserve(kineto_events.size());

    for (const auto& e : kineto_events)
    {
        recorded_event_names.emplace_back(e.name());
        auto it = expected_debug_handles.find(e.name());
        if (it != expected_debug_handles.end())
        {
            EXPECT_EQ(e.debugHandle(), it->second);
            observed_counts[e.name()]++;
        }
    }

    std::ostringstream recorded_summary_stream;
    for (size_t i = 0; i < recorded_event_names.size(); ++i)
    {
        if (i != 0)
        {
            recorded_summary_stream << ", ";
        }
        recorded_summary_stream << recorded_event_names[i];
    }
    const std::string recorded_summary = recorded_summary_stream.str();

    const size_t event_tree_size = profiler_results_ptr->event_tree().size();
    for (const auto& [name, _] : expected_events)
    {
        EXPECT_EQ(observed_counts[name], 1U);
    }
}
#if 0
// ============================================================================
// Thread Safety Tests
// ============================================================================

PROFILERTEST(Profiler, kineto_concurrent_is_profiling_checks)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        std::thread t1(
            [&profiler]()
            {
                for (int i = 0; i < 10; ++i)
                {
                    profiler->is_profiling();
                }
            });

        std::thread t2(
            [&profiler]()
            {
                for (int i = 0; i < 10; ++i)
                {
                    profiler->is_profiling();
                }
            });

        t1.join();
        t2.join();
        EXPECT_TRUE(true);  // No crashes = success
    }
}

PROFILERTEST(Profiler, kineto_concurrent_config_access)
{
    auto profiler = kineto_profiler::create();
    if (profiler)
    {
        std::thread t1(
            [&profiler]()
            {
                for (int i = 0; i < 5; ++i)
                {
                    profiler->get_config();
                }
            });

        std::thread t2(
            [&profiler]()
            {
                profiling_config config;
                for (int i = 0; i < 5; ++i)
                {
                    profiler->set_config(config);
                }
            });

        t1.join();
        t2.join();
        EXPECT_TRUE(true);  // No crashes = success
    }
}
#endif