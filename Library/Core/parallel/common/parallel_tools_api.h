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

#ifndef PARALLEL_TOOLS_API_H
#define PARALLEL_TOOLS_API_H

#include <memory>

#include "common/configure.h"
#include "common/export.h"
#include "parallel/common/parallel_tools_impl.h"
#include "parallel/parallel.h"

#if QUARISMA_HAS_TBB
#include "parallel/tbb/parallel_tools_impl.h"
#elif QUARISMA_HAS_OPENMP
#include "parallel/openmp/parallel_tools_impl.h"
#else
#include "parallel/std_thread/parallel_tools_impl.h"
#endif

namespace quarisma
{
namespace detail
{
namespace parallel
{

// Compile-time backend selection based on availability
// Priority: TBB > OpenMP > std_thread
#if QUARISMA_HAS_TBB
constexpr backend_type selected_backend_tools = backend_type::TBB;
#elif QUARISMA_HAS_OPENMP
constexpr backend_type selected_backend_tools = backend_type::OpenMP;
#else
constexpr backend_type selected_backend_tools = backend_type::std_thread;
#endif

using selected_parallel_tools_impl = parallel_tools_impl<selected_backend_tools>;

class QUARISMA_VISIBILITY parallel_tools_api
{
public:
    //--------------------------------------------------------------------------------
    QUARISMA_API static parallel_tools_api& instance();

    //--------------------------------------------------------------------------------
    QUARISMA_API static backend_type get_backend_type();

    //--------------------------------------------------------------------------------
    QUARISMA_API static const char* get_backend();

    //--------------------------------------------------------------------------------
    QUARISMA_API bool set_backend(const char* type);

    //--------------------------------------------------------------------------------
    QUARISMA_API void initialize(int num_threads = 0);

    //--------------------------------------------------------------------------------
    QUARISMA_API int estimated_number_of_threads();

    //--------------------------------------------------------------------------------
    QUARISMA_API int estimated_default_number_of_threads();

    //------------------------------------------------------------------------------
    QUARISMA_API void set_nested_parallelism(bool is_nested);

    //--------------------------------------------------------------------------------
    QUARISMA_API bool nested_parallelism();

    //--------------------------------------------------------------------------------
    QUARISMA_API bool is_parallel_scope();

    //--------------------------------------------------------------------------------
    QUARISMA_API bool single_thread();

    //--------------------------------------------------------------------------------
    int get_internal_desired_number_of_thread() { return desired_number_of_thread_; }

    //------------------------------------------------------------------------------
    template <typename Config, typename T>
    void local_scope(Config const& config, T&& lambda)
    {
        const Config old_config(*this);
        *this << config;
        try
        {
            lambda();
        }
        catch (...)
        {
            *this << old_config;
            throw;
        }
        *this << old_config;
    }

    //--------------------------------------------------------------------------------
    // Direct backend execution using compile-time selected backend
    //--------------------------------------------------------------------------------
    template <typename FunctorInternal>
    void parallel_for(size_t first, size_t last, size_t grain, FunctorInternal& fi)
    {
        backend_impl_.parallel_for(first, last, grain, fi);
    }

    // disable copying
    parallel_tools_api(parallel_tools_api const&) = delete;
    void operator=(parallel_tools_api const&)     = delete;

private:
    //--------------------------------------------------------------------------------
    QUARISMA_API parallel_tools_api();

    //--------------------------------------------------------------------------------
    QUARISMA_API void refresh_number_of_thread();

    //--------------------------------------------------------------------------------
    template <typename Config>
    parallel_tools_api& operator<<(Config const& config)
    {
        this->initialize(config.max_number_of_threads_);
        this->set_backend(config.backend_.c_str());
        this->set_nested_parallelism(config.nested_parallelism_);
        return *this;
    }

    /**
   * Desired number of threads
   */
    int desired_number_of_thread_ = 0;

    /**
   * Single backend implementation selected at compile-time
   */
    selected_parallel_tools_impl backend_impl_;
};

}  // namespace parallel
}  // namespace detail
}  // namespace quarisma

#endif
