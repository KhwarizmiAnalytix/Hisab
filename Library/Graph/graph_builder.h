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

#include <memory>
#include <string>
#include <vector>

#include "common/graph_export.h"
#include "dependency_graph.h"
#include "graph_types.h"

namespace graph
{

/**
 * @brief Incrementally assembles a graph, then validates and freezes it.
 *
 * Unlike the project's typical `with_<field>` config builders (e.g.
 * profiler_session_builder — see root /CLAUDE.md's builder-class
 * convention), graph_builder grows a variable-size node/edge collection
 * rather than configuring a fixed set of fields. `add_node`/`depends_on`
 * are named for what they do rather than forced into `with_<field>`; it
 * still ends in `_builder`, is still fluent where it can be (depends_on
 * returns *this), and still ends with a single `build()` call.
 */
class GRAPH_VISIBILITY graph_builder
{
public:
    /// Registers a node and returns the id used to reference it in depends_on().
    GRAPH_API node_id add_node(std::string name, node_work work);

    /// Declares that `node` must run after `dependency` has completed, and that
    /// `dependency`'s result is passed as one of `node`'s work inputs, in the
    /// order depends_on() was called for `node`.
    GRAPH_API graph_builder& depends_on(node_id node, node_id dependency);

    /**
     * @brief Validates the accumulated nodes/edges and, on success, produces
     * an immutable graph.
     *
     * Runs Kahn's algorithm: repeatedly peels zero-remaining-in-degree nodes
     * into a topological order. A node left over once no more can be peeled
     * means the edges contain a cycle. `out` is left untouched on failure.
     */
    GRAPH_API graph_status build(std::shared_ptr<dependency_graph>& out) const;

private:
    struct pending_node
    {
        std::string name_;
        node_work   work_;
    };

    // (node, dependency) pairs, recorded as-is. node/dependency ids are only
    // validated in build(), so an out-of-range id passed to depends_on() is a
    // graceful build() error instead of undefined behavior at call time.
    struct pending_edge
    {
        node_id node_;
        node_id dependency_;
    };

    std::vector<pending_node> nodes_;
    std::vector<pending_edge> edges_;
};

}  // namespace graph
