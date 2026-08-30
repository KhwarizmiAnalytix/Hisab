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

#include "graph_builder.h"

#include <queue>

namespace graph
{

node_id graph_builder::add_node(std::string name, node_work work)
{
    const node_id id = nodes_.size();
    nodes_.push_back(pending_node{std::move(name), std::move(work)});
    return id;
}

graph_builder& graph_builder::depends_on(node_id node, node_id dependency)
{
    edges_.push_back(pending_edge{node, dependency});
    return *this;
}

graph_status graph_builder::build(std::shared_ptr<dependency_graph>& out) const
{
    const std::size_t count = nodes_.size();

    std::vector<std::vector<node_id>> successors(count);
    std::vector<std::vector<node_id>> dependencies(count);
    std::vector<std::size_t>          in_degree(count, 0);
    for (const auto& edge : edges_)
    {
        if (edge.node_ >= count || edge.dependency_ >= count)
        {
            return graph_status::failure(
                graph_build_status_code::unknown_node_reference,
                "depends_on() referenced a node id outside the graph");
        }
        successors[edge.dependency_].push_back(edge.node_);
        // edges_ preserves depends_on() call order, so dependencies[node] does
        // too -- this is what node_work's documented input ordering relies on
        // (graph_types.h's node_work doc comment).
        dependencies[edge.node_].push_back(edge.dependency_);
        ++in_degree[edge.node_];
    }

    std::queue<node_id> ready;
    for (node_id id = 0; id < count; ++id)
    {
        if (in_degree[id] == 0)
        {
            ready.push(id);
        }
    }

    std::vector<node_id> topological_order;
    topological_order.reserve(count);
    while (!ready.empty())
    {
        const node_id id = ready.front();
        ready.pop();
        topological_order.push_back(id);

        for (const node_id successor : successors[id])
        {
            if (--in_degree[successor] == 0)
            {
                ready.push(successor);
            }
        }
    }

    if (topological_order.size() != count)
    {
        return graph_status::failure(
            graph_build_status_code::cycle_detected,
            "graph_builder::build(): dependency edges contain a cycle");
    }

    std::vector<dependency_graph::node> built_nodes;
    built_nodes.reserve(count);
    for (node_id id = 0; id < count; ++id)
    {
        built_nodes.push_back(dependency_graph::node{
            nodes_[id].name_,
            nodes_[id].work_,
            std::move(successors[id]),
            std::move(dependencies[id])});
    }

    out = std::shared_ptr<dependency_graph>(
        new dependency_graph(std::move(built_nodes), std::move(topological_order)));
    return graph_status::success();
}

}  // namespace graph
