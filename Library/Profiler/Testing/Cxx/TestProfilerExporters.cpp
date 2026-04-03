#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include "baseTest.h"
#include "native/exporters/chrome_trace_exporter.h"
#include "native/exporters/xplane/xplane_builder.h"

using namespace quarisma;
using namespace quarisma::profiler_impl;

// ============================================================================
// Chrome Trace Exporter Tests
// ============================================================================

QUARISMATEST(Profiler, chrome_trace_export_empty_space)
{
    x_space     space;
    std::string json = export_to_chrome_trace_json(space);

    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("traceEvents"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_with_single_plane)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("CPU"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_with_events)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");
    line->set_timestamp_ns(1000000);

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("compute");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("compute"), std::string::npos);
    EXPECT_NE(json.find("\"ph\":\"X\""), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_pretty_print)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    std::string json = export_to_chrome_trace_json(space, true);

    EXPECT_FALSE(json.empty());
    // Pretty printed JSON should have newlines
    EXPECT_NE(json.find("\n"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_json_escaping)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("test\"quote");

    std::string json = export_to_chrome_trace_json(space);

    // Should escape quotes
    EXPECT_NE(json.find("\\\""), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_multiple_threads)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    for (int i = 0; i < 3; ++i)
    {
        auto* line = plane->add_lines();
        line->set_id(100 + i);
        line->set_name("Thread-" + std::to_string(i));
    }

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("Thread-0"), std::string::npos);
    EXPECT_NE(json.find("Thread-1"), std::string::npos);
    EXPECT_NE(json.find("Thread-2"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_with_stats)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    auto* stat = event->add_stats();
    stat->set_value(int64_t(42));

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("compute");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("compute"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_multiple_planes)
{
    x_space space;

    for (int i = 0; i < 2; ++i)
    {
        auto* plane = space.add_planes();
        plane->set_id(i + 1);
        plane->set_name("Plane-" + std::to_string(i));
    }

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("Plane-0"), std::string::npos);
    EXPECT_NE(json.find("Plane-1"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_large_timestamps)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");
    line->set_timestamp_ns(9223372036854775000LL);  // Large timestamp

    auto* event = line->add_events();
    event->set_offset_ps(1000000000000LL);
    event->set_duration_ps(500000000000LL);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("compute");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("compute"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_special_characters_in_names)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread\\with\\backslash");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("func\nwith\nnewlines");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_FALSE(json.empty());
    // Should properly escape special characters
    EXPECT_NE(json.find("\\\\"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_zero_duration_events)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(0);  // Zero duration
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("instant_event");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_NE(json.find("instant_event"), std::string::npos);
    EXPECT_NE(json.find("\"dur\":0"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_many_events)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");

    // Add many events
    for (int i = 0; i < 100; ++i)
    {
        auto* event = line->add_events();
        event->set_offset_ps(i * 1000000);
        event->set_duration_ps(500000);
        event->set_metadata_id(1);
    }

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("event");

    std::string json = export_to_chrome_trace_json(space);

    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("event"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_file_write)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(100);
    line->set_name("Thread-1");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(500000);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("compute");

    // Try to write to invalid path (should fail gracefully)
    bool result = export_to_chrome_trace_json_file(space, "/invalid/path/trace.json");
    EXPECT_FALSE(result);
}

QUARISMATEST(Profiler, chrome_trace_export_display_time_unit)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    std::string json = export_to_chrome_trace_json(space);

    // Should contain displayTimeUnit
    EXPECT_NE(json.find("displayTimeUnit"), std::string::npos);
    EXPECT_NE(json.find("\"ns\""), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_process_metadata)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(42);
    plane->set_name("CustomProcess");

    std::string json = export_to_chrome_trace_json(space);

    // Should contain process metadata
    EXPECT_NE(json.find("process_name"), std::string::npos);
    EXPECT_NE(json.find("CustomProcess"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_thread_metadata)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(1);
    plane->set_name("CPU");

    auto* line = plane->add_lines();
    line->set_id(123);
    line->set_name("CustomThread");

    std::string json = export_to_chrome_trace_json(space);

    // Should contain thread metadata
    EXPECT_NE(json.find("thread_name"), std::string::npos);
    EXPECT_NE(json.find("CustomThread"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_handles_all_stat_types)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(0);  // Force fallback PID generation.
    plane->set_name("Plane\"with\"special/\b\f\n\r\t\\chars");

    auto* line = plane->add_lines();
    line->set_id(0);  // Force fallback TID generation.
    line->set_name("Thread\"Special/\n\t\\");
    line->set_timestamp_ns(10);

    auto* event = line->add_events();
    event->set_offset_ps(3000);    // 3 ns offset.
    event->set_duration_ps(4000);  // 4 ns duration.
    event->set_metadata_id(999);   // Unknown metadata -> "unknown".

    auto* stat_metadata = plane->mutable_stat_metadata();
    (*stat_metadata)[1].set_name("int_stat");
    (*stat_metadata)[2].set_name("uint_stat");
    (*stat_metadata)[3].set_name("double_stat");
    (*stat_metadata)[4].set_name("string_stat");
    (*stat_metadata)[5].set_name("ref_stat");
    (*stat_metadata)[7].set_name("empty_stat");

    auto* stat_int = event->add_stats();
    stat_int->set_metadata_id(1);
    stat_int->set_value(int64_t(-42));

    auto* stat_uint = event->add_stats();
    stat_uint->set_metadata_id(2);
    stat_uint->set_value(uint64_t(7));

    auto* stat_double = event->add_stats();
    stat_double->set_metadata_id(3);
    stat_double->set_value(1.5);

    auto* stat_string = event->add_stats();
    stat_string->set_metadata_id(4);
    stat_string->set_value(
        std::string(
            "quote\" slash/ backslash\\ newline\n tab\t backspace\b formfeed\f carriage\r"));

    auto* stat_ref = event->add_stats();
    stat_ref->set_metadata_id(5);
    stat_ref->set_ref_value(99);

    auto* stat_bytes = event->add_stats();
    stat_bytes->set_metadata_id(6);
    stat_bytes->set_value(std::string_view("bytes"));

    auto* stat_empty = event->add_stats();
    stat_empty->set_metadata_id(7);

    std::string json = export_to_chrome_trace_json(space, true);

    EXPECT_NE(json.find("\"pid\":1"), std::string::npos);
    EXPECT_NE(json.find("\"tid\":1"), std::string::npos);
    EXPECT_NE(json.find("\"ts\":13"), std::string::npos);
    EXPECT_NE(json.find("\"dur\":4"), std::string::npos);

    EXPECT_NE(json.find("\"name\":\"unknown\""), std::string::npos);
    EXPECT_NE(json.find("\"int_stat\":-42"), std::string::npos);
    EXPECT_NE(json.find("\"uint_stat\":7"), std::string::npos);
    EXPECT_NE(json.find("\"double_stat\":1.500000"), std::string::npos);

    // Check for string_stat with proper escaping
    // The string contains: quote" slash/ backslash\ newline\n tab\t backspace\b formfeed\f carriage\r
    // In JSON, this should be escaped as: quote\" slash\/ backslash\\ newline\n tab\t backspace\b formfeed\f carriage\r
    // In the C++ string literal, we need to escape the backslashes again
    EXPECT_NE(json.find("\"string_stat\":\"quote\\\""), std::string::npos);
    EXPECT_NE(json.find("slash\\/"), std::string::npos);
    EXPECT_NE(json.find("backslash\\\\"), std::string::npos);
    EXPECT_NE(json.find("newline\\n"), std::string::npos);
    EXPECT_NE(json.find("tab\\t"), std::string::npos);
    EXPECT_NE(json.find("backspace\\b"), std::string::npos);
    EXPECT_NE(json.find("formfeed\\f"), std::string::npos);
    EXPECT_NE(json.find("carriage\\r"), std::string::npos);

    EXPECT_NE(json.find("\"ref_stat\":99"), std::string::npos);
    EXPECT_NE(json.find("\"stat_6\":null"), std::string::npos);
    // Note: empty_stat is initialized with default double value (0.0), not null
    // The variant defaults to double type when no value is set
    EXPECT_NE(json.find("\"empty_stat\":0.000000"), std::string::npos);
}

QUARISMATEST(Profiler, chrome_trace_export_file_success_path)
{
    x_space space;
    auto*   plane = space.add_planes();
    plane->set_id(2);
    plane->set_name("ExportPlane");

    auto* line = plane->add_lines();
    line->set_id(7);
    line->set_name("ExporterThread");

    auto* event = line->add_events();
    event->set_offset_ps(0);
    event->set_duration_ps(1000);
    event->set_metadata_id(1);

    auto* metadata = plane->mutable_event_metadata();
    (*metadata)[1].set_name("write_event");

    const auto temp_dir  = std::filesystem::temp_directory_path();
    const auto file_path = temp_dir / "quarisma_chrome_trace_test.json";

    std::error_code ec;
    std::filesystem::remove(file_path, ec);

    bool success = export_to_chrome_trace_json_file(space, file_path.string(), true);
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(file_path));

    std::ifstream file(file_path);
    ASSERT_TRUE(file.is_open());
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    EXPECT_NE(content.find("write_event"), std::string::npos);

    file.close();
    std::filesystem::remove(file_path, ec);
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
