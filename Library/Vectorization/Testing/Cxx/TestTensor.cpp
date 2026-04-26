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

#include "terminals/tensor.h"
#include "terminals/vector.h"
#include "util/logger.h"
#include "xsigmaTest.h"

namespace
{
template <typename T>
void test_tensor()
{
    EXPECT_EQ(vectorization::tensor<T>::length(), vectorization::packet<T>::length());

    size_t n1 = 4;
    size_t n2 = 3;
    size_t n3 = 9;

    std::vector<T> v(n1 * n2 * n3);

    vectorization::tensor<T> t1({n1, n2, n3});
    t1 = 0.l;

    vectorization::tensor<T> t2(v.data(), {n1, n2, n3});

    vectorization::tensor<T> tMP((void*)v.data(), {n1, n2, n3});

    t2 = static_cast<T>(3.5);

    t1.deepcopy(t2);

    t2.at({n1 - 1, n2 - 1, n3 - 1}) = static_cast<T>(0.2);

    t2.at({n1 - 1, n2 - 1});

    t2.size();

    t2.rank();

    t2.dimension(0);

    t2.dimensions();

    using dimensions_t = typename vectorization::tensor<T>::dimensions_type;

    dimensions_t indices = {0};

    t2.get_matrix(indices);

    vectorization::tensor<T> t = t1 + t2;

    vectorization::tensor<T> t3 = exp(t1);
}
}  // namespace

VECTORIZATIONTEST(Math, Tensor)
{
    test_tensor<float>();

    test_tensor<double>();

    END_TEST();
}
