/*
 * Quarisma: High-Performance Quantitative Library
 *
 * Original work Copyright 2015 The TensorFlow Authors
 * Modified work Copyright 2025 Quarisma Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file contains code modified from TensorFlow (Apache 2.0 licensed)
 * and is part of Quarisma, licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * MODIFICATIONS FROM ORIGINAL:
 * - Extracted from memory/helper/mem.cpp for better modularity
 * - Added conditional compilation support for optional compression
 * - Enhanced error handling and platform compatibility
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#include "compression/snappy.h"

#include <cstddef>
#include <string>

#include "common/macros.h"

#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
#include "snappy.h"
#endif
#endif

namespace quarisma
{
namespace compression
{
namespace snappy
{

bool compress(const char* input, size_t length, std::string* output)
{
#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    output->resize(::snappy::MaxCompressedLength(length));
    size_t outlen;
    ::snappy::RawCompress(input, length, &(*output)[0], &outlen);
    output->resize(outlen);
    return true;
#else
    // Compression enabled but Snappy not selected
    (void)input;
    (void)length;
    (void)output;
    return false;
#endif
#else
    // Compression disabled at compile time
    (void)input;
    (void)length;
    (void)output;
    return false;
#endif
}

bool compress_from_io_vec(const struct iovec* iov, size_t uncompressed_length, std::string* output)
{
#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    output->resize(::snappy::MaxCompressedLength(uncompressed_length));
    size_t outlen;

    // Platform-specific handling of iovec structure
#ifdef _WIN32
    // Windows requires casting to snappy::iovec
    const ::snappy::iovec* snappy_iov = reinterpret_cast<const ::snappy::iovec*>(iov);
    ::snappy::RawCompressFromIOVec(snappy_iov, uncompressed_length, &(*output)[0], &outlen);
#else
    // Unix/Linux can use iovec directly
    ::snappy::RawCompressFromIOVec(iov, uncompressed_length, &(*output)[0], &outlen);
#endif

    output->resize(outlen);
    return true;
#else
    // Compression enabled but Snappy not selected
    (void)iov;
    (void)uncompressed_length;
    (void)output;
    return false;
#endif
#else
    // Compression disabled at compile time
    (void)iov;
    (void)uncompressed_length;
    (void)output;
    return false;
#endif
}

bool get_uncompressed_length(const char* input, size_t length, size_t* result)
{
#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    return ::snappy::GetUncompressedLength(input, length, result);
#else
    // Compression enabled but Snappy not selected
    (void)input;
    (void)length;
    (void)result;
    return false;
#endif
#else
    // Compression disabled at compile time
    (void)input;
    (void)length;
    (void)result;
    return false;
#endif
}

bool uncompress(const char* input, size_t length, char* output)
{
#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    return ::snappy::RawUncompress(input, length, output);
#else
    // Compression enabled but Snappy not selected
    (void)input;
    (void)length;
    (void)output;
    return false;
#endif
#else
    // Compression disabled at compile time
    (void)input;
    (void)length;
    (void)output;
    return false;
#endif
}

bool uncompress_to_io_vec(
    const char* compressed, size_t compressed_length, const struct iovec* iov, size_t iov_cnt)
{
#if QUARISMA_HAS_COMPRESSION
#if defined(QUARISMA_COMPRESSION_TYPE_SNAPPY)
    // Platform-specific handling of iovec structure
#ifdef _WIN32
    // Windows requires casting to snappy::iovec
    const ::snappy::iovec* snappy_iov = reinterpret_cast<const ::snappy::iovec*>(iov);
    return ::snappy::RawUncompressToIOVec(compressed, compressed_length, snappy_iov, iov_cnt);
#else
    // Unix/Linux can use iovec directly
    return ::snappy::RawUncompressToIOVec(compressed, compressed_length, iov, iov_cnt);
#endif
#else
    // Compression enabled but Snappy not selected
    (void)compressed;
    (void)compressed_length;
    (void)iov;
    (void)iov_cnt;
    return false;
#endif
#else
    // Compression disabled at compile time
    (void)compressed;
    (void)compressed_length;
    (void)iov;
    (void)iov_cnt;
    return false;
#endif
}

}  // namespace snappy
}  // namespace compression
}  // namespace quarisma
