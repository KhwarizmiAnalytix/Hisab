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

#include "native/cpu/threadpool_listener_state.h"

#include <atomic>

namespace quarisma::profiler_impl::threadpool_listener
{
namespace
{
std::atomic<int> g_enabled{0};
}  // namespace

bool IsEnabled()
{ return g_enabled.load(std::memory_order_acquire) != 0; }

void Activate()
{ g_enabled.store(1, std::memory_order_release); }

void Deactivate()
{ g_enabled.store(0, std::memory_order_release); }

}  // namespace quarisma::profiler_impl::threadpool_listener
