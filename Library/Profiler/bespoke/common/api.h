#pragma once

#include "bespoke/common/orchestration/observer.h"

// There are some components which use these symbols. Until we migrate them
// we have to mirror them in the old autograd namespace.

// TODO: Quarisma-specific types commented out
namespace quarisma::autograd::profiler
{
using quarisma::profiler::impl::ActivityType;
using quarisma::profiler::impl::getProfilerConfig;
using quarisma::profiler::impl::ProfilerConfig;
using quarisma::profiler::impl::profilerEnabled;
using quarisma::profiler::impl::ProfilerState;
}  // namespace quarisma::autograd::profiler
