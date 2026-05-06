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

#ifndef VECTORIZATION_FORCE_INLINE
#if defined(_MSC_VER)
#define VECTORIZATION_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define VECTORIZATION_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define VECTORIZATION_FORCE_INLINE inline
#endif
#endif

#if defined(_MSC_VER)
#define VECTORIZATION_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define VECTORIZATION_NOINLINE __attribute__((noinline))
#else
#define VECTORIZATION_NOINLINE
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define VECTORIZATION_VECTORCALL __vectorcall
#else
#define VECTORIZATION_VECTORCALL
#endif

#if defined(__CUDACC__) || defined(__HIPCC__)
#define VECTORIZATION_CUDA_FUNCTION_TYPE __host__ __device__
#else
#define VECTORIZATION_CUDA_FUNCTION_TYPE
#endif
//------------------------------------------------------------------------
// VECTORIZATION_SIMD_RETURN_TYPE marks packet<> static methods.
// The CUDA_FUNCTION_TYPE qualifier makes them __host__ __device__ when
// compiling with NVCC/HIPCC so the methods are callable from both host
// and device contexts (needed when the packet type is instantiated inside
// a .cu translation unit even if the SIMD path is inactive on the device).
#ifdef NDEBUG
#define VECTORIZATION_SIMD_RETURN_TYPE \
    VECTORIZATION_FORCE_INLINE VECTORIZATION_CUDA_FUNCTION_TYPE static void VECTORIZATION_VECTORCALL
#else
#define VECTORIZATION_SIMD_RETURN_TYPE VECTORIZATION_CUDA_FUNCTION_TYPE static void
#endif


// True when compiling the device-side pass (GPU kernel body).
// Safe to use inside __host__ __device__ functions to select device-only paths.
#if defined(__CUDA_ARCH__) || defined(__HIP_DEVICE_COMPILE__)
#define VECTORIZATION_ON_GPU_DEVICE 1
#else
#define VECTORIZATION_ON_GPU_DEVICE 0
#endif

// packet::pow: force noinline on host; omit on GPU device pass (noinline + __device__ is ill-formed).
#if VECTORIZATION_ON_GPU_DEVICE
#define VECTORIZATION_SIMD_NON_INLINE
#else
#define VECTORIZATION_SIMD_NON_INLINE VECTORIZATION_NOINLINE
#endif

#ifdef NDEBUG
#define VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE \
    VECTORIZATION_SIMD_NON_INLINE VECTORIZATION_CUDA_FUNCTION_TYPE static void VECTORIZATION_VECTORCALL
#else
#define VECTORIZATION_SIMD_RETURN_TYPE_NON_INLINE \
    VECTORIZATION_SIMD_NON_INLINE VECTORIZATION_CUDA_FUNCTION_TYPE static void
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
#endif  // __cplusplus >= 201703L

#if defined(_MSC_VER)
#if (_MSC_VER < 1900)
// Visual studio until 2015 is not supporting standard 'alignas' keyword
#ifdef alignas
// This check can be removed when verified that for all other versions alignas
// works as requested
#error "VECTORIZATION error: alignas already defined"
#else
#define alignas(alignment) __declspec(align(alignment))
#endif
#endif

#ifdef alignas
#define VECTORIZATION_ALIGN(alignment) alignas(alignment)
#else
#define VECTORIZATION_ALIGN(alignment) __declspec(align(alignment))
#endif
#elif defined(__GNUC__)
#define VECTORIZATION_ALIGN(alignment) __attribute__((aligned(alignment)))
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#endif

#ifdef VECTORIZATION_MOBILE
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 16;
#else
inline constexpr std::size_t VECTORIZATION_ALIGNMENT = 64;
#endif

// Logging uses fmt-style placeholders ({}) in format strings. Include logger.h first (unique to
// Logging) so LOGGING_LOG is always defined. Include Logging's exception header explicitly:
// link Logging::Logging before Memory::Memory on the Vectorization target (see CMakeLists.txt).
#if VECTORIZATION_HAS_LOGGING
#  include "logger/logger.h"
#  include "util/logging_exception.h"

#  define VECTORIZATION_LOGF(verbosity_name, format_string, ...) \
      LOGGING_LOG(verbosity_name, format_string, ##__VA_ARGS__)
#  define VECTORIZATION_LOG_DEBUG(format_string, ...) LOGGING_LOG_INFO_DEBUG(format_string, ##__VA_ARGS__)
#  define VECTORIZATION_WARNING(format_string, ...)   LOGGING_LOG_WARNING(format_string, ##__VA_ARGS__)
#  define VECTORIZATION_ERROR(format_string, ...)     LOGGING_LOG_ERROR(format_string, ##__VA_ARGS__)
#  define VECTORIZATION_FATAL(format_string, ...)     LOGGING_LOG_FATAL(format_string, ##__VA_ARGS__)

#  define VECTORIZATION_CHECK(cond, ...)         LOGGING_CHECK(cond, __VA_ARGS__)
#  ifndef NDEBUG
#    define VECTORIZATION_CHECK_DEBUG(cond, ...) LOGGING_CHECK_DEBUG(cond, __VA_ARGS__)
#  else
#    define VECTORIZATION_CHECK_DEBUG(cond, ...) ((void)0)
#  endif
#  define VECTORIZATION_THROW(format_str, ...)          LOGGING_THROW(format_str, ##__VA_ARGS__)
#  define VECTORIZATION_NOT_IMPLEMENTED(format_str, ...) LOGGING_NOT_IMPLEMENTED(format_str, ##__VA_ARGS__)

#else

#  include <stdexcept>
#  include <string>

#  define VECTORIZATION_CHECK(cond, ...) assert((cond))
#  ifndef NDEBUG
#    define VECTORIZATION_CHECK_DEBUG(cond, ...) assert((cond))
#  else
#    define VECTORIZATION_CHECK_DEBUG(cond, ...) ((void)0)
#  endif

#  ifdef NDEBUG
#    define VECTORIZATION_LOGF(verbosity_name, format_string, ...)         ((void)0)
#    define VECTORIZATION_LOG_DEBUG(format_string, ...)                   ((void)0)
#    define VECTORIZATION_WARNING(format_string, ...)                     ((void)0)
#    define VECTORIZATION_ERROR(format_string, ...)                       ((void)0)
#    define VECTORIZATION_FATAL(format_string, ...)                       ((void)0)
#  else
#    include <cstdio>
#    define VECTORIZATION_LOGF(verbosity_name, format_string, ...)                          \
      std::printf("[VECTORIZATION][" #verbosity_name "] " format_string "\n", ##__VA_ARGS__)
#    define VECTORIZATION_LOG_DEBUG(format_string, ...)                                     \
      std::printf("[VECTORIZATION][DEBUG] " format_string "\n", ##__VA_ARGS__)
#    define VECTORIZATION_WARNING(format_string, ...)                                       \
      std::fprintf(stderr, "[VECTORIZATION][WARNING] " format_string "\n", ##__VA_ARGS__)
#    define VECTORIZATION_ERROR(format_string, ...)                                         \
      std::fprintf(stderr, "[VECTORIZATION][ERROR] " format_string "\n", ##__VA_ARGS__)
#    define VECTORIZATION_FATAL(format_string, ...)                                         \
      std::fprintf(stderr, "[VECTORIZATION][FATAL] " format_string "\n", ##__VA_ARGS__)
#  endif

#  define VECTORIZATION_THROW(format_str, ...) \
      throw std::runtime_error(std::string("vectorization: ") + (format_str))
#  define VECTORIZATION_NOT_IMPLEMENTED(format_str, ...) \
      throw std::logic_error(std::string("vectorization not implemented: ") + (format_str))

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
