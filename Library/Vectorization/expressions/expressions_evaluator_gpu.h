/*
 * XSigma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#pragma once

// GPU expression evaluation — supports both CUDA (nvcc) and HIP (hipcc).
//
// Compiled only when VECTORIZATION_HAS_CUDA=1 (nvcc) or VECTORIZATION_HAS_HIP=1 (hipcc).
//
// Provides:
//   gpu_eval_kernel<E,T>   — __global__ kernel: one thread per element
//   gpu_fill_kernel<T>     — __global__ kernel for fill
//   run_gpu<E,T>           — host launcher for gpu_eval_kernel
//   fill_gpu<T>            — host launcher for gpu_fill_kernel
//
// Kernel bodies are identical for CUDA and HIP; only the launch syntax differs
// (<<<>>> for CUDA, hipLaunchKernelGGL for HIP).
//
// All GPU tensor pointers in the expression tree must be valid device pointers
// (allocated with device_enum::CUDA).  The expression struct is passed by value
// to the kernel so captured pointers are copied to device.

#if VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP

#if VECTORIZATION_HAS_CUDA && defined(__CUDACC__)
#include <cuda_runtime.h>
#elif VECTORIZATION_HAS_HIP && defined(__HIPCC__)
#include <hip/hip_runtime.h>
#endif

#include "expressions/expression_interface_loader.h"

namespace vectorization
{

// ---------------------------------------------------------------------------
// gpu_stream_t — platform-neutral stream handle alias
// ---------------------------------------------------------------------------
#if VECTORIZATION_HAS_CUDA && defined(__CUDACC__)
using gpu_stream_t = cudaStream_t;
#elif VECTORIZATION_HAS_HIP && defined(__HIPCC__)
using gpu_stream_t = hipStream_t;
#else
// Host-only compilation unit: provide a dummy type so signatures compile.
using gpu_stream_t = void*;
#endif

// ---------------------------------------------------------------------------
// __global__ kernels — shared body between CUDA and HIP
// ---------------------------------------------------------------------------
#if (VECTORIZATION_HAS_CUDA && defined(__CUDACC__)) || (VECTORIZATION_HAS_HIP && defined(__HIPCC__))

template <typename E, typename T>
__global__ void gpu_eval_kernel(E expr, T* __restrict__ out, size_t n)
{
    const size_t tid = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (tid < n)
        out[tid] = expression_loader<E, false, false>::evaluate(expr, tid);
}

template <typename T>
__global__ void gpu_fill_kernel(T* __restrict__ out, T value, size_t n)
{
    const size_t tid = static_cast<size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (tid < n)
        out[tid] = value;
}

// ---------------------------------------------------------------------------
// Host launchers — differ only in the kernel-launch syntax
// ---------------------------------------------------------------------------

template <typename E, typename T>
void run_gpu(E const& expr, T* data, size_t n, gpu_stream_t stream = nullptr)
{
    if (n == 0)
        return;
    constexpr unsigned block_size = 256;
    const unsigned     grid_size  = static_cast<unsigned>((n + block_size - 1) / block_size);

#if VECTORIZATION_HAS_CUDA && defined(__CUDACC__)
    gpu_eval_kernel<E, T><<<dim3(grid_size), dim3(block_size), 0, stream>>>(expr, data, n);
#else
    hipLaunchKernelGGL(
        (gpu_eval_kernel<E, T>), dim3(grid_size), dim3(block_size), 0, stream, expr, data, n);
#endif
}

template <typename T>
void fill_gpu(T* data, T value, size_t n, gpu_stream_t stream = nullptr)
{
    if (n == 0)
        return;
    constexpr unsigned block_size = 256;
    const unsigned     grid_size  = static_cast<unsigned>((n + block_size - 1) / block_size);

#if VECTORIZATION_HAS_CUDA && defined(__CUDACC__)
    gpu_fill_kernel<T><<<dim3(grid_size), dim3(block_size), 0, stream>>>(data, value, n);
#else
    hipLaunchKernelGGL(
        (gpu_fill_kernel<T>), dim3(grid_size), dim3(block_size), 0, stream, data, value, n);
#endif
}

#endif  // CUDACC || HIPCC

}  // namespace vectorization

#endif  // VECTORIZATION_HAS_CUDA || VECTORIZATION_HAS_HIP
