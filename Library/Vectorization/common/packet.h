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



#include <array>

#include "common/intrin.h"
#include "common/vectorization_macros.h"
#include "common/scalar_helper_functions.h"

#if VECTORIZATION_HAS_AVX512
#include "backend/avx512/double/simd.h"
#include "backend/avx512/float/simd.h"
#elif VECTORIZATION_HAS_AVX2 || VECTORIZATION_HAS_AVX
#include "backend/avx/double/simd.h"
#include "backend/avx/float/simd.h"
#elif VECTORIZATION_HAS_SSE
#include "backend/sse/double/simd.h"
#include "backend/sse/float/simd.h"
#endif

namespace vectorization
{
#define LOAD(op)                                                           \
    simd<value_t>::op(from, data.data_[0]);                                \
    simd<value_t>::op(from + simd<value_t>::size, data.data_[1]);          \
    if constexpr (N >= 4)                                                  \
    {                                                                      \
        const auto* ptr = from + 2 * simd<value_t>::size;                  \
        simd<value_t>::op(ptr, data.data_[2]);                             \
        simd<value_t>::op(ptr + simd<value_t>::size, data.data_[3]);       \
    }                                                                      \
    if constexpr (N >= 8)                                                  \
    {                                                                      \
        const auto* ptr = from + 4 * simd<value_t>::size;                  \
                                                                           \
        simd<value_t>::op(ptr, data.data_[4]);                             \
        simd<value_t>::op(ptr + simd<value_t>::size, data.data_[5]);       \
        simd<value_t>::op(ptr + 2 * simd<value_t>::size, data.data_[6]);   \
        simd<value_t>::op(ptr + 3 * simd<value_t>::size, data.data_[7]);   \
    }                                                                      \
    if constexpr (N >= 16)                                                 \
    {                                                                      \
        const auto* ptr = from + 8 * simd<value_t>::size;                  \
                                                                           \
        simd<value_t>::op(ptr, data.data_[8]);                             \
        simd<value_t>::op(ptr + simd<value_t>::size, data.data_[9]);       \
        simd<value_t>::op(ptr + 2 * simd<value_t>::size, data.data_[10]);  \
        simd<value_t>::op(ptr + 3 * simd<value_t>::size, data.data_[11]);  \
        simd<value_t>::op(ptr + 4 * simd<value_t>::size, data.data_[12]);  \
        simd<value_t>::op(ptr + 5 * simd<value_t>::size, data.data_[13]);  \
        simd<value_t>::op(ptr + 6 * simd<value_t>::size, data.data_[14]);  \
        simd<value_t>::op(ptr + 7 * simd<value_t>::size, data.data_[15]);  \
    }                                                                      \
    if constexpr (N >= 32)                                                 \
    {                                                                      \
        const auto* ptr = from + 16 * simd<value_t>::size;                 \
                                                                           \
        simd<value_t>::op(ptr, data.data_[16]);                            \
        simd<value_t>::op(ptr + simd<value_t>::size, data.data_[17]);      \
        simd<value_t>::op(ptr + 2 * simd<value_t>::size, data.data_[18]);  \
        simd<value_t>::op(ptr + 3 * simd<value_t>::size, data.data_[19]);  \
        simd<value_t>::op(ptr + 4 * simd<value_t>::size, data.data_[20]);  \
        simd<value_t>::op(ptr + 5 * simd<value_t>::size, data.data_[21]);  \
        simd<value_t>::op(ptr + 6 * simd<value_t>::size, data.data_[22]);  \
        simd<value_t>::op(ptr + 7 * simd<value_t>::size, data.data_[23]);  \
        simd<value_t>::op(ptr + 8 * simd<value_t>::size, data.data_[24]);  \
        simd<value_t>::op(ptr + 9 * simd<value_t>::size, data.data_[25]);  \
        simd<value_t>::op(ptr + 10 * simd<value_t>::size, data.data_[26]); \
        simd<value_t>::op(ptr + 11 * simd<value_t>::size, data.data_[27]); \
        simd<value_t>::op(ptr + 12 * simd<value_t>::size, data.data_[28]); \
        simd<value_t>::op(ptr + 13 * simd<value_t>::size, data.data_[29]); \
        simd<value_t>::op(ptr + 14 * simd<value_t>::size, data.data_[30]); \
        simd<value_t>::op(ptr + 15 * simd<value_t>::size, data.data_[31]); \
    }

#define STORE(op)                                                          \
    simd<value_t>::op(data.data_[0], to);                                  \
    simd<value_t>::op(data.data_[1], to + simd<value_t>::size);            \
    if constexpr (N >= 4)                                                  \
    {                                                                      \
        auto* ptr = to + 2 * simd<value_t>::size;                          \
        simd<value_t>::op(data.data_[2], ptr);                             \
        simd<value_t>::op(data.data_[3], ptr + simd<value_t>::size);       \
    }                                                                      \
    if constexpr (N >= 8)                                                  \
    {                                                                      \
        auto* ptr = to + 4 * simd<value_t>::size;                          \
                                                                           \
        simd<value_t>::op(data.data_[4], ptr);                             \
        simd<value_t>::op(data.data_[5], ptr + simd<value_t>::size);       \
        simd<value_t>::op(data.data_[6], ptr + 2 * simd<value_t>::size);   \
        simd<value_t>::op(data.data_[7], ptr + 3 * simd<value_t>::size);   \
    }                                                                      \
    if constexpr (N >= 16)                                                 \
    {                                                                      \
        auto* ptr = to + 8 * simd<value_t>::size;                          \
                                                                           \
        simd<value_t>::op(data.data_[8], ptr);                             \
        simd<value_t>::op(data.data_[9], ptr + simd<value_t>::size);       \
        simd<value_t>::op(data.data_[10], ptr + 2 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[11], ptr + 3 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[12], ptr + 4 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[13], ptr + 5 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[14], ptr + 6 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[15], ptr + 7 * simd<value_t>::size);  \
    }                                                                      \
    if constexpr (N >= 32)                                                 \
    {                                                                      \
        auto* ptr = to + 16 * simd<value_t>::size;                         \
                                                                           \
        simd<value_t>::op(data.data_[16], ptr);                            \
        simd<value_t>::op(data.data_[17], ptr + simd<value_t>::size);      \
        simd<value_t>::op(data.data_[18], ptr + 2 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[19], ptr + 3 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[20], ptr + 4 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[21], ptr + 5 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[22], ptr + 6 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[23], ptr + 7 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[24], ptr + 8 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[25], ptr + 9 * simd<value_t>::size);  \
        simd<value_t>::op(data.data_[26], ptr + 10 * simd<value_t>::size); \
        simd<value_t>::op(data.data_[27], ptr + 11 * simd<value_t>::size); \
        simd<value_t>::op(data.data_[28], ptr + 12 * simd<value_t>::size); \
        simd<value_t>::op(data.data_[29], ptr + 13 * simd<value_t>::size); \
        simd<value_t>::op(data.data_[30], ptr + 14 * simd<value_t>::size); \
        simd<value_t>::op(data.data_[31], ptr + 15 * simd<value_t>::size); \
    }

#define FUNCTION_THREE_ARGS(op)                                                   \
    simd<value_t>::op(x.data_[0], y.data_[0], z.data_[0], data.data_[0]);         \
    simd<value_t>::op(x.data_[1], y.data_[1], z.data_[1], data.data_[1]);         \
    if constexpr (N >= 4)                                                         \
    {                                                                             \
        simd<value_t>::op(x.data_[2], y.data_[2], z.data_[2], data.data_[2]);     \
        simd<value_t>::op(x.data_[3], y.data_[3], z.data_[3], data.data_[3]);     \
    }                                                                             \
    if constexpr (N >= 8)                                                         \
    {                                                                             \
        simd<value_t>::op(x.data_[4], y.data_[4], z.data_[4], data.data_[4]);     \
        simd<value_t>::op(x.data_[5], y.data_[5], z.data_[5], data.data_[5]);     \
        simd<value_t>::op(x.data_[6], y.data_[6], z.data_[6], data.data_[6]);     \
        simd<value_t>::op(x.data_[7], y.data_[7], z.data_[7], data.data_[7]);     \
    }                                                                             \
    if constexpr (N >= 16)                                                        \
    {                                                                             \
        simd<value_t>::op(x.data_[8], y.data_[8], z.data_[8], data.data_[8]);     \
        simd<value_t>::op(x.data_[9], y.data_[9], z.data_[9], data.data_[9]);     \
        simd<value_t>::op(x.data_[10], y.data_[10], z.data_[10], data.data_[10]); \
        simd<value_t>::op(x.data_[11], y.data_[11], z.data_[11], data.data_[11]); \
        simd<value_t>::op(x.data_[12], y.data_[12], z.data_[12], data.data_[12]); \
        simd<value_t>::op(x.data_[13], y.data_[13], z.data_[13], data.data_[13]); \
        simd<value_t>::op(x.data_[14], y.data_[14], z.data_[14], data.data_[14]); \
        simd<value_t>::op(x.data_[15], y.data_[15], z.data_[15], data.data_[15]); \
    }                                                                             \
    if constexpr (N >= 32)                                                        \
    {                                                                             \
        simd<value_t>::op(x.data_[16], y.data_[16], z.data_[16], data.data_[16]); \
        simd<value_t>::op(x.data_[17], y.data_[17], z.data_[17], data.data_[17]); \
        simd<value_t>::op(x.data_[18], y.data_[18], z.data_[18], data.data_[18]); \
        simd<value_t>::op(x.data_[19], y.data_[19], z.data_[19], data.data_[19]); \
        simd<value_t>::op(x.data_[20], y.data_[20], z.data_[20], data.data_[20]); \
        simd<value_t>::op(x.data_[21], y.data_[21], z.data_[21], data.data_[21]); \
        simd<value_t>::op(x.data_[22], y.data_[22], z.data_[22], data.data_[22]); \
        simd<value_t>::op(x.data_[23], y.data_[23], z.data_[23], data.data_[23]); \
        simd<value_t>::op(x.data_[24], y.data_[24], z.data_[24], data.data_[24]); \
        simd<value_t>::op(x.data_[25], y.data_[25], z.data_[25], data.data_[25]); \
        simd<value_t>::op(x.data_[26], y.data_[26], z.data_[26], data.data_[26]); \
        simd<value_t>::op(x.data_[27], y.data_[27], z.data_[27], data.data_[27]); \
        simd<value_t>::op(x.data_[28], y.data_[28], z.data_[28], data.data_[28]); \
        simd<value_t>::op(x.data_[29], y.data_[29], z.data_[29], data.data_[29]); \
        simd<value_t>::op(x.data_[30], y.data_[30], z.data_[30], data.data_[30]); \
        simd<value_t>::op(x.data_[31], y.data_[31], z.data_[31], data.data_[31]); \
    }
#define FUNCTION_THREE_ARGS_X_SCALAR(op)                                \
    simd<value_t>::op(x, y.data_[0], z.data_[0], data.data_[0]);        \
    simd<value_t>::op(x, y.data_[1], z.data_[1], data.data_[1]);        \
    if constexpr (N >= 4)                                               \
    {                                                                   \
        simd<value_t>::op(x, y.data_[2], z.data_[2], data.data_[2]);    \
        simd<value_t>::op(x, y.data_[3], z.data_[3], data.data_[3]);    \
    }                                                                   \
    if constexpr (N >= 8)                                               \
    {                                                                   \
        simd<value_t>::op(x, y.data_[4], z.data_[4], data.data_[4]);    \
        simd<value_t>::op(x, y.data_[5], z.data_[5], data.data_[5]);    \
        simd<value_t>::op(x, y.data_[6], z.data_[6], data.data_[6]);    \
        simd<value_t>::op(x, y.data_[7], z.data_[7], data.data_[7]);    \
    }                                                                   \
    if constexpr (N >= 16)                                              \
    {                                                                   \
        simd<value_t>::op(x, y.data_[8], z.data_[8], data.data_[8]);    \
        simd<value_t>::op(x, y.data_[9], z.data_[9], data.data_[9]);    \
        simd<value_t>::op(x, y.data_[10], z.data_[10], data.data_[10]); \
        simd<value_t>::op(x, y.data_[11], z.data_[11], data.data_[11]); \
        simd<value_t>::op(x, y.data_[12], z.data_[12], data.data_[12]); \
        simd<value_t>::op(x, y.data_[13], z.data_[13], data.data_[13]); \
        simd<value_t>::op(x, y.data_[14], z.data_[14], data.data_[14]); \
        simd<value_t>::op(x, y.data_[15], z.data_[15], data.data_[15]); \
    }                                                                   \
    if constexpr (N >= 32)                                              \
    {                                                                   \
        simd<value_t>::op(x, y.data_[16], z.data_[16], data.data_[16]); \
        simd<value_t>::op(x, y.data_[17], z.data_[17], data.data_[17]); \
        simd<value_t>::op(x, y.data_[18], z.data_[18], data.data_[18]); \
        simd<value_t>::op(x, y.data_[19], z.data_[19], data.data_[19]); \
        simd<value_t>::op(x, y.data_[20], z.data_[20], data.data_[20]); \
        simd<value_t>::op(x, y.data_[21], z.data_[21], data.data_[21]); \
        simd<value_t>::op(x, y.data_[22], z.data_[22], data.data_[22]); \
        simd<value_t>::op(x, y.data_[23], z.data_[23], data.data_[23]); \
        simd<value_t>::op(x, y.data_[24], z.data_[24], data.data_[24]); \
        simd<value_t>::op(x, y.data_[25], z.data_[25], data.data_[25]); \
        simd<value_t>::op(x, y.data_[26], z.data_[26], data.data_[26]); \
        simd<value_t>::op(x, y.data_[27], z.data_[27], data.data_[27]); \
        simd<value_t>::op(x, y.data_[28], z.data_[28], data.data_[28]); \
        simd<value_t>::op(x, y.data_[29], z.data_[29], data.data_[29]); \
        simd<value_t>::op(x, y.data_[30], z.data_[30], data.data_[30]); \
        simd<value_t>::op(x, y.data_[31], z.data_[31], data.data_[31]); \
    }

#define FUNCTION_THREE_ARGS_Y_SCALAR(op)                                \
    simd<value_t>::op(x.data_[0], y, z.data_[0], data.data_[0]);        \
    simd<value_t>::op(x.data_[1], y, z.data_[1], data.data_[1]);        \
    if constexpr (N >= 4)                                               \
    {                                                                   \
        simd<value_t>::op(x.data_[2], y, z.data_[2], data.data_[2]);    \
        simd<value_t>::op(x.data_[3], y, z.data_[3], data.data_[3]);    \
    }                                                                   \
    if constexpr (N >= 8)                                               \
    {                                                                   \
        simd<value_t>::op(x.data_[4], y, z.data_[4], data.data_[4]);    \
        simd<value_t>::op(x.data_[5], y, z.data_[5], data.data_[5]);    \
        simd<value_t>::op(x.data_[6], y, z.data_[6], data.data_[6]);    \
        simd<value_t>::op(x.data_[7], y, z.data_[7], data.data_[7]);    \
    }                                                                   \
    if constexpr (N >= 16)                                              \
    {                                                                   \
        simd<value_t>::op(x.data_[8], y, z.data_[8], data.data_[8]);    \
        simd<value_t>::op(x.data_[9], y, z.data_[9], data.data_[9]);    \
        simd<value_t>::op(x.data_[10], y, z.data_[10], data.data_[10]); \
        simd<value_t>::op(x.data_[11], y, z.data_[11], data.data_[11]); \
        simd<value_t>::op(x.data_[12], y, z.data_[12], data.data_[12]); \
        simd<value_t>::op(x.data_[13], y, z.data_[13], data.data_[13]); \
        simd<value_t>::op(x.data_[14], y, z.data_[14], data.data_[14]); \
        simd<value_t>::op(x.data_[15], y, z.data_[15], data.data_[15]); \
    }                                                                   \
    if constexpr (N >= 32)                                              \
    {                                                                   \
        simd<value_t>::op(x.data_[16], y, z.data_[16], data.data_[16]); \
        simd<value_t>::op(x.data_[17], y, z.data_[17], data.data_[17]); \
        simd<value_t>::op(x.data_[18], y, z.data_[18], data.data_[18]); \
        simd<value_t>::op(x.data_[19], y, z.data_[19], data.data_[19]); \
        simd<value_t>::op(x.data_[20], y, z.data_[20], data.data_[20]); \
        simd<value_t>::op(x.data_[21], y, z.data_[21], data.data_[21]); \
        simd<value_t>::op(x.data_[22], y, z.data_[22], data.data_[22]); \
        simd<value_t>::op(x.data_[23], y, z.data_[23], data.data_[23]); \
        simd<value_t>::op(x.data_[24], y, z.data_[24], data.data_[24]); \
        simd<value_t>::op(x.data_[25], y, z.data_[25], data.data_[25]); \
        simd<value_t>::op(x.data_[26], y, z.data_[26], data.data_[26]); \
        simd<value_t>::op(x.data_[27], y, z.data_[27], data.data_[27]); \
        simd<value_t>::op(x.data_[28], y, z.data_[28], data.data_[28]); \
        simd<value_t>::op(x.data_[29], y, z.data_[29], data.data_[29]); \
        simd<value_t>::op(x.data_[30], y, z.data_[30], data.data_[30]); \
        simd<value_t>::op(x.data_[31], y, z.data_[31], data.data_[31]); \
    }

#define FUNCTION_THREE_ARGS_XZ_SCALAR(op)                     \
    simd<value_t>::op(x, y.data_[0], z, data.data_[0]);       \
    simd<value_t>::op(x, y.data_[1], z, data.data_[1]);       \
    if constexpr (N >= 4)                                     \
    {                                                         \
        simd<value_t>::op(x, y.data_[2], z, data.data_[2]);   \
        simd<value_t>::op(x, y.data_[3], z, data.data_[3]);   \
    }                                                         \
    if constexpr (N >= 8)                                     \
    {                                                         \
        simd<value_t>::op(x, y.data_[4], z, data.data_[4]);   \
        simd<value_t>::op(x, y.data_[5], z, data.data_[5]);   \
        simd<value_t>::op(x, y.data_[6], z, data.data_[6]);   \
        simd<value_t>::op(x, y.data_[7], z, data.data_[7]);   \
    }                                                         \
    if constexpr (N >= 16)                                    \
    {                                                         \
        simd<value_t>::op(x, y.data_[8], z, data.data_[8]);   \
        simd<value_t>::op(x, y.data_[9], z, data.data_[9]);   \
        simd<value_t>::op(x, y.data_[10], z, data.data_[10]); \
        simd<value_t>::op(x, y.data_[11], z, data.data_[11]); \
        simd<value_t>::op(x, y.data_[12], z, data.data_[12]); \
        simd<value_t>::op(x, y.data_[13], z, data.data_[13]); \
        simd<value_t>::op(x, y.data_[14], z, data.data_[14]); \
        simd<value_t>::op(x, y.data_[15], z, data.data_[15]); \
    }                                                         \
    if constexpr (N >= 32)                                    \
    {                                                         \
        simd<value_t>::op(x, y.data_[16], z, data.data_[16]); \
        simd<value_t>::op(x, y.data_[17], z, data.data_[17]); \
        simd<value_t>::op(x, y.data_[18], z, data.data_[18]); \
        simd<value_t>::op(x, y.data_[19], z, data.data_[19]); \
        simd<value_t>::op(x, y.data_[20], z, data.data_[20]); \
        simd<value_t>::op(x, y.data_[21], z, data.data_[21]); \
        simd<value_t>::op(x, y.data_[22], z, data.data_[22]); \
        simd<value_t>::op(x, y.data_[23], z, data.data_[23]); \
        simd<value_t>::op(x, y.data_[24], z, data.data_[24]); \
        simd<value_t>::op(x, y.data_[25], z, data.data_[25]); \
        simd<value_t>::op(x, y.data_[26], z, data.data_[26]); \
        simd<value_t>::op(x, y.data_[27], z, data.data_[27]); \
        simd<value_t>::op(x, y.data_[28], z, data.data_[28]); \
        simd<value_t>::op(x, y.data_[29], z, data.data_[29]); \
        simd<value_t>::op(x, y.data_[30], z, data.data_[30]); \
        simd<value_t>::op(x, y.data_[31], z, data.data_[31]); \
    }

#define FUNCTION_THREE_ARGS_Z_SCALAR(op)                                \
    simd<value_t>::op(x.data_[0], y.data_[0], z, data.data_[0]);        \
    simd<value_t>::op(x.data_[1], y.data_[1], z, data.data_[1]);        \
    if constexpr (N >= 4)                                               \
    {                                                                   \
        simd<value_t>::op(x.data_[2], y.data_[2], z, data.data_[2]);    \
        simd<value_t>::op(x.data_[3], y.data_[3], z, data.data_[3]);    \
    }                                                                   \
    if constexpr (N >= 8)                                               \
    {                                                                   \
        simd<value_t>::op(x.data_[4], y.data_[4], z, data.data_[4]);    \
        simd<value_t>::op(x.data_[5], y.data_[5], z, data.data_[5]);    \
        simd<value_t>::op(x.data_[6], y.data_[6], z, data.data_[6]);    \
        simd<value_t>::op(x.data_[7], y.data_[7], z, data.data_[7]);    \
    }                                                                   \
    if constexpr (N >= 16)                                              \
    {                                                                   \
        simd<value_t>::op(x.data_[8], y.data_[8], z, data.data_[8]);    \
        simd<value_t>::op(x.data_[9], y.data_[9], z, data.data_[9]);    \
        simd<value_t>::op(x.data_[10], y.data_[10], z, data.data_[10]); \
        simd<value_t>::op(x.data_[11], y.data_[11], z, data.data_[11]); \
        simd<value_t>::op(x.data_[12], y.data_[12], z, data.data_[12]); \
        simd<value_t>::op(x.data_[13], y.data_[13], z, data.data_[13]); \
        simd<value_t>::op(x.data_[14], y.data_[14], z, data.data_[14]); \
        simd<value_t>::op(x.data_[15], y.data_[15], z, data.data_[15]); \
    }                                                                   \
    if constexpr (N >= 32)                                              \
    {                                                                   \
        simd<value_t>::op(x.data_[16], y.data_[16], z, data.data_[16]); \
        simd<value_t>::op(x.data_[17], y.data_[17], z, data.data_[17]); \
        simd<value_t>::op(x.data_[18], y.data_[18], z, data.data_[18]); \
        simd<value_t>::op(x.data_[19], y.data_[19], z, data.data_[19]); \
        simd<value_t>::op(x.data_[20], y.data_[20], z, data.data_[20]); \
        simd<value_t>::op(x.data_[21], y.data_[21], z, data.data_[21]); \
        simd<value_t>::op(x.data_[22], y.data_[22], z, data.data_[22]); \
        simd<value_t>::op(x.data_[23], y.data_[23], z, data.data_[23]); \
        simd<value_t>::op(x.data_[24], y.data_[24], z, data.data_[24]); \
        simd<value_t>::op(x.data_[25], y.data_[25], z, data.data_[25]); \
        simd<value_t>::op(x.data_[26], y.data_[26], z, data.data_[26]); \
        simd<value_t>::op(x.data_[27], y.data_[27], z, data.data_[27]); \
        simd<value_t>::op(x.data_[28], y.data_[28], z, data.data_[28]); \
        simd<value_t>::op(x.data_[29], y.data_[29], z, data.data_[29]); \
        simd<value_t>::op(x.data_[30], y.data_[30], z, data.data_[30]); \
        simd<value_t>::op(x.data_[31], y.data_[31], z, data.data_[31]); \
    }
#define FUNCTION_TWO_ARGS(op)                                        \
    simd<value_t>::op(x.data_[0], y.data_[0], data.data_[0]);        \
    simd<value_t>::op(x.data_[1], y.data_[1], data.data_[1]);        \
    if constexpr (N >= 4)                                            \
    {                                                                \
        simd<value_t>::op(x.data_[2], y.data_[2], data.data_[2]);    \
        simd<value_t>::op(x.data_[3], y.data_[3], data.data_[3]);    \
    }                                                                \
    if constexpr (N >= 8)                                            \
    {                                                                \
        simd<value_t>::op(x.data_[4], y.data_[4], data.data_[4]);    \
        simd<value_t>::op(x.data_[5], y.data_[5], data.data_[5]);    \
        simd<value_t>::op(x.data_[6], y.data_[6], data.data_[6]);    \
        simd<value_t>::op(x.data_[7], y.data_[7], data.data_[7]);    \
    }                                                                \
    if constexpr (N >= 16)                                           \
    {                                                                \
        simd<value_t>::op(x.data_[8], y.data_[8], data.data_[8]);    \
        simd<value_t>::op(x.data_[9], y.data_[9], data.data_[9]);    \
        simd<value_t>::op(x.data_[10], y.data_[10], data.data_[10]); \
        simd<value_t>::op(x.data_[11], y.data_[11], data.data_[11]); \
        simd<value_t>::op(x.data_[12], y.data_[12], data.data_[12]); \
        simd<value_t>::op(x.data_[13], y.data_[13], data.data_[13]); \
        simd<value_t>::op(x.data_[14], y.data_[14], data.data_[14]); \
        simd<value_t>::op(x.data_[15], y.data_[15], data.data_[15]); \
    }                                                                \
    if constexpr (N >= 32)                                           \
    {                                                                \
        simd<value_t>::op(x.data_[16], y.data_[16], data.data_[16]); \
        simd<value_t>::op(x.data_[17], y.data_[17], data.data_[17]); \
        simd<value_t>::op(x.data_[18], y.data_[18], data.data_[18]); \
        simd<value_t>::op(x.data_[19], y.data_[19], data.data_[19]); \
        simd<value_t>::op(x.data_[20], y.data_[20], data.data_[20]); \
        simd<value_t>::op(x.data_[21], y.data_[21], data.data_[21]); \
        simd<value_t>::op(x.data_[22], y.data_[22], data.data_[22]); \
        simd<value_t>::op(x.data_[23], y.data_[23], data.data_[23]); \
        simd<value_t>::op(x.data_[24], y.data_[24], data.data_[24]); \
        simd<value_t>::op(x.data_[25], y.data_[25], data.data_[25]); \
        simd<value_t>::op(x.data_[26], y.data_[26], data.data_[26]); \
        simd<value_t>::op(x.data_[27], y.data_[27], data.data_[27]); \
        simd<value_t>::op(x.data_[28], y.data_[28], data.data_[28]); \
        simd<value_t>::op(x.data_[29], y.data_[29], data.data_[29]); \
        simd<value_t>::op(x.data_[30], y.data_[30], data.data_[30]); \
        simd<value_t>::op(x.data_[31], y.data_[31], data.data_[31]); \
    }

#define FUNCTION_TWO_ARGS_X_SCALAR(op)                     \
    simd<value_t>::op(x, y.data_[0], data.data_[0]);       \
    simd<value_t>::op(x, y.data_[1], data.data_[1]);       \
    if constexpr (N >= 4)                                  \
    {                                                      \
        simd<value_t>::op(x, y.data_[2], data.data_[2]);   \
        simd<value_t>::op(x, y.data_[3], data.data_[3]);   \
    }                                                      \
    if constexpr (N >= 8)                                  \
    {                                                      \
        simd<value_t>::op(x, y.data_[4], data.data_[4]);   \
        simd<value_t>::op(x, y.data_[5], data.data_[5]);   \
        simd<value_t>::op(x, y.data_[6], data.data_[6]);   \
        simd<value_t>::op(x, y.data_[7], data.data_[7]);   \
    }                                                      \
    if constexpr (N >= 16)                                 \
    {                                                      \
        simd<value_t>::op(x, y.data_[8], data.data_[8]);   \
        simd<value_t>::op(x, y.data_[9], data.data_[9]);   \
        simd<value_t>::op(x, y.data_[10], data.data_[10]); \
        simd<value_t>::op(x, y.data_[11], data.data_[11]); \
        simd<value_t>::op(x, y.data_[12], data.data_[12]); \
        simd<value_t>::op(x, y.data_[13], data.data_[13]); \
        simd<value_t>::op(x, y.data_[14], data.data_[14]); \
        simd<value_t>::op(x, y.data_[15], data.data_[15]); \
    }                                                      \
    if constexpr (N >= 32)                                 \
    {                                                      \
        simd<value_t>::op(x, y.data_[16], data.data_[16]); \
        simd<value_t>::op(x, y.data_[17], data.data_[17]); \
        simd<value_t>::op(x, y.data_[18], data.data_[18]); \
        simd<value_t>::op(x, y.data_[19], data.data_[19]); \
        simd<value_t>::op(x, y.data_[20], data.data_[20]); \
        simd<value_t>::op(x, y.data_[21], data.data_[21]); \
        simd<value_t>::op(x, y.data_[22], data.data_[22]); \
        simd<value_t>::op(x, y.data_[23], data.data_[23]); \
        simd<value_t>::op(x, y.data_[24], data.data_[24]); \
        simd<value_t>::op(x, y.data_[25], data.data_[25]); \
        simd<value_t>::op(x, y.data_[26], data.data_[26]); \
        simd<value_t>::op(x, y.data_[27], data.data_[27]); \
        simd<value_t>::op(x, y.data_[28], data.data_[28]); \
        simd<value_t>::op(x, y.data_[29], data.data_[29]); \
        simd<value_t>::op(x, y.data_[30], data.data_[30]); \
        simd<value_t>::op(x, y.data_[31], data.data_[31]); \
    }

#define FUNCTION_TWO_ARGS_Y_SCALAR(op)                     \
    simd<value_t>::op(x.data_[0], y, data.data_[0]);       \
    simd<value_t>::op(x.data_[1], y, data.data_[1]);       \
    if constexpr (N >= 4)                                  \
    {                                                      \
        simd<value_t>::op(x.data_[2], y, data.data_[2]);   \
        simd<value_t>::op(x.data_[3], y, data.data_[3]);   \
    }                                                      \
    if constexpr (N >= 8)                                  \
    {                                                      \
        simd<value_t>::op(x.data_[4], y, data.data_[4]);   \
        simd<value_t>::op(x.data_[5], y, data.data_[5]);   \
        simd<value_t>::op(x.data_[6], y, data.data_[6]);   \
        simd<value_t>::op(x.data_[7], y, data.data_[7]);   \
    }                                                      \
    if constexpr (N >= 16)                                 \
    {                                                      \
        simd<value_t>::op(x.data_[8], y, data.data_[8]);   \
        simd<value_t>::op(x.data_[9], y, data.data_[9]);   \
        simd<value_t>::op(x.data_[10], y, data.data_[10]); \
        simd<value_t>::op(x.data_[11], y, data.data_[11]); \
        simd<value_t>::op(x.data_[12], y, data.data_[12]); \
        simd<value_t>::op(x.data_[13], y, data.data_[13]); \
        simd<value_t>::op(x.data_[14], y, data.data_[14]); \
        simd<value_t>::op(x.data_[15], y, data.data_[15]); \
    }                                                      \
    if constexpr (N >= 32)                                 \
    {                                                      \
        simd<value_t>::op(x.data_[16], y, data.data_[16]); \
        simd<value_t>::op(x.data_[17], y, data.data_[17]); \
        simd<value_t>::op(x.data_[18], y, data.data_[18]); \
        simd<value_t>::op(x.data_[19], y, data.data_[19]); \
        simd<value_t>::op(x.data_[20], y, data.data_[20]); \
        simd<value_t>::op(x.data_[21], y, data.data_[21]); \
        simd<value_t>::op(x.data_[22], y, data.data_[22]); \
        simd<value_t>::op(x.data_[23], y, data.data_[23]); \
        simd<value_t>::op(x.data_[24], y, data.data_[24]); \
        simd<value_t>::op(x.data_[25], y, data.data_[25]); \
        simd<value_t>::op(x.data_[26], y, data.data_[26]); \
        simd<value_t>::op(x.data_[27], y, data.data_[27]); \
        simd<value_t>::op(x.data_[28], y, data.data_[28]); \
        simd<value_t>::op(x.data_[29], y, data.data_[29]); \
        simd<value_t>::op(x.data_[30], y, data.data_[30]); \
        simd<value_t>::op(x.data_[31], y, data.data_[31]); \
    }

#define FUNCTION_HORIZANTAL(op)               \
    simd<value_t>::op(x, y.data_[0], x);      \
    simd<value_t>::op(x, y.data_[1], x);      \
    if constexpr (N >= 4)                     \
    {                                         \
        simd<value_t>::op(x, y.data_[2], x);  \
        simd<value_t>::op(x, y.data_[3], x);  \
    }                                         \
    if constexpr (N >= 8)                     \
    {                                         \
        simd<value_t>::op(x, y.data_[4], x);  \
        simd<value_t>::op(x, y.data_[5], x);  \
        simd<value_t>::op(x, y.data_[6], x);  \
        simd<value_t>::op(x, y.data_[7], x);  \
    }                                         \
    if constexpr (N >= 16)                    \
    {                                         \
        simd<value_t>::op(x, y.data_[8], x);  \
        simd<value_t>::op(x, y.data_[9], x);  \
        simd<value_t>::op(x, y.data_[10], x); \
        simd<value_t>::op(x, y.data_[11], x); \
        simd<value_t>::op(x, y.data_[12], x); \
        simd<value_t>::op(x, y.data_[13], x); \
        simd<value_t>::op(x, y.data_[14], x); \
        simd<value_t>::op(x, y.data_[15], x); \
    }                                         \
    if constexpr (N >= 32)                    \
    {                                         \
        simd<value_t>::op(x, y.data_[16], x); \
        simd<value_t>::op(x, y.data_[17], x); \
        simd<value_t>::op(x, y.data_[18], x); \
        simd<value_t>::op(x, y.data_[19], x); \
        simd<value_t>::op(x, y.data_[20], x); \
        simd<value_t>::op(x, y.data_[21], x); \
        simd<value_t>::op(x, y.data_[22], x); \
        simd<value_t>::op(x, y.data_[23], x); \
        simd<value_t>::op(x, y.data_[24], x); \
        simd<value_t>::op(x, y.data_[25], x); \
        simd<value_t>::op(x, y.data_[26], x); \
        simd<value_t>::op(x, y.data_[27], x); \
        simd<value_t>::op(x, y.data_[28], x); \
        simd<value_t>::op(x, y.data_[29], x); \
        simd<value_t>::op(x, y.data_[30], x); \
        simd<value_t>::op(x, y.data_[31], x); \
    }

#define FUNCTION_ONE_ARGS(op)                           \
    simd<value_t>::op(x.data_[0], data.data_[0]);       \
    simd<value_t>::op(x.data_[1], data.data_[1]);       \
    if constexpr (N >= 4)                               \
    {                                                   \
        simd<value_t>::op(x.data_[2], data.data_[2]);   \
        simd<value_t>::op(x.data_[3], data.data_[3]);   \
    }                                                   \
    if constexpr (N >= 8)                               \
    {                                                   \
        simd<value_t>::op(x.data_[4], data.data_[4]);   \
        simd<value_t>::op(x.data_[5], data.data_[5]);   \
        simd<value_t>::op(x.data_[6], data.data_[6]);   \
        simd<value_t>::op(x.data_[7], data.data_[7]);   \
    }                                                   \
    if constexpr (N >= 16)                              \
    {                                                   \
        simd<value_t>::op(x.data_[8], data.data_[8]);   \
        simd<value_t>::op(x.data_[9], data.data_[9]);   \
        simd<value_t>::op(x.data_[10], data.data_[10]); \
        simd<value_t>::op(x.data_[11], data.data_[11]); \
        simd<value_t>::op(x.data_[12], data.data_[12]); \
        simd<value_t>::op(x.data_[13], data.data_[13]); \
        simd<value_t>::op(x.data_[14], data.data_[14]); \
        simd<value_t>::op(x.data_[15], data.data_[15]); \
    }                                                   \
    if constexpr (N >= 32)                              \
    {                                                   \
        simd<value_t>::op(x.data_[16], data.data_[16]); \
        simd<value_t>::op(x.data_[17], data.data_[17]); \
        simd<value_t>::op(x.data_[18], data.data_[18]); \
        simd<value_t>::op(x.data_[19], data.data_[19]); \
        simd<value_t>::op(x.data_[20], data.data_[20]); \
        simd<value_t>::op(x.data_[21], data.data_[21]); \
        simd<value_t>::op(x.data_[22], data.data_[22]); \
        simd<value_t>::op(x.data_[23], data.data_[23]); \
        simd<value_t>::op(x.data_[24], data.data_[24]); \
        simd<value_t>::op(x.data_[25], data.data_[25]); \
        simd<value_t>::op(x.data_[26], data.data_[26]); \
        simd<value_t>::op(x.data_[27], data.data_[27]); \
        simd<value_t>::op(x.data_[28], data.data_[28]); \
        simd<value_t>::op(x.data_[29], data.data_[29]); \
        simd<value_t>::op(x.data_[30], data.data_[30]); \
        simd<value_t>::op(x.data_[31], data.data_[31]); \
    }
}  // namespace vectorization

namespace vectorization
{
template <typename simd_t, uint32_t N>
struct array
{
    VECTORIZATION_ALIGN(VECTORIZATION_ALIGNMENT)
    simd_t data_[N];

    VECTORIZATION_NODISCARD constexpr const auto& operator[](const ptrdiff_t _Off) const noexcept
    {
        return data_[_Off];
    }

    VECTORIZATION_NODISCARD constexpr auto& operator[](const ptrdiff_t _Off) noexcept
    {
        return data_[_Off];
    }

    VECTORIZATION_NODISCARD constexpr const auto& data() const { return data_; }

    VECTORIZATION_NODISCARD constexpr auto& data() { return data_; }
};

template <typename T, uint32_t N = packet_size<T>::value>
struct packet
{
    using value_t = T;

#if VECTORIZATION_VECTORIZED
    using simd_t                     = typename simd<value_t>::simd_t;
    using mask_t                     = typename simd<value_t>::mask_t;
    static constexpr uint32_t Length = N * simd<value_t>::size;
#else
    using simd_t                     = value_t;
    using mask_t                     = bool;
    static constexpr uint32_t Length = N;
#endif  // VECTORIZATION_VECTORIZED

    using array_simd_t = typename vectorization::array<simd_t, N>;
    using array_mask_t = typename vectorization::array<mask_t, N>;

    static constexpr uint32_t size() noexcept { return N; }

    static constexpr uint32_t length() noexcept { return Length; }

    // static constexpr uint32_t alignment() noexcept { return VECTORIZATION_ALIGNMENT; }

    VECTORIZATION_SIMD_RETURN_TYPE prefetch(value_t const* from)
    {
        simd<value_t>::prefetch(from);
        simd<value_t>::prefetch(from + simd<value_t>::size);
        if constexpr (N >= 4)
        {
            const auto* ptr = from + 2 * simd<value_t>::size;
            simd<value_t>::prefetch(ptr);
            simd<value_t>::prefetch(ptr + simd<value_t>::size);
        }
        if constexpr (N >= 8)
        {
            const auto* ptr = from + 4 * simd<value_t>::size;

            simd<value_t>::prefetch(ptr);
            simd<value_t>::prefetch(ptr + simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 2 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 3 * simd<value_t>::size);
        }
        if constexpr (N >= 16)
        {
            const auto* ptr = from + 8 * simd<value_t>::size;

            simd<value_t>::prefetch(ptr);
            simd<value_t>::prefetch(ptr + simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 2 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 3 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 4 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 5 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 6 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 7 * simd<value_t>::size);
        }
        if constexpr (N >= 32)
        {
            const auto* ptr = from + 16 * simd<value_t>::size;

            simd<value_t>::prefetch(ptr);
            simd<value_t>::prefetch(ptr + simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 2 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 3 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 4 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 5 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 6 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 7 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 8 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 9 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 10 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 11 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 12 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 13 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 14 * simd<value_t>::size);
            simd<value_t>::prefetch(ptr + 15 * simd<value_t>::size);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE load(value_t const* from, array_simd_t& data){LOAD(load)}

    VECTORIZATION_SIMD_RETURN_TYPE loadu(value_t const* from, array_simd_t& data)
    {
        // LOAD(loadu)
        simd<value_t>::loadu(from, data.data_[0]);
        simd<value_t>::loadu(from + simd<value_t>::size, data.data_[1]);
        if constexpr (N >= 4)
        {
            const auto* ptr = from + 2 * simd<value_t>::size;
            simd<value_t>::loadu(ptr, data.data_[2]);
            simd<value_t>::loadu(ptr + simd<value_t>::size, data.data_[3]);
        }
        if constexpr (N >= 8)
        {
            const auto* ptr = from + 4 * simd<value_t>::size;

            simd<value_t>::loadu(ptr, data.data_[4]);
            simd<value_t>::loadu(ptr + simd<value_t>::size, data.data_[5]);
            simd<value_t>::loadu(ptr + 2 * simd<value_t>::size, data.data_[6]);
            simd<value_t>::loadu(ptr + 3 * simd<value_t>::size, data.data_[7]);
        }
        if constexpr (N >= 16)
        {
            const auto* ptr = from + 8 * simd<value_t>::size;

            simd<value_t>::loadu(ptr, data.data_[8]);
            simd<value_t>::loadu(ptr + simd<value_t>::size, data.data_[9]);
            simd<value_t>::loadu(ptr + 2 * simd<value_t>::size, data.data_[10]);
            simd<value_t>::loadu(ptr + 3 * simd<value_t>::size, data.data_[11]);
            simd<value_t>::loadu(ptr + 4 * simd<value_t>::size, data.data_[12]);
            simd<value_t>::loadu(ptr + 5 * simd<value_t>::size, data.data_[13]);
            simd<value_t>::loadu(ptr + 6 * simd<value_t>::size, data.data_[14]);
            simd<value_t>::loadu(ptr + 7 * simd<value_t>::size, data.data_[15]);
        }
        if constexpr (N >= 32)
        {
            const auto* ptr = from + 16 * simd<value_t>::size;

            simd<value_t>::loadu(ptr, data.data_[16]);
            simd<value_t>::loadu(ptr + simd<value_t>::size, data.data_[17]);
            simd<value_t>::loadu(ptr + 2 * simd<value_t>::size, data.data_[18]);
            simd<value_t>::loadu(ptr + 3 * simd<value_t>::size, data.data_[19]);
            simd<value_t>::loadu(ptr + 4 * simd<value_t>::size, data.data_[20]);
            simd<value_t>::loadu(ptr + 5 * simd<value_t>::size, data.data_[21]);
            simd<value_t>::loadu(ptr + 6 * simd<value_t>::size, data.data_[22]);
            simd<value_t>::loadu(ptr + 7 * simd<value_t>::size, data.data_[23]);
            simd<value_t>::loadu(ptr + 8 * simd<value_t>::size, data.data_[24]);
            simd<value_t>::loadu(ptr + 9 * simd<value_t>::size, data.data_[25]);
            simd<value_t>::loadu(ptr + 10 * simd<value_t>::size, data.data_[26]);
            simd<value_t>::loadu(ptr + 11 * simd<value_t>::size, data.data_[27]);
            simd<value_t>::loadu(ptr + 12 * simd<value_t>::size, data.data_[28]);
            simd<value_t>::loadu(ptr + 13 * simd<value_t>::size, data.data_[29]);
            simd<value_t>::loadu(ptr + 14 * simd<value_t>::size, data.data_[30]);
            simd<value_t>::loadu(ptr + 15 * simd<value_t>::size, data.data_[31]);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE store(array_simd_t const& data, value_t* to) { STORE(store); }

    VECTORIZATION_SIMD_RETURN_TYPE storeu(array_simd_t const& data, value_t* to) { STORE(storeu); }

    VECTORIZATION_SIMD_RETURN_TYPE set(value_t a, array_simd_t& data)
    {
        simd<value_t>::set(a, data.data_[0]);
        simd<value_t>::set(a, data.data_[1]);
        if constexpr (N >= 4)
        {
            simd<value_t>::set(a, data.data_[2]);
            simd<value_t>::set(a, data.data_[3]);
        }
        if constexpr (N >= 8)
        {
            simd<value_t>::set(a, data.data_[4]);
            simd<value_t>::set(a, data.data_[5]);
            simd<value_t>::set(a, data.data_[6]);
            simd<value_t>::set(a, data.data_[7]);
        }
        if constexpr (N >= 16)
        {
            simd<value_t>::set(a, data.data_[8]);
            simd<value_t>::set(a, data.data_[9]);
            simd<value_t>::set(a, data.data_[10]);
            simd<value_t>::set(a, data.data_[11]);
            simd<value_t>::set(a, data.data_[12]);
            simd<value_t>::set(a, data.data_[13]);
            simd<value_t>::set(a, data.data_[14]);
            simd<value_t>::set(a, data.data_[15]);
        }
        if constexpr (N >= 32)
        {
            simd<value_t>::set(a, data.data_[16]);
            simd<value_t>::set(a, data.data_[17]);
            simd<value_t>::set(a, data.data_[18]);
            simd<value_t>::set(a, data.data_[19]);
            simd<value_t>::set(a, data.data_[20]);
            simd<value_t>::set(a, data.data_[21]);
            simd<value_t>::set(a, data.data_[22]);
            simd<value_t>::set(a, data.data_[23]);
            simd<value_t>::set(a, data.data_[24]);
            simd<value_t>::set(a, data.data_[25]);
            simd<value_t>::set(a, data.data_[26]);
            simd<value_t>::set(a, data.data_[27]);
            simd<value_t>::set(a, data.data_[28]);
            simd<value_t>::set(a, data.data_[29]);
            simd<value_t>::set(a, data.data_[30]);
            simd<value_t>::set(a, data.data_[31]);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE setzero(array_simd_t& data)
    {
        simd<value_t>::setzero(data.data_[0]);
        simd<value_t>::setzero(data.data_[1]);
        if constexpr (N >= 4)
        {
            simd<value_t>::setzero(data.data_[2]);
            simd<value_t>::setzero(data.data_[3]);
        }
        if constexpr (N >= 8)
        {
            simd<value_t>::setzero(data.data_[4]);
            simd<value_t>::setzero(data.data_[5]);
            simd<value_t>::setzero(data.data_[6]);
            simd<value_t>::setzero(data.data_[7]);
        }
        if constexpr (N >= 16)
        {
            simd<value_t>::setzero(data.data_[8]);
            simd<value_t>::setzero(data.data_[9]);
            simd<value_t>::setzero(data.data_[10]);
            simd<value_t>::setzero(data.data_[11]);
            simd<value_t>::setzero(data.data_[12]);
            simd<value_t>::setzero(data.data_[13]);
            simd<value_t>::setzero(data.data_[14]);
            simd<value_t>::setzero(data.data_[15]);
        }
        if constexpr (N >= 32)
        {
            simd<value_t>::setzero(data.data_[16]);
            simd<value_t>::setzero(data.data_[17]);
            simd<value_t>::setzero(data.data_[18]);
            simd<value_t>::setzero(data.data_[19]);
            simd<value_t>::setzero(data.data_[20]);
            simd<value_t>::setzero(data.data_[21]);
            simd<value_t>::setzero(data.data_[22]);
            simd<value_t>::setzero(data.data_[23]);
            simd<value_t>::setzero(data.data_[24]);
            simd<value_t>::setzero(data.data_[25]);
            simd<value_t>::setzero(data.data_[26]);
            simd<value_t>::setzero(data.data_[27]);
            simd<value_t>::setzero(data.data_[28]);
            simd<value_t>::setzero(data.data_[29]);
            simd<value_t>::setzero(data.data_[30]);
            simd<value_t>::setzero(data.data_[31]);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int const* offsets, array_simd_t& data)
    {
        simd<value_t>::gather(from, offsets, data.data_[0]);
        simd<value_t>::gather(from, offsets + simd<value_t>::size, data.data_[1]);
        if constexpr (N >= 4)
        {
            const auto* from_offsets = offsets + 2 * simd<value_t>::size;

            simd<value_t>::gather(from, from_offsets, data.data_[2]);
            simd<value_t>::gather(from, from_offsets + simd<value_t>::size, data.data_[3]);
        }
        if constexpr (N >= 8)
        {
            const auto* from_offsets = offsets + 4 * simd<value_t>::size;

            simd<value_t>::gather(from, from_offsets, data.data_[4]);
            simd<value_t>::gather(from, from_offsets + simd<value_t>::size, data.data_[5]);
            simd<value_t>::gather(from, from_offsets + 2 * simd<value_t>::size, data.data_[6]);
            simd<value_t>::gather(from, from_offsets + 3 * simd<value_t>::size, data.data_[7]);
        }
        if constexpr (N >= 16)
        {
            const auto* from_offsets = offsets + 8 * simd<value_t>::size;

            simd<value_t>::gather(from, from_offsets, data.data_[8]);
            simd<value_t>::gather(from, from_offsets + simd<value_t>::size, data.data_[9]);
            simd<value_t>::gather(from, from_offsets + 2 * simd<value_t>::size, data.data_[10]);
            simd<value_t>::gather(from, from_offsets + 3 * simd<value_t>::size, data.data_[11]);
            simd<value_t>::gather(from, from_offsets + 4 * simd<value_t>::size, data.data_[12]);
            simd<value_t>::gather(from, from_offsets + 5 * simd<value_t>::size, data.data_[13]);
            simd<value_t>::gather(from, from_offsets + 6 * simd<value_t>::size, data.data_[14]);
            simd<value_t>::gather(from, from_offsets + 7 * simd<value_t>::size, data.data_[15]);
        }
        if constexpr (N >= 32)
        {
            const auto* from_offsets = offsets + 16 * simd<value_t>::size;

            simd<value_t>::gather(from, from_offsets, data.data_[16]);
            simd<value_t>::gather(from, from_offsets + simd<value_t>::size, data.data_[17]);
            simd<value_t>::gather(from, from_offsets + 2 * simd<value_t>::size, data.data_[18]);
            simd<value_t>::gather(from, from_offsets + 3 * simd<value_t>::size, data.data_[19]);
            simd<value_t>::gather(from, from_offsets + 4 * simd<value_t>::size, data.data_[20]);
            simd<value_t>::gather(from, from_offsets + 5 * simd<value_t>::size, data.data_[21]);
            simd<value_t>::gather(from, from_offsets + 6 * simd<value_t>::size, data.data_[22]);
            simd<value_t>::gather(from, from_offsets + 7 * simd<value_t>::size, data.data_[23]);
            simd<value_t>::gather(from, from_offsets + 8 * simd<value_t>::size, data.data_[24]);
            simd<value_t>::gather(from, from_offsets + 9 * simd<value_t>::size, data.data_[25]);
            simd<value_t>::gather(from, from_offsets + 10 * simd<value_t>::size, data.data_[26]);
            simd<value_t>::gather(from, from_offsets + 11 * simd<value_t>::size, data.data_[27]);
            simd<value_t>::gather(from, from_offsets + 12 * simd<value_t>::size, data.data_[28]);
            simd<value_t>::gather(from, from_offsets + 13 * simd<value_t>::size, data.data_[29]);
            simd<value_t>::gather(from, from_offsets + 14 * simd<value_t>::size, data.data_[30]);
            simd<value_t>::gather(from, from_offsets + 15 * simd<value_t>::size, data.data_[31]);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE gather(value_t const* from, int const offset, array_simd_t& data)
    {
        const auto stride = offset * simd<value_t>::size;

        simd<value_t>::gather(from, offset, data.data_[0]);
        simd<value_t>::gather(from + stride, offset, data.data_[1]);
        if constexpr (N >= 4)
        {
            const auto* ptr = from + 2 * stride;

            simd<value_t>::gather(ptr, offset, data.data_[2]);
            simd<value_t>::gather(ptr + stride, offset, data.data_[3]);
        }
        if constexpr (N >= 8)
        {
            const auto* ptr = from + 4 * stride;

            simd<value_t>::gather(ptr, offset, data.data_[4]);
            simd<value_t>::gather(ptr + stride, offset, data.data_[5]);
            simd<value_t>::gather(ptr + 2 * stride, offset, data.data_[6]);
            simd<value_t>::gather(ptr + 3 * stride, offset, data.data_[7]);
        }
        if constexpr (N >= 16)
        {
            const auto* ptr = from + 8 * stride;

            simd<value_t>::gather(ptr, offset, data.data_[8]);
            simd<value_t>::gather(ptr + stride, offset, data.data_[9]);
            simd<value_t>::gather(ptr + 2 * stride, offset, data.data_[10]);
            simd<value_t>::gather(ptr + 3 * stride, offset, data.data_[11]);
            simd<value_t>::gather(ptr + 4 * stride, offset, data.data_[12]);
            simd<value_t>::gather(ptr + 5 * stride, offset, data.data_[13]);
            simd<value_t>::gather(ptr + 6 * stride, offset, data.data_[14]);
            simd<value_t>::gather(ptr + 7 * stride, offset, data.data_[15]);
        }
        if constexpr (N >= 32)
        {
            const auto* ptr = from + 16 * stride;

            simd<value_t>::gather(ptr, offset, data.data_[16]);
            simd<value_t>::gather(ptr + stride, offset, data.data_[17]);
            simd<value_t>::gather(ptr + 2 * stride, offset, data.data_[18]);
            simd<value_t>::gather(ptr + 3 * stride, offset, data.data_[19]);
            simd<value_t>::gather(ptr + 4 * stride, offset, data.data_[20]);
            simd<value_t>::gather(ptr + 5 * stride, offset, data.data_[21]);
            simd<value_t>::gather(ptr + 6 * stride, offset, data.data_[22]);
            simd<value_t>::gather(ptr + 7 * stride, offset, data.data_[23]);
            simd<value_t>::gather(ptr + 8 * stride, offset, data.data_[24]);
            simd<value_t>::gather(ptr + 9 * stride, offset, data.data_[25]);
            simd<value_t>::gather(ptr + 10 * stride, offset, data.data_[26]);
            simd<value_t>::gather(ptr + 11 * stride, offset, data.data_[27]);
            simd<value_t>::gather(ptr + 12 * stride, offset, data.data_[28]);
            simd<value_t>::gather(ptr + 13 * stride, offset, data.data_[29]);
            simd<value_t>::gather(ptr + 14 * stride, offset, data.data_[30]);
            simd<value_t>::gather(ptr + 15 * stride, offset, data.data_[31]);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(array_simd_t const& data, int const* offsets, value_t* to)
    {
        simd<value_t>::scatter(data.data_[0], offsets, to);
        simd<value_t>::scatter(data.data_[1], offsets + simd<value_t>::size, to);
        if constexpr (N >= 4)
        {
            const auto* to_offsets = offsets + 2 * simd<value_t>::size;

            simd<value_t>::scatter(data.data_[2], to_offsets, to);
            simd<value_t>::scatter(data.data_[3], to_offsets + simd<value_t>::size, to);
        }
        if constexpr (N >= 8)
        {
            const auto* to_offsets = offsets + 4 * simd<value_t>::size;

            simd<value_t>::scatter(data.data_[4], to_offsets, to);
            simd<value_t>::scatter(data.data_[5], to_offsets + simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[6], to_offsets + 2 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[7], to_offsets + 3 * simd<value_t>::size, to);
        }
        if constexpr (N >= 16)
        {
            const auto* to_offsets = offsets + 8 * simd<value_t>::size;

            simd<value_t>::scatter(data.data_[8], to_offsets, to);
            simd<value_t>::scatter(data.data_[9], to_offsets + simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[10], to_offsets + 2 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[11], to_offsets + 3 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[12], to_offsets + 4 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[13], to_offsets + 5 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[14], to_offsets + 6 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[15], to_offsets + 7 * simd<value_t>::size, to);
        }
        if constexpr (N >= 32)
        {
            const auto* to_offsets = offsets + 16 * simd<value_t>::size;

            simd<value_t>::scatter(data.data_[16], to_offsets, to);
            simd<value_t>::scatter(data.data_[17], to_offsets + simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[18], to_offsets + 2 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[19], to_offsets + 3 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[20], to_offsets + 4 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[21], to_offsets + 5 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[22], to_offsets + 6 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[23], to_offsets + 7 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[24], to_offsets + 8 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[25], to_offsets + 9 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[26], to_offsets + 10 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[27], to_offsets + 11 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[28], to_offsets + 12 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[29], to_offsets + 13 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[30], to_offsets + 14 * simd<value_t>::size, to);
            simd<value_t>::scatter(data.data_[31], to_offsets + 15 * simd<value_t>::size, to);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE scatter(array_simd_t const& data, int const offset, value_t* to)
    {
        const auto stride = offset * simd<value_t>::size;

        simd<value_t>::scatter(data.data_[0], offset, to);
        simd<value_t>::scatter(data.data_[1], offset, to + stride);
        if constexpr (N >= 4)
        {
            auto* ptr = to + 2 * stride;
            simd<value_t>::scatter(data.data_[2], offset, ptr);
            simd<value_t>::scatter(data.data_[3], offset, ptr + stride);
        }
        if constexpr (N >= 8)
        {
            auto* ptr = to + 4 * stride;

            simd<value_t>::scatter(data.data_[4], offset, ptr);
            simd<value_t>::scatter(data.data_[5], offset, ptr + stride);
            simd<value_t>::scatter(data.data_[6], offset, ptr + 2 * stride);
            simd<value_t>::scatter(data.data_[7], offset, ptr + 3 * stride);
        }
        if constexpr (N >= 16)
        {
            auto* ptr = to + 8 * stride;

            simd<value_t>::scatter(data.data_[8], offset, ptr);
            simd<value_t>::scatter(data.data_[9], offset, ptr + stride);
            simd<value_t>::scatter(data.data_[10], offset, ptr + 2 * stride);
            simd<value_t>::scatter(data.data_[11], offset, ptr + 3 * stride);
            simd<value_t>::scatter(data.data_[12], offset, ptr + 4 * stride);
            simd<value_t>::scatter(data.data_[13], offset, ptr + 5 * stride);
            simd<value_t>::scatter(data.data_[14], offset, ptr + 6 * stride);
            simd<value_t>::scatter(data.data_[15], offset, ptr + 7 * stride);
        }
        if constexpr (N >= 32)
        {
            auto* ptr = to + 16 * stride;

            simd<value_t>::scatter(data.data_[16], offset, ptr);
            simd<value_t>::scatter(data.data_[17], offset, ptr + stride);
            simd<value_t>::scatter(data.data_[18], offset, ptr + 2 * stride);
            simd<value_t>::scatter(data.data_[19], offset, ptr + 3 * stride);
            simd<value_t>::scatter(data.data_[20], offset, ptr + 4 * stride);
            simd<value_t>::scatter(data.data_[21], offset, ptr + 5 * stride);
            simd<value_t>::scatter(data.data_[22], offset, ptr + 6 * stride);
            simd<value_t>::scatter(data.data_[23], offset, ptr + 7 * stride);
            simd<value_t>::scatter(data.data_[24], offset, ptr + 8 * stride);
            simd<value_t>::scatter(data.data_[25], offset, ptr + 9 * stride);
            simd<value_t>::scatter(data.data_[26], offset, ptr + 10 * stride);
            simd<value_t>::scatter(data.data_[27], offset, ptr + 11 * stride);
            simd<value_t>::scatter(data.data_[28], offset, ptr + 12 * stride);
            simd<value_t>::scatter(data.data_[29], offset, ptr + 13 * stride);
            simd<value_t>::scatter(data.data_[30], offset, ptr + 14 * stride);
            simd<value_t>::scatter(data.data_[31], offset, ptr + 15 * stride);
        }
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(add);
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(add);
    }

    VECTORIZATION_SIMD_RETURN_TYPE add(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(add);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(sub);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(sub);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sub(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(sub);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(mul);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(mul);
    }

    VECTORIZATION_SIMD_RETURN_TYPE mul(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(mul);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(div);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(div);
    }

    VECTORIZATION_SIMD_RETURN_TYPE div(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(div);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(pow);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(pow);
    }

    VECTORIZATION_SIMD_RETURN_TYPE pow(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(pow);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(hypot);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(hypot);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hypot(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(hypot);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(min);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(min);
    }

    VECTORIZATION_SIMD_RETURN_TYPE min(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(min);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(max);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(max);
    }

    VECTORIZATION_SIMD_RETURN_TYPE max(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(max);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(
        array_simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS(signcopy);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(array_simd_t const& x, simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(signcopy);
    }

    VECTORIZATION_SIMD_RETURN_TYPE signcopy(simd_t const& x, array_simd_t const& y, array_simd_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(signcopy);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        simd_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_X_SCALAR(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_Y_SCALAR(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& x, array_simd_t const& y, simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_Z_SCALAR(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        simd_t const& x, array_simd_t const& y, simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_XZ_SCALAR(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fma(
        array_simd_t const& y, simd_t const& x, simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_XZ_SCALAR(fma);
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, array_simd_t const& y, array_simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS(if_else);
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, simd_t const y, array_simd_t const& z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_Y_SCALAR(if_else);
    }

    VECTORIZATION_SIMD_RETURN_TYPE if_else(
        array_mask_t const& x, array_simd_t const& y, simd_t const z, array_simd_t& data)
    {
        FUNCTION_THREE_ARGS_Z_SCALAR(if_else);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(eq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(eq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE eq(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(eq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(neq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(neq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neq(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(neq);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(gt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(gt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE gt(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(gt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(ge);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(ge);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ge(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(ge);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(lt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(lt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lt(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(lt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(array_simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(le);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(array_simd_t const& x, simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(le);
    }

    VECTORIZATION_SIMD_RETURN_TYPE le(simd_t const& x, array_simd_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(le);
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(and_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(or_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(array_mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS(xor_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(and_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(or_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(mask_t const& x, array_mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_X_SCALAR(xor_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE land(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(and_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lor(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(or_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lxor(array_mask_t const& x, mask_t const& y, array_mask_t& data)
    {
        FUNCTION_TWO_ARGS_Y_SCALAR(xor_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE lnot(array_mask_t const& x, array_mask_t& data)
    {
        FUNCTION_ONE_ARGS(not_mask);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqrt(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(sqrt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sqr(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(sqr);
    }

    VECTORIZATION_SIMD_RETURN_TYPE ceil(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(ceil);
    }

    VECTORIZATION_SIMD_RETURN_TYPE floor(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(floor);
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(exp);
    }

    VECTORIZATION_SIMD_RETURN_TYPE expm1(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(expm1);
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp2(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(exp2);
    }

    VECTORIZATION_SIMD_RETURN_TYPE exp10(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(exp10);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(log);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log1p(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(log1p);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log2(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(log2);
    }

    VECTORIZATION_SIMD_RETURN_TYPE log10(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(log10);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sin(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(sin);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cos(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(cos);
    }

    VECTORIZATION_SIMD_RETURN_TYPE tan(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(tan);
    }

    VECTORIZATION_SIMD_RETURN_TYPE asin(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(asin);
    }

    VECTORIZATION_SIMD_RETURN_TYPE acos(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(acos);
    }

    VECTORIZATION_SIMD_RETURN_TYPE atan(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(atan);
    }

    VECTORIZATION_SIMD_RETURN_TYPE sinh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(sinh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cosh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(cosh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE tanh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(tanh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE asinh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(asinh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE acosh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(acosh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE atanh(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(atanh);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cbrt(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(cbrt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE cdf(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(cdf);
    }

    VECTORIZATION_SIMD_RETURN_TYPE inv_cdf(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(inv_cdf);
    }

    VECTORIZATION_SIMD_RETURN_TYPE trunc(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(trunc);
    }

    VECTORIZATION_SIMD_RETURN_TYPE invsqrt(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(invsqrt);
    }

    VECTORIZATION_SIMD_RETURN_TYPE fabs(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(fabs);
    }

    VECTORIZATION_SIMD_RETURN_TYPE neg(array_simd_t const& x, array_simd_t& data)
    {
        FUNCTION_ONE_ARGS(neg);
    }

    VECTORIZATION_SIMD_RETURN_TYPE accumulate(array_simd_t const& y, simd_t& x)
    {
        FUNCTION_HORIZANTAL(add);
    }

    VECTORIZATION_SIMD_RETURN_TYPE hmax(array_simd_t const& y, simd_t& x) { FUNCTION_HORIZANTAL(max); }

    VECTORIZATION_SIMD_RETURN_TYPE hmin(array_simd_t const& y, simd_t& x) { FUNCTION_HORIZANTAL(min); }
};  // namespace packet
}  // namespace vectorization

