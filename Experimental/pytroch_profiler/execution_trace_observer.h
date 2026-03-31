#pragma once

#include <string>

#include "common/export.h"

namespace quarisma::profiler_impl::impl
{

// Adds the execution trace observer as a global callback function, the data
// will be written to output file path.
QUARISMA_API bool addExecutionTraceObserver(const std::string& output_file_path);

// Remove the execution trace observer from the global callback functions.
QUARISMA_API void removeExecutionTraceObserver();

// Enables execution trace observer.
QUARISMA_API void enableExecutionTraceObserver();

// Disables execution trace observer.
QUARISMA_API void disableExecutionTraceObserver();

}  // namespace quarisma::profiler_impl::impl
