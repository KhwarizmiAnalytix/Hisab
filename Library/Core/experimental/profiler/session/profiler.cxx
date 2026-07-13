/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

/**
 * @file profiler.cxx
 * @brief Implementation of the enhanced profiler system for XSigma applications
 *
 * Provides high-performance, thread-safe profiling capabilities with comprehensive
 * timing, memory tracking, and statistical analysis features.
 *
 * @author XSigma Development Team
 * @version 1.0
 * @date 2024
 */

#include "profiler.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "experimental/profiler/analysis/statistical_analyzer.h"
#include "experimental/profiler/memory/memory_tracker.h"
#include "experimental/profiler/session/profiler_report.h"

// Prevent Windows min/max macros from interfering with std::numeric_limits
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>
#include <limits>

namespace xsigma
{

// Static thread-local storage for current scope (DLL-compatible implementation)
thread_local xsigma::profiler_scope_data* profiler_session::thread_current_scope_ = nullptr;

// Static current session management with atomic operations for thread safety
static std::atomic<xsigma::profiler_session*> g_current_session{nullptr};

//=============================================================================
// timing_stats Implementation
//=============================================================================

void timing_stats::add_sample(double time_ms)
{
    min_time_ = std::min(time_ms, min_time_);
    max_time_ = std::max(time_ms, max_time_);
    total_time_ += time_ms;
    ++sample_count_;
}

void timing_stats::calculate_statistics()
{
    if (sample_count_ == 0)
    {
        return;
    }

    mean_time_ = total_time_ / sample_count_;

    // For standard deviation calculation, we would need to store all samples
    // This is a simplified implementation that could be enhanced with sample storage
    std_deviation_ = 0.0;  // Would need sample storage for proper calculation

    // Initialize percentiles vector if needed (25th, 50th, 75th, 90th, 95th, 99th)
    if (percentiles_.empty())
    {
        percentiles_.resize(6, 0.0);
    }
}

void timing_stats::reset()
{
    min_time_      = (std::numeric_limits<double>::max)();
    max_time_      = 0.0;
    total_time_    = 0.0;
    mean_time_     = 0.0;
    std_deviation_ = 0.0;
    sample_count_  = 0;
    percentiles_.clear();
}

//=============================================================================
// profiler_scope_data Implementation
//=============================================================================

double profiler_scope_data::get_duration_ms() const
{
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_);
    return duration.count() / 1000.0;
}

double profiler_scope_data::get_duration_us() const
{
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time_ - start_time_);
    return static_cast<double>(duration.count());
}

double profiler_scope_data::get_duration_ns() const
{
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_ - start_time_);
    return static_cast<double>(duration.count());
}

//=============================================================================
// profiler_session Implementation
//=============================================================================

profiler_session::profiler_session(xsigma::profiler_options options) : options_(std::move(options))
{
    initialize_components();
}

profiler_session::~profiler_session()
{
    if (active_.load())
    {
        stop();
    }
    cleanup_components();
}

bool profiler_session::start()
{
    if (active_.exchange(true))
    {
        return false;  // Already active
    }

    start_time_ = std::chrono::high_resolution_clock::now();

    // Initialize root scope for hierarchical profiling
    if (options_.enable_hierarchical_profiling_)
    {
        std::scoped_lock const lock(scope_mutex_);
        root_scope_               = std::make_unique<xsigma::profiler_scope_data>();
        root_scope_->name_        = "ROOT";
        root_scope_->start_time_  = start_time_;
        root_scope_->thread_id_   = std::this_thread::get_id();
        root_scope_->depth_level_ = 0;
        current_scope_            = root_scope_.get();
        thread_current_scope_     = current_scope_;
    }

    // Start memory tracking
    if (options_.enable_memory_tracking_ && memory_tracker_)
    {
        memory_tracker_->start_tracking();
    }

    // Start statistical analysis
    if (options_.enable_statistical_analysis_ && statistical_analyzer_)
    {
        statistical_analyzer_->start_analysis();
    }

    set_current_session(this);

    return true;
}

bool profiler_session::stop()
{
    if (!active_.exchange(false))
    {
        return false;  // Not active
    }

    end_time_ = std::chrono::high_resolution_clock::now();

    // Finalize root scope
    if (options_.enable_hierarchical_profiling_ && root_scope_)
    {
        std::scoped_lock const lock(scope_mutex_);
        root_scope_->end_time_ = end_time_;
        thread_current_scope_  = nullptr;
    }

    // Stop memory tracking
    if (options_.enable_memory_tracking_ && memory_tracker_)
    {
        memory_tracker_->stop_tracking();
    }

    // Stop statistical analysis
    if (options_.enable_statistical_analysis_ && statistical_analyzer_)
    {
        statistical_analyzer_->stop_analysis();
    }

    if (current_session() == this)
    {
        set_current_session(nullptr);
    }

    return true;
}

std::unique_ptr<xsigma::profiler_scope> profiler_session::create_scope(const std::string& name)
{
    return std::make_unique<xsigma::profiler_scope>(name, this);
}

std::unique_ptr<xsigma::profiler_report> profiler_session::generate_report() const
{
    return std::make_unique<xsigma::profiler_report>(*this);
}

void profiler_session::export_report(const std::string& filename) const
{
    auto report = generate_report();
    report->export_to_file(filename, options_.output_format_);
}

void profiler_session::print_report() const
{
    auto report = generate_report();
    report->print_detailed_report();
}

profiler_session* profiler_session::current_session()
{
    return g_current_session.load();
}

void profiler_session::set_current_session(xsigma::profiler_session* session)
{
    g_current_session.store(session);
}

void profiler_session::initialize_components()
{
    if (options_.enable_memory_tracking_)
    {
        memory_tracker_ = std::make_unique<xsigma::memory_tracker>();
    }

    if (options_.enable_statistical_analysis_)
    {
        statistical_analyzer_ = std::make_unique<xsigma::statistical_analyzer>();
    }
}

void profiler_session::cleanup_components()
{
    memory_tracker_.reset();  //NOLINT
    memory_tracker_ = nullptr;

    statistical_analyzer_.reset();  //NOLINT

    std::scoped_lock const lock(scope_mutex_);
    root_scope_.reset();  //NOLINT
    current_scope_ = nullptr;
}
void profiler_session::register_scope_start(xsigma::profiler_scope* scope)
{
    if (!options_.enable_hierarchical_profiling_ || !active_.load())
    {
        return;
    }

    std::scoped_lock const lock(scope_mutex_);

    // Find the current scope for this thread
    xsigma::profiler_scope_data* parent_scope = thread_current_scope_;
    if (parent_scope == nullptr)
    {
        parent_scope = root_scope_.get();
    }

    if (parent_scope != nullptr)
    {
        // Add as child to current scope
        auto child_scope          = std::make_unique<xsigma::profiler_scope_data>();
        child_scope->name_        = scope->data().name_;
        child_scope->start_time_  = scope->data().start_time_;
        child_scope->thread_id_   = std::this_thread::get_id();
        child_scope->depth_level_ = parent_scope->depth_level_ + 1;
        child_scope->parent_      = parent_scope;

        xsigma::profiler_scope_data* child_ptr = child_scope.get();
        parent_scope->children_.push_back(std::move(child_scope));

        // Update thread-local current scope
        thread_current_scope_ = child_ptr;
    }
}

void profiler_session::register_scope_end(xsigma::profiler_scope* scope)
{
    if (!options_.enable_hierarchical_profiling_ || !active_.load())
    {
        return;
    }

    std::scoped_lock const lock(scope_mutex_);

    if (thread_current_scope_ != nullptr)
    {
        thread_current_scope_->end_time_     = scope->data().end_time_;
        thread_current_scope_->memory_stats_ = scope->data().memory_stats_;
        thread_current_scope_->timing_stats_ = scope->data().timing_stats_;

        // Move back to parent scope
        thread_current_scope_ = thread_current_scope_->parent_;
    }
}

//=============================================================================
// profiler_scope Implementation
//=============================================================================

profiler_scope::profiler_scope(const std::string& name, xsigma::profiler_session* session)
    : session_((session != nullptr) ? session : xsigma::profiler_session::current_session())
{
    data_             = std::make_unique<xsigma::profiler_scope_data>();
    data_->name_      = name;
    data_->thread_id_ = std::this_thread::get_id();

    // Auto-start if session is active
    if ((session_ != nullptr) && session_->is_active())
    {
        start();
    }
}

profiler_scope::~profiler_scope()
{
    if (started_ && !stopped_)
    {
        stop();
    }
}

void profiler_scope::start()
{
    if (started_)
    {
        return;
    }

    started_           = true;
    data_->start_time_ = std::chrono::high_resolution_clock::now();

    // Register with session for hierarchical tracking
    if (session_ != nullptr)
    {
        session_->register_scope_start(this);

        // Start memory tracking for this scope
        if (session_->options_.enable_memory_tracking_ && session_->memory_tracker_)
        {
            data_->memory_stats_ = session_->memory_tracker_->get_current_stats();
        }
    }
}

void profiler_scope::stop()
{
    if (!started_ || stopped_)
    {
        return;
    }

    stopped_         = true;
    data_->end_time_ = std::chrono::high_resolution_clock::now();

    // Calculate timing statistics
    double const duration_ms = data_->get_duration_ms();
    data_->timing_stats_.add_sample(duration_ms);
    data_->timing_stats_.calculate_statistics();

    // Update memory statistics
    if (session_ != nullptr)
    {
        if (session_->options_.enable_memory_tracking_ && session_->memory_tracker_)
        {
            auto current_stats = session_->memory_tracker_->get_current_stats();
            data_->memory_stats_.delta_since_start_ =
                static_cast<int64_t>(current_stats.current_usage_) -
                static_cast<int64_t>(data_->memory_stats_.current_usage_);
        }

        // Add timing sample to statistical analyzer
        if (session_->options_.enable_statistical_analysis_ && session_->statistical_analyzer_)
        {
            session_->statistical_analyzer_->add_timing_sample(data_->name_, duration_ms);
        }

        // Register scope end with session
        session_->register_scope_end(this);
    }
}

}  // namespace xsigma
