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

// Phase 2 milestone test for the Metal GPU backend: exercises the fixed kernel set
// through backend/gpu/metal/metal_dispatch.h directly, with no tensor<T>/expression
// templates involved yet (that wiring is Phase 3, expressions_evaluator_metal.h).

#include "VectorizationTest.h"

#if VECTORIZATION_HAS_METAL

#include <cmath>
#include <vector>

#include "allocator.h"
#include "backend/gpu/metal/metal_dispatch.h"
#include "common/device.h"

namespace
{
using alloc_t = memory::allocator<float>;

std::vector<float> download(const float* metal_ptr, size_t n)
{
    std::vector<float> host(n);
    alloc_t::copy(metal_ptr, n, host.data(), memory::device_enum::METAL, memory::device_enum::CPU);
    return host;
}

float* upload(const std::vector<float>& host)
{
    float* ptr = alloc_t::allocate(host.size(), memory::device_enum::METAL);
    alloc_t::copy(host.data(), host.size(), ptr, memory::device_enum::CPU, memory::device_enum::METAL);
    return ptr;
}
}  // namespace

VECTORIZATIONTEST(MetalDispatch, DeviceAvailable)
{
    EXPECT_TRUE(vectorization::metal_backend::device_available());
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, Fill)
{
    constexpr size_t n = 512;
    float*           out = alloc_t::allocate(n, memory::device_enum::METAL);

    vectorization::metal_backend::dispatch_fill(out, 3.5f, n);

    auto host = download(out, n);
    for (float v : host)
        EXPECT_FLOAT_EQ(v, 3.5f);

    alloc_t::free(out, memory::device_enum::METAL);
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, Add)
{
    constexpr size_t   n = 300;
    std::vector<float> a(n), b(n);
    for (size_t i = 0; i < n; ++i)
    {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(2 * i);
    }

    float* da  = upload(a);
    float* db  = upload(b);
    float* dout = alloc_t::allocate(n, memory::device_enum::METAL);

    const void* ins[] = {da, db};
    vectorization::metal_backend::dispatch("add", ins, 2, dout, n);

    auto host = download(dout, n);
    for (size_t i = 0; i < n; ++i)
        EXPECT_FLOAT_EQ(host[i], a[i] + b[i]);

    alloc_t::free(da, memory::device_enum::METAL);
    alloc_t::free(db, memory::device_enum::METAL);
    alloc_t::free(dout, memory::device_enum::METAL);
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, Sin)
{
    constexpr size_t   n = 200;
    std::vector<float> a(n);
    for (size_t i = 0; i < n; ++i)
        a[i] = static_cast<float>(i) * 0.01f;

    float* da   = upload(a);
    float* dout = alloc_t::allocate(n, memory::device_enum::METAL);

    const void* ins[] = {da};
    vectorization::metal_backend::dispatch("sin", ins, 1, dout, n);

    auto host = download(dout, n);
    for (size_t i = 0; i < n; ++i)
        EXPECT_NEAR(host[i], std::sin(a[i]), 1e-5f);

    alloc_t::free(da, memory::device_enum::METAL);
    alloc_t::free(dout, memory::device_enum::METAL);
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, Fma)
{
    constexpr size_t   n = 128;
    std::vector<float> a(n), b(n), c(n);
    for (size_t i = 0; i < n; ++i)
    {
        a[i] = static_cast<float>(i) * 0.5f;
        b[i] = static_cast<float>(i) * 0.25f;
        c[i] = 1.0f;
    }

    float* da   = upload(a);
    float* db   = upload(b);
    float* dc   = upload(c);
    float* dout = alloc_t::allocate(n, memory::device_enum::METAL);

    const void* ins[] = {da, db, dc};
    vectorization::metal_backend::dispatch("fma", ins, 3, dout, n);

    auto host = download(dout, n);
    for (size_t i = 0; i < n; ++i)
        EXPECT_NEAR(host[i], a[i] * b[i] + c[i], 1e-5f);

    alloc_t::free(da, memory::device_enum::METAL);
    alloc_t::free(db, memory::device_enum::METAL);
    alloc_t::free(dc, memory::device_enum::METAL);
    alloc_t::free(dout, memory::device_enum::METAL);
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, ReduceSum)
{
    constexpr size_t   n = 512;
    std::vector<float> a(n);
    float               expected = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        a[i] = static_cast<float>(i) * 0.01f - 1.0f;
        expected += a[i];
    }

    float* da = upload(a);
    float  got = vectorization::metal_backend::reduce_sum(da, n);
    EXPECT_NEAR(got, expected, 1e-2f);

    alloc_t::free(da, memory::device_enum::METAL);
    END_TEST();
}

VECTORIZATIONTEST(MetalDispatch, ReduceSumNonPowerOfTwo)
{
    constexpr size_t   n = 300;
    std::vector<float> a(n);
    float               expected = 0.0f;
    for (size_t i = 0; i < n; ++i)
    {
        a[i] = 1.0f;
        expected += a[i];
    }

    float* da  = upload(a);
    float  got = vectorization::metal_backend::reduce_sum(da, n);
    EXPECT_NEAR(got, expected, 1e-3f);

    alloc_t::free(da, memory::device_enum::METAL);
    END_TEST();
}

#endif  // VECTORIZATION_HAS_METAL
