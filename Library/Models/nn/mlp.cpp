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

#include "nn/mlp.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace models
{
namespace
{

std::size_t weight_count(const std::vector<int>& sizes)
{
    std::size_t n = 0;
    for (std::size_t i = 0; i + 1 < sizes.size(); ++i)
    {
        n += static_cast<std::size_t>(sizes[i]) * static_cast<std::size_t>(sizes[i + 1]);
    }
    return n;
}

std::size_t bias_count(const std::vector<int>& sizes)
{
    if (sizes.size() < 2)
    {
        return 0;
    }
    return std::accumulate(
        sizes.begin() + 1,
        sizes.end(),
        std::size_t{0},
        [](std::size_t n, int size) { return n + static_cast<std::size_t>(size); });
}

}  // namespace

bool mlp::configure(
    const int*    layer_sizes,
    std::size_t   n_sizes,
    const double* weights,
    std::size_t   n_weights,
    const double* biases,
    std::size_t   n_biases)
{
    sizes_.clear();
    weights_.clear();
    biases_.clear();

    if (layer_sizes == nullptr || n_sizes < 2 || weights == nullptr || biases == nullptr)
    {
        return false;
    }
    for (std::size_t i = 0; i < n_sizes; ++i)
    {
        if (layer_sizes[i] <= 0)
        {
            return false;
        }
        sizes_.push_back(layer_sizes[i]);
    }
    if (n_weights != weight_count(sizes_) || n_biases != bias_count(sizes_))
    {
        sizes_.clear();
        return false;
    }
    weights_.assign(weights, weights + n_weights);
    biases_.assign(biases, biases + n_biases);
    return true;
}

bool mlp::forward(const double* input, std::size_t n_in, double* output, std::size_t n_out) const
{
    if (empty() || input == nullptr || output == nullptr)
    {
        return false;
    }
    if (n_in != static_cast<std::size_t>(sizes_.front()) ||
        n_out != static_cast<std::size_t>(sizes_.back()))
    {
        return false;
    }

    std::vector<double> cur(input, input + n_in);
    std::size_t         w_off = 0;
    std::size_t         b_off = 0;
    for (std::size_t layer = 0; layer + 1 < sizes_.size(); ++layer)
    {
        const int           n_from = sizes_[layer];
        const int           n_to   = sizes_[layer + 1];
        std::vector<double> nxt(static_cast<std::size_t>(n_to), 0.0);
        const bool          last = (layer + 2 == sizes_.size());
        for (int j = 0; j < n_to; ++j)
        {
            double acc = biases_[b_off + static_cast<std::size_t>(j)];
            for (int i = 0; i < n_from; ++i)
            {
                acc += weights_[w_off + static_cast<std::size_t>(j) * static_cast<std::size_t>(n_from) +
                                static_cast<std::size_t>(i)] *
                       cur[static_cast<std::size_t>(i)];
            }
            nxt[static_cast<std::size_t>(j)] = last ? acc : std::max(0.0, acc);
        }
        w_off += static_cast<std::size_t>(n_from) * static_cast<std::size_t>(n_to);
        b_off += static_cast<std::size_t>(n_to);
        cur.swap(nxt);
    }
    std::copy(cur.begin(), cur.end(), output);
    return true;
}

int mlp::input_size() const
{
    return sizes_.empty() ? 0 : sizes_.front();
}

int mlp::output_size() const
{
    return sizes_.empty() ? 0 : sizes_.back();
}

bool mlp::empty() const
{
    return sizes_.size() < 2;
}

}  // namespace models
