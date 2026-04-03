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

// Test disabling quarismaLogger's printing of a stack trace when a signal is handled.

#include <cstdlib>  // for abort, EXIT_SUCCESS

#include "logging/logger.h"  // for logger, logger::EnableUnsafeSignalHandler

int main(int /*unused*/, char* /*unused*/[])
{
    // When set to false, no stack trace should be emitted when quarismaLogger
    // catches the SIGABRT signal below.
    quarisma::logger::EnableUnsafeSignalHandler = false;
    quarisma::logger::Init();

    abort();

    return EXIT_SUCCESS;
}
