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

// QA-trained inverse: market smile + (T, F, β, γ) → (α, ν, ρ).
// β and γ are desk-chosen. polish_iters > 0 runs a short Nelder–Mead
// polish on the teacher from the network warm-start (0 = network only).
class MODELS_VISIBILITY qa_calibrator
{
public:
    MODELS_API qa_calibrator();

    MODELS_API bool ready() const;

    MODELS_API std::optional<zabr_params> calibrate(
        const double* market_vols,
        std::size_t   n_vols,
        double        forward,
        double        expiry,
        double        beta,
        double        gamma,
        int           polish_iters = 12) const;

private:
    bool ready_ = false;
};

}  // namespace models
