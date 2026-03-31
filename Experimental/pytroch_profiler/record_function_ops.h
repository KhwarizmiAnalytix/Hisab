#pragma once

#if 0
#include <quarisma/custom_class.h>

#include <optional>

#include "record_function.h"

namespace quarisma::autograd::profiler_impl
{

class QUARISMA_VISIBILITY PythonRecordFunction : public quarisma::CustomClassHolder
{
    quarisma::RecordFunction record;

    explicit PythonRecordFunction(quarisma::RecordScope scope = quarisma::RecordScope::FUNCTION) : record(scope)
    {
    }
};

// Creates a new profiling scope using RecordFunction and invokes its starting
// callbacks.
QUARISMA_API quarisma::intrusive_ptr<PythonRecordFunction> record_function_enter_new(
    const std::string& name, const std::optional<std::string>& args = std::nullopt);

// Schedules RecordFunction's end callbacks to be run on completion of a future.
QUARISMA_API quarisma::intrusive_ptr<quarisma::ivalue::Future> _call_end_callbacks_on_fut_new(
    const quarisma::intrusive_ptr<PythonRecordFunction>&   record,
    const quarisma::intrusive_ptr<quarisma::ivalue::Future>& fut);

}  // namespace quarisma::autograd::profiler_impl
#endif
