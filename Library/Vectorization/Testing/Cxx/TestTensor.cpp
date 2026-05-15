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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "VectorizationTest.h"
#include "common/vectorization_macros.h"
#include "terminals/tensor.h"

namespace
{
template <typename T>
void test_tensor()
{
    using tensor_t = vectorization::tensor<T>;
    using dims_t   = typename tensor_t::dimensions_type;
    const T eps    = std::is_same<T, float>::value ? T(1e-5) : T(1e-10);

    // -----------------------------------------------------------------------
    // SIMD metadata
    // -----------------------------------------------------------------------
    EXPECT_EQ(tensor_t::length(), vectorization::packet<T>::length());

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    // 1-D
    {
        tensor_t v(6u);
        EXPECT_EQ(v.rank(), 1u);
        EXPECT_EQ(v.size(), 6u);
        EXPECT_EQ(v.size(0), 6);
        EXPECT_EQ(v.stride(0), 1);
        EXPECT_FALSE(v.empty());
        EXPECT_TRUE(v.is_contiguous());
    }

    // 1-D linspace
    {
        tensor_t ls(T(0), T(4), 5u);
        EXPECT_EQ(ls.size(), 5u);
        EXPECT_NEAR(static_cast<T>(ls[0]), T(0), eps);
        EXPECT_NEAR(static_cast<T>(ls[2]), T(2), eps);
        EXPECT_NEAR(static_cast<T>(ls[4]), T(4), eps);
    }

    // 1-D initializer_list
    {
        tensor_t il{T(1), T(2), T(3)};
        EXPECT_EQ(il.size(), 3u);
        EXPECT_EQ(static_cast<T>(il[0]), T(1));
        EXPECT_EQ(static_cast<T>(il[1]), T(2));
        EXPECT_EQ(static_cast<T>(il[2]), T(3));
    }

    // 2-D
    {
        tensor_t m(3u, 4u);
        EXPECT_EQ(m.rank(), 2u);
        EXPECT_EQ(m.dimension(0), 3u);
        EXPECT_EQ(m.dimension(1), 4u);
        EXPECT_EQ(m.size(), 12u);
        EXPECT_EQ(m.stride(0), 4);
        EXPECT_EQ(m.stride(1), 1);
        EXPECT_TRUE(m.is_contiguous());
    }

    // 2-D initializer_list
    {
        tensor_t il2{{T(1), T(2)}, {T(3), T(4)}};
        EXPECT_EQ(il2.dimension(0), 2u);
        EXPECT_EQ(il2.dimension(1), 2u);
        EXPECT_EQ(static_cast<T>(il2.at(0u, 0u)), T(1));
        EXPECT_EQ(static_cast<T>(il2.at(0u, 1u)), T(2));
        EXPECT_EQ(static_cast<T>(il2.at(1u, 0u)), T(3));
        EXPECT_EQ(static_cast<T>(il2.at(1u, 1u)), T(4));
    }

    // N-D const ref dims
    {
        tensor_t nd(dims_t{2, 3, 4});
        EXPECT_EQ(nd.rank(), 3u);
        EXPECT_EQ(nd.size(), 24u);
        EXPECT_EQ(nd.dimension(0), 2u);
        EXPECT_EQ(nd.dimension(1), 3u);
        EXPECT_EQ(nd.dimension(2), 4u);
        EXPECT_EQ(nd.stride(0), 12);
        EXPECT_EQ(nd.stride(1), 4);
        EXPECT_EQ(nd.stride(2), 1);
    }

    // N-D move dims
    {
        dims_t   d{2, 5};
        tensor_t nd(std::move(d));
        EXPECT_EQ(nd.rank(), 2u);
        EXPECT_EQ(nd.size(), 10u);
    }

    // External data wrap (non-owning, clone=false default)
    {
        std::vector<T> buf(6, T(9));
        tensor_t       ext(buf.data(), dims_t{2, 3});
        EXPECT_EQ(ext.dimension(0), 2u);
        EXPECT_EQ(ext.dimension(1), 3u);
        EXPECT_EQ(ext.size(), 6u);
        EXPECT_EQ(ext.data(), buf.data());
        EXPECT_EQ(static_cast<T>(ext.at(0u, 0u)), T(9));
        EXPECT_EQ(static_cast<T>(ext.at(1u, 2u)), T(9));
    }

    // Copy constructor (clone=false: shares data pointer)
    {
        tensor_t orig(4u);
        orig = T(7);
        tensor_t cp(orig);
        EXPECT_EQ(cp.size(), orig.size());
        EXPECT_EQ(cp.rank(), orig.rank());
        EXPECT_EQ(cp.data(), orig.data());
        EXPECT_TRUE(cp == orig);
    }

    // Move constructor
    {
        tensor_t src(5u);
        src              = T(3);
        T*       raw_ptr = src.data();
        tensor_t moved(std::move(src));
        EXPECT_EQ(moved.size(), 5u);
        EXPECT_EQ(moved.data(), raw_ptr);
    }

    // Copy assignment (clone=false: redirects pointer, old alloc responsibility stays with source)
    {
        tensor_t a(3u), b(3u);
        a = T(1);
        b = T(2);
        a = b;
        EXPECT_EQ(a.data(), b.data());
        EXPECT_TRUE(a == b);
    }

    // Move assignment
    {
        tensor_t a(3u), b(4u);
        b          = T(5);
        T* raw_ptr = b.data();
        a          = std::move(b);
        EXPECT_EQ(a.size(), 4u);
        EXPECT_EQ(a.data(), raw_ptr);
    }

    // -----------------------------------------------------------------------
    // Shape metadata
    // -----------------------------------------------------------------------
    {
        tensor_t m(2u, 5u);

        // dimensions() span
        auto dims = m.dimensions();
        EXPECT_EQ(dims.size(), 2u);
        EXPECT_EQ(dims[0], 2);
        EXPECT_EQ(dims[1], 5);

        // strides() span
        auto strs = m.strides();
        EXPECT_EQ(strs.size(), 2u);
        EXPECT_EQ(strs[0], 5);
        EXPECT_EQ(strs[1], 1);

        // size(dim) / stride(dim) / dimension(n)
        EXPECT_EQ(m.size(0), 2);
        EXPECT_EQ(m.size(1), 5);
        EXPECT_EQ(m.stride(0), 5);
        EXPECT_EQ(m.stride(1), 1);
        EXPECT_EQ(m.dimension(0), 2u);
        EXPECT_EQ(m.dimension(1), 5u);
    }

    // empty()
    {
        tensor_t empty_t;
        EXPECT_TRUE(empty_t.empty());
        tensor_t non_empty(1u);
        EXPECT_FALSE(non_empty.empty());
    }

    // -----------------------------------------------------------------------
    // Element access
    // -----------------------------------------------------------------------
    {
        // operator[] and at(i)
        tensor_t v(5u);
        v    = T(0);
        v[0] = T(10);
        v[4] = T(40);
        EXPECT_EQ(static_cast<T>(v[0]), T(10));
        EXPECT_EQ(static_cast<T>(v.at(size_t(0))), T(10));
        EXPECT_EQ(static_cast<T>(v[4]), T(40));

        // begin / end iteration
        T sum = T(0);
        for (auto it = v.begin(); it != v.end(); ++it)
            sum += *it;
        EXPECT_NEAR(sum, T(50), eps);

        // at(i, j)
        tensor_t m(2u, 3u);
        m            = T(0);
        m.at(0u, 1u) = T(5);
        m.at(1u, 2u) = T(7);
        EXPECT_EQ(static_cast<T>(m.at(0u, 1u)), T(5));
        EXPECT_EQ(static_cast<T>(m.at(1u, 2u)), T(7));

        // at(dimensions_type) — N-D
        tensor_t t3(dims_t{2, 3, 4});
        t3                     = T(0);
        t3.at(dims_t{1, 2, 3}) = T(99);
        EXPECT_EQ(static_cast<T>(t3.at(dims_t{1, 2, 3})), T(99));
    }

    // -----------------------------------------------------------------------
    // Clone (deep copy)
    // -----------------------------------------------------------------------
    {
        tensor_t orig(3u);
        orig     = T(7);
        auto dst = orig.clone();
        EXPECT_EQ(dst.size(), orig.size());
        EXPECT_TRUE(dst == orig);
        // Verify independence: modifying orig doesn't affect dst
        orig[0] = T(99);
        EXPECT_NE(static_cast<T>(dst[0]), T(99));
    }

    // -----------------------------------------------------------------------
    // Views
    // -----------------------------------------------------------------------

    // t() — transpose: swaps shape and strides, shares data, non-contiguous
    {
        tensor_t m(2u, 3u);
        m            = T(0);
        m.at(0u, 2u) = T(6);
        auto tv      = m.t();
        EXPECT_EQ(tv.dimension(0), 3u);
        EXPECT_EQ(tv.dimension(1), 2u);
        EXPECT_EQ(tv.stride(0), m.stride(1));
        EXPECT_EQ(tv.stride(1), m.stride(0));
        EXPECT_FALSE(tv.is_contiguous());
        EXPECT_EQ(tv.data(), m.data());
        // (0,2) in m == (2,0) in tv: access via pointer + stride
        const T* p = tv.data();
        EXPECT_EQ(*(p + 2 * tv.stride(0) + 0 * tv.stride(1)), T(6));
    }

    // permute(): reorders axes
    {
        tensor_t nd(dims_t{2, 3, 4});
        nd      = T(0);
        auto pv = nd.permute(dims_t{2, 0, 1});
        EXPECT_EQ(pv.dimension(0), 4u);
        EXPECT_EQ(pv.dimension(1), 2u);
        EXPECT_EQ(pv.dimension(2), 3u);
        EXPECT_EQ(pv.stride(0), nd.stride(2));
        EXPECT_EQ(pv.stride(1), nd.stride(0));
        EXPECT_EQ(pv.stride(2), nd.stride(1));
        EXPECT_EQ(pv.data(), nd.data());
    }

    // view(): reinterprets shape with new contiguous strides, shares data
    {
        tensor_t v(12u);
        for (size_t i = 0; i < 12; ++i)
            v[i] = static_cast<T>(i);
        auto vw = v.view(dims_t{3, 4});
        EXPECT_EQ(vw.dimension(0), 3u);
        EXPECT_EQ(vw.dimension(1), 4u);
        EXPECT_EQ(vw.size(), 12u);
        EXPECT_TRUE(vw.is_contiguous());
        EXPECT_EQ(vw.data(), v.data());
        // Modification through view is reflected in source
        vw.at(0u, 0u) = T(99);
        EXPECT_EQ(static_cast<T>(v[0]), T(99));
    }

    // reshape(): same semantics as view for contiguous source
    {
        tensor_t v(6u);
        for (size_t i = 0; i < 6; ++i)
            v[i] = static_cast<T>(i);
        auto rs = v.reshape(dims_t{2, 3});
        EXPECT_EQ(rs.dimension(0), 2u);
        EXPECT_EQ(rs.dimension(1), 3u);
        EXPECT_EQ(rs.size(), 6u);
        EXPECT_EQ(rs.data(), v.data());
        EXPECT_TRUE(rs.is_contiguous());
    }

    // slice(dim, start, stop, step): shrinks one dimension, multiplies its stride
    {
        tensor_t v(10u);
        for (size_t i = 0; i < 10; ++i)
            v[i] = static_cast<T>(i);

        // indices 1, 4, 7 (step=3 through [1,8))
        auto sl = v.slice(0, 1, 8, 3);
        EXPECT_EQ(sl.size(), 3u);
        EXPECT_EQ(sl.stride(0), 3);
        EXPECT_EQ(sl.data(), v.data() + 1);
        EXPECT_EQ(*(sl.data() + 0 * sl.stride(0)), T(1));
        EXPECT_EQ(*(sl.data() + 1 * sl.stride(0)), T(4));
        EXPECT_EQ(*(sl.data() + 2 * sl.stride(0)), T(7));

        // stop==-1 means to end
        auto sl2 = v.slice(0, 5);
        EXPECT_EQ(sl2.size(), 5u);
        EXPECT_EQ(sl2.data(), v.data() + 5);
    }

    // -----------------------------------------------------------------------
    // Predicates
    // -----------------------------------------------------------------------
    {
        // is_zero
        tensor_t z(4u);
        z = T(0);
        EXPECT_TRUE(z.is_zero());
        z[2] = T(1);
        EXPECT_FALSE(z.is_zero());

        // non_negative
        tensor_t nn(3u);
        nn = T(1);
        EXPECT_TRUE(nn.non_negative());
        nn[0] = T(-1);
        EXPECT_FALSE(nn.non_negative());

        // positive
        tensor_t pos(3u);
        pos = T(1);
        EXPECT_TRUE(pos.positive());
        pos[0] = T(0);
        EXPECT_FALSE(pos.positive());

        // symmetric
        tensor_t sym{{T(1), T(2)}, {T(2), T(1)}};
        EXPECT_TRUE(sym.symmetric());
        sym.at(0u, 1u) = T(3);
        EXPECT_FALSE(sym.symmetric());

        // identity
        tensor_t id(2u, 2u);
        id            = T(0);
        id.at(0u, 0u) = T(1);
        id.at(1u, 1u) = T(1);
        EXPECT_TRUE(id.identity());
        id.at(0u, 1u) = T(1);
        EXPECT_FALSE(id.identity());

        // trace
        tensor_t tr{{T(3), T(0)}, {T(0), T(5)}};
        EXPECT_NEAR(static_cast<T>(tr.trace()), T(8), eps);

        // is_correlation: unit diagonal, symmetric, all |off-diag| <= 1
        tensor_t corr{{T(1), T(0.5)}, {T(0.5), T(1)}};
        EXPECT_TRUE(corr.is_correlation());
        tensor_t ncorr{{T(1), T(2)}, {T(2), T(1)}};
        EXPECT_FALSE(ncorr.is_correlation());

        // operator== and !=
        tensor_t a(3u), b(3u);
        a = T(2);
        b = T(2);
        EXPECT_TRUE(a == b);
        b[1] = T(3);
        EXPECT_FALSE(a == b);
        EXPECT_TRUE(a != b);
    }

    // -----------------------------------------------------------------------
    // Expressions
    // -----------------------------------------------------------------------
    {
        tensor_t a(4u), b(4u);
        a = T(3);
        b = T(2);

        // Expression construction
        tensor_t c = a + b;
        EXPECT_EQ(c.size(), 4u);
        EXPECT_NEAR(static_cast<T>(c[0]), T(5), eps);

        // Scalar assignment via expression
        tensor_t d(4u);
        d = a + b;
        EXPECT_NEAR(static_cast<T>(d[0]), T(5), eps);

        // Unary expression
        tensor_t ex = exp(a);
        EXPECT_EQ(ex.size(), 4u);
        EXPECT_NEAR(static_cast<T>(ex[0]), std::exp(T(3)), T(1e-4));
    }

    // -----------------------------------------------------------------------
    // Formatting
    // -----------------------------------------------------------------------
    {
        tensor_t    v{T(1), T(2)};
        std::string s = v.to_string();
        EXPECT_FALSE(s.empty());
        EXPECT_NE(s, std::string("[]"));

        std::ostringstream oss;
        oss << v;
        EXPECT_EQ(oss.str(), s);
    }
}
}  // namespace

VECTORIZATIONTEST(Math, Tensor)
{
    test_tensor<float>();
    test_tensor<double>();

    END_TEST();
}
