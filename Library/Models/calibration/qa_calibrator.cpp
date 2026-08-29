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

#include "calibration/qa_calibrator.h"

#include <cmath>
#include <vector>

#include "calibration/zabr_calibrator.h"
#include "nn/mlp.h"
#include "nn/qa_zabr_weights.h"

namespace models
{
namespace
{

double sigmoid(double x)
{
    if (x > 40.0)
    {
        return 1.0;
    }
    if (x < -40.0)
    {
        return 0.0;
    }
    return 1.0 / (1.0 + std::exp(-x));
}

mlp make_default_net()
{
    mlp net;
    net.configure(
        qa_zabr_weights::k_layer_sizes,
        static_cast<std::size_t>(qa_zabr_weights::k_n_layers),
        qa_zabr_weights::k_weights,
        static_cast<std::size_t>(qa_zabr_weights::k_n_weights),
        qa_zabr_weights::k_biases,
        static_cast<std::size_t>(qa_zabr_weights::k_n_biases));
    return net;
}

const mlp& default_net()
{
    static const mlp net = make_default_net();
    return net;
}

}  // namespace

qa_calibrator::qa_calibrator() : ready_(!default_net().empty()) {}

bool qa_calibrator::ready() const
{
    return ready_;
}

std::optional<zabr_params> qa_calibrator::calibrate(
    const double* market_vols,
    std::size_t   n_vols,
    double        forward,
    double        expiry,
    double        beta,
    double        gamma,
    int           polish_iters) const
{
    if (!ready_ || market_vols == nullptr || n_vols != static_cast<std::size_t>(k_smile_points) ||
        qa_zabr_weights::k_n_features != k_smile_points + 4)
    {
        return std::nullopt;
    }
    if (!std::isfinite(forward) || forward <= 0.0 || !std::isfinite(expiry) || expiry <= 0.0)
    {
        return std::nullopt;
    }
    if (!std::isfinite(beta) || beta < 0.0 || beta > 1.0 || !std::isfinite(gamma) || gamma < 0.0 ||
        gamma > 1.0)
    {
        return std::nullopt;
    }

    std::vector<double> features(static_cast<std::size_t>(qa_zabr_weights::k_n_features));
    const auto          n_smile = static_cast<std::size_t>(k_smile_points);
    for (std::size_t i = 0; i < n_smile; ++i)
    {
        if (!std::isfinite(market_vols[i]) || market_vols[i] <= 0.0)
        {
            return std::nullopt;
        }
        features[i] = market_vols[i];
    }
    features[n_smile]     = expiry;
    features[n_smile + 1] = std::log(forward);
    features[n_smile + 2] = beta;
    features[n_smile + 3] = gamma;

    for (std::size_t i = 0; i < static_cast<std::size_t>(qa_zabr_weights::k_n_features); ++i)
    {
        features[i] =
            (features[i] - qa_zabr_weights::k_input_mean[i]) / qa_zabr_weights::k_input_std[i];
    }

    double raw[3] = {0.0, 0.0, 0.0};
    if (!default_net().forward(
            features.data(),
            features.size(),
            raw,
            static_cast<std::size_t>(qa_zabr_weights::k_n_outputs)))
    {
        return std::nullopt;
    }

    zabr_params out;
    out.beta  = beta;
    out.gamma = gamma;
    out.alpha = qa_zabr_weights::k_alpha_min +
                (qa_zabr_weights::k_alpha_max - qa_zabr_weights::k_alpha_min) * sigmoid(raw[0]);
    out.nu = qa_zabr_weights::k_nu_min +
             (qa_zabr_weights::k_nu_max - qa_zabr_weights::k_nu_min) * sigmoid(raw[1]);
    out.rho = qa_zabr_weights::k_rho_max * std::tanh(raw[2]);
    if (!is_valid(out, forward, expiry))
    {
        return std::nullopt;
    }
    if (polish_iters <= 0)
    {
        return out;
    }
    const auto polished =
        calibrate_zabr(market_vols, n_vols, forward, expiry, beta, gamma, &out, polish_iters);
    if (!polished.has_value() || !is_valid(polished->params, forward, expiry))
    {
        return out;
    }
    return polished->params;
}

}  // namespace models
