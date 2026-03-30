#pragma once
#include <Quarisma/record_function.h>
#include <torch/custom_class.h>

#include <optional>

namespace torch::autograd::profiler
{

struct PythonRecordFunction : public torch::CustomClassHolder
{
    quarisma::record_function record;

    explicit PythonRecordFunction(quarisma::RecordScope scope = quarisma::RecordScope::FUNCTION)
        : record(scope)
    {
    }
};

// Creates a new profiling scope using record_function and invokes its starting
// callbacks.
TORCH_API quarisma::intrusive_ptr<PythonRecordFunction> record_function_enter_new(
    const std::string& name, const std::optional<std::string>& args = std::nullopt);

// Schedules record_function's end callbacks to be run on completion of a future.
TORCH_API quarisma::intrusive_ptr<quarisma::ivalue::Future> _call_end_callbacks_on_fut_new(
    const quarisma::intrusive_ptr<PythonRecordFunction>&     record,
    const quarisma::intrusive_ptr<quarisma::ivalue::Future>& fut);

}  // namespace torch::autograd::profiler
