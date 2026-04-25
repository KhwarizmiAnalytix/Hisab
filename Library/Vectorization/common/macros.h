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

// =============================================================================
// macros.h — portable compiler-attribute and utility macros for the
// Vectorization library.
//
// All macros are prefixed VECTORIZATION_ to avoid clashing with other
// modules. configure.h must be included before this header so that compiler
// identification macros (__VECTORIZATION_COMPILER_MSVC__ etc.) are available.
// =============================================================================

// -----------------------------------------------------------------------------
// VECTORIZATION_SIMD_RETURN_TYPE
// Member SIMD functions return results through out-parameter references
// (e.g. `simd_t& ret`) rather than by value, so the return type is void.
// -----------------------------------------------------------------------------
#define VECTORIZATION_SIMD_RETURN_TYPE void

// -----------------------------------------------------------------------------
// VECTORIZATION_FORCE_INLINE
// Requests the compiler to always inline a function regardless of its size.
// -----------------------------------------------------------------------------
#if defined(_MSC_VER)
#  define VECTORIZATION_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define VECTORIZATION_FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define VECTORIZATION_FORCE_INLINE inline
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_VECTORCALL
// MSVC __vectorcall passes SIMD vector arguments in registers, reducing
// copies for short SVML wrapper functions. Empty on all other toolchains.
// -----------------------------------------------------------------------------
#if defined(_MSC_VER) && !defined(__clang__)
#  define VECTORIZATION_VECTORCALL __vectorcall
#else
#  define VECTORIZATION_VECTORCALL
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_FUNCTION_ATTRIBUTE
// Applied to inline helper methods in class bodies.
// -----------------------------------------------------------------------------
#define VECTORIZATION_FUNCTION_ATTRIBUTE inline

// -----------------------------------------------------------------------------
// VECTORIZATION_CUDA_FUNCTION_TYPE
// Marks host/device functions when compiling with nvcc / hipcc.
// Expands to nothing in pure CPU builds.
// -----------------------------------------------------------------------------
#if defined(__CUDACC__) || defined(__HIPCC__)
#  define VECTORIZATION_CUDA_FUNCTION_TYPE __host__ __device__
#else
#  define VECTORIZATION_CUDA_FUNCTION_TYPE
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_NODISCARD / VECTORIZATION_UNUSED
// -----------------------------------------------------------------------------
#if __cplusplus >= 201703L
#  define VECTORIZATION_NODISCARD [[nodiscard]]
#  define VECTORIZATION_UNUSED    [[maybe_unused]]
#else
#  define VECTORIZATION_NODISCARD
#  define VECTORIZATION_UNUSED
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_ALIGN(n) — placement alignment specifier
// VECTORIZATION_ALIGNMENT — default alignment in bytes (64 = cache line / AVX-512)
// -----------------------------------------------------------------------------
#define VECTORIZATION_ALIGN(n) alignas(n)

#ifdef VECTORIZATION_MOBILE
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 16;
#else
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 64;
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_CHECK(cond, msg)       — always-on assertion
// VECTORIZATION_CHECK_DEBUG(cond, msg) — debug-only assertion (NDEBUG suppresses)
// -----------------------------------------------------------------------------
#define VECTORIZATION_CHECK(cond, msg)       assert((cond) && (msg))

#ifdef NDEBUG
#  define VECTORIZATION_CHECK_DEBUG(cond, msg) ((void)0)
#else
#  define VECTORIZATION_CHECK_DEBUG(cond, msg) assert((cond) && (msg))
#endif

// -----------------------------------------------------------------------------
// VECTORIZATION_LOGF(fmt, ...) — lightweight diagnostic logging
// Routes to printf in debug builds; suppressed in release.
// -----------------------------------------------------------------------------
#ifndef NDEBUG
#  include <cstdio>
#  define VECTORIZATION_LOGF(fmt, ...) std::printf("[VECTORIZATION] " fmt "\n", ##__VA_ARGS__)
#else
#  define VECTORIZATION_LOGF(fmt, ...) ((void)0)
#endif
