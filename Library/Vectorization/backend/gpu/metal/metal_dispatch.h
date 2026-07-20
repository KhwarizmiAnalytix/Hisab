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

// Plain C++ surface over the fixed Metal kernel set (backend/gpu/metal/kernels.metal) —
// no Objective-C types cross this boundary, so expressions_evaluator_metal.h (header-only,
// compiled by ordinary clang++) can call into it without becoming Objective-C++ itself.
//
// in_buffers/out_buffer are raw host pointers previously returned by
// memory::allocator<float>::allocate(..., device_enum::METAL) — i.e. an MTLBuffer's
// `.contents` under MTLResourceStorageModeShared. dispatch() resolves them back to their
// owning id<MTLBuffer> via memory::metal::mtl_buffer_handle() before binding them as
// kernel arguments.

#include <cstddef>

namespace vectorization::metal_backend
{
bool device_available();

// Dispatches the kernel named kernel_name (e.g. "add" -> "add_float") over n_elems
// threads. in_buffers holds n_in input pointers, consumed in the same order the
// corresponding .metal kernel declares its [[buffer(k)]] arguments. Synchronous
// (waits for GPU completion before returning).
void dispatch(
    const char* kernel_name, const void* const* in_buffers, int n_in, void* out_buffer,
    std::size_t n_elems);

void dispatch_fill(void* out_buffer, float value, std::size_t n_elems);

// Single-threadgroup reduction (sum). Requires n_elems <= the device's
// maxTotalThreadsPerThreadgroup (throws std::invalid_argument otherwise) — this is not
// a general multi-block reduction, only enough to back a small fixed-N benchmark; see
// the design note above reduce_sum_float in kernels.metal.
float reduce_sum(const void* buffer, std::size_t n_elems);

}  // namespace vectorization::metal_backend
