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

#include "parallel/common/parallel_tools_impl.h"

#include <charconv>
#include <cstdlib>  // For std::getenv()
#include <memory>   // For std::unique_ptr
#include <mutex>    // For std::mutex
#include <stack>    // For std::stack
#include <string>

#include "parallel/tbb/parallel_tools_impl.h"

#ifdef _MSC_VER
#pragma push_macro("__TBB_NO_IMPLICIT_LINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1  // NOLINT(bugprone-reserved-identifier)
#endif

#include <tbb/task_arena.h>  // For tbb:task_arena

#ifdef _MSC_VER
#pragma pop_macro("__TBB_NO_IMPLICIT_LINKAGE")
#endif

namespace quarisma
{
namespace detail
{
namespace parallel
{

static std::unique_ptr<tbb::task_arena> task_arena;
static std::unique_ptr<std::mutex>      quarisma_parallel_tools_cs;
static std::unique_ptr<std::stack<int>> thread_id_stack;
static std::unique_ptr<std::mutex>      thread_id_stack_lock;
static int                              specified_num_threads_tbb;  // Default initialized to zero

//------------------------------------------------------------------------------
// Must NOT be initialized. Default initialization to zero is necessary.
// NOTE: This variable must NOT be static - it needs to be visible across
// translation units for proper initialization/deinitialization tracking.
static unsigned int parallel_tools_impl_tbb_initialize_count;

//------------------------------------------------------------------------------
parallel_tools_impl_tbb_initialize::parallel_tools_impl_tbb_initialize()
{
    if (++parallel_tools_impl_tbb_initialize_count == 1)
    {
        task_arena                 = std::make_unique<tbb::task_arena>();
        quarisma_parallel_tools_cs = std::make_unique<std::mutex>();
        thread_id_stack            = std::make_unique<std::stack<int>>();
        thread_id_stack_lock       = std::make_unique<std::mutex>();
    }
}

//------------------------------------------------------------------------------
parallel_tools_impl_tbb_initialize::
    ~parallel_tools_impl_tbb_initialize()  // NOLINT(modernize-use-equals-default)
{
    // Empty destructor - cleanup is handled by unique_ptr members
    // Cannot use = default due to DLL export issues on Windows
}

//------------------------------------------------------------------------------
template <>
parallel_tools_impl<backend_type::TBB>::parallel_tools_impl() : nested_activated_(true)
{
}

//------------------------------------------------------------------------------
template <>
void parallel_tools_impl<backend_type::TBB>::initialize(int num_threads)
{
    quarisma_parallel_tools_cs->lock();

    if (num_threads == 0)
    {
        const char* parallel_num_threads = std::getenv("PARALLEL_MAX_THREADS");
        if (parallel_num_threads != nullptr)
        {
            std::string str(parallel_num_threads);
            auto        result = std::from_chars(str.data(), str.data() + str.size(), num_threads);
            if (result.ec != std::errc())
            {
                num_threads = 0;
            }
        }
        else if (task_arena->is_active())
        {
            task_arena->terminate();
            specified_num_threads_tbb = 0;
        }
    }
    if (num_threads > 0 &&
        num_threads <=
            parallel_tools_impl<backend_type::TBB>::estimated_default_number_of_threads())
    {
        if (task_arena->is_active())
        {
            task_arena->terminate();
        }
        task_arena->initialize(num_threads);
        specified_num_threads_tbb = num_threads;
    }

    quarisma_parallel_tools_cs->unlock();
}

//------------------------------------------------------------------------------
template <>
int parallel_tools_impl<backend_type::TBB>::estimated_number_of_threads()
{
    return specified_num_threads_tbb > 0
               ? specified_num_threads_tbb
               : parallel_tools_impl<backend_type::TBB>::estimated_default_number_of_threads();
}

//------------------------------------------------------------------------------
template <>
int parallel_tools_impl<backend_type::TBB>::estimated_default_number_of_threads()
{ return task_arena->max_concurrency(); }

//------------------------------------------------------------------------------
template <>
bool parallel_tools_impl<backend_type::TBB>::single_thread()
{
    // Check if we're inside a parallel region
    // If the stack is empty, we're not in a parallel region
    thread_id_stack_lock->lock();
    const bool is_empty = thread_id_stack->empty();
    thread_id_stack_lock->unlock();

    if (is_empty)
    {
        return false;
    }

    thread_id_stack_lock->lock();
    const int top_id = thread_id_stack->top();
    thread_id_stack_lock->unlock();

    return top_id == tbb::this_task_arena::current_thread_index();
}

//------------------------------------------------------------------------------
void parallel_tools_impl_for_tbb(
    size_t                   first,
    size_t                   last,
    size_t                   grain,
    execute_functor_ptr_type functor_executer,
    void*                    functor)
{
    thread_id_stack_lock->lock();
    thread_id_stack->emplace(tbb::this_task_arena::current_thread_index());
    thread_id_stack_lock->unlock();

    if (task_arena->is_active())
    {
        task_arena->execute([&] { functor_executer(functor, first, last, grain); });
    }
    else
    {
        functor_executer(functor, first, last, grain);
    }

    thread_id_stack_lock->lock();
    thread_id_stack->pop();
    thread_id_stack_lock->unlock();
}

}  // namespace parallel
}  // namespace detail
}  // namespace quarisma
