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

#include "terminals/matrix.h"
#include "util/logger.h"
#include "xsigmaTest.h"

namespace
{
template <typename T>
void test_matrix()
{
    EXPECT_EQ(quarisma::matrix<T>::dimensions(), 2);
    EXPECT_EQ(quarisma::matrix<T>::length(), quarisma::packet<T>::length());

    quarisma::matrix<T> v = {{1.}, {2.}};

    quarisma::matrix<T> v1(v.data(), 2, 1);

    quarisma::matrix<T> v2(2, 1);

    v2.deepcopy(v1);

    v2 = std::move(v);

    v2.is_correlation();
    v2.is_zero();
    v2.symmetric();

    v2 = 0.;

    v2.is_correlation();
    v2.is_zero();
    v2.symmetric();

    quarisma::matrix<T> v3 = {{1., -0.5}, {-.5, 1.}};

    v3.is_correlation();
    v3.is_zero();
    v3.symmetric();
    v3.identity();
    v3.non_negative();
}
}  // namespace

VECTORIZATIONTEST(Math, Matrix)
{
    test_matrix<double>();
    test_matrix<float>();

    END_TEST();
}
