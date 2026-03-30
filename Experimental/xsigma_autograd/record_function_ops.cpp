#include <Quarisma/ThreadLocalState.h>
#include <Quarisma/cpp_custom_type_hack.h>
#include <Quarisma/record_function.h>
#include <torch/csrc/autograd/record_function_ops.h>
#include <torch/csrc/jit/runtime/operator.h>
#include <torch/library.h>

namespace caffe2
{
// Required for cpp_custom_type_hack to work
// NOLINTNEXTLINE(bugprone-exception-escape)
CAFFE_KNOWN_TYPE(quarisma::record_function)
}  // namespace caffe2

namespace torch::autograd::profiler
{

// Creates a new profiling scope using record_function and invokes its starting
// callbacks.
static void record_function_enter(
    const std::string& name, const std::optional<std::string>& args, quarisma::record_function& rec)
{
    if (rec.isActive())
    {
        if (rec.needsInputs() && args.has_value())
        {
            rec.before(
                name, quarisma::ArrayRef<const quarisma::IValue>{quarisma::IValue{args.value()}});
        }
        else
        {
            rec.before(name);
        }
    }
}

// Legacy signature using cpp_custom_type_hack
static quarisma::Tensor record_function_enter_legacy(
    const std::string& name, const std::optional<std::string>& args)
{
    auto rec = std::make_unique<quarisma::record_function>(quarisma::RecordScope::USER_SCOPE);
    record_function_enter(name, args, *rec);
    return quarisma::cpp_custom_type_hack::create(std::move(rec), quarisma::TensorOptions());
}

// New signature using custom_class
quarisma::intrusive_ptr<PythonRecordFunction> record_function_enter_new(
    const std::string& name, const std::optional<std::string>& args)
{
    auto rec = quarisma::make_intrusive<PythonRecordFunction>(quarisma::RecordScope::USER_SCOPE);
    record_function_enter(name, args, rec->record);
    return rec;
}

static quarisma::record_function& getRecordFunctionFromTensor(const quarisma::Tensor& handle)
{
    auto& rec = quarisma::cpp_custom_type_hack::cast<quarisma::record_function>(handle);
    return rec;
}

// Ends the profiling scope created with record_function_enter.
static void record_function_exit(quarisma::record_function& rec)
{
    rec.end();
}

// Legacy signature using cpp_custom_type_hack
static void record_function_exit_legacy(const quarisma::Tensor& handle)
{
    // We don't actually need to do anything with handle just need to persist the
    // lifetime until now.
    auto& rec = getRecordFunctionFromTensor(handle);
    record_function_exit(rec);
}

// New signature using custom_class
static void record_function_exit_new(const quarisma::intrusive_ptr<PythonRecordFunction>& record)
{
    record_function_exit(record->record);
}

template <typename Func>
static quarisma::intrusive_ptr<quarisma::ivalue::Future> _call_end_callbacks_on_fut(
    Func get_record, const quarisma::intrusive_ptr<quarisma::ivalue::Future>& fut)
{
    // Profiling callback that ends the associated record_function
    // and returns the value of the passed in future.
    auto futureProfilingFunc = [get_record = std::move(get_record)](quarisma::ivalue::Future& fut)
    {
        auto& rec = get_record();
        rec.end();
        // Note: this future is returned to the user to ensure that a call to
        // wait() ensures that profiling callbacks have ran. To ensure that this
        // is transparent, we must make this future propagate the value of the
        // RPC future. Use value() here instead of constValue() to ensure we
        // propagate errors.
        return fut.value();
    };
    // Define a future that completes after the profiling callbacks are run.
    auto profiledFut = fut->then(
        quarisma::wrapPropagateTLSState(std::move(futureProfilingFunc)), fut->elementType());
    return profiledFut;
}

// Legacy signature using cpp_custom_type_hack
static quarisma::intrusive_ptr<quarisma::ivalue::Future> _call_end_callbacks_on_fut_legacy(
    const quarisma::Tensor& handle, const quarisma::intrusive_ptr<quarisma::ivalue::Future>& fut)
{
    return _call_end_callbacks_on_fut(
        [handle]() -> quarisma::record_function&
        {
            TORCH_INTERNAL_ASSERT(
                handle.defined(),
                "Undefined record_function handle. This can happen if the handle is "
                "not correctly persisted and is destroyed before the future is "
                "realized.");

            return getRecordFunctionFromTensor(handle);
        },
        fut);
}

// New signature using custom_class
quarisma::intrusive_ptr<quarisma::ivalue::Future> _call_end_callbacks_on_fut_new(
    const quarisma::intrusive_ptr<PythonRecordFunction>&     record,
    const quarisma::intrusive_ptr<quarisma::ivalue::Future>& fut)
{
    return _call_end_callbacks_on_fut(
        [record]() -> quarisma::record_function& { return record->record; }, fut);
}

// Internal only, do not use directly, use Python's record_function()
TORCH_LIBRARY_FRAGMENT(profiler, m)
{
    m.class_<PythonRecordFunction>("_RecordFunction");

    m.def(
        "_record_function_enter(str name, str? args=None) -> Tensor",
        &record_function_enter_legacy);
    m.def(
        "_record_function_enter_new(str name, str? args=None) -> "
        "__torch__.torch.classes.profiler._RecordFunction",
        &record_function_enter_new);
    m.def("_record_function_exit", &record_function_exit_legacy);
    m.def("_record_function_exit._RecordFunction", &record_function_exit_new);

    torch::jit::registerOperator(
        torch::jit::Operator(
            "profiler::_call_end_callbacks_on_jit_fut(Tensor x, Future(t) y) -> Future(t)",
            [](jit::Stack& stack)
            {
                // Pop inputs, which should be a future and a tensor
                auto fut         = jit::pop(stack).toFuture();
                auto tensor      = jit::pop(stack).toTensor();
                auto profiledFut = _call_end_callbacks_on_fut_legacy(tensor, fut);
                // return future that completes when profiling callbacks have run.
                jit::push(stack, std::move(profiledFut));
            },
            quarisma::AliasAnalysisKind::FROM_SCHEMA));
    torch::jit::registerOperator(
        torch::jit::Operator(
            "profiler::_call_end_callbacks_on_jit_fut._RecordFunction("
            "__torch__.torch.classes.profiler._RecordFunction x, Future(t) y) -> Future(t)",
            [](quarisma::Stack& stack)
            {
                // Pop inputs, which should be a future and a PythonRecordFunction
                auto fut         = torch::jit::pop(stack).toFuture();
                auto tensor      = torch::jit::pop(stack).toCustomClass<PythonRecordFunction>();
                auto profiledFut = _call_end_callbacks_on_fut_new(tensor, fut);
                // return future that completes when profiling callbacks have run.
                torch::jit::push(stack, std::move(profiledFut));
            },
            quarisma::AliasAnalysisKind::FROM_SCHEMA));
}

}  // namespace torch::autograd::profiler
