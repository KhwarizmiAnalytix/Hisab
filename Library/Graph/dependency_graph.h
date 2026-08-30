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

#include <string>
#include <vector>

#include "common/graph_export.h"
#include "graph_types.h"

namespace graph
{

class graph_builder;

/**
 * @brief Immutable dependency DAG, produced by graph_builder::build().
 *
 * Holds each node's work callable, its outgoing (successors_) and incoming
 * (dependencies_) edges, and a topological order computed once at build
 * time (Kahn's algorithm) so graph_executor never has to recompute it, the
 * cycle check, or the dependency inversion on every run().
 */
class GRAPH_VISIBILITY dependency_graph
{
public:
    struct node
    {
        std::string name_;
        node_work   work_;

        /// Nodes that depend on this one (used to drive Kahn's algorithm).
        std::vector<node_id> successors_;

        /// This node's own dependencies, in the order graph_builder::depends_on
        /// declared them -- node_work receives their results in this same
        /// order.
        std::vector<node_id> dependencies_;
    };

    std::size_t node_count() const { return nodes_.size(); }

    const node& node_at(node_id id) const { return nodes_[id]; }

    /// Nodes in an order where every dependency appears before its dependents.
    const std::vector<node_id>& topological_order() const { return topological_order_; }

private:
    friend class graph_builder;

    dependency_graph(std::vector<node> nodes, std::vector<node_id> topological_order)
        : nodes_(std::move(nodes)), topological_order_(std::move(topological_order))
    {
    }

    std::vector<node>    nodes_;
    std::vector<node_id> topological_order_;
};

}  // namespace graph
