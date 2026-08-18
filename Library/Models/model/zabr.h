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
#include <optional>

#include "common/models_export.h"

namespace models
{

// ZABR (Andreasen–Huge): SABR with a CEV power γ on the volatility process.
//   dF = α F^β dW
//   dα = ν α^γ dZ
//   <dW, dZ> = ρ dt
// γ = 1 recovers SABR. Implied Black vol uses Hagan (2002) with the
// first-order ZABR reduction ν_eff = ν α^{γ-1}.
struct MODELS_VISIBILITY zabr_params
{
    double alpha = 0.2;
    double beta  = 0.5;
    double nu    = 0.4;
    double rho   = -0.3;
    double gamma = 1.0;
};

inline constexpr int    k_smile_points                 = 9;
inline constexpr double k_log_moneyness[k_smile_points] = {
    -0.4, -0.3, -0.2, -0.1, 0.0, 0.1, 0.2, 0.3, 0.4};

MODELS_API bool is_valid(const zabr_params& params, double forward, double expiry);

// Hagan SABR Black implied vol. Returns nullopt for invalid inputs or a
// numerically undefined expansion (negative kernel, non-finite result).
MODELS_API std::optional<double> sabr_black_vol(
    double forward,
    double strike,
    double expiry,
    double alpha,
    double beta,
    double rho,
    double nu);

// ZABR Black implied vol (SABR when gamma == 1).
MODELS_API std::optional<double> zabr_black_vol(
    double             forward,
    double             strike,
    double             expiry,
    const zabr_params& params);

// Fill implied vols on k_log_moneyness. Returns false if any point fails.
MODELS_API bool zabr_smile(
    double             forward,
    double             expiry,
    const zabr_params& params,
    double*            vols,
    std::size_t        n_vols);

}  // namespace models
