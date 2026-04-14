#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

#include "ProfilerTest.h"
//#include "logger.h"

#if PROFILER_HAS_KINETO
#include "bespoke/common/api.h"
#include "bespoke/common/record_function.h"
#include "bespoke/kineto/profiler_kineto.h"

namespace
{

std::string makeTracePath()
{
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return "profiler_autograd_trace.json";
}

void runSampleWork()
{
    // Use RECORD_USER_SCOPE to create a profiling scope
    //RECORD_USER_SCOPE("autograd_profiler_sample_work");

    const auto start_time = std::chrono::steady_clock::now();

    auto step_callbacks = profiler::getStepCallbacksUnlessEmpty(profiler::RecordScope::FUNCTION);
    if PROFILER_UNLIKELY (step_callbacks.has_value())
    {
        profiler::RecordFunction guard(std::move(*step_callbacks));
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

}  // namespace

#endif  // PROFILER_HAS_KINETO

#if PROFILER_HAS_KINETO

PROFILERTEST(profiler, autograd_chrome_trace_export)
{
    const std::set<profiler::autograd::profiler_impl::ActivityType> activities{
        profiler::autograd::profiler_impl::ActivityType::CPU,
    };

    // Enable RecordFunction FIRST before enabling profiler
    //profiler::RecordFunctionGuard record_function_guard(/*is_enabled=*/true);

    profiler::autograd::profiler_impl::ProfilerConfig config(
        profiler::autograd::profiler_impl::ProfilerState::KINETO,
        /*report_input_shapes=*/true,
        /*profile_memory=*/true,
        /*with_stack=*/true,
        /*with_flops=*/true,
        /*with_modules=*/false);

    // Specify USER_SCOPE to capture RECORD_USER_SCOPE events
    const std::unordered_set<profiler::RecordScope> scopes = {profiler::RecordScope::FUNCTION};

    profiler::autograd::profiler_impl::prepareProfiler(config, activities);
    profiler::autograd::profiler_impl::enableProfiler(config, activities, scopes);

    EXPECT_TRUE(profiler::hasCallbacks());

    std::cout << "Callbacks registered: " << profiler::hasCallbacks() << std::endl;

    runSampleWork();

    auto result = profiler::autograd::profiler_impl::disableProfiler();
    EXPECT_NE(result, nullptr);
    const auto trace_path = makeTracePath();
    result->save(trace_path);

    std::ifstream trace_input(trace_path, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(trace_input.is_open());
    const auto file_size = static_cast<std::size_t>(trace_input.tellg());
    EXPECT_GT(file_size, 0);
    trace_input.close();

    // Keep the file for inspection - comment out deletion
    // std::remove(trace_path.c_str());
    std::cout << "Trace file saved to: " << trace_path << " (size: " << file_size << " bytes)"
              << std::endl;
}

#endif  // PROFILER_HAS_KINETO
