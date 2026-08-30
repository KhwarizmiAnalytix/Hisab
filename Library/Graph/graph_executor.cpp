/*
 * XSigma: High-Performance Computational Library
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

#include "graph_executor.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <utility>

#include "tools/threaded_callback_queue.h"

namespace graph
{

graph_executor::graph_executor(graph_executor_options options) : options_(options) {}

graph_execution_status graph_executor::run(
    const dependency_graph& g, std::unordered_map<node_id, std::any>& out_results)
{
    const std::size_t count = g.node_count();

    threaded_callback_queue queue;
    queue.set_number_of_threads(options_.number_of_threads);
    using future_ptr = threaded_callback_queue::shared_future_pointer<std::any>;

    // Sized once and only ever written via operator[] below (never resized),
    // so distinct elements can be safely written by this thread and read by
    // worker threads without a data race -- see the class doc comment in
    // graph_executor.h for the happens-before argument (a node's task only
    // becomes eligible to run once push_dependent's contract guarantees its
    // dependencies' futures are READY, which happens strictly after this
    // thread already stored those dependencies' entries here, since nodes
    // are scheduled in topological order).
    std::vector<future_ptr>        futures(count);
    std::vector<std::atomic<bool>> node_failed(count);

    // First-failure-wins: exactly one worker's compare_exchange_strong can
    // flip failure_recorded, so only that thread ever writes failed_node_id_
    // / failed_message_ -- the mutex is for visibility on the calling
    // thread's read after wait(), not for mutual exclusion between workers.
    std::atomic<bool> failure_recorded{false};
    std::mutex        failure_mutex;
    node_id           failed_node_id_ = 0;
    std::string       failed_message_;

    auto record_failure = [&](node_id id, std::string message)
    {
        bool expected = false;
        if (failure_recorded.compare_exchange_strong(expected, true))
        {
            const std::lock_guard<std::mutex> lock(failure_mutex);
            failed_node_id_ = id;
            failed_message_ = std::move(message);
        }
    };

    for (const node_id id : g.topological_order())
    {
        const std::vector<node_id>& deps = g.node_at(id).dependencies_;

        auto wrapped = [&g, &futures, &node_failed, &record_failure, id]() -> std::any
        {
            const std::vector<node_id>& node_deps = g.node_at(id).dependencies_;

            bool upstream_failed = false;
            for (const node_id dep : node_deps)
            {
                if (node_failed[dep].load(std::memory_order_acquire))
                {
                    upstream_failed = true;
                    break;
                }
            }
            if (upstream_failed)
            {
                node_failed[id].store(true, std::memory_order_release);
                return std::any{};
            }

            std::vector<std::any> inputs;
            inputs.reserve(node_deps.size());
            for (const node_id dep : node_deps)
            {
                // Copied, not moved: a fan-out dependency's result is read
                // once per consumer here, so it must survive for siblings.
                inputs.push_back(futures[dep]->get());
            }

            try
            {
                return g.node_at(id).work_(inputs);
            }
            catch (const std::exception& e)
            {
                node_failed[id].store(true, std::memory_order_release);
                record_failure(id, e.what());
                return std::any{};
            }
            catch (...)
            {
                node_failed[id].store(true, std::memory_order_release);
                record_failure(id, "unknown exception in node \"" + g.node_at(id).name_ + "\"");
                return std::any{};
            }
        };

        std::vector<future_ptr> dep_futures;
        dep_futures.reserve(deps.size());
        for (const node_id dep : deps)
        {
            dep_futures.push_back(futures[dep]);
        }
        // push_dependent() falls back to an ordinary push() internally when
        // dep_futures is empty (threaded_callback_queue.h's must_wait()),
        // so no separate empty-dependency branch is needed here.
        futures[id] = queue.push_dependent(dep_futures, wrapped);
    }

    queue.wait(futures);

    out_results.clear();
    out_results.reserve(count);
    for (node_id id = 0; id < count; ++id)
    {
        // Read exactly once per node here (unlike the fan-out reads above),
        // so moving the stored value out is safe.
        out_results.emplace(id, std::move(futures[id]->get()));
    }

    if (failure_recorded.load())
    {
        const std::lock_guard<std::mutex> lock(failure_mutex);
        return graph_execution_status::failure(failed_node_id_, failed_message_);
    }
    return graph_execution_status::success();
}

}  // namespace graph
