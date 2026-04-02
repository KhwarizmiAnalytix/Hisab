#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include "Testing/baseTest.h"
#include "bespoke/common/api.h"
#include "bespoke/common/record_function.h"
#include "bespoke/kineto/profiler_kineto.h"
//#include "logging/logger.h"

namespace
{
#if PROFILER_HAS_KINETO

std::string makeTracePath()
{
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return "quarisma_autograd_trace.json";
}

void runSampleWork()
{
    // Use RECORD_USER_SCOPE to create a profiling scope
    //RECORD_USER_SCOPE("autograd_profiler_sample_work");

    const auto start_time = std::chrono::steady_clock::now();

    auto step_callbacks = quarisma::getStepCallbacksUnlessEmpty(quarisma::RecordScope::FUNCTION);
    if PROFILER_UNLIKELY (step_callbacks.has_value())
    {
        quarisma::RecordFunction guard(std::move(*step_callbacks));
        auto                     f = [](int n)
        {
            double accumulator = 0.;
            for (int i = 0; i < n; ++i)
            {
                double x = static_cast<double>(i / 1000.0);
                accumulator += sinh(x) / x;
            }

            if (accumulator == -1)
            {
                accumulator = 0;
            }
        };
        guard.before("test", -1);
        f(10000);
    }
}

#endif  // PROFILER_HAS_KINETO
}  // namespace

#if PROFILER_HAS_KINETO

QUARISMATEST(profiler, autograd_chrome_trace_export)
{
    const std::set<quarisma::autograd::profiler_impl::ActivityType> activities{
        quarisma::autograd::profiler_impl::ActivityType::CPU,
    };

    // Enable RecordFunction FIRST before enabling profiler
    //quarisma::RecordFunctionGuard record_function_guard(/*is_enabled=*/true);

    quarisma::autograd::profiler_impl::ProfilerConfig config(
        quarisma::autograd::profiler_impl::ProfilerState::KINETO,
        /*report_input_shapes=*/true,
        /*profile_memory=*/true,
        /*with_stack=*/true,
        /*with_flops=*/true,
        /*with_modules=*/false);

    // Specify USER_SCOPE to capture RECORD_USER_SCOPE events
    const std::unordered_set<quarisma::RecordScope> scopes = {quarisma::RecordScope::FUNCTION};

    quarisma::autograd::profiler_impl::prepareProfiler(config, activities);
    quarisma::autograd::profiler_impl::enableProfiler(config, activities, scopes);

    EXPECT_TRUE(quarisma::hasCallbacks()) << "RecordFunction callbacks not registered for profiler";

    std::cout << "Callbacks registered: " << quarisma::hasCallbacks() << std::endl;

    runSampleWork();

    auto result = quarisma::autograd::profiler_impl::disableProfiler();
    EXPECT_NE(result, nullptr);
    const auto trace_path = makeTracePath();
    result->save(trace_path);

    std::ifstream trace_input(trace_path, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(trace_input.is_open());
    const auto file_size = static_cast<std::size_t>(trace_input.tellg());
    EXPECT_GT(file_size, 0) << "Trace file is empty";
    trace_input.close();

    // Keep the file for inspection - comment out deletion
    // std::remove(trace_path.c_str());
    std::cout << "Trace file saved to: " << trace_path << " (size: " << file_size << " bytes)"
              << std::endl;
}

#endif  // PROFILER_HAS_KINETO
