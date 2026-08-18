/*
 * Quarisma: High-Performance Computational Library
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

#if defined(MODELS_STATIC_DEFINE)
#define MODELS_API
#define MODELS_VISIBILITY

#elif defined(MODELS_SHARED_DEFINE)
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef MODELS_BUILDING_DLL
#define MODELS_API __declspec(dllexport)
#else
#define MODELS_API __declspec(dllimport)
#endif
#define MODELS_VISIBILITY
#elif defined(__GNUC__) && __GNUC__ >= 4
#define MODELS_API __attribute__((visibility("default")))
#define MODELS_VISIBILITY __attribute__((visibility("default")))
#else
#define MODELS_API
#define MODELS_VISIBILITY
#endif

#else
#define MODELS_API
#define MODELS_VISIBILITY
#endif
