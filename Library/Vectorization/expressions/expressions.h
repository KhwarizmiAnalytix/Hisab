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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>

#include "common/configure.h"  // IWYU pragma: keep
#include "common/macros.h"
#include "common/scalar_helper_functions.h"
#include "common/vectorization_type_traits.h"
#include "util/exception.h"

#define __VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__
#include "expressions/expression_interface.h"
#include "expressions/expressions_functors.h"
#include "expressions/expressions_matrix.h"
#undef __VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__

#define __VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__
#include "expressions/expressions_builder.h"
#include "expressions/expressions_evaluator.h"
#undef __VECTORIZATION_EXPRESSIONS_INCLUDES_INSIDE__

