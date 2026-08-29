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

#include <cmath>

#include "ModelsTest.h"
#include "model/zabr.h"

using namespace models;

TEST(Zabr, atm_vol_near_alpha_when_beta_one)
{
    const double vol = sabr_black_vol(1.0, 1.0, 1.0, 0.2, 1.0, 0.0, 0.3).value();
    EXPECT_NEAR(vol, 0.2015, 1.0e-6);
}

TEST(Zabr, gamma_one_matches_sabr)
{
    zabr_params p;
    p.alpha        = 0.25;
    p.beta         = 0.5;
    p.nu           = 0.6;
    p.rho          = -0.4;
    p.gamma        = 1.0;
    const double z = zabr_black_vol(1.0, 0.9, 2.0, p).value();
    const double s = sabr_black_vol(1.0, 0.9, 2.0, p.alpha, p.beta, p.rho, p.nu).value();
    EXPECT_NEAR(z, s, 1.0e-12);
}

TEST(Zabr, smile_fills_standard_grid)
{
    zabr_params p;
    double      vols[k_smile_points];
    EXPECT_TRUE(zabr_smile(1.0, 1.0, p, vols, k_smile_points));
    for (int i = 0; i < k_smile_points; ++i)
    {
        EXPECT_GT(vols[i], 0.0);
        EXPECT_TRUE(std::isfinite(vols[i]));
    }
}

TEST(Zabr, rejects_invalid_inputs)
{
    EXPECT_FALSE(sabr_black_vol(0.0, 1.0, 1.0, 0.2, 1.0, 0.0, 0.3).has_value());
    EXPECT_FALSE(sabr_black_vol(1.0, 1.0, 1.0, 0.2, 1.0, 1.5, 0.3).has_value());
    zabr_params p;
    p.alpha = -0.1;
    EXPECT_FALSE(is_valid(p, 1.0, 1.0));
    double vols[k_smile_points];
    EXPECT_FALSE(zabr_smile(1.0, 1.0, p, vols, k_smile_points));
    EXPECT_FALSE(zabr_smile(1.0, 1.0, zabr_params{}, nullptr, k_smile_points));
    EXPECT_FALSE(zabr_smile(1.0, 1.0, zabr_params{}, vols, 3));
}

TEST(Zabr, wing_vol_finite)
{
    zabr_params p;
    p.gamma        = 0.5;
    const auto vol = zabr_black_vol(1.0, 0.67, 3.0, p);
    ASSERT_TRUE(vol.has_value());
    EXPECT_GT(*vol, 0.0);
}
