#pragma once

#include "bespoke/common/unwind/unwind.h"
#include "common/profiler_export.h"

namespace quarisma
{
PROFILER_API bool get_cpp_stacktraces_enabled();
PROFILER_API quarisma::unwind::Mode get_symbolize_mode();
}  // namespace quarisma
