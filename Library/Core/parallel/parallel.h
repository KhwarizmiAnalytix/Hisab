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
 *
 * Portions of this code are based on VTK (Visualization Toolkit):

 *   Licensed under BSD-3-Clause
 */

/**
 * @file parallel.h
 * @brief PARALLEL (Shared Memory Parallelism) configuration and backend definitions
 */

#ifndef PARALLEL_H
#define PARALLEL_H

// Maximum number of backends
#define PARALLEL_MAX_BACKENDS_NB 3

namespace quarisma
{
namespace detail
{
namespace parallel
{

/**
 * Backend types available for SMP execution
 */
enum class backend_type
{
    std_thread = 0,
    TBB        = 1,
    OpenMP     = 2
};

// Default backend selection
// Uses QUARISMA_HAS_TBB and QUARISMA_HAS_OPENMP which are defined by CMake
// as compile definitions with explicit values (1 or 0)
#if QUARISMA_HAS_TBB
constexpr backend_type default_backend = backend_type::TBB;
#elif QUARISMA_HAS_OPENMP
constexpr backend_type default_backend = backend_type::OpenMP;
#else
constexpr backend_type default_backend = backend_type::std_thread;
#endif

}  // namespace parallel
}  // namespace detail
}  // namespace quarisma

#endif  // PARALLEL_H
