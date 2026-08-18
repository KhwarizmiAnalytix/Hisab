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

#include "ModelsTest.h"
#include "nn/mlp.h"

using namespace models;

TEST(Mlp, rejects_bad_configure)
{
    mlp net;
    EXPECT_TRUE(net.empty());
    const int    sizes[]   = {2, 2};
    const double weights[] = {1.0, 0.0, 0.0, 1.0};
    const double biases[]  = {0.0, 0.0};
    EXPECT_FALSE(net.configure(nullptr, 2, weights, 4, biases, 2));
    EXPECT_FALSE(net.configure(sizes, 1, weights, 4, biases, 2));
    EXPECT_FALSE(net.configure(sizes, 2, weights, 3, biases, 2));
    EXPECT_TRUE(net.empty());
}

TEST(Mlp, identity_linear_layer)
{
    mlp              net;
    const int        sizes[]   = {2, 2};
    const double     weights[] = {1.0, 0.0, 0.0, 1.0};
    const double     biases[]  = {0.1, -0.2};
    ASSERT_TRUE(net.configure(sizes, 2, weights, 4, biases, 2));
    const double in[] = {1.5, -3.0};
    double       out[2];
    ASSERT_TRUE(net.forward(in, 2, out, 2));
    EXPECT_NEAR(out[0], 1.6, 1.0e-12);
    EXPECT_NEAR(out[1], -3.2, 1.0e-12);
}

TEST(Mlp, relu_hides_negative_preactivation)
{
    mlp              net;
    const int        sizes[]   = {1, 1, 1};
    const double     weights[] = {-1.0, 1.0};
    const double     biases[]  = {0.0, 0.0};
    ASSERT_TRUE(net.configure(sizes, 3, weights, 2, biases, 2));
    const double in[] = {2.0};
    double       out[1];
    ASSERT_TRUE(net.forward(in, 1, out, 1));
    EXPECT_NEAR(out[0], 0.0, 1.0e-12);
}

TEST(Mlp, rejects_size_mismatch)
{
    mlp              net;
    const int        sizes[]   = {2, 1};
    const double     weights[] = {1.0, 1.0};
    const double     biases[]  = {0.0};
    ASSERT_TRUE(net.configure(sizes, 2, weights, 2, biases, 1));
    const double in[] = {1.0, 2.0};
    double       out[1];
    EXPECT_FALSE(net.forward(in, 1, out, 1));
    EXPECT_FALSE(net.forward(in, 2, out, 2));
}
