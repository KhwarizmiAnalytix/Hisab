
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "Testing/baseTest.h"
#include "bespoke/base/base.h"
#include "bespoke/common/orchestration/observer.h"
#include "bespoke/common/record_function.h"
#include "bespoke/kineto/profiler_kineto.h"
#include "native/session/profiler.h"

namespace
{

void busy_wait_for(std::chrono::microseconds duration)
{
    const auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < duration)
    {
        PROFILER_UNUSED volatile int spin = 0;
        ++spin;
    }
}

std::unordered_set<quarisma::RecordScope> user_scopes()
{ return {quarisma::RecordScope::USER_SCOPE}; }

}  // namespace

#if PROFILER_HAS_KINETO

namespace
{

quarisma::autograd::profiler_impl::ProfilerConfig make_kineto_config(bool with_stack = false)
{
    return quarisma::autograd::profiler_impl::ProfilerConfig(
        quarisma::autograd::profiler_impl::ProfilerState::KINETO,
        /*report_input_shapes=*/false,
        /*profile_memory=*/false,
        /*with_stack=*/with_stack,
        /*with_flops=*/false,
        /*with_modules=*/false);
}

}  // namespace

namespace
{

struct kineto_session_guard
{
    kineto_session_guard(
        quarisma::autograd::profiler_impl::ProfilerConfig         cfg,
        std::set<quarisma::autograd::profiler_impl::ActivityType> activities,
        std::unordered_set<quarisma::RecordScope>                 scopes = user_scopes())
        : config_{std::move(cfg)}, activities_{std::move(activities)}, scopes_{std::move(scopes)}
    {
        try
        {
            quarisma::autograd::profiler_impl::prepareProfiler(config_, activities_);
            quarisma::autograd::profiler_impl::enableProfiler(config_, activities_, scopes_);
            active_ = true;
        }
        catch (const std::exception& ex)
        {
            error_message_ = ex.what();
        }
    }

    kineto_session_guard(const kineto_session_guard&)            = delete;
    kineto_session_guard(kineto_session_guard&&)                 = delete;
    kineto_session_guard& operator=(const kineto_session_guard&) = delete;
    kineto_session_guard& operator=(kineto_session_guard&&)      = delete;

    ~kineto_session_guard()
    {
        if (active_)
        {
            try
            {
                quarisma::autograd::profiler_impl::disableProfiler();
            }
            catch (...)
            {
                // Never let a failed disable propagate from a test helper.
            }
        }
    }

    bool active() const { return active_; }

    const std::string& error() const { return error_message_; }

    std::unique_ptr<quarisma::autograd::profiler_impl::ProfilerResult> stop()
    {
        if (!active_)
        {
            return nullptr;
        }
        active_ = false;
        return quarisma::autograd::profiler_impl::disableProfiler();
    }

private:
    quarisma::autograd::profiler_impl::ProfilerConfig         config_;
    std::set<quarisma::autograd::profiler_impl::ActivityType> activities_;
    std::unordered_set<quarisma::RecordScope>                 scopes_;
    bool                                                      active_{false};
    std::string                                               error_message_;
};

const quarisma::autograd::profiler_impl::KinetoEvent* find_event_by_name(
    const std::vector<quarisma::autograd::profiler_impl::KinetoEvent>& events,
    const std::string&                                                 name)
{
    for (const auto& event : events)
    {
        if (event.name() == name)
        {
            return &event;
        }
    }
    return nullptr;
}

}  // namespace

//QUARISMATEST(KinetoIntegration, captures_basic_scope_lifecycle)
//{
//    kineto_session_guard session(
//        make_kineto_config(), {quarisma::autograd::profiler_impl::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    {
//        RECORD_USER_SCOPE("kineto_basic_scope");
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    const auto* scope_event = find_event_by_name(events, "kineto_basic_scope");
//    ASSERT_NE(scope_event, nullptr);
//    EXPECT_GT(scope_event->durationNs(), 0);
//    EXPECT_FALSE(scope_event->isHiddenEvent());
//}
//
//QUARISMATEST(KinetoIntegration, nested_scopes_preserve_parent_child_timing)
//{
//    kineto_session_guard session(
//        make_kineto_config(/*with_stack=*/true), {quarisma::autograd::profiler_impl::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    {
//        RECORD_USER_SCOPE("kineto_parent_scope");
//        busy_wait_for(std::chrono::milliseconds(1));
//        {
//            RECORD_USER_SCOPE("kineto_child_scope");
//            busy_wait_for(std::chrono::milliseconds(1));
//        }
//    }
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    const auto* parent_event = find_event_by_name(events, "kineto_parent_scope");
//    const auto* child_event  = find_event_by_name(events, "kineto_child_scope");
//    if (parent_event == nullptr || child_event == nullptr)
//    {
//        GTEST_SKIP() << "Required Kineto events not emitted";
//    }
//
//    EXPECT_LE(parent_event->startNs(), child_event->startNs());
//    EXPECT_GE(parent_event->endNs(), child_event->endNs());
//    EXPECT_GE(parent_event->durationNs(), child_event->durationNs());
//}
//
//QUARISMATEST(KinetoIntegration, thread_local_participation_requires_opt_in)
//{
//    kineto_session_guard session(
//        make_kineto_config(), {quarisma::autograd::profiler_impl::ActivityType::CPU});
//    if (!session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << session.error();
//    }
//
//    constexpr const char* detached_scope_name = "kineto_thread_scope_detached";
//    constexpr const char* attached_scope_name = "kineto_thread_scope_attached";
//
//    std::thread detached_thread(
//        [detached_scope_name]()
//        {
//            RECORD_USER_SCOPE(detached_scope_name);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        });
//
//    std::thread attached_thread(
//        [attached_scope_name]()
//        {
//            quarisma::autograd::profiler_impl::enableProfilerInChildThread();
//            RECORD_USER_SCOPE(attached_scope_name);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            quarisma::autograd::profiler_impl::disableProfilerInChildThread();
//        });
//
//    detached_thread.join();
//    attached_thread.join();
//
//    auto profiler_result = session.stop();
//    ASSERT_NE(profiler_result, nullptr);
//    const auto& events = profiler_result->events();
//    if (events.empty())
//    {
//        GTEST_SKIP() << "Kineto backend produced no CPU events in this environment";
//    }
//
//    bool found_detached = false;
//    bool found_attached = false;
//    for (const auto& event : events)
//    {
//        if (event.name() == detached_scope_name)
//        {
//            found_detached = true;
//        }
//        if (event.name() == attached_scope_name)
//        {
//            found_attached = true;
//        }
//    }
//
//    EXPECT_TRUE(found_attached);
//    EXPECT_FALSE(found_detached);
//}

//QUARISMATEST(KinetoIntegration, enable_disable_cycles_reset_global_state)
//{
//    kineto_session_guard first_session(
//        make_kineto_config(), {quarisma::autograd::profiler_impl::ActivityType::CPU});
//    if (!first_session.active())
//    {
//        GTEST_SKIP() << "Kineto profiler unavailable: " << first_session.error();
//    }
//
//    EXPECT_TRUE(quarisma::autograd::profiler_impl::isProfilerEnabledInMainThread());
//    first_session.stop();
//    EXPECT_FALSE(quarisma::autograd::profiler_impl::isProfilerEnabledInMainThread());
//
//    kineto_session_guard second_session(
//        make_kineto_config(), {quarisma::autograd::profiler_impl::ActivityType::CPU});
//    ASSERT_TRUE(second_session.active());
//    EXPECT_TRUE(quarisma::autograd::profiler_impl::isProfilerEnabledInMainThread());
//    auto result = second_session.stop();
//    EXPECT_FALSE(quarisma::autograd::profiler_impl::isProfilerEnabledInMainThread());
//    EXPECT_NE(result, nullptr);
//}

#endif  // PROFILER_HAS_KINETO

#if PROFILER_HAS_ITT

namespace
{

class recording_itt_stub : public quarisma::profiler_impl::impl::ProfilerStubs
{
public:
    void record(
        int16_t*, quarisma::profiler_impl::impl::ProfilerVoidEventStub*, int64_t*) const override
    {
    }

    float elapsed(
        const quarisma::profiler_impl::impl::ProfilerVoidEventStub*,
        const quarisma::profiler_impl::impl::ProfilerVoidEventStub*) const override
    { return 0.0F; }

    void mark(const char* name) const override
    {
        if (name != nullptr)
        {
            marks_.emplace_back(name);
        }
    }

    void rangePush(const char* name) const override
    {
        if (name != nullptr)
        {
            pushes_.emplace_back(name);
            active_stack_.emplace_back(name);
        }
        else
        {
            ++null_pushes_;
        }
    }

    void rangePop() const override
    {
        if (!active_stack_.empty())
        {
            closed_.emplace_back(active_stack_.back());
            active_stack_.pop_back();
        }
        ++pops_;
    }

    bool enabled() const override { return true; }

    void onEachDevice(std::function<void(int)>) const override {}

    void synchronize() const override {}

    mutable std::vector<std::string> pushes_;
    mutable std::vector<std::string> closed_;
    mutable std::vector<std::string> marks_;
    mutable std::vector<std::string> active_stack_;
    mutable size_t                   pops_{0};
    mutable size_t                   null_pushes_{0};
};

class scoped_itt_stub
{
public:
    explicit scoped_itt_stub(recording_itt_stub& stub)
        : stub_(stub),
          previous_(
              const_cast<quarisma::profiler_impl::impl::ProfilerStubs*>(
                  quarisma::profiler_impl::impl::ittStubs()))
    { quarisma::profiler_impl::impl::registerITTMethods(&stub_); }

    scoped_itt_stub(const scoped_itt_stub&)            = delete;
    scoped_itt_stub(scoped_itt_stub&&)                 = delete;
    scoped_itt_stub& operator=(const scoped_itt_stub&) = delete;
    scoped_itt_stub& operator=(scoped_itt_stub&&)      = delete;

    ~scoped_itt_stub() { quarisma::profiler_impl::impl::registerITTMethods(previous_); }

private:
    recording_itt_stub&                                 stub_;
    quarisma::profiler_impl::impl::ProfilerStubs* const previous_;
};

}  // namespace

QUARISMATEST(ITTIntegration, records_basic_scope_sequence)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    quarisma::autograd::profiler_impl::ProfilerConfig config(
        quarisma::autograd::profiler_impl::ProfilerState::ITT);
    try
    {
        quarisma::autograd::profiler_impl::enableProfiler(
            config, {quarisma::autograd::profiler_impl::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("itt_basic_scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    PROFILER_UNUSED auto result = quarisma::autograd::profiler_impl::disableProfiler();

    ASSERT_FALSE(stub.pushes_.empty());
    EXPECT_EQ(stub.pushes_.front(), "itt_basic_scope");
    EXPECT_EQ(stub.pops_, stub.pushes_.size());
}

QUARISMATEST(ITTIntegration, nested_scopes_close_in_lifo_order)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    quarisma::autograd::profiler_impl::ProfilerConfig config(
        quarisma::autograd::profiler_impl::ProfilerState::ITT);
    try
    {
        quarisma::autograd::profiler_impl::enableProfiler(
            config, {quarisma::autograd::profiler_impl::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("itt_parent_scope");
        busy_wait_for(std::chrono::milliseconds(1));
        {
            RECORD_USER_SCOPE("itt_child_scope");
            busy_wait_for(std::chrono::milliseconds(1));
        }
    }

    PROFILER_UNUSED auto result = quarisma::autograd::profiler_impl::disableProfiler();

    ASSERT_GE(stub.closed_.size(), 2U);
    EXPECT_EQ(stub.closed_.front(), "itt_child_scope");
    EXPECT_EQ(stub.closed_.back(), "itt_parent_scope");
}

//QUARISMATEST(ITTIntegration, callbacks_are_thread_local)
//{
//    recording_itt_stub stub;
//    scoped_itt_stub    stub_guard(stub);
//
//    quarisma::autograd::profiler_impl::ProfilerConfig config(
//        quarisma::autograd::profiler_impl::ProfilerState::ITT);
//    try
//    {
//        quarisma::autograd::profiler_impl::enableProfiler(
//            config, {quarisma::autograd::profiler_impl::ActivityType::CPU}, user_scopes());
//    }
//    catch (const std::exception& ex)
//    {
//        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
//    }
//
//    constexpr const char* main_scope   = "itt_thread_main_scope";
//    constexpr const char* worker_scope = "itt_thread_worker_scope";
//
//    std::thread worker(
//        []()
//        {
//            RECORD_USER_SCOPE("worker_scope");
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        });
//
//    {
//        RECORD_USER_SCOPE("main_scope");
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//
//    worker.join();
//
//    PROFILER_UNUSED auto result = quarisma::autograd::profiler_impl::disableProfiler();
//
//    bool saw_main_scope   = false;
//    bool saw_worker_scope = false;
//    for (const auto& name : stub.pushes_)
//    {
//        if (name == main_scope)
//        {
//            saw_main_scope = true;
//        }
//        if (name == worker_scope)
//        {
//            saw_worker_scope = true;
//        }
//    }
//
//    EXPECT_TRUE(saw_main_scope);
//    EXPECT_FALSE(saw_worker_scope);
//}

QUARISMATEST(ITTIntegration, handles_empty_and_null_range_names)
{
    recording_itt_stub stub;
    scoped_itt_stub    stub_guard(stub);

    quarisma::autograd::profiler_impl::ProfilerConfig config(
        quarisma::autograd::profiler_impl::ProfilerState::ITT);
    try
    {
        quarisma::autograd::profiler_impl::enableProfiler(
            config, {quarisma::autograd::profiler_impl::ActivityType::CPU}, user_scopes());
    }
    catch (const std::exception& ex)
    {
        GTEST_SKIP() << "ITT callbacks unavailable: " << ex.what();
    }

    {
        RECORD_USER_SCOPE("");
        busy_wait_for(std::chrono::milliseconds(1));
    }

    quarisma::profiler_impl::impl::ittStubs()->rangePush(nullptr);
    quarisma::profiler_impl::impl::ittStubs()->rangePop();

    PROFILER_UNUSED auto result = quarisma::autograd::profiler_impl::disableProfiler();

    bool saw_empty_name = false;
    for (const auto& name : stub.pushes_)
    {
        if (name.empty())
        {
            saw_empty_name = true;
            break;
        }
    }

    EXPECT_TRUE(saw_empty_name);
    EXPECT_EQ(stub.null_pushes_, 1U);
}

#endif  // PROFILER_HAS_ITT

namespace
{

std::filesystem::path make_temp_trace_path(const std::string& name)
{
    auto            path = std::filesystem::temp_directory_path() / name;
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return path;
}

}  // namespace

QUARISMATEST(ProfilerChromeTrace, exports_nested_scope_metadata)
{
    quarisma::profiler_session_builder builder;
    auto session = builder.with_hierarchical_profiling(true).with_memory_tracking(false).build();
    ASSERT_NE(session, nullptr);
    ASSERT_TRUE(session->start());

    {
        quarisma::profiler_scope outer_scope("chrome_trace_parent", session.get());
        busy_wait_for(std::chrono::milliseconds(1));
        {
            quarisma::profiler_scope inner_scope("chrome_trace_child", session.get());
            busy_wait_for(std::chrono::milliseconds(1));
        }
    }

    ASSERT_TRUE(session->stop());

    auto trace_path = make_temp_trace_path("quarisma_profiler_trace.json");
    ASSERT_TRUE(session->write_chrome_trace(trace_path.string()));

    std::ifstream trace_file(trace_path);
    ASSERT_TRUE(trace_file.good());
    const std::string trace_contents(
        (std::istreambuf_iterator<char>(trace_file)), std::istreambuf_iterator<char>());

    EXPECT_NE(trace_contents.find("\"traceEvents\""), std::string::npos);
    EXPECT_NE(trace_contents.find("chrome_trace_parent"), std::string::npos);
    EXPECT_NE(trace_contents.find("chrome_trace_child"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(trace_path, ec);
}

QUARISMATEST(ProfilerChromeTrace, write_chrome_trace_rejects_empty_path)
{
    quarisma::profiler_options opts;
    auto                       session = std::make_unique<quarisma::profiler_session>(opts);
    ASSERT_TRUE(session->start());
    {
        quarisma::profiler_scope scope("chrome_trace_invalid_path", session.get());
        busy_wait_for(std::chrono::milliseconds(1));
    }
    ASSERT_TRUE(session->stop());
    EXPECT_FALSE(session->write_chrome_trace(""));
}
