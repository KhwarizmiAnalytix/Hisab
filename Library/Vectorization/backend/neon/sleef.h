/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#pragma once

#if VECTORIZATION_HAS_NEON && VECTORIZATION_HAS_SLEEF

#include <sleef.h>

// clang-format off

// ---------------------------------------------------------------------------
// Float (float32x4_t) — 4-wide, 1 ULP accuracy
// ---------------------------------------------------------------------------
#define sleef_sinf(x)    Sleef_sinf4_u10(x)
#define sleef_cosf(x)    Sleef_cosf4_u10(x)
#define sleef_tanf(x)    Sleef_tanf4_u10(x)
#define sleef_asinf(x)   Sleef_asinf4_u10(x)
#define sleef_acosf(x)   Sleef_acosf4_u10(x)
#define sleef_atanf(x)   Sleef_atanf4_u10(x)
#define sleef_sinhf(x)   Sleef_sinhf4_u10(x)
#define sleef_coshf(x)   Sleef_coshf4_u10(x)
#define sleef_tanhf(x)   Sleef_tanhf4_u10(x)
#define sleef_asinhf(x)  Sleef_asinhf4_u10(x)
#define sleef_acoshf(x)  Sleef_acoshf4_u10(x)
#define sleef_atanhf(x)  Sleef_atanhf4_u10(x)
#define sleef_expf(x)    Sleef_expf4_u10(x)
#define sleef_exp2f(x)   Sleef_exp2f4_u10(x)
#define sleef_exp10f(x)  Sleef_exp10f4_u10(x)
#define sleef_expm1f(x)  Sleef_expm1f4_u10(x)
#define sleef_logf(x)    Sleef_logf4_u10(x)
#define sleef_log2f(x)   Sleef_log2f4_u10(x)
#define sleef_log10f(x)  Sleef_log10f4_u10(x)
#define sleef_log1pf(x)  Sleef_log1pf4_u10(x)
#define sleef_cbrtf(x)   Sleef_cbrtf4_u10(x)
#define sleef_powf(x, y) Sleef_powf4_u10((x), (y))

// ---------------------------------------------------------------------------
// Double (float64x2_t) — 2-wide, 1 ULP accuracy
// ---------------------------------------------------------------------------
#define sleef_sind(x)    Sleef_sind2_u10(x)
#define sleef_cosd(x)    Sleef_cosd2_u10(x)
#define sleef_tand(x)    Sleef_tand2_u10(x)
#define sleef_asind(x)   Sleef_asind2_u10(x)
#define sleef_acosd(x)   Sleef_acosd2_u10(x)
#define sleef_atand(x)   Sleef_atand2_u10(x)
#define sleef_sinhd(x)   Sleef_sinhd2_u10(x)
#define sleef_coshd(x)   Sleef_coshd2_u10(x)
#define sleef_tanhd(x)   Sleef_tanhd2_u10(x)
#define sleef_asinhd(x)  Sleef_asinhd2_u10(x)
#define sleef_acoshd(x)  Sleef_acoshd2_u10(x)
#define sleef_atanhd(x)  Sleef_atanhd2_u10(x)
#define sleef_expd(x)    Sleef_expd2_u10(x)
#define sleef_exp2d(x)   Sleef_exp2d2_u10(x)
#define sleef_exp10d(x)  Sleef_exp10d2_u10(x)
#define sleef_expm1d(x)  Sleef_expm1d2_u10(x)
#define sleef_logd(x)    Sleef_logd2_u10(x)
#define sleef_log2d(x)   Sleef_log2d2_u10(x)
#define sleef_log10d(x)  Sleef_log10d2_u10(x)
#define sleef_log1pd(x)  Sleef_log1pd2_u10(x)
#define sleef_cbrtd(x)   Sleef_cbrtd2_u10(x)
#define sleef_powd(x, y) Sleef_powd2_u10((x), (y))

// clang-format on

#endif  // VECTORIZATION_HAS_NEON && VECTORIZATION_HAS_SLEEF
