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

#include <cassert>
#include <cstddef>

#if defined(_MSC_VER)
#define VECTORIZATION_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define VECTORIZATION_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define VECTORIZATION_FORCE_INLINE inline
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define VECTORIZATION_VECTORCALL __vectorcall
#else
#define VECTORIZATION_VECTORCALL
#endif

//------------------------------------------------------------------------
#ifdef NDEBUG
#define VECTORIZATION_SIMD_RETURN_TYPE VECTORIZATION_FORCE_INLINE static void VECTORIZATION_VECTORCALL
#else
#define VECTORIZATION_SIMD_RETURN_TYPE static void
#endif

#if defined(__CUDACC__) || defined(__HIPCC__)
#define VECTORIZATION_CUDA_FUNCTION_TYPE __host__ __device__
#else
#define VECTORIZATION_CUDA_FUNCTION_TYPE
#endif

#ifdef NDEBUG
#define VECTORIZATION_FUNCTION_ATTRIBUTE VECTORIZATION_FORCE_INLINE VECTORIZATION_CUDA_FUNCTION_TYPE
#else
#define VECTORIZATION_FUNCTION_ATTRIBUTE VECTORIZATION_CUDA_FUNCTION_TYPE
#endif

#if __cplusplus >= 201703L
#define VECTORIZATION_NODISCARD [[nodiscard]]
#define VECTORIZATION_UNUSED [[maybe_unused]]
#else
#define VECTORIZATION_NODISCARD
#define VECTORIZATION_UNUSED
#endif

#define VECTORIZATION_ALIGN(n) alignas(n)

#ifdef VECTORIZATION_MOBILE
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 16;
#else
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 64;
#endif

#define VECTORIZATION_CHECK(cond, msg) assert((cond) && (msg))

#ifdef NDEBUG
#define VECTORIZATION_CHECK_DEBUG(cond, msg) ((void)0)
#else
#define VECTORIZATION_CHECK_DEBUG(cond, msg) assert((cond) && (msg))
#endif

#ifndef NDEBUG
#include <cstdio>
#define VECTORIZATION_LOGF(fmt, ...) std::printf("[VECTORIZATION] " fmt "\n", ##__VA_ARGS__)
#else
#define VECTORIZATION_LOGF(fmt, ...) ((void)0)
#endif

// Memory integration — pulls in allocator, data_ptr, and device_enum when the Memory library is
// present, and aliases them into namespace vectorization so terminal headers can use them unqualified.
#if VECTORIZATION_HAS_MEMORY
#  include "allocator.h"
#  include "common/data_ptr.h"
#  include "common/device.h"
namespace vectorization
{
using memory::allocator;
using memory::data_ptr;
using memory::device_enum;
}  // namespace vectorization
#endif
