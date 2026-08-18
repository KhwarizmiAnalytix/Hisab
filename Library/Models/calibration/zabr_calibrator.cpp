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

#include "calibration/zabr_calibrator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace models
{
namespace
{

constexpr int    k_dim         = 3;
constexpr int    k_default_iter = 80;
constexpr double k_alpha_min   = 1.0e-4;
constexpr double k_alpha_max   = 2.0;
constexpr double k_nu_min      = 1.0e-4;
constexpr double k_nu_max      = 5.0;
constexpr double k_rho_cap     = 0.999;

struct packed_x
{
    double log_alpha;
    double log_nu;
    double artanh_rho;
};

zabr_params unpack(const packed_x& x, double beta, double gamma)
{
    zabr_params p;
    p.alpha = std::exp(x.log_alpha);
    p.nu    = std::exp(x.log_nu);
    p.rho   = std::tanh(x.artanh_rho);
    p.beta  = beta;
    p.gamma = gamma;
    p.alpha = std::min(std::max(p.alpha, k_alpha_min), k_alpha_max);
    p.nu    = std::min(std::max(p.nu, k_nu_min), k_nu_max);
    p.rho   = std::min(std::max(p.rho, -k_rho_cap), k_rho_cap);
    return p;
}

packed_x pack(const zabr_params& p)
{
    packed_x x;
    x.log_alpha   = std::log(std::max(p.alpha, k_alpha_min));
    x.log_nu      = std::log(std::max(p.nu, k_nu_min));
    const double r = std::min(std::max(p.rho, -k_rho_cap), k_rho_cap);
    x.artanh_rho   = 0.5 * std::log((1.0 + r) / (1.0 - r));
    return x;
}

double rmse_vols(
    const packed_x& x,
    double          forward,
    double          expiry,
    double          beta,
    double          gamma,
    const double*   market,
    std::size_t     n)
{
    const zabr_params params = unpack(x, beta, gamma);
    double            acc    = 0.0;
    int               count  = 0;
    for (std::size_t i = 0; i < n; ++i)
    {
        const double strike = forward * std::exp(k_log_moneyness[i]);
        const auto   vol    = zabr_black_vol(forward, strike, expiry, params);
        if (!vol.has_value())
        {
            return 1.0e6;
        }
        const double d = *vol - market[i];
        acc += d * d;
        ++count;
    }
    if (count == 0)
    {
        return 1.0e6;
    }
    return std::sqrt(acc / static_cast<double>(count));
}

packed_x from_array(const std::array<double, k_dim>& a)
{
    packed_x x;
    x.log_alpha  = a[0];
    x.log_nu     = a[1];
    x.artanh_rho = a[2];
    return x;
}

std::array<double, k_dim> to_array(const packed_x& x)
{
    return {x.log_alpha, x.log_nu, x.artanh_rho};
}

}  // namespace

std::optional<zabr_calibration_result> calibrate_zabr(
    const double*      market_vols,
    std::size_t        n_vols,
    double             forward,
    double             expiry,
    double             beta,
    double             gamma,
    const zabr_params* initial,
    int                max_iter)
{
    if (market_vols == nullptr || n_vols != static_cast<std::size_t>(k_smile_points))
    {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < n_vols; ++i)
    {
        if (!std::isfinite(market_vols[i]) || market_vols[i] <= 0.0)
        {
            return std::nullopt;
        }
    }

    zabr_params guess;
    if (initial != nullptr)
    {
        guess = *initial;
    }
    else
    {
        guess.alpha = market_vols[k_smile_points / 2];
        guess.nu    = 0.4;
        guess.rho   = -0.2;
    }
    guess.beta  = beta;
    guess.gamma = gamma;
    if (!is_valid(guess, forward, expiry))
    {
        return std::nullopt;
    }

    packed_x start = pack(guess);
    auto     eval  = [&](const std::array<double, k_dim>& a) {
        return rmse_vols(from_array(a), forward, expiry, beta, gamma, market_vols, n_vols);
    };

    const int n_iter = (max_iter < 0) ? k_default_iter : max_iter;

    // Nelder–Mead simplex on (log α, log ν, artanh ρ). Short warm-start
    // polishes (initial + max_iter <= 24) use a tighter simplex.
    std::array<std::array<double, k_dim>, k_dim + 1> simplex{};
    std::array<double, k_dim + 1>                    scores{};
    simplex[0] = to_array(start);
    const bool local_polish = (initial != nullptr) && (n_iter <= 24);
    const double scale      = local_polish ? 0.05 : 0.15;
    const std::array<double, k_dim> step = {scale, scale, scale * 4.0 / 3.0};
    for (std::size_t i = 0; i < static_cast<std::size_t>(k_dim); ++i)
    {
        simplex[i + 1]    = simplex[0];
        simplex[i + 1][i] += step[i];
    }
    for (int i = 0; i <= k_dim; ++i)
    {
        scores[static_cast<std::size_t>(i)] = eval(simplex[static_cast<std::size_t>(i)]);
    }

    for (int iter = 0; iter < n_iter; ++iter)
    {
        std::array<int, k_dim + 1> order{};
        for (int i = 0; i <= k_dim; ++i)
        {
            order[static_cast<std::size_t>(i)] = i;
        }
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            return scores[static_cast<std::size_t>(a)] < scores[static_cast<std::size_t>(b)];
        });

        const int best_i  = order[0];
        const int worst_i = order[k_dim];
        const int second_i = order[k_dim - 1];
        if (scores[static_cast<std::size_t>(worst_i)] - scores[static_cast<std::size_t>(best_i)] <
            1.0e-8)
        {
            break;
        }

        std::array<double, k_dim> centroid{};
        for (int i = 0; i < k_dim; ++i)
        {
            const int idx = order[static_cast<std::size_t>(i)];
            for (int d = 0; d < k_dim; ++d)
            {
                centroid[static_cast<std::size_t>(d)] +=
                    simplex[static_cast<std::size_t>(idx)][static_cast<std::size_t>(d)];
            }
        }
        for (int d = 0; d < k_dim; ++d)
        {
            centroid[static_cast<std::size_t>(d)] /= static_cast<double>(k_dim);
        }

        auto reflect = centroid;
        for (int d = 0; d < k_dim; ++d)
        {
            reflect[static_cast<std::size_t>(d)] =
                centroid[static_cast<std::size_t>(d)] +
                1.0 * (centroid[static_cast<std::size_t>(d)] -
                       simplex[static_cast<std::size_t>(worst_i)][static_cast<std::size_t>(d)]);
        }
        const double f_ref = eval(reflect);

        if (f_ref < scores[static_cast<std::size_t>(best_i)])
        {
            auto expand = centroid;
            for (int d = 0; d < k_dim; ++d)
            {
                expand[static_cast<std::size_t>(d)] =
                    centroid[static_cast<std::size_t>(d)] +
                    2.0 * (reflect[static_cast<std::size_t>(d)] - centroid[static_cast<std::size_t>(d)]);
            }
            const double f_exp = eval(expand);
            if (f_exp < f_ref)
            {
                simplex[static_cast<std::size_t>(worst_i)] = expand;
                scores[static_cast<std::size_t>(worst_i)]  = f_exp;
            }
            else
            {
                simplex[static_cast<std::size_t>(worst_i)] = reflect;
                scores[static_cast<std::size_t>(worst_i)]  = f_ref;
            }
        }
        else if (f_ref < scores[static_cast<std::size_t>(second_i)])
        {
            simplex[static_cast<std::size_t>(worst_i)] = reflect;
            scores[static_cast<std::size_t>(worst_i)]  = f_ref;
        }
        else
        {
            auto contract = centroid;
            const double coeff = (f_ref < scores[static_cast<std::size_t>(worst_i)]) ? 0.5 : -0.5;
            for (int d = 0; d < k_dim; ++d)
            {
                const double dir =
                    (coeff > 0.0)
                        ? (reflect[static_cast<std::size_t>(d)] - centroid[static_cast<std::size_t>(d)])
                        : (simplex[static_cast<std::size_t>(worst_i)][static_cast<std::size_t>(d)] -
                           centroid[static_cast<std::size_t>(d)]);
                contract[static_cast<std::size_t>(d)] =
                    centroid[static_cast<std::size_t>(d)] + std::abs(coeff) * dir;
            }
            const double f_con = eval(contract);
            if (f_con < scores[static_cast<std::size_t>(worst_i)])
            {
                simplex[static_cast<std::size_t>(worst_i)] = contract;
                scores[static_cast<std::size_t>(worst_i)]  = f_con;
            }
            else
            {
                for (int i = 1; i <= k_dim; ++i)
                {
                    const int idx = order[static_cast<std::size_t>(i)];
                    for (int d = 0; d < k_dim; ++d)
                    {
                        simplex[static_cast<std::size_t>(idx)][static_cast<std::size_t>(d)] =
                            0.5 * (simplex[static_cast<std::size_t>(best_i)][static_cast<std::size_t>(d)] +
                                   simplex[static_cast<std::size_t>(idx)][static_cast<std::size_t>(d)]);
                    }
                    scores[static_cast<std::size_t>(idx)] =
                        eval(simplex[static_cast<std::size_t>(idx)]);
                }
            }
        }
    }

    int best = 0;
    for (int i = 1; i <= k_dim; ++i)
    {
        if (scores[static_cast<std::size_t>(i)] < scores[static_cast<std::size_t>(best)])
        {
            best = i;
        }
    }
    zabr_calibration_result out;
    out.params = unpack(from_array(simplex[static_cast<std::size_t>(best)]), beta, gamma);
    out.rmse   = scores[static_cast<std::size_t>(best)];
    if (!is_valid(out.params, forward, expiry) || !std::isfinite(out.rmse))
    {
        return std::nullopt;
    }
    return out;
}

}  // namespace models
