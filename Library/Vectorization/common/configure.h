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

// =============================================================================
// configure.h — bridge between CMake VECTORIZATION_HAS_* compile definitions
// and the C++ preprocessor.
//
// CMake emits -DVECTORIZATION_HAS_SSE=1/0 (and likewise for AVX/AVX2/AVX512/SVML).
// Code that uses "#if defined(VECTORIZATION_HAS_X)" relies on the macro being
// absent when the feature is off, so this header normalises them: any HAS_* that
// CMake set to 0 is undefined here, making "#if defined(...)" checks reliable.
//
// VECTORIZATION_VECTORIZED is then defined (empty) when at least one SIMD tier
// is active — this is the single token that gates intrinsic inclusion in intrin.h.
// =============================================================================

// --- Normalise HAS_* from value-based to presence-based ---
// After this block, HAS_X is defined iff it is actively enabled (value == 1).
#if defined(VECTORIZATION_HAS_SSE) && !VECTORIZATION_HAS_SSE
#  undef VECTORIZATION_HAS_SSE
#endif

#if defined(VECTORIZATION_HAS_AVX) && !VECTORIZATION_HAS_AVX
#  undef VECTORIZATION_HAS_AVX
#endif

#if defined(VECTORIZATION_HAS_AVX2) && !VECTORIZATION_HAS_AVX2
#  undef VECTORIZATION_HAS_AVX2
#endif

#if defined(VECTORIZATION_HAS_AVX512) && !VECTORIZATION_HAS_AVX512
#  undef VECTORIZATION_HAS_AVX512
#endif

#if defined(VECTORIZATION_HAS_SVML) && !VECTORIZATION_HAS_SVML
#  undef VECTORIZATION_HAS_SVML
#endif

// --- VECTORIZATION_VECTORIZED ---
// Defined (with no value) when at least one SIMD tier is active.
#if defined(VECTORIZATION_HAS_AVX512) || defined(VECTORIZATION_HAS_AVX2) \
    || defined(VECTORIZATION_HAS_AVX) || defined(VECTORIZATION_HAS_SSE)
#  define VECTORIZATION_VECTORIZED
#endif

// --- Compiler identification ---
// __VECTORIZATION_COMPILER_MSVC__ is used by avx512/svml.h for MSC version guards.
// __VECTORIZATION_COMPILER_PGI__  is used by intrin.h for PREFETCH_PTR_TYPE.
#if defined(_MSC_VER) && !defined(__clang__)
#  define __VECTORIZATION_COMPILER_MSVC__ 1
#else
#  define __VECTORIZATION_COMPILER_MSVC__ 0
#endif

#if defined(__PGI)
#  define __VECTORIZATION_COMPILER_PGI__ 1
#else
#  define __VECTORIZATION_COMPILER_PGI__ 0
#endif
