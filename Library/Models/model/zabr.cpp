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

#include "model/zabr.h"

#include <algorithm>
#include <cmath>

namespace models
{
namespace
{

constexpr double k_min_positive = 1.0e-12;
constexpr double k_atm_rel      = 1.0e-12;
constexpr double k_z_tiny       = 1.0e-8;
constexpr double k_rho_cap      = 0.999999;

bool is_finite_positive(double x)
{
    return std::isfinite(x) && x > k_min_positive;
}

double clamp_rho(double rho)
{
    if (rho > k_rho_cap)
    {
        return k_rho_cap;
    }
    if (rho < -k_rho_cap)
    {
        return -k_rho_cap;
    }
    return rho;
}

}  // namespace

bool is_valid(const zabr_params& params, double forward, double expiry)
{
    if (!is_finite_positive(forward) || !is_finite_positive(expiry))
    {
        return false;
    }
    if (!is_finite_positive(params.alpha) || !is_finite_positive(params.nu))
    {
        return false;
    }
    if (!std::isfinite(params.beta) || params.beta < 0.0 || params.beta > 1.0)
    {
        return false;
    }
    if (!std::isfinite(params.gamma) || params.gamma < 0.0 || params.gamma > 1.0)
    {
        return false;
    }
    if (!std::isfinite(params.rho) || params.rho <= -1.0 || params.rho >= 1.0)
    {
        return false;
    }
    return true;
}

std::optional<double> sabr_black_vol(
    double forward, double strike, double expiry, double alpha, double beta, double rho, double nu)
{
    if (!is_finite_positive(forward) || !is_finite_positive(strike) ||
        !is_finite_positive(expiry) || !is_finite_positive(alpha) || !is_finite_positive(nu))
    {
        return std::nullopt;
    }
    if (!std::isfinite(beta) || beta < 0.0 || beta > 1.0)
    {
        return std::nullopt;
    }
    if (!std::isfinite(rho) || rho <= -1.0 || rho >= 1.0)
    {
        return std::nullopt;
    }

    rho                     = clamp_rho(rho);
    const double one_m_beta = 1.0 - beta;
    const double f_pow      = std::pow(forward, one_m_beta);

    auto time_corr = [&](double fk_1m_beta, double fk_half)
    {
        return (one_m_beta * one_m_beta / 24.0) * alpha * alpha / fk_1m_beta +
               0.25 * rho * beta * nu * alpha / fk_half + (2.0 - 3.0 * rho * rho) / 24.0 * nu * nu;
    };

    if (std::abs(forward - strike) <= k_atm_rel * forward)
    {
        const double corr = time_corr(f_pow * f_pow, f_pow);
        const double vol  = (alpha / f_pow) * (1.0 + corr * expiry);
        if (!std::isfinite(vol) || vol <= 0.0)
        {
            return std::nullopt;
        }
        return vol;
    }

    const double log_fk = std::log(forward / strike);
    const double fk     = forward * strike;
    const double fk_pow = std::pow(fk, 0.5 * one_m_beta);
    const double z      = (nu / alpha) * fk_pow * log_fk;

    double z_over_x = 1.0;
    if (std::abs(z) > k_z_tiny)
    {
        const double inner = 1.0 - 2.0 * rho * z + z * z;
        if (inner < 0.0)
        {
            return std::nullopt;
        }
        const double number = std::sqrt(inner) + z - rho;
        const double denom = 1.0 - rho;
        if (number <= k_min_positive || denom <= k_min_positive)
        {
            return std::nullopt;
        }
        const double x = std::log(number / denom);
        if (std::abs(x) <= k_min_positive)
        {
            return std::nullopt;
        }
        z_over_x = z / x;
    }

    const double log2 = log_fk * log_fk;
    const double geom_corr =
        1.0 + (one_m_beta * one_m_beta / 24.0) * log2 +
        (one_m_beta * one_m_beta * one_m_beta * one_m_beta / 1920.0) * log2 * log2;
    const double corr = time_corr(std::pow(fk, one_m_beta), fk_pow);
    const double vol  = (alpha / (fk_pow * geom_corr)) * z_over_x * (1.0 + corr * expiry);
    if (!std::isfinite(vol) || vol <= 0.0)
    {
        return std::nullopt;
    }
    return vol;
}

std::optional<double> zabr_black_vol(
    double forward, double strike, double expiry, const zabr_params& params)
{
    if (!is_valid(params, forward, expiry))
    {
        return std::nullopt;
    }
    const double alpha_floor = std::max(params.alpha, k_min_positive);
    const double nu_eff      = params.nu * std::pow(alpha_floor, params.gamma - 1.0);
    if (!std::isfinite(nu_eff) || nu_eff <= 0.0)
    {
        return std::nullopt;
    }
    return sabr_black_vol(forward, strike, expiry, params.alpha, params.beta, params.rho, nu_eff);
}

bool zabr_smile(
    double forward, double expiry, const zabr_params& params, double* vols, std::size_t n_vols)
{
    if (vols == nullptr || n_vols != static_cast<std::size_t>(k_smile_points))
    {
        return false;
    }
    for (int i = 0; i < k_smile_points; ++i)
    {
        const double strike = forward * std::exp(k_log_moneyness[i]);
        const auto   vol    = zabr_black_vol(forward, strike, expiry, params);
        if (!vol.has_value())
        {
            return false;
        }
        vols[i] = *vol;
    }
    return true;
}

}  // namespace models
