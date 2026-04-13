#include <string>
#include <vector>

#include "CoreTest.h"
#include "logging/back_trace.h"

namespace quarisma
{
namespace
{
// Helper functions to create a deeper call stack
void level_3()
{
    // Capture stack trace at this level
    auto trace = back_trace::print(0, 10, false);
    EXPECT_FALSE(trace.empty());
}

void level_2()
{
    level_3();
}

void level_1()
{
    level_2();
}

}  // namespace
}  // namespace quarisma
using namespace quarisma;
// ============================================================================
// Basic Functionality Tests
// ============================================================================

QUARISMATEST(BackTrace, basic_print)
{
    auto trace = back_trace::print();
    EXPECT_FALSE(trace.empty());
    END_TEST();
}

QUARISMATEST(BackTrace, print_with_skip_frames)
{
    auto trace_no_skip = back_trace::print(0, 5, false);
    auto trace_skip_2  = back_trace::print(2, 5, false);

    EXPECT_FALSE(trace_no_skip.empty());
    EXPECT_FALSE(trace_skip_2.empty());
    END_TEST();
}

QUARISMATEST(BackTrace, print_with_max_frames)
{
    auto trace_5  = back_trace::print(0, 5, false);
    auto trace_10 = back_trace::print(0, 10, false);

    EXPECT_FALSE(trace_5.empty());
    EXPECT_FALSE(trace_10.empty());
    END_TEST();
}

// ============================================================================
// Enhanced API Tests
// ============================================================================

QUARISMATEST(BackTrace, capture_and_format)
{
    // Capture raw frames
    backtrace_options options;
    options.frames_to_skip           = 0;
    options.maximum_number_of_frames = 10;
    options.skip_python_frames       = false;

    auto frames = back_trace::capture(options);
    EXPECT_FALSE(frames.empty());

    // Verify frame structure
    for (const auto& frame : frames)
    {
        EXPECT_FALSE(frame.function_name.empty());
        EXPECT_NE(frame.return_address, nullptr);
    }

    // Format the captured frames
    auto formatted = back_trace::format(frames, options);
    EXPECT_FALSE(formatted.empty());
    END_TEST();
}

QUARISMATEST(BackTrace, compact_format)
{
    backtrace_options options;
    options.frames_to_skip           = 0;
    options.maximum_number_of_frames = 5;
    options.compact_format           = true;
    options.include_addresses        = false;
    options.include_offsets          = false;

    auto trace = back_trace::print(options);
    EXPECT_FALSE(trace.empty());

    // Compact format should contain " -> " separators
    EXPECT_NE(trace.find(" -> "), std::string::npos);
    END_TEST();
}

QUARISMATEST(BackTrace, compact_helper)
{
    auto trace = back_trace::compact(5);
    EXPECT_FALSE(trace.empty());

    // Should contain function call chain
    EXPECT_NE(trace.find(" -> "), std::string::npos);
    END_TEST();
}

QUARISMATEST(BackTrace, detailed_format_with_options)
{
    backtrace_options options;
    options.frames_to_skip           = 0;
    options.maximum_number_of_frames = 5;
    options.compact_format           = false;
    options.include_addresses        = true;
    options.include_offsets          = true;

    auto trace = back_trace::print(options);
    EXPECT_FALSE(trace.empty());

    // Detailed format should contain "frame #" markers
    EXPECT_NE(trace.find("frame #"), std::string::npos);
    END_TEST();
}

QUARISMATEST(BackTrace, detailed_format_without_addresses)
{
    backtrace_options options;
    options.frames_to_skip           = 0;
    options.maximum_number_of_frames = 5;
    options.include_addresses        = false;
    options.include_offsets          = false;

    auto trace = back_trace::print(options);
    EXPECT_FALSE(trace.empty());
    END_TEST();
}

// ============================================================================
// Call Stack Depth Tests
// ============================================================================

QUARISMATEST(BackTrace, deep_call_stack)
{
    level_1();
    END_TEST();
}

// ============================================================================
// Platform Support Tests
// ============================================================================

QUARISMATEST(BackTrace, is_supported)
{
    bool supported = back_trace::is_supported();

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    // Should be supported on major platforms
    EXPECT_TRUE(supported);
#endif
    END_TEST();
}

// ============================================================================
// Edge Cases
// ============================================================================

QUARISMATEST(BackTrace, empty_frames_format)
{
    std::vector<stack_frame> empty_frames;
    auto                     formatted = back_trace::format(empty_frames);

    EXPECT_FALSE(formatted.empty());
    EXPECT_NE(formatted.find("No stack trace available"), std::string::npos);
    END_TEST();
}

QUARISMATEST(BackTrace, zero_max_frames)
{
    backtrace_options options;
    options.maximum_number_of_frames = 0;

    auto frames = back_trace::capture(options);
    // Should return empty or minimal frames - exact behavior is implementation dependent
    END_TEST();
}

// ============================================================================
// Configuration Tests
// ============================================================================

QUARISMATEST(BackTrace, set_stack_trace_on_error)
{
    // Test the configuration method (currently a no-op placeholder)
    back_trace::set_stack_trace_on_error(1);
    back_trace::set_stack_trace_on_error(0);
    // No assertions needed as this is currently a placeholder
    END_TEST();
}

// ============================================================================
// Integration Tests
// ============================================================================

QUARISMATEST(BackTrace, usage_in_logging)
{
    QUARISMA_LOG_INFO("Error occurred at:\n{}", back_trace::print(0, 5));
    END_TEST();
}

QUARISMATEST(BackTrace, usage_in_compact_logging)
{
    // Test that compact backtrace can be used in logging
    QUARISMA_LOG_INFO("Call chain: {}", back_trace::compact(5));
    END_TEST();
}
