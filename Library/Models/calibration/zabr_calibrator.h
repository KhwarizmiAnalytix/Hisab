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

#include <cstddef>
#include <optional>

#include "common/models_export.h"
#include "model/zabr.h"

namespace models
{

struct MODELS_VISIBILITY zabr_calibration_result
{
    zabr_params params;
    double      rmse = 0.0;
};

// Derivative-free calibration of α, ν, ρ with β and γ held fixed.
// Used as a standalone solver and as a short polish after qa_calibrator.
// max_iter is the Nelder–Mead iteration cap (default 80).
MODELS_API std::optional<zabr_calibration_result> calibrate_zabr(
    const double*      market_vols,
    std::size_t        n_vols,
    double             forward,
    double             expiry,
    double             beta,
    double             gamma,
    const zabr_params* initial  = nullptr,
    int                max_iter = 80);

}  // namespace models
