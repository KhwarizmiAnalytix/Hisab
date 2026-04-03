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
 *
 * Portions of this code are based on VTK (Visualization Toolkit):

 *   Licensed under BSD-3-Clause
 */

#ifndef PARALLEL_TOOLS_IMPL_H
#define PARALLEL_TOOLS_IMPL_H

#include <atomic>

#include "common/export.h"
#include "parallel/parallel.h"

namespace quarisma
{
namespace detail
{
namespace parallel
{

template <backend_type Backend>
class parallel_tools_impl
{
public:
    //--------------------------------------------------------------------------------
    void initialize(int num_threads = 0);

    //--------------------------------------------------------------------------------
    int estimated_number_of_threads();

    //--------------------------------------------------------------------------------
    static int estimated_default_number_of_threads();

    //--------------------------------------------------------------------------------
    void set_nested_parallelism(bool is_nested);

    //--------------------------------------------------------------------------------
    bool nested_parallelism();

    //--------------------------------------------------------------------------------
    bool is_parallel_scope();

    //--------------------------------------------------------------------------------
    bool single_thread();

    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void parallel_for(size_t first, size_t last, size_t grain, FunctorInternal& fi);

    //--------------------------------------------------------------------------------
    parallel_tools_impl();

    //--------------------------------------------------------------------------------
    parallel_tools_impl(const parallel_tools_impl& other);

    //--------------------------------------------------------------------------------
    void operator=(const parallel_tools_impl& other);

private:
    bool              nested_activated_ = false;
    std::atomic<bool> is_parallel_{false};
};

template <backend_type Backend>
void parallel_tools_impl<Backend>::set_nested_parallelism(bool is_nested)
{
    nested_activated_ = is_nested;
}

template <backend_type Backend>
bool parallel_tools_impl<Backend>::nested_parallelism()
{
    return nested_activated_;
}

template <backend_type Backend>
bool parallel_tools_impl<Backend>::is_parallel_scope()
{
    return is_parallel_;
}

template <backend_type Backend>
parallel_tools_impl<Backend>::parallel_tools_impl() : nested_activated_(true), is_parallel_(false)
{
}

template <backend_type Backend>
parallel_tools_impl<Backend>::parallel_tools_impl(const parallel_tools_impl& other)
    : nested_activated_(other.nested_activated_), is_parallel_(other.is_parallel_.load())
{
}

template <backend_type Backend>
void parallel_tools_impl<Backend>::operator=(const parallel_tools_impl& other)
{
    nested_activated_ = other.nested_activated_;
    is_parallel_      = other.is_parallel_.load();
}

using execute_functor_ptr_type = void (*)(void*, size_t, size_t, size_t);

}  // namespace parallel
}  // namespace detail
}  // namespace quarisma

#endif
