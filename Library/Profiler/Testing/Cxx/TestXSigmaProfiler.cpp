/*
 * Quarisma Profiler API Tests
 *
 * Tests for the high-level Quarisma profiler API including:
 * - profiler_session singleton
 * - profiler_guard RAII behavior
 * - record_function scope tracking
 * - Profiler state transitions
 * - Configuration management
 */

#include "ProfilerTest.h"

#if PROFILER_HAS_PROFILER

#include <chrono>
#include <thread>

#include "profiler_api.h"
#include "profiler_guard.h"

using namespace profiler::profiler_impl;

// ============================================================================
// Test Fixture for profiler_session Tests
// ============================================================================

class ProfilerSessionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset profiler to Disabled state before each test
        auto& session = profiler_session::instance();
        if (session.is_profiling())
        {
            session.stop();
        }
        session.reset();
    }

    void TearDown() override
    {
        // Cleanup after each test
        auto& session = profiler_session::instance();
        if (session.is_profiling())
        {
            session.stop();
        }
        session.reset();
    }
};

// ============================================================================
// profiler_session Tests
// ============================================================================

TEST_F(ProfilerSessionTest, SingletonInstance)
{
    // Test that profiler_session returns the same instance
    auto& session1 = profiler_session::instance();
    auto& session2 = profiler_session::instance();

    EXPECT_EQ(&session1, &session2);
}

TEST_F(ProfilerSessionTest, InitialState)
{
    // Test that profiler starts in Disabled or Ready state
    auto&               session = profiler_session::instance();
    profiler_state_enum state   = get_profiler_state();

    // After cleanup, profiler should be in Disabled or Ready state
    EXPECT_TRUE(state == profiler_state_enum::Disabled || state == profiler_state_enum::Ready);
}

TEST_F(ProfilerSessionTest, StartProfiler)
{
    // Test starting the profiler
    auto& session = profiler_session::instance();

    profiler_config config;
    config.activities = {activity_type_enum::CPU};
    config.verbose    = false;

    bool started = session.start(config);
    EXPECT_TRUE(started);

    profiler_state_enum state = get_profiler_state();
    EXPECT_EQ(state, profiler_state_enum::Recording);
}

TEST_F(ProfilerSessionTest, StopProfiler)
{
    // Test stopping the profiler
    auto& session = profiler_session::instance();

    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    session.start(config);
    bool stopped = session.stop();

    EXPECT_TRUE(stopped);

    profiler_state_enum state = get_profiler_state();
    EXPECT_EQ(state, profiler_state_enum::Ready);
}

TEST_F(ProfilerSessionTest, GetProfilerConfig)
{
    // Test retrieving profiler configuration
    auto& session = profiler_session::instance();

    profiler_config config;
    config.activities = {activity_type_enum::CPU, activity_type_enum::CUDA};
    config.verbose    = true;

    session.start(config);

    const profiler_config& retrieved_config = get_profiler_config();
    EXPECT_EQ(retrieved_config.verbose, true);
}

// ============================================================================
// profiler_guard Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ProfilerGuardRAII)
{
    // Test that profiler_guard starts profiler on construction
    // and stops on destruction
    {
        profiler_config config;
        config.activities = {activity_type_enum::CPU};

        profiler_guard guard(config);

        profiler_state_enum state = get_profiler_state();
        EXPECT_EQ(state, profiler_state_enum::Recording);
    }

    // After guard destruction, profiler should be stopped
    profiler_state_enum state = get_profiler_state();
    EXPECT_EQ(state, profiler_state_enum::Ready);
}

TEST_F(ProfilerSessionTest, ProfilerGuardMultiple)
{
    // Test multiple profiler_guard instances
    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    {
        profiler_guard guard1(config);
        EXPECT_EQ(get_profiler_state(), profiler_state_enum::Recording);

        {
            profiler_guard guard2(config);
            EXPECT_EQ(get_profiler_state(), profiler_state_enum::Recording);
        }

        // After inner guard, profiler should still be recording
        EXPECT_EQ(get_profiler_state(), profiler_state_enum::Recording);
    }

    // After all guards, profiler should be stopped
    EXPECT_EQ(get_profiler_state(), profiler_state_enum::Ready);
}

// ============================================================================
// record_function Tests
// ============================================================================

TEST_F(ProfilerSessionTest, RecordFunctionScope)
{
    // Test record_function scope tracking
    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    auto& session = profiler_session::instance();
    session.start(config);

    {
        record_function record("test_function");
        // Function is being recorded
        EXPECT_TRUE(profiler_enabled());
    }
}

TEST_F(ProfilerSessionTest, RecordFunctionNested)
{
    // Test nested record_function scopes
    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    auto& session = profiler_session::instance();
    session.start(config);

    {
        record_function outer("outer_function");
        {
            record_function inner("inner_function");
            EXPECT_TRUE(profiler_enabled());
        }
        EXPECT_TRUE(profiler_enabled());
    }
}

// ============================================================================
// scoped_activity Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ScopedActivityTracking)
{
    // Test scoped_activity for tracking activities
    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    auto& session = profiler_session::instance();
    session.start(config);

    {
        scoped_activity activity("test_activity");
        EXPECT_TRUE(profiler_enabled());
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ConfigurationActivities)
{
    // Test configuration with multiple activity types
    profiler_config config;
    config.activities = {
        activity_type_enum::CPU, activity_type_enum::CUDA, activity_type_enum::Memory};
    config.verbose = false;

    auto& session = profiler_session::instance();
    bool  started = session.start(config);

    EXPECT_TRUE(started);
    EXPECT_EQ(get_profiler_state(), profiler_state_enum::Recording);
}

TEST_F(ProfilerSessionTest, ConfigurationVerbose)
{
    // Test verbose configuration
    profiler_config config;
    config.activities = {activity_type_enum::CPU};
    config.verbose    = true;

    auto& session = profiler_session::instance();
    session.start(config);

    const profiler_config& retrieved = get_profiler_config();
    EXPECT_EQ(retrieved.verbose, true);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ProfilerSessionTest, ThreadSafety)
{
    // Test that profiler is thread-safe
    profiler_config config;
    config.activities = {activity_type_enum::CPU};

    auto& session = profiler_session::instance();
    session.start(config);

    std::thread t1(
        [&]()
        {
            record_function record("thread1_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

    std::thread t2(
        [&]()
        {
            record_function record("thread2_function");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

    t1.join();
    t2.join();
}

// ============================================================================
// State Transition Tests
// ============================================================================

TEST_F(ProfilerSessionTest, StateTransitions)
{
    // Test valid state transitions: Disabled/Ready -> Recording -> Ready
    auto& session = profiler_session::instance();

    // Initial state (after cleanup)
    profiler_state_enum initial_state = get_profiler_state();
    EXPECT_TRUE(
        initial_state == profiler_state_enum::Disabled ||
        initial_state == profiler_state_enum::Ready);

    // Start profiler
    profiler_config config;
    config.activities = {activity_type_enum::CPU};
    session.start(config);
    EXPECT_EQ(get_profiler_state(), profiler_state_enum::Recording);

    // Stop profiler
    session.stop();
    EXPECT_EQ(get_profiler_state(), profiler_state_enum::Ready);
}

#endif  // PROFILER_HAS_PROFILER
