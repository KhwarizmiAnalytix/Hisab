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

#include "common/vectorization_macros.h"
#include "terminals/tensor.h"
#include "VectorizationTest.h"

namespace
{
template <typename T>
void test_vector()
{
    constexpr auto stride = vectorization::packet<T>::length();
    EXPECT_EQ(vectorization::vector<T>::length(), stride);

    vectorization::vector<T> v = {1., 2.};

    vectorization::vector<T> v1(v.data(), 2);

    vectorization::vector<T> v2(2);

    v2 = {0., 1.};

    v2 = std::move(v);

    v2.data(0);

    vectorization::vector<T> v3(0., 1., 100);
    vectorization::vector<T> v4(-1., 1., 100);
    {
        using value_t = vectorization::unary_expression<vectorization::vector<T>, vectorization::exp_evaluator>;

        value_t expr = exp(v3);

        VECTORIZATION_UNUSED auto sum = vectorization::accumulate(log(expr));
        EXPECT_EQ(value_t::length(), stride);
    }
    {
        using value_t = vectorization::trinary_expression<
            vectorization::vector<T>,
            vectorization::vector<T>,
            vectorization::vector<T>,
            vectorization::fma_evaluator>;
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
