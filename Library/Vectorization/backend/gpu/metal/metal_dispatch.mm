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

#include "backend/gpu/metal/metal_dispatch.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "allocator.h"
#include "backend/gpu/metal/metal_kernels_source.h"
#include "common/device.h"
#include "common/vectorization_macros.h"
#include "gpu/metal/metal_buffer_allocator.h"

#if VECTORIZATION_HAS_PROFILER
#include "native/gpu/gpu_tracer.h"
#endif

namespace vectorization::metal_backend
{
namespace
{
id<MTLDevice> device()
{
    static id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
    return dev;
}

id<MTLCommandQueue> command_queue()
{
    static id<MTLCommandQueue> q = [device() newCommandQueue];
    return q;
}

// Compiled once, lazily, from the embedded kernels.metal source (kMetalKernelSource).
id<MTLLibrary> library()
{
    static id<MTLLibrary> lib = []
    {
        NSString*      src = [NSString stringWithUTF8String:kMetalKernelSource];
        NSError*       err = nil;
        id<MTLLibrary> l   = [device() newLibraryWithSource:src options:nil error:&err];
        if (l == nil)
        {
            throw std::runtime_error(
                "Metal kernel library compile failed: " +
                std::string([[err localizedDescription] UTF8String]));
        }
        return l;
    }();
    return lib;
}

std::mutex& pipeline_cache_mutex()
{
    static std::mutex m;
    return m;
}
std::unordered_map<std::string, id<MTLComputePipelineState>>& pipeline_cache()
{
    static std::unordered_map<std::string, id<MTLComputePipelineState>> cache;
    return cache;
}

id<MTLComputePipelineState> pipeline_for(const std::string& function_name)
{
    std::lock_guard<std::mutex> lock(pipeline_cache_mutex());
    auto                        it = pipeline_cache().find(function_name);
    if (it != pipeline_cache().end())
    {
        return it->second;
    }

    NSString*       ns_name = [NSString stringWithUTF8String:function_name.c_str()];
    id<MTLFunction> fn      = [library() newFunctionWithName:ns_name];
    if (fn == nil)
    {
        throw std::runtime_error("Metal kernel function not found: " + function_name);
    }

    NSError*                    err = nil;
    id<MTLComputePipelineState> pso = [device() newComputePipelineStateWithFunction:fn error:&err];
    if (pso == nil)
    {
        throw std::runtime_error(
            "Metal pipeline creation failed for " + function_name + ": " +
            std::string([[err localizedDescription] UTF8String]));
    }

    pipeline_cache().emplace(function_name, pso);
    return pso;
}

id<MTLBuffer> buffer_for(const void* host_ptr)
{
    void* handle = memory::metal::mtl_buffer_handle(const_cast<void*>(host_ptr));
    if (handle == nullptr)
    {
        throw std::invalid_argument("Metal dispatch: pointer is not a tracked METAL allocation");
    }
    return (__bridge id<MTLBuffer>)handle;
}

NSUInteger buffer_offset_for(const void* host_ptr)
{
    return static_cast<NSUInteger>(memory::metal::mtl_buffer_offset(const_cast<void*>(host_ptr)));
}

std::size_t next_pow2(std::size_t n)
{
    std::size_t p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

void record_completed_command_buffer(
    VECTORIZATION_UNUSED std::string_view name, VECTORIZATION_UNUSED id<MTLCommandBuffer> cb)
{
#if VECTORIZATION_HAS_PROFILER
    if (!profiler::profiler_impl::gpu_tracer_is_recording())
    {
        return;
    }
    if (@available(macOS 10.15, *))
    {
        const CFTimeInterval gpu_start = cb.GPUStartTime;
        const CFTimeInterval gpu_end   = cb.GPUEndTime;
        if (gpu_end <= gpu_start)
        {
            return;
        }
        profiler::profiler_impl::gpu_tracer_event event;
        event.type          = profiler::profiler_impl::gpu_tracer_event_type::kernel;
        event.name          = std::string(name);
        event.start_time_ns = static_cast<uint64_t>(gpu_start * 1e9);
        event.end_time_ns   = static_cast<uint64_t>(gpu_end * 1e9);
        profiler::profiler_impl::add_gpu_tracer_event(std::move(event));
    }
#endif
}

// Dispatches `pso` with `n` threads, `threadsPerThreadgroup` capped at the pipeline's
// max — synchronous (waits for completion before returning; no stream/async support
// in the starter backend, see the Metal backend design notes).
void run(
    id<MTLComputePipelineState> pso,
    void (^encode_args)(id<MTLComputeCommandEncoder>),
    std::size_t      n,
    std::string_view event_name)
{
    id<MTLCommandBuffer>         cb  = [command_queue() commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
    [enc setComputePipelineState:pso];
    encode_args(enc);

    NSUInteger tg_size = pso.maxTotalThreadsPerThreadgroup;
    if (tg_size > n)
    {
        tg_size = n;
    }
    if (tg_size == 0)
    {
        tg_size = 1;
    }
    [enc dispatchThreads:MTLSizeMake(n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg_size, 1, 1)];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];

    if (cb.status == MTLCommandBufferStatusError)
    {
        throw std::runtime_error(
            "Metal kernel dispatch failed: " +
            std::string([[cb.error localizedDescription] UTF8String]));
    }
    record_completed_command_buffer(event_name, cb);
}

}  // namespace

bool device_available()
{
    return device() != nil;
}

void dispatch(
    const char*        kernel_name,
    const void* const* in_buffers,
    int                n_in,
    void*              out_buffer,
    std::size_t        n_elems)
{
    if (n_elems == 0)
    {
        return;
    }

    std::string                 function_name = std::string(kernel_name) + "_float";
    id<MTLComputePipelineState> pso           = pipeline_for(function_name);

    uint32_t n32 = static_cast<uint32_t>(n_elems);

    run(
        pso,
        ^(id<MTLComputeCommandEncoder> enc) {
          for (int i = 0; i < n_in; ++i)
          {
              [enc setBuffer:buffer_for(in_buffers[i])
                      offset:buffer_offset_for(in_buffers[i])
                     atIndex:i];
          }
          [enc setBuffer:buffer_for(out_buffer) offset:buffer_offset_for(out_buffer) atIndex:n_in];
          [enc setBytes:&n32 length:sizeof(n32) atIndex:n_in + 1];
        },
        n_elems,
        function_name);
}

void dispatch_fill(void* out_buffer, float value, std::size_t n_elems)
{
    if (n_elems == 0)
    {
        return;
    }

    id<MTLComputePipelineState> pso = pipeline_for("fill_float");
    uint32_t                    n32 = static_cast<uint32_t>(n_elems);

    run(
        pso,
        ^(id<MTLComputeCommandEncoder> enc) {
          [enc setBuffer:buffer_for(out_buffer) offset:buffer_offset_for(out_buffer) atIndex:0];
          [enc setBytes:&value length:sizeof(value) atIndex:1];
          [enc setBytes:&n32 length:sizeof(n32) atIndex:2];
        },
        n_elems,
        "fill_float");
}

float reduce_sum(const void* buffer, std::size_t n_elems)
{
    if (n_elems == 0)
    {
        return 0.0f;
    }

    id<MTLComputePipelineState> pso     = pipeline_for("reduce_sum_float");
    std::size_t                 tg_size = next_pow2(n_elems);
    if (tg_size > pso.maxTotalThreadsPerThreadgroup)
    {
        throw std::invalid_argument(
            "Metal reduce_sum: n_elems (" + std::to_string(n_elems) +
            ") exceeds this device's single-threadgroup reduction limit (" +
            std::to_string(pso.maxTotalThreadsPerThreadgroup) +
            ") — this is a single-threadgroup reduction only, see kernels.metal");
    }

    // One-element shared-storage output buffer, read back directly via its host pointer
    // (MTLResourceStorageModeShared — no explicit device->host copy needed).
    using metal_alloc_t = memory::allocator<float>;
    float* out_ptr      = metal_alloc_t::allocate(1, memory::device_enum::METAL);

    uint32_t n32 = static_cast<uint32_t>(n_elems);

    id<MTLCommandBuffer>         cb  = [command_queue() commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
    [enc setComputePipelineState:pso];
    [enc setBuffer:buffer_for(buffer) offset:buffer_offset_for(buffer) atIndex:0];
    [enc setBuffer:buffer_for(out_ptr) offset:buffer_offset_for(out_ptr) atIndex:1];
    [enc setBytes:&n32 length:sizeof(n32) atIndex:2];
    [enc setThreadgroupMemoryLength:(tg_size * sizeof(float)) atIndex:0];
    // Exactly one threadgroup, sized to cover the whole (small, fixed-N) input — see the
    // design note above reduce_sum_float in kernels.metal.
    [enc dispatchThreadgroups:MTLSizeMake(1, 1, 1)
        threadsPerThreadgroup:MTLSizeMake(tg_size, 1, 1)];
    [enc endEncoding];
    [cb commit];
    [cb waitUntilCompleted];

    if (cb.status == MTLCommandBufferStatusError)
    {
        metal_alloc_t::free(out_ptr, memory::device_enum::METAL);
        throw std::runtime_error(
            "Metal reduce_sum dispatch failed: " +
            std::string([[cb.error localizedDescription] UTF8String]));
    }
    record_completed_command_buffer("reduce_sum_float", cb);

    float result = *out_ptr;
    metal_alloc_t::free(out_ptr, memory::device_enum::METAL);
    return result;
}

}  // namespace vectorization::metal_backend
