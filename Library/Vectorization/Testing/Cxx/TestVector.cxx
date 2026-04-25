/*
 * Quarisma: High-Performance Quantitative Library
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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "terminals/vector.h"
#include "util/logger.h"
#include "xsigmaTest.h"

namespace
{
template <typename T>
void test_vector()
{
    constexpr auto stride = quarisma::packet<T>::length();
    EXPECT_EQ(quarisma::vector<T>::dimensions(), 1);
    EXPECT_EQ(quarisma::vector<T>::length(), stride);

    quarisma::vector<T> v = {1., 2.};

    quarisma::vector<T> v1(v.data(), 2);

    quarisma::vector<T> v2(2);

    v2 = {0., 1.};

    v2 = std::move(v);

    v2.data(0);

    quarisma::vector<T> v3(0., 1., 100);
    quarisma::vector<T> v4(-1., 1., 100);
    {
        using value_t = quarisma::unary_expression<quarisma::vector<T>, quarisma::exp_evaluator>;

        value_t expr = exp(v3);

        VECTORIZATION_UNUSED auto sum = quarisma::accumulate(log(expr));
        EXPECT_EQ(value_t::length(), stride);
    }
    {
        using value_t = quarisma::trinary_expression<
            quarisma::vector<T>,
            quarisma::vector<T>,
            quarisma::vector<T>,
            quarisma::fma_evaluator>;
        EXPECT_EQ(value_t::length(), stride);
    }
}
}  // namespace

VECTORIZATIONTEST(Math, Vector)
{
    test_vector<float>();

    test_vector<double>();

    END_TEST();
}
