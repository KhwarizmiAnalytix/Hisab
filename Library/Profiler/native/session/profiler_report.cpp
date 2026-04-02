/*
 * Quarisma: High-Performance Quantitative Library
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

#include "profiler_report.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

//#include "logging/logger.h"
#include "native/analysis/statistical_analyzer.h"
#include "native/session/profiler.h"

namespace quarisma
{
namespace
{
struct scope_snapshot
{
    const profiler_scope_data* scope;
    size_t                     depth;
};

std::string to_string_thread_id(const std::thread::id& thread_id)
{
    std::stringstream ss;
    ss << thread_id;
    return ss.str();
}

void collect_scope_snapshots(
    const profiler_scope_data* scope, size_t depth, std::vector<scope_snapshot>& snapshots)
{
    if (scope == nullptr)
    {
        return;
    }

    snapshots.push_back({scope, depth});
    for (const auto& child : scope->children_)
    {
        collect_scope_snapshots(child.get(), depth + 1, snapshots);
    }
}

std::vector<scope_snapshot> collect_scope_snapshots(const profiler_scope_data* root)
{
    std::vector<scope_snapshot> snapshots;
    collect_scope_snapshots(root, 0, snapshots);
    return snapshots;
}

size_t compute_max_depth(const std::vector<scope_snapshot>& snapshots)
{
    // Use std::accumulate to find maximum depth
    return std::accumulate(
        snapshots.begin(),
        snapshots.end(),
        size_t{0},
        [](size_t max_depth, const scope_snapshot& snapshot)
        { return (std::max)(max_depth, snapshot.depth); });
}

std::unordered_map<std::string, size_t> build_thread_histogram(
    const std::vector<scope_snapshot>& snapshots)
{
    std::unordered_map<std::string, size_t> histogram;
    for (const auto& snapshot : snapshots)
    {
        std::string const thread_id = to_string_thread_id(snapshot.scope->thread_id_);
        ++histogram[thread_id];
    }
    return histogram;
}

template <typename ValueT>
std::vector<std::pair<std::string, ValueT>> sort_map_by_value_desc(
    const std::unordered_map<std::string, ValueT>& input)
{
    std::vector<std::pair<std::string, ValueT>> entries(input.begin(), input.end());
    std::sort(
        entries.begin(),
        entries.end(),
        [](const auto& lhs, const auto& rhs)
        {
            if (lhs.second == rhs.second)
            {
                return lhs.first < rhs.first;
            }
            return lhs.second > rhs.second;
        });
    return entries;
}

}  // namespace

//=============================================================================
// profiler_report Implementation
//=============================================================================

profiler_report::profiler_report(const quarisma::profiler_session& session) : session_(session) {}

std::string profiler_report::generate_console_report() const
{
    std::stringstream ss;

    ss << generate_header_section();
    ss << generate_summary_section();

    if (include_hierarchical_data_)
    {
        ss << generate_hierarchical_section();
    }

    ss << generate_timing_section();
    ss << generate_memory_section();
    ss << generate_statistical_section();

    if (include_thread_info_)
    {
        ss << generate_thread_section();
    }

    return ss.str();
}

std::string profiler_report::generate_json_report() const
{
    auto const* root = session_.get_root_scope();
    auto const  snapshots =
        root != nullptr ? collect_scope_snapshots(root) : std::vector<scope_snapshot>();

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"header\": {\n";
    ss << "    \"active\": " << (session_.is_active() ? "true" : "false") << ",\n";
    ss << "    \"scope_count\": " << snapshots.size() << ",\n";
    ss << "    \"max_depth\": " << compute_max_depth(snapshots) << ",\n";

    auto const start_time = session_.session_start_time();
    auto const end_time   = session_.session_end_time();
    if (end_time > start_time)
    {
        auto const duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        ss << "    \"duration_ms\": " << format_double(duration_ns / 1'000'000.0) << "\n";
    }
    else
    {
        ss << "    \"duration_ms\": 0.0\n";
    }
    ss << "  },\n";

    if (include_hierarchical_data_ && (root != nullptr))
    {
        ss << "  \"scopes\": [\n";
        process_scope_data_json_recursive(*root, ss, 4);
        ss << "\n  ],\n";
    }
    else
    {
        ss << "  \"scopes\": [],\n";
    }

    std::vector<scope_snapshot> by_duration = snapshots;
    std::sort(
        by_duration.begin(),
        by_duration.end(),
        [](const auto& lhs, const auto& rhs)
        { return lhs.scope->get_duration_ms() > rhs.scope->get_duration_ms(); });

    ss << "  \"top_durations\": [\n";
    size_t const duration_limit =
        (std::min)(static_cast<size_t>(10), by_duration.size());  // top 10 entries
    for (size_t i = 0; i < duration_limit; ++i)
    {
        const auto& snapshot = by_duration[i];
        const auto* scope    = snapshot.scope;
        ss << "    {\n";
        ss << "      \"name\": " << escape_json_string(scope->name_) << ",\n";
        ss << "      \"duration_ms\": " << format_double(scope->get_duration_ms()) << ",\n";
        ss << "      \"depth\": " << snapshot.depth << ",\n";
        ss << "      \"thread\": " << escape_json_string(format_thread_id(scope->thread_id_))
           << "\n";
        ss << "    }";
        if (i + 1 < duration_limit)
        {
            ss << ",\n";
        }
    }
    if (duration_limit > 0)
    {
        ss << "\n";
    }
    ss << "  ],\n";

    ss << "  \"memory\": {\n";
    if (auto const* tracker = session_.memory_tracker_ptr())
    {
        auto const stats = tracker->get_current_stats();
        ss << "    \"current_bytes\": " << stats.current_usage_ << ",\n";
        ss << "    \"peak_bytes\": " << stats.peak_usage_ << ",\n";
        ss << "    \"total_allocated_bytes\": " << stats.total_allocated_ << ",\n";
        ss << "    \"total_deallocated_bytes\": " << stats.total_deallocated_ << "\n";
    }
    else
    {
        ss << "    \"enabled\": false\n";
    }
    ss << "  },\n";

    ss << "  \"threads\": [\n";
    auto const thread_histogram = sort_map_by_value_desc(build_thread_histogram(snapshots));
    for (size_t i = 0; i < thread_histogram.size(); ++i)
    {
        if (i != 0)
        {
            ss << ",\n";
        }
        ss << "    {\n";
        ss << "      \"thread\": " << escape_json_string(thread_histogram[i].first) << ",\n";
        ss << "      \"scope_count\": " << thread_histogram[i].second << "\n";
        ss << "    }";
    }
    if (!thread_histogram.empty())
    {
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}

std::string profiler_report::generate_csv_report() const
{
    std::stringstream ss;

    ss << generate_csv_header() << "\n";
    if (include_hierarchical_data_ && (session_.get_root_scope() != nullptr))
    {
        std::vector<std::string> rows;
        process_scope_data_csv_recursive(*session_.get_root_scope(), rows);
        for (const auto& row : rows)
        {
            ss << row << "\n";
        }
    }

    return ss.str();
}

std::string profiler_report::generate_xml_report() const
{
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<profiler_report>\n";

    ss << "  <header>\n" << generate_header_section() << "  </header>\n";
    ss << "  <summary>\n" << generate_summary_section() << "  </summary>\n";

    if (include_hierarchical_data_)
    {
        ss << "  <hierarchical_data>\n"
           << generate_hierarchical_section() << "  </hierarchical_data>\n";
    }

    ss << "  <timing>\n" << generate_timing_section() << "  </timing>\n";
    ss << "  <memory>\n" << generate_memory_section() << "  </memory>\n";
    ss << "  <statistics>\n" << generate_statistical_section() << "  </statistics>\n";

    if (include_thread_info_)
    {
        ss << "  <threads>\n" << generate_thread_section() << "  </threads>\n";
    }

    ss << "</profiler_report>\n";

    return ss.str();
}

bool profiler_report::export_to_file(
    const std::string& filename, quarisma::profiler_options::output_format_enum format) const
{
    std::string content;

    switch (format)
    {
    case quarisma::profiler_options::output_format_enum::CONSOLE:
    case quarisma::profiler_options::output_format_enum::FILE:
        content = generate_console_report();
        break;
    case quarisma::profiler_options::output_format_enum::JSON:
        content = generate_json_report();
        break;
    case quarisma::profiler_options::output_format_enum::CSV:
        content = generate_csv_report();
        break;
    case quarisma::profiler_options::output_format_enum::STRUCTURED:
        content = generate_xml_report();
        break;
    default:
        content = generate_console_report();
        break;
    }

    std::ofstream file(filename);
    if (!file.is_open())
    {
        return false;
    }

    file << content;
    file.close();
    return true;
}

bool profiler_report::export_console_report(const std::string& filename) const
{
    return export_to_file(filename, quarisma::profiler_options::output_format_enum::CONSOLE);
}

bool profiler_report::export_json_report(const std::string& filename) const
{
    return export_to_file(filename, quarisma::profiler_options::output_format_enum::JSON);
}

bool profiler_report::export_csv_report(const std::string& filename) const
{
    return export_to_file(filename, quarisma::profiler_options::output_format_enum::CSV);
}

bool profiler_report::export_xml_report(const std::string& filename) const
{
    return export_to_file(filename, quarisma::profiler_options::output_format_enum::STRUCTURED);
}

void profiler_report::print_summary()
{
    /* PROFILER_LOG_WARNING(
        "profiler_report::print_summary() requires a report instance. "
        "Create a profiler_session report via profiler_session::generate_report()."); */
}

void profiler_report::print_detailed_report() const
{
    //PROFILER_LOG_INFO("{}", generate_console_report());
}

void profiler_report::print_memory_report()
{
    /* PROFILER_LOG_WARNING(
        "profiler_report::print_memory_report() is deprecated. "
        "Use profiler_session::generate_report()->generate_memory_section()."); */
}

void profiler_report::print_timing_report()
{
    /* PROFILER_LOG_WARNING(
        "profiler_report::print_timing_report() is deprecated. "
        "Use profiler_session::generate_report()->generate_timing_section()."); */
}

void profiler_report::print_statistical_report()
{
    /* PROFILER_LOG_WARNING(
        "profiler_report::print_statistical_report() is deprecated. "
        "Use profiler_session::generate_report()->generate_statistical_section()."); */
}

std::string profiler_report::format_duration(double duration_ns) const
{
    if (time_unit_ == "ms")
    {
        return format_double(duration_ns / 1'000'000.0) + " ms";
    }
    if (time_unit_ == "us")
    {
        return format_double(duration_ns / 1'000.0) + " us";
    }
    return format_double(duration_ns) + " ns";
}

std::string profiler_report::format_memory_size(size_t bytes) const
{
    if (memory_unit_ == "MB")
    {
        return format_double(bytes / (1024.0 * 1024.0)) + " MB";
    }
    if (memory_unit_ == "KB")
    {
        return format_double(bytes / 1024.0) + " KB";
    }
    return format_double(static_cast<double>(bytes)) + " bytes";
}

std::string profiler_report::format_memory_delta(int64_t bytes) const
{
    if (bytes == 0)
    {
        return "0";
    }
    std::string const sign = (bytes > 0) ? "+" : "-";
    return sign + format_memory_size(static_cast<size_t>(std::abs(bytes)));
}

std::string profiler_report::format_percentage(double value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << value * 100.0 << "%";
    return ss.str();
}

std::string profiler_report::format_thread_id(const std::thread::id& thread_id)
{
    std::stringstream ss;
    ss << thread_id;
    return ss.str();
}

std::string profiler_report::format_double(double value) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision_) << value;
    return ss.str();
}

std::string profiler_report::generate_header_section() const
{
    std::stringstream ss;
    ss << "=== Quarisma Profiler Report ===\n";
    ss << "Session active: " << (session_.is_active() ? "yes" : "no") << "\n";

    auto const start_time = session_.session_start_time();
    auto const end_time   = session_.session_end_time();
    if (end_time > start_time)
    {
        auto const duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        ss << "Duration: " << format_duration(static_cast<double>(duration_ns)) << "\n";
    }
    else
    {
        ss << "Duration: n/a\n";
    }

    auto const* root = session_.get_root_scope();
    auto const  snapshots =
        root != nullptr ? collect_scope_snapshots(root) : std::vector<scope_snapshot>();
    ss << "Total scopes: " << snapshots.size() << "\n";
    ss << "Max depth: " << compute_max_depth(snapshots) << "\n";
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_summary_section() const
{
    std::stringstream ss;
    ss << "=== Summary ===\n";
    auto const* root = session_.get_root_scope();
    if (root == nullptr)
    {
        ss << "No profiling scopes were recorded.\n\n";
        return ss.str();
    }

    ss << "Root scope: " << root->name_ << "\n";
    ss << "Total duration: " << format_double(root->get_duration_ms()) << " ms\n";
    ss << "Root thread: " << format_thread_id(root->thread_id_) << "\n";

    if (auto const* tracker = session_.memory_tracker_ptr())
    {
        auto const stats = tracker->get_current_stats();
        ss << "Current memory: " << format_memory_size(stats.current_usage_) << "\n";
        ss << "Peak memory: " << format_memory_size(stats.peak_usage_) << "\n";
        ss << "Total allocated: " << format_memory_size(stats.total_allocated_) << "\n";
        ss << "Total deallocated: " << format_memory_size(stats.total_deallocated_) << "\n";
    }
    else
    {
        ss << "Memory tracking disabled for this session.\n";
    }
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_timing_section() const
{
    std::stringstream ss;
    ss << "=== Timing Analysis ===\n";
    auto const* root = session_.get_root_scope();
    if (root == nullptr)
    {
        ss << "No timing data available.\n\n";
        return ss.str();
    }

    auto snapshots = collect_scope_snapshots(root);
    std::sort(
        snapshots.begin(),
        snapshots.end(),
        [](const auto& lhs, const auto& rhs)
        { return lhs.scope->get_duration_ms() > rhs.scope->get_duration_ms(); });

    size_t rank = 1;
    for (const auto& snapshot : snapshots)
    {
        const auto* scope = snapshot.scope;
        ss << "#" << rank++ << " ";
        ss << scope->name_ << " - " << format_double(scope->get_duration_ms()) << " ms"
           << " (depth " << snapshot.depth << ", thread " << format_thread_id(scope->thread_id_)
           << ")\n";

        if (rank > 10)
        {
            break;
        }
    }
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_memory_section() const
{
    std::stringstream ss;
    ss << "=== Memory Analysis ===\n";
    auto const* root = session_.get_root_scope();
    if (root == nullptr)
    {
        ss << "No memory data captured.\n\n";
        return ss.str();
    }

    std::vector<scope_snapshot> snapshots = collect_scope_snapshots(root);
    std::sort(
        snapshots.begin(),
        snapshots.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return std::abs(lhs.scope->memory_stats_.delta_since_start_) >
                   std::abs(rhs.scope->memory_stats_.delta_since_start_);
        });

    size_t displayed = 0;
    for (const auto& snapshot : snapshots)
    {
        const auto*   scope = snapshot.scope;
        int64_t const delta = scope->memory_stats_.delta_since_start_;
        if (delta == 0)
        {
            continue;
        }
        ss << scope->name_ << " (depth " << snapshot.depth << "): delta "
           << format_memory_delta(delta) << ", current "
           << format_memory_size(scope->memory_stats_.current_usage_) << ", peak "
           << format_memory_size(scope->memory_stats_.peak_usage_) << "\n";
        if (++displayed >= 10)
        {
            break;
        }
    }
    if (displayed == 0)
    {
        ss << "No significant memory deltas observed.\n";
    }
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_hierarchical_section() const
{
    std::stringstream ss;
    ss << "=== Hierarchical Analysis ===\n";
    auto const* root = session_.get_root_scope();
    if (root == nullptr)
    {
        ss << "No scope hierarchy available.\n\n";
        return ss.str();
    }

    process_scope_data_recursive(*root, ss, 0);
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_statistical_section() const
{
    std::stringstream ss;
    ss << "=== Statistical Analysis ===\n";
    auto const* analyzer = session_.statistical_analyzer_ptr();
    if (analyzer == nullptr)
    {
        ss << "Statistical analysis disabled for this session.\n\n";
        return ss.str();
    }

    auto const timing_metrics = analyzer->calculate_all_timing_stats();
    if (timing_metrics.empty())
    {
        ss << "No timing statistics recorded.\n\n";
        return ss.str();
    }

    size_t count = 0;
    for (const auto& entry : timing_metrics)
    {
        auto const& metrics = entry.second;
        if (!metrics.is_valid())
        {
            continue;
        }
        ss << entry.first << ": mean " << format_double(metrics.mean) << " ms, ";
        ss << "std-dev " << format_double(metrics.std_deviation) << " ms, ";
        ss << "count " << metrics.count << "\n";
        if (++count >= 10)
        {
            break;
        }
    }
    ss << "\n";
    return ss.str();
}

std::string profiler_report::generate_thread_section() const
{
    std::stringstream ss;
    ss << "=== Thread Analysis ===\n";
    auto const* root = session_.get_root_scope();
    if (root == nullptr)
    {
        ss << "No thread information available.\n\n";
        return ss.str();
    }

    auto const snapshots = collect_scope_snapshots(root);
    auto const histogram = sort_map_by_value_desc(build_thread_histogram(snapshots));
    size_t     rank      = 1;
    for (const auto& entry : histogram)
    {
        ss << "#" << rank++ << " " << entry.first << ": " << entry.second << " scope(s)\n";
        if (rank > 10)
        {
            break;
        }
    }
    if (histogram.empty())
    {
        ss << "No scopes were recorded per-thread.\n";
    }
    ss << "\n";
    return ss.str();
}

std::string profiler_report::escape_json_string(const std::string& str)
{
    std::ostringstream ss;
    ss << '"';
    for (char const c : str)
    {
        switch (c)
        {
        case '"':
        case '\\':
            ss << '\\' << c;
            break;
        case '\b':
            ss << "\\b";
            break;
        case '\f':
            ss << "\\f";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                ss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                   << static_cast<int>(static_cast<unsigned char>(c)) << std::dec;
            }
            else
            {
                ss << c;
            }
        }
    }
    ss << '"';
    return ss.str();
}

std::string profiler_report::escape_csv_field(const std::string& field)
{
    bool const requires_quotes = field.find_first_of(",\"\n") != std::string::npos;
    if (!requires_quotes)
    {
        return field;
    }
    std::string escaped = "\"";
    for (char const c : field)
    {
        if (c == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += c;
        }
    }
    escaped += '"';
    return escaped;
}

std::string profiler_report::generate_csv_header()
{
    return "Scope,Depth,Thread,Duration(ms),Memory Current,Memory Peak,Memory Delta";
}

std::string profiler_report::generate_csv_row(const std::vector<std::string>& fields)
{
    std::string result;
    for (size_t i = 0; i < fields.size(); ++i)
    {
        if (i != 0)
        {
            result += ",";
        }
        result += escape_csv_field(fields[i]);
    }
    return result;
}

std::string profiler_report::escape_xml_string(const std::string& str)
{
    std::string result;
    result.reserve(str.size());
    for (char const c : str)
    {
        switch (c)
        {
        case '&':
            result += "&amp;";
            break;
        case '<':
            result += "&lt;";
            break;
        case '>':
            result += "&gt;";
            break;
        case '"':
            result += "&quot;";
            break;
        case '\'':
            result += "&apos;";
            break;
        default:
            result += c;
        }
    }
    return result;
}

std::string profiler_report::generate_xml_element(
    const std::string& tag, const std::string& content)
{
    return "<" + tag + ">" + content + "</" + tag + ">";
}

std::string profiler_report::generate_xml_attribute(
    const std::string& name, const std::string& value)
{
    return name + "=\"" + escape_xml_string(value) + "\"";
}

void profiler_report::process_scope_data_recursive(
    const profiler_scope_data& scope, std::stringstream& ss, int indent) const
{
    std::string const prefix(static_cast<size_t>(indent) * 2, ' ');
    ss << prefix << "- " << scope.name_ << " | duration " << format_double(scope.get_duration_ms())
       << " ms" << " | thread " << format_thread_id(scope.thread_id_) << " | memory "
       << format_memory_delta(scope.memory_stats_.delta_since_start_) << "\n";

    for (const auto& child : scope.children_)
    {
        process_scope_data_recursive(*child, ss, indent + 1);
    }
}

void profiler_report::process_scope_data_json_recursive(
    const profiler_scope_data& scope, std::stringstream& ss, int indent) const
{
    std::string const indent_str(static_cast<size_t>(indent), ' ');
    ss << indent_str << "{\n";
    ss << indent_str << "  \"name\": " << escape_json_string(scope.name_) << ",\n";
    ss << indent_str << "  \"duration_ms\": " << format_double(scope.get_duration_ms()) << ",\n";
    ss << indent_str << "  \"thread\": " << escape_json_string(format_thread_id(scope.thread_id_))
       << ",\n";
    ss << indent_str << "  \"memory\": {\n";
    ss << indent_str << "    \"current_bytes\": " << scope.memory_stats_.current_usage_ << ",\n";
    ss << indent_str << "    \"peak_bytes\": " << scope.memory_stats_.peak_usage_ << ",\n";
    ss << indent_str << "    \"delta_bytes\": " << scope.memory_stats_.delta_since_start_ << "\n";
    ss << indent_str << "  }";

    if (!scope.children_.empty())
    {
        ss << ",\n" << indent_str << "  \"children\": [\n";
        for (size_t i = 0; i < scope.children_.size(); ++i)
        {
            process_scope_data_json_recursive(*scope.children_[i], ss, indent + 4);
            if (i + 1 < scope.children_.size())
            {
                ss << ",\n";
            }
        }
        ss << "\n" << indent_str << "  ]\n" << indent_str << "}";
    }
    else
    {
        ss << "\n" << indent_str << "}";
    }
}

void profiler_report::process_scope_data_csv_recursive(
    const profiler_scope_data& scope, std::vector<std::string>& rows, int depth) const
{
    rows.push_back(generate_csv_row({
        std::string(static_cast<size_t>(depth) * 2, ' ') + scope.name_,
        std::to_string(depth),
        format_thread_id(scope.thread_id_),
        format_double(scope.get_duration_ms()),
        format_memory_size(scope.memory_stats_.current_usage_),
        format_memory_size(scope.memory_stats_.peak_usage_),
        format_memory_delta(scope.memory_stats_.delta_since_start_),
    }));

    for (const auto& child : scope.children_)
    {
        process_scope_data_csv_recursive(*child, rows, depth + 1);
    }
}

//=============================================================================
// profiler_report_builder Implementation
//=============================================================================

profiler_report_builder::profiler_report_builder(const quarisma::profiler_session& session)
    : session_(session)
{
}

std::unique_ptr<quarisma::profiler_report> profiler_report_builder::build() const
{
    auto report = std::make_unique<quarisma::profiler_report>(session_);
    report->set_precision(precision_);
    report->set_time_unit(time_unit_);
    report->set_memory_unit(memory_unit_);
    report->set_include_thread_info(include_thread_info_);
    report->set_include_hierarchical_data(include_hierarchical_data_);
    return report;
}

}  // namespace quarisma
