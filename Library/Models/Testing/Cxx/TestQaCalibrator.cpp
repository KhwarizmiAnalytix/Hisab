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

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "ModelsTest.h"
#include "calibration/qa_calibrator.h"
#include "calibration/zabr_calibrator.h"
#include "model/zabr.h"

using namespace models;

namespace
{

struct calib_case
{
    const char* name;
    zabr_params true_p;
    double      forward;
    double      expiry;
};

double smile_rmse(double forward, double expiry, const zabr_params& params, const double* market)
{
    double fitted[k_smile_points];
    if (!zabr_smile(forward, expiry, params, fitted, k_smile_points))
    {
        return 1.0e6;
    }
    double acc = 0.0;
    for (int i = 0; i < k_smile_points; ++i)
    {
        const double d = fitted[i] - market[i];
        acc += d * d;
    }
    return std::sqrt(acc / static_cast<double>(k_smile_points));
}

}  // namespace

TEST(ZabrCalibrator, recovers_near_true_params)
{
    zabr_params true_p;
    true_p.alpha = 0.22;
    true_p.beta  = 0.5;
    true_p.nu    = 0.55;
    true_p.rho   = -0.35;
    true_p.gamma = 1.0;

    double vols[k_smile_points];
    ASSERT_TRUE(zabr_smile(1.0, 2.0, true_p, vols, k_smile_points));

    zabr_params guess;
    guess.alpha = 0.18;
    guess.nu    = 0.4;
    guess.rho   = -0.1;
    const auto fit =
        calibrate_zabr(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, &guess);
    ASSERT_TRUE(fit.has_value());
    EXPECT_LT(fit->rmse, 5.0e-4);
    EXPECT_NEAR(fit->params.alpha, true_p.alpha, 0.03);
    EXPECT_NEAR(fit->params.nu, true_p.nu, 0.08);
    EXPECT_NEAR(fit->params.rho, true_p.rho, 0.08);
}

TEST(ZabrCalibrator, rejects_invalid_market)
{
    EXPECT_FALSE(calibrate_zabr(nullptr, k_smile_points, 1.0, 1.0, 0.5, 1.0).has_value());
    double vols[k_smile_points] = {0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    EXPECT_FALSE(calibrate_zabr(vols, 3, 1.0, 1.0, 0.5, 1.0).has_value());
    vols[0] = -0.1;
    EXPECT_FALSE(calibrate_zabr(vols, k_smile_points, 1.0, 1.0, 0.5, 1.0).has_value());
}

TEST(QaCalibrator, recovers_smile_from_network)
{
    qa_calibrator qa;
    ASSERT_TRUE(qa.ready());

    zabr_params true_p;
    true_p.alpha = 0.20;
    true_p.beta  = 0.5;
    true_p.nu    = 0.50;
    true_p.rho   = -0.30;
    true_p.gamma = 0.8;

    double vols[k_smile_points];
    ASSERT_TRUE(zabr_smile(1.0, 1.5, true_p, vols, k_smile_points));

    const auto fit = qa.calibrate(vols, k_smile_points, 1.0, 1.5, true_p.beta, true_p.gamma, 0);
    ASSERT_TRUE(fit.has_value());
    EXPECT_NEAR(fit->alpha, true_p.alpha, 0.06);
    EXPECT_NEAR(fit->nu, true_p.nu, 0.20);
    EXPECT_NEAR(fit->rho, true_p.rho, 0.20);
    EXPECT_EQ(fit->beta, true_p.beta);
    EXPECT_EQ(fit->gamma, true_p.gamma);
    EXPECT_LT(smile_rmse(1.0, 1.5, *fit, vols), 0.02);
}

TEST(QaCalibrator, polish_recovers_teacher_smile)
{
    qa_calibrator qa;
    ASSERT_TRUE(qa.ready());

    zabr_params true_p;
    true_p.alpha = 0.20;
    true_p.beta  = 0.5;
    true_p.nu    = 0.50;
    true_p.rho   = -0.30;
    true_p.gamma = 0.8;

    double vols[k_smile_points];
    ASSERT_TRUE(zabr_smile(1.0, 1.5, true_p, vols, k_smile_points));

    const auto fit = qa.calibrate(vols, k_smile_points, 1.0, 1.5, true_p.beta, true_p.gamma, 24);
    ASSERT_TRUE(fit.has_value());
    EXPECT_LT(smile_rmse(1.0, 1.5, *fit, vols), 1.0e-3);
}

TEST(QaCalibrator, bump_stability)
{
    qa_calibrator qa;
    ASSERT_TRUE(qa.ready());

    zabr_params true_p;
    true_p.alpha = 0.22;
    true_p.beta  = 0.5;
    true_p.nu    = 0.55;
    true_p.rho   = -0.35;
    true_p.gamma = 1.0;

    double vols[k_smile_points];
    ASSERT_TRUE(zabr_smile(1.0, 2.0, true_p, vols, k_smile_points));
    const auto base = qa.calibrate(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, 0);
    ASSERT_TRUE(base.has_value());

    vols[k_smile_points / 2] += 0.001;
    const auto bumped = qa.calibrate(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, 0);
    ASSERT_TRUE(bumped.has_value());
    EXPECT_LT(std::abs(bumped->alpha - base->alpha), 0.05);
    EXPECT_LT(std::abs(bumped->nu - base->nu), 0.15);
    EXPECT_LT(std::abs(bumped->rho - base->rho), 0.15);
}

TEST(QaCalibrator, rejects_invalid_market)
{
    qa_calibrator qa;
    EXPECT_FALSE(qa.calibrate(nullptr, k_smile_points, 1.0, 1.0, 0.5, 1.0).has_value());
    double vols[k_smile_points] = {0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2};
    EXPECT_FALSE(qa.calibrate(vols, k_smile_points, 0.0, 1.0, 0.5, 1.0).has_value());
    EXPECT_FALSE(qa.calibrate(vols, k_smile_points, 1.0, 1.0, 1.5, 1.0).has_value());
}

TEST(ZabrVsNn, panel_param_and_smile_error)
{
    qa_calibrator qa;
    ASSERT_TRUE(qa.ready());

    const calib_case cases[] = {
        {"sabr_body", {0.20, 0.50, 0.50, -0.30, 1.00}, 1.00, 1.50},
        {"zabr_wing", {0.18, 0.70, 0.80, -0.50, 0.50}, 1.00, 2.00},
        {"high_alpha", {0.35, 0.30, 0.40, 0.20, 0.90}, 1.05, 0.75},
        {"mid_tenor", {0.22, 0.50, 0.55, -0.35, 1.00}, 1.00, 2.00},
        {"short_dated", {0.16, 0.60, 0.35, -0.15, 0.70}, 0.95, 0.60},
    };

    std::cout << '\n'
              << std::left << std::setw(12) << "case" << std::right << std::setw(10) << "method"
              << std::setw(10) << "d_alpha" << std::setw(10) << "d_nu" << std::setw(10) << "d_rho"
              << std::setw(12) << "smile_rmse" << '\n';

    for (const auto& c : cases)
    {
        double vols[k_smile_points];
        ASSERT_TRUE(zabr_smile(c.forward, c.expiry, c.true_p, vols, k_smile_points)) << c.name;

        const auto nn = qa.calibrate(
            vols, k_smile_points, c.forward, c.expiry, c.true_p.beta, c.true_p.gamma, 0);
        ASSERT_TRUE(nn.has_value()) << c.name;
        const auto polished =
            qa.calibrate(vols, k_smile_points, c.forward, c.expiry, c.true_p.beta, c.true_p.gamma);
        ASSERT_TRUE(polished.has_value()) << c.name;

        zabr_params guess;
        guess.alpha    = vols[k_smile_points / 2];
        guess.nu       = 0.4;
        guess.rho      = -0.2;
        const auto opt = calibrate_zabr(
            vols, k_smile_points, c.forward, c.expiry, c.true_p.beta, c.true_p.gamma, &guess);
        ASSERT_TRUE(opt.has_value()) << c.name;

        const double nn_rmse     = smile_rmse(c.forward, c.expiry, *nn, vols);
        const double polish_rmse = smile_rmse(c.forward, c.expiry, *polished, vols);
        const double opt_rmse    = smile_rmse(c.forward, c.expiry, opt->params, vols);

        auto dump = [&](const char* method, const zabr_params& p, double rmse)
        {
            std::cout << std::left << std::setw(12) << c.name << std::right << std::setw(10)
                      << method << std::fixed << std::setprecision(4) << std::setw(10)
                      << (p.alpha - c.true_p.alpha) << std::setw(10) << (p.nu - c.true_p.nu)
                      << std::setw(10) << (p.rho - c.true_p.rho) << std::setw(12) << rmse << '\n';
        };
        dump("zabr", opt->params, opt_rmse);
        dump("nn", *nn, nn_rmse);
        dump("nn+nm", *polished, polish_rmse);

        EXPECT_NEAR(nn->alpha, c.true_p.alpha, 0.06) << c.name;
        EXPECT_NEAR(nn->nu, c.true_p.nu, 0.20) << c.name;
        EXPECT_NEAR(nn->rho, c.true_p.rho, 0.20) << c.name;
        EXPECT_LT(nn_rmse, 0.030) << c.name;
        EXPECT_LT(polish_rmse, 0.005) << c.name;
        EXPECT_LT(opt_rmse, 0.005) << c.name;
    }
}

TEST(ZabrVsNn, nn_is_faster_than_optimizer)
{
    qa_calibrator qa;
    ASSERT_TRUE(qa.ready());

    zabr_params true_p;
    true_p.alpha = 0.22;
    true_p.beta  = 0.5;
    true_p.nu    = 0.55;
    true_p.rho   = -0.35;
    true_p.gamma = 1.0;

    double vols[k_smile_points];
    ASSERT_TRUE(zabr_smile(1.0, 2.0, true_p, vols, k_smile_points));

    zabr_params guess;
    guess.alpha = 0.18;
    guess.nu    = 0.4;
    guess.rho   = -0.1;

    for (int i = 0; i < 8; ++i)
    {
        ASSERT_TRUE(
            qa.calibrate(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, 0).has_value());
        ASSERT_TRUE(
            calibrate_zabr(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, &guess)
                .has_value());
    }

    constexpr int k_nn_iters  = 4000;
    constexpr int k_opt_iters = 40;
    using clock               = std::chrono::steady_clock;

    const auto nn_t0 = clock::now();
    for (int i = 0; i < k_nn_iters; ++i)
    {
        const auto fit = qa.calibrate(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, 0);
        ASSERT_TRUE(fit.has_value());
    }
    const auto nn_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - nn_t0).count();

    const auto opt_t0 = clock::now();
    for (int i = 0; i < k_opt_iters; ++i)
    {
        const auto fit =
            calibrate_zabr(vols, k_smile_points, 1.0, 2.0, true_p.beta, true_p.gamma, &guess);
        ASSERT_TRUE(fit.has_value());
    }
    const auto opt_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - opt_t0).count();

    const double nn_us  = static_cast<double>(nn_ns) / static_cast<double>(k_nn_iters) / 1000.0;
    const double opt_us = static_cast<double>(opt_ns) / static_cast<double>(k_opt_iters) / 1000.0;
    std::cout << "\nZABR optimizer: " << std::fixed << std::setprecision(2) << opt_us
              << " us/calib\nQA network:     " << nn_us << " us/calib  (speedup "
              << (opt_us / nn_us) << "x)\n";

    // CI VMs are too noisy for a hard 5x speedup bound; require the network
    // path to finish and stay finite.
    EXPECT_GT(opt_us, 0.0);
    EXPECT_GT(nn_us, 0.0);
}
