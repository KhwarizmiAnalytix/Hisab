/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Packet tests: two-argument (binary) packet operations vs scalar std/math.
 */

#include "VectorizationTest.h"
#if VECTORIZATION_VECTORIZED

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>

#include "common/packet.h"
#include "common/vectorization_macros.h"

namespace vectorization
{
namespace packet_tests
{

#define XSIGMA_PACKET_BINARY_TAG(Scalar, name) \
    ((std::is_same_v<Scalar, float>) ? (#name ".f32") : (#name ".f64"))

inline constexpr int kRandomTrials = 8;

template <typename value_t>
double scalar_as_double(value_t x)
{
    return static_cast<double>(x);
}

template <typename value_t, std::size_t N>
void fill_uniform(std::array<value_t, N>& xs, std::mt19937& gen, value_t lo, value_t hi)
{
    std::uniform_real_distribution<value_t> dist(lo, hi);
    for (auto& x : xs)
        x = dist(gen);
}

template <typename value_t, std::size_t N>
void fill_binary_div_safe(std::array<value_t, N>& xs, std::array<value_t, N>& ys, std::mt19937& gen)
{
    std::uniform_real_distribution<value_t> wide(
        static_cast<value_t>(-50), static_cast<value_t>(50));
    value_t const guard = std::max(
        static_cast<value_t>(1e-3),
        std::numeric_limits<value_t>::epsilon() * static_cast<value_t>(1024));
    for (std::size_t i = 0; i < N; ++i)
    {
        xs[i]     = wide(gen);
        value_t y = wide(gen);
        while (std::fabs(y) < guard)
            y = wide(gen);
        ys[i] = y;
    }
}

template <typename value_t, typename FillXY, typename PacketOp, typename RefOp>
void binary_random_vs_std(
    char const* case_tag, value_t tolerance, FillXY&& fill_xy, PacketOp&& packet_op, RefOp&& ref_op)
{
    constexpr std::size_t n = packet<value_t>::length();
    using packet_t          = packet<value_t>;
    using array_simd_t      = typename packet_t::array_simd_t;

    std::array<value_t, n> xs{};
    std::array<value_t, n> ys{};
    std::array<value_t, n> out{};
    std::mt19937           gen(5489u);

    for (int trial = 0; trial < kRandomTrials; ++trial)
    {
        SCOPED_TRACE(
            ::testing::Message() << case_tag << " trial=" << trial << " (binary packet vs std)");
        fill_xy(xs, ys, gen);
        array_simd_t a;
        array_simd_t b;
        array_simd_t c;
        packet_t::loadu(xs.data(), a);
        packet_t::loadu(ys.data(), b);
        packet_t::setzero(c);
        packet_op(a, b, c);
        packet_t::storeu(c, out.data());
        double err = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            value_t const ref = ref_op(xs[i], ys[i]);
            err = std::max<double>(err, std::fabs(out[i] - ref));
        }
        EXPECT_LE(err, tolerance);
    }
}

template <typename value_t, typename PacketBinary, typename RefBinary>
void binary_uniform_boxes_vs_std(
    char const*    case_tag,
    value_t        tolerance,
    value_t        lo1,
    value_t        hi1,
    value_t        lo2,
    value_t        hi2,
    PacketBinary&& packet_op,
    RefBinary&&    ref_op)
{
    binary_random_vs_std<value_t>(
        case_tag,
        tolerance,
        [=](std::array<value_t, packet<value_t>::length()>& xs,
            std::array<value_t, packet<value_t>::length()>& ys,
            std::mt19937&                                   gen)
        {
            fill_uniform(xs, gen, lo1, hi1);
            fill_uniform(ys, gen, lo2, hi2);
        },
        std::forward<PacketBinary>(packet_op),
        std::forward<RefBinary>(ref_op));
}

template <typename value_t>
void test_add(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, add),
        tolerance,
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::add(a, b, c); },
        [](value_t x, value_t y) { return std::add(x, y); });
}

template <typename value_t>
void test_sub(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, sub),
        tolerance,
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::sub(a, b, c); },
        [](value_t x, value_t y) { return std::sub(x, y); });
}

template <typename value_t>
void test_mul(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, mul),
        tolerance,
        static_cast<value_t>(-25),
        static_cast<value_t>(25),
        static_cast<value_t>(-25),
        static_cast<value_t>(25),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::mul(a, b, c); },
        [](value_t x, value_t y) { return std::mul(x, y); });
}

template <typename value_t>
void test_min(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, min),
        tolerance,
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::min(a, b, c); },
        [](value_t x, value_t y) { return std::min(x, y); });
}

template <typename value_t>
void test_max(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, max),
        tolerance,
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        static_cast<value_t>(-40),
        static_cast<value_t>(40),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::max(a, b, c); },
        [](value_t x, value_t y) { return std::max(x, y); });
}

template <typename value_t>
void test_hypot(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, hypot),
        tolerance,
        static_cast<value_t>(-80),
        static_cast<value_t>(80),
        static_cast<value_t>(-80),
        static_cast<value_t>(80),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::hypot(a, b, c); },
        [](value_t x, value_t y) { return std::hypot(x, y); });
}

template <typename value_t>
void test_signcopy(value_t tolerance)
{
    using packet_t     = packet<value_t>;
    using array_simd_t = typename packet_t::array_simd_t;
    binary_uniform_boxes_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, signcopy),
        tolerance,
        static_cast<value_t>(-80),
        static_cast<value_t>(80),
        static_cast<value_t>(-80),
        static_cast<value_t>(80),
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::signcopy(a, b, c); },
        [](value_t x, value_t y) { return std::signcopy(x, y); });
}

template <typename value_t>
void test_div(value_t tolerance)
{
    using packet_t        = packet<value_t>;
    using array_simd_t    = typename packet_t::array_simd_t;
    value_t const div_tol = tolerance * static_cast<value_t>(2);
    binary_random_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, div),
        div_tol,
        [](std::array<value_t, packet<value_t>::length()>& xs,
           std::array<value_t, packet<value_t>::length()>& ys,
           std::mt19937& gen) { fill_binary_div_safe(xs, ys, gen); },
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::div(a, b, c); },
        [](value_t x, value_t y) { return std::div(x, y); });
}

template <typename value_t>
void test_pow(value_t tolerance)
{
    using packet_t        = packet<value_t>;
    using array_simd_t    = typename packet_t::array_simd_t;
    value_t const pow_tol = tolerance * static_cast<value_t>(2);
    binary_random_vs_std<value_t>(
        XSIGMA_PACKET_BINARY_TAG(value_t, pow),
        pow_tol,
        [](std::array<value_t, packet<value_t>::length()>& xs,
           std::array<value_t, packet<value_t>::length()>& ys,
           std::mt19937&                                   gen)
        {
            fill_uniform(xs, gen, static_cast<value_t>(0.05), static_cast<value_t>(12));
            fill_uniform(ys, gen, static_cast<value_t>(-4), static_cast<value_t>(4));
        },
        [](array_simd_t const& a, array_simd_t const& b, array_simd_t& c)
        { packet_t::pow(a, b, c); },
        [](value_t x, value_t y) { return std::pow(x, y); });
}

template <typename value_t>
void test_all_packet_binary(value_t tolerance)
{
    test_add(tolerance);
    test_sub(tolerance);
    test_mul(tolerance);
    test_div(tolerance);
    test_min(tolerance);
    test_max(tolerance);
    //test_pow(tolerance);
    test_hypot(tolerance);
    test_signcopy(tolerance);
}

#undef XSIGMA_PACKET_BINARY_TAG

}  // namespace packet_tests
}  // namespace vectorization

VECTORIZATIONTEST(Math, PacketF2)
{
    using namespace vectorization::packet_tests;
    constexpr float  kPacketTolF = 0.0007f;
    constexpr double kPacketTolD = 5.e-12;
    test_all_packet_binary<float>(kPacketTolF);
    test_all_packet_binary<double>(kPacketTolD);
    END_TEST();
}
#else
VECTORIZATIONTEST(Math, PacketF2)
{
    END_TEST();
}

#endif  // VECTORIZATION_VECTORIZED
