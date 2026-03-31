#pragma once

#include "observer.h"

// There are some components which use these symbols. Until we migrate them
// we have to mirror them in the old autograd namespace.

namespace quarisma::autograd::profiler_impl
{
using quarisma::profiler_impl::impl::ActivityType;
using quarisma::profiler_impl::impl::getProfilerConfig;
using quarisma::profiler_impl::impl::ProfilerConfig;
using quarisma::profiler_impl::impl::profilerEnabled;
using quarisma::profiler_impl::impl::ProfilerState;
}  // namespace quarisma::autograd::profiler_impl
