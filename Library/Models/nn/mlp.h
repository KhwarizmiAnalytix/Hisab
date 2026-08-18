/*
 * Quarisma: High-Performance Computational Library
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

#pragma once

#include <cstddef>
#include <vector>

#include "common/models_export.h"

namespace models
{

// Dense MLP: ReLU hidden layers, linear output. Weights are row-major
// (out_features, in_features) per layer.
class MODELS_VISIBILITY mlp
{
public:
    mlp() = default;

    bool configure(
        const int*    layer_sizes,
        std::size_t   n_sizes,
        const double* weights,
        std::size_t   n_weights,
        const double* biases,
        std::size_t   n_biases);

    bool forward(const double* input, std::size_t n_in, double* output, std::size_t n_out) const;

    int input_size() const;
    int output_size() const;
    bool empty() const;

private:
    std::vector<int>    sizes_;
    std::vector<double> weights_;
    std::vector<double> biases_;
};

}  // namespace models
