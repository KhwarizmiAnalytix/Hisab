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

#pragma once

#include <any>
#include <unordered_map>

#include "common/graph_export.h"
#include "dependency_graph.h"
#include "graph_types.h"

namespace graph
{

/// Configuration for graph_executor. Default matches
/// threaded_callback_queue's own default of one worker thread
/// (Library/Parallel/tools/threaded_callback_queue.cpp), so a
/// default-constructed graph_executor's behavior is unchanged.
struct graph_executor_options
{
    int number_of_threads = 1;
};

/**
 * @brief Runs a graph's nodes in parallel, respecting dependency order.
 *
 * Built on Library/Parallel's threaded_callback_queue::push_dependent(),
 * the same continuation-chaining primitive
 * Library/Parallel/Testing/Cxx/TestThreadedCallbackQueue.cpp already
 * exercises, rather than a bespoke thread pool / ready-queue. This mirrors
 * the dependency-count-driven, worker-pulls-when-ready shape of PyTorch's
 * autograd engine (torch/csrc/autograd/engine.cpp), minus the manual
 * atomic-decrement bookkeeping push_dependent already provides. Actual
 * concurrency is controlled by graph_executor_options::number_of_threads.
 *
 * A node's work_ may throw. graph_executor catches it (instead of letting
 * it escape a worker thread, which would otherwise call std::terminate()),
 * records the first such failure, and skips every node that depends,
 * directly or transitively, on the failed node -- see run()'s status
 * return.
 */
class GRAPH_VISIBILITY graph_executor
{
public:
    GRAPH_API explicit graph_executor(graph_executor_options options = {});

    /// Runs every node of `g` to completion. `out_results` receives one
    /// entry per node (a default-constructed, empty std::any for any node
    /// skipped due to an upstream failure). Blocks until the whole graph
    /// has finished.
    GRAPH_API graph_execution_status
    run(const dependency_graph& g, std::unordered_map<node_id, std::any>& out_results);

private:
    graph_executor_options options_;
};

}  // namespace graph
