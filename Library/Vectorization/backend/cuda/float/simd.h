/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#pragma once

// CUDA GPU scalar backend for float.
// The simd<float> specialisation is identical to the HIP one: both compilers
// expose the same __host__ __device__ device-math builtins (sinf, expf,
// erfinvf, ...).  Reuse the HIP header rather than duplicating it.

#include "backend/hip/float/simd.h"
