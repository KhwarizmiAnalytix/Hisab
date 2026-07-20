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

#pragma once

// Metal expression evaluation — the counterpart to expressions_evaluator_gpu.h for
// CUDA/HIP, but architecturally different: nvcc/hipcc compile the same templated C++
// header for host AND device, so run_gpu<E,T> can instantiate one fused kernel per
// expression-tree type E. Metal's shader compiler (MSL) is a separate toolchain with no
// such unified pass — .metal source never sees tensor<T>/the expression templates at
// all (see backend/gpu/metal/kernels.metal). Instead:
//
//   * A small, fixed set of hand-written .metal kernels covers the starter op surface
//     (fill, add/sub/mul/div/fma, sqrt/exp/log/sin/cos/tanh/fabs/neg) — see
//     backend/gpu/metal/metal_dispatch.h.
//   * This file is ordinary host C++ (compiled by clang++, not a device pass) that
//     walks an arbitrary expression tree's *type* at compile time (via function-template
//     overloading on the exact node shape — unary_expression<LHS,EVALUATOR>,
//     binary_expression<LHS,RHS,EVALUATOR>, trinary_expression<...>, or a tensor<float>
//     leaf) and lowers it to a *sequence* of fixed-kernel dispatches through temporary
//     buffers, rather than one fused kernel. No fusion; correctness over performance for
//     this starter backend (see the Metal backend design notes for the full rationale
//     and the explicit follow-up list — comparisons, gather/scatter, cdf/inv_cdf, and
//     the rest of the ~40-op simd<T> surface are not covered here).
//
// float-only: MSL has no double type on any Apple GPU. memory::allocator<double> already
// throws at allocation time for device_enum::METAL (see Library/Memory/allocator.h), so
// a tensor<double> with device()==METAL can never exist to reach this code — no
// additional guard is needed here.
//
// Compiled only when VECTORIZATION_HAS_METAL=1.

#if VECTORIZATION_HAS_METAL

#include <cstddef>
#include <stdexcept>
#include <type_traits>

#include "allocator.h"
#include "backend/gpu/metal/metal_dispatch.h"
#include "common/device.h"

namespace vectorization
{
namespace metal_detail
{
using metal_alloc_t = memory::allocator<float>;

inline float* alloc_temp(std::size_t n)
{
    return metal_alloc_t::allocate(n, memory::device_enum::METAL);
}

inline void free_temp(float*& ptr)
{
    metal_alloc_t::free(ptr, memory::device_enum::METAL);
}

// A lowered operand: either a real tensor's own storage (is_temp=false, never freed
// here — the tensor owns it) or a freshly allocated intermediate result that the
// caller must release once consumed.
struct metal_value
{
    float* ptr;
    bool   is_temp;
};

// Maps an expression-functor evaluator type (add_evaluator, sin_evaluator, ...) to the
// .metal kernel name it dispatches to.
//
// This is deliberately NOT a compile-time rejection (e.g. static_assert in the primary
// template) for unsupported ops: expressions_evaluator::run<E,T> is instantiated for
// every expression type E ever used with T=tensor<float> ANYWHERE in the codebase,
// regardless of which device a given tensor actually uses at runtime (the device()
// check inside run() is a runtime `if`, not `if constexpr`) — e.g. plain CPU code doing
// `out = min(a, b);` on tensor<float> would force-instantiate metal_kernel_traits<
// min_evaluator> even though no METAL tensor is involved. A hard compile-time error
// here would therefore break unrelated CPU-only code using an op outside the Metal
// starter set. Instead, is_supported is checked via `if constexpr` in metal_lower
// (below) and unsupported ops throw std::runtime_error only if genuinely reached at
// runtime with a METAL-device tensor.
template <typename EVALUATOR>
struct metal_kernel_traits
{
    static constexpr bool        is_supported = false;
    static constexpr const char* name         = nullptr;
};

#define VECTORIZATION_METAL_KERNEL(EVAL, NAME)          \
    template <>                                         \
    struct metal_kernel_traits<EVAL>                     \
    {                                                     \
        static constexpr bool        is_supported = true; \
        static constexpr const char* name         = NAME; \
    };

VECTORIZATION_METAL_KERNEL(add_evaluator, "add")
VECTORIZATION_METAL_KERNEL(sub_evaluator, "sub")
VECTORIZATION_METAL_KERNEL(mul_evaluator, "mul")
VECTORIZATION_METAL_KERNEL(div_evaluator, "div")
VECTORIZATION_METAL_KERNEL(fma_evaluator, "fma")
VECTORIZATION_METAL_KERNEL(sqrt_evaluator, "sqrt")
VECTORIZATION_METAL_KERNEL(exp_evaluator, "exp")
VECTORIZATION_METAL_KERNEL(log_evaluator, "log")
VECTORIZATION_METAL_KERNEL(sin_evaluator, "sin")
VECTORIZATION_METAL_KERNEL(cos_evaluator, "cos")
VECTORIZATION_METAL_KERNEL(tanh_evaluator, "tanh")
VECTORIZATION_METAL_KERNEL(fabs_evaluator, "fabs")
VECTORIZATION_METAL_KERNEL(neg_evaluator, "neg")

#undef VECTORIZATION_METAL_KERNEL

// ---------------------------------------------------------------------------
// metal_lower — recursively lowers an expression (sub)tree into a sequence of fixed
// kernel dispatches, returning the buffer holding its result. Overloaded (not
// specialized) on the exact node type so LHS/RHS/MHS/EVALUATOR fall out of normal
// template argument deduction, the same way expression_loader::evaluate dispatches
// polymorphically per node type (expression_interface_loader.h) rather than manually
// destructuring types.
// ---------------------------------------------------------------------------

// Leaf: a real tensor operand — its own storage is used directly, no copy. Templated
// (rather than a plain inline function) purely so its body — which calls tensor<float>
// member functions — is only type-checked once instantiated: this header is included
// from expressions_evaluator.h partway through expressions.h's aggregation, before
// terminals/tensor.h has defined the tensor<T> class body (only forward-declared at
// that point), so a non-template function referencing t.data() here would fail with
// "implicit instantiation of undefined template" immediately at parse time.
template <bool Clone>
metal_value metal_lower(tensor<float, Clone> const& t, std::size_t /*n*/)
{
    VECTORIZATION_CHECK_DEBUG(
        t.device() == memory::device_enum::METAL,
        "Metal expression mixes a non-METAL tensor operand");
    return {const_cast<float*>(t.data()), false};
}

// Scalar broadcast (e.g. `2.0f * a`): materialize into a temp buffer via fill, then
// treat identically to a tensor operand. Trades one extra kernel launch for zero extra
// kernel variants (no add_scalar_float etc.) — acceptable for the starter set.
template <typename S, std::enable_if_t<std::is_fundamental<S>::value, bool> = true>
metal_value metal_lower(S value, std::size_t n)
{
    float* buf = alloc_temp(n);
    metal_backend::dispatch_fill(buf, static_cast<float>(value), n);
    return {buf, true};
}

template <typename LHS, typename EVALUATOR>
metal_value metal_lower(unary_expression<LHS, EVALUATOR> const& e, std::size_t n)
{
    if constexpr (metal_kernel_traits<EVALUATOR>::is_supported)
    {
        metal_value src = metal_lower(e.rhs(), n);

        float*      dst   = alloc_temp(n);
        const void* ins[] = {src.ptr};
        metal_backend::dispatch(metal_kernel_traits<EVALUATOR>::name, ins, 1, dst, n);

        if (src.is_temp)
            free_temp(src.ptr);
        return {dst, true};
    }
    else
    {
        throw std::runtime_error(
            "Metal backend: operator has no Metal kernel (starter set only: fill, "
            "add/sub/mul/div/fma, sqrt/exp/log/sin/cos/tanh/fabs/neg)");
    }
}

template <typename LHS, typename RHS, typename EVALUATOR>
metal_value metal_lower(binary_expression<LHS, RHS, EVALUATOR> const& e, std::size_t n)
{
    if constexpr (metal_kernel_traits<EVALUATOR>::is_supported)
    {
        metal_value lhs = metal_lower(e.lhs(), n);
        metal_value rhs = metal_lower(e.rhs(), n);

        float*      dst   = alloc_temp(n);
        const void* ins[] = {lhs.ptr, rhs.ptr};
        metal_backend::dispatch(metal_kernel_traits<EVALUATOR>::name, ins, 2, dst, n);

        if (lhs.is_temp)
            free_temp(lhs.ptr);
        if (rhs.is_temp)
            free_temp(rhs.ptr);
        return {dst, true};
    }
    else
    {
        throw std::runtime_error(
            "Metal backend: operator has no Metal kernel (starter set only: fill, "
            "add/sub/mul/div/fma, sqrt/exp/log/sin/cos/tanh/fabs/neg)");
    }
}

template <typename LHS, typename MHS, typename RHS, typename EVALUATOR>
metal_value metal_lower(trinary_expression<LHS, MHS, RHS, EVALUATOR> const& e, std::size_t n)
{
    if constexpr (metal_kernel_traits<EVALUATOR>::is_supported)
    {
        metal_value lhs = metal_lower(e.lhs(), n);
        metal_value mhs = metal_lower(e.mhs(), n);
        metal_value rhs = metal_lower(e.rhs(), n);

        float*      dst   = alloc_temp(n);
        const void* ins[] = {lhs.ptr, mhs.ptr, rhs.ptr};
        metal_backend::dispatch(metal_kernel_traits<EVALUATOR>::name, ins, 3, dst, n);

        if (lhs.is_temp)
            free_temp(lhs.ptr);
        if (mhs.is_temp)
            free_temp(mhs.ptr);
        if (rhs.is_temp)
            free_temp(rhs.ptr);
        return {dst, true};
    }
    else
    {
        throw std::runtime_error(
            "Metal backend: operator has no Metal kernel (starter set only: fill, "
            "add/sub/mul/div/fma, sqrt/exp/log/sin/cos/tanh/fabs/neg)");
    }
}

}  // namespace metal_detail

// ---------------------------------------------------------------------------
// Root entry points, called from expressions_evaluator.h — mirrors run_gpu/fill_gpu's
// signatures (expressions_evaluator_gpu.h) so the two backends plug into run()/fill()
// the same way.
// ---------------------------------------------------------------------------

template <typename E, typename T>
void run_metal(E const& expr, T& rhs)
{
    using RE                  = vectorization::remove_cvref_t<E>;
    const std::size_t n       = rhs.size();

    if constexpr (vectorization::is_base_expression<RE>::value)
    {
        // Plain tensor-to-tensor assignment (`c = a;`) — direct copy, no kernel needed.
        metal_detail::metal_alloc_t::copy(
            expr.data(), n, rhs.data(), memory::device_enum::METAL, memory::device_enum::METAL);
    }
    else
    {
        static_assert(
            vectorization::is_pure_expression<RE>::value,
            "run_metal: expression is neither a tensor leaf nor a unary/binary/trinary node");

        metal_detail::metal_value result = metal_detail::metal_lower(expr, n);
        metal_detail::metal_alloc_t::copy(
            result.ptr, n, rhs.data(), memory::device_enum::METAL, memory::device_enum::METAL);
        if (result.is_temp)
            metal_detail::free_temp(result.ptr);
    }
}

template <typename T>
void fill_metal(T& rhs, float value)
{
    metal_backend::dispatch_fill(rhs.data(), value, rhs.size());
}

}  // namespace vectorization

#endif  // VECTORIZATION_HAS_METAL
