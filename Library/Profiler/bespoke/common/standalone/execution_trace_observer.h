#pragma once

#include <string>

#include "common/profiler_export.h"

namespace profiler::profiler_impl::impl
{

// Adds the execution trace observer as a global callback function, the data
// will be written to output file path.
PROFILER_API bool addExecutionTraceObserver(const std::string& output_file_path);

// Remove the execution trace observer from the global callback functions.
PROFILER_API void removeExecutionTraceObserver();

// Enables execution trace observer.
PROFILER_API void enableExecutionTraceObserver();

// Disables execution trace observer.
PROFILER_API void disableExecutionTraceObserver();

}  // namespace profiler::profiler_impl::impl
