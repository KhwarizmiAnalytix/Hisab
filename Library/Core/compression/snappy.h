/*
 * Quarisma: High-Performance Computational Library
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
 * - Extracted from memory/helper/mem.h for better modularity
 * - Added conditional compilation support for optional compression
 * - Enhanced documentation and error handling
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

#pragma once

#include <cstddef>
#include <string>

#include "common/macros.h"

#ifdef _WIN32
#include <sys/uio.h>  // For iovec on Windows
#else
#include <sys/uio.h>  // For iovec on Unix/Linux
#endif

namespace quarisma
{
namespace compression
{

/**
 * @brief High-performance Snappy compression interface for Quarisma.
 *
 * Provides a unified interface for Snappy compression operations with
 * conditional compilation support. When compression is disabled, all
 * functions return false to indicate unavailability.
 *
 * **Key Features**:
 * - Cross-platform I/O vector support
 * - Conditional compilation for optional builds
 * - High-performance compression and decompression
 * - Memory-efficient streaming operations
 *
 * **Performance Characteristics**:
 * - Compression: ~250-500 MB/s depending on data
 * - Decompression: ~800-1500 MB/s depending on data
 * - Memory overhead: Minimal temporary buffers
 *
 * **Thread Safety**: All functions are thread-safe
 */
namespace snappy
{

/**
 * @brief Compress data using Snappy compression algorithm.
 *
 * Compresses the input data using Google's Snappy compression algorithm,
 * which prioritizes speed over compression ratio. Ideal for real-time
 * data processing and network transmission.
 *
 * @param input Input data to compress
 * @param length Length of input data in bytes
 * @param output Output string to store compressed data (resized automatically)
 * @return true if compression succeeded, false if compression unavailable or failed
 *
 * **Performance**: O(n) where n is input size
 * **Memory**: Temporary buffer ~1.5x input size during compression
 * **Compression Ratio**: Typically 2-4x depending on data entropy
 *
 * **Example**:
 * ```cpp
 * std::string compressed;
 * if (snappy::compress(data.c_str(), data.size(), &compressed)) {
 *     // Use compressed data
 * }
 * ```
 */
QUARISMA_API bool compress(const char* input, size_t length, std::string* output);

/**
 * @brief Compress data from I/O vector using Snappy compression.
 *
 * Compresses data from multiple memory regions specified by I/O vectors,
 * avoiding the need to copy data into a contiguous buffer before compression.
 * Particularly efficient for scattered data layouts.
 *
 * @param iov Array of I/O vectors containing input data
 * @param uncompressed_length Total length of uncompressed data across all vectors
 * @param output Output string to store compressed data (resized automatically)
 * @return true if compression succeeded, false if compression unavailable or failed
 *
 * **Performance**: O(n) where n is total uncompressed size
 * **Memory**: No intermediate copying required
 * **Use Cases**: Network packet compression, scattered buffer compression
 *
 * **Platform Notes**: Handles Windows/Unix iovec differences automatically
 */
QUARISMA_API bool compress_from_io_vec(
    const struct iovec* iov, size_t uncompressed_length, std::string* output);

/**
 * @brief Get the uncompressed length of Snappy-compressed data.
 *
 * Extracts the original uncompressed size from Snappy-compressed data
 * without performing full decompression. Essential for pre-allocating
 * output buffers for efficient decompression.
 *
 * @param input Compressed input data
 * @param length Length of compressed data in bytes
 * @param result Pointer to store the uncompressed length
 * @return true if length retrieval succeeded, false if compression unavailable or data invalid
 *
 * **Performance**: O(1) - reads header only
 * **Memory**: No additional allocation
 * **Validation**: Performs basic header validation
 *
 * **Example**:
 * ```cpp
 * size_t uncompressed_size;
 * if (snappy::get_uncompressed_length(compressed.data(), compressed.size(), &uncompressed_size)) {
 *     std::vector<char> buffer(uncompressed_size);
 *     // Proceed with decompression
 * }
 * ```
 */
QUARISMA_API bool get_uncompressed_length(const char* input, size_t length, size_t* result);

/**
 * @brief Uncompress Snappy-compressed data.
 *
 * Decompresses Snappy-compressed data into a pre-allocated output buffer.
 * The output buffer must be large enough to hold the uncompressed data
 * (use get_uncompressed_length() to determine required size).
 *
 * @param input Compressed input data
 * @param length Length of compressed data in bytes
 * @param output Pre-allocated output buffer for uncompressed data
 * @return true if decompression succeeded, false if compression unavailable or failed
 *
 * **Performance**: O(n) where n is uncompressed size
 * **Memory**: No additional allocation (uses provided buffer)
 * **Buffer Requirements**: Output buffer must be exactly the right size
 *
 * **Example**:
 * ```cpp
 * size_t size;
 * if (snappy::get_uncompressed_length(compressed.data(), compressed.size(), &size)) {
 *     std::vector<char> buffer(size);
 *     if (snappy::uncompress(compressed.data(), compressed.size(), buffer.data())) {
 *         // Use uncompressed data
 *     }
 * }
 * ```
 */
QUARISMA_API bool uncompress(const char* input, size_t length, char* output);

/**
 * @brief Uncompress Snappy-compressed data to I/O vector.
 *
 * Decompresses data directly into multiple memory regions specified by
 * I/O vectors, avoiding intermediate buffer allocation. Efficient for
 * scattered output layouts and zero-copy operations.
 *
 * @param compressed Compressed input data
 * @param compressed_length Length of compressed data in bytes
 * @param iov Array of I/O vectors to store uncompressed data
 * @param iov_cnt Number of I/O vectors in the array
 * @return true if decompression succeeded, false if compression unavailable or failed
 *
 * **Performance**: O(n) where n is uncompressed size
 * **Memory**: No intermediate copying required
 * **Buffer Requirements**: Total iovec capacity must match uncompressed size
 *
 * **Platform Notes**: Handles Windows/Unix iovec differences automatically
 */
QUARISMA_API bool uncompress_to_io_vec(
    const char* compressed, size_t compressed_length, const struct iovec* iov, size_t iov_cnt);

}  // namespace snappy
}  // namespace compression
}  // namespace quarisma
