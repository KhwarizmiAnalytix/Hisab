#pragma once

#include <Quarisma/core/Dimname.h>
#include <Quarisma/core/class_type.h>
#include <Quarisma/core/jit_type.h>
#include <Quarisma/core/stack.h>
#include <Quarisma/core/symbol.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/jit/frontend/source_range.h>
#include <torch/csrc/utils/variadic.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "util/exception.h"

namespace torch::jit
{
struct Node;
struct Value;
struct Graph;
struct Module;

namespace tracer
{

using ::quarisma::ivalue::Shared;

using ::quarisma::IValue;
using ::quarisma::ivalue::Future;

using ::quarisma::ArrayRef;
using ::quarisma::TupleType;
using ::quarisma::TupleTypePtr;
using ::quarisma::ivalue::ConstantString;

using torch::autograd::Variable;
using variable_list = std::vector<Variable>;

TORCH_API std::atomic<bool>& getTracerStateWarnMode();

struct TORCH_API TracingState : public std::enable_shared_from_this<TracingState>
{
    TracingState();
    ~TracingState();

    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::shared_ptr<Graph> graph;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    bool warn = getTracerStateWarnMode();
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    bool strict = true;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    bool force_outplace = false;
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::function<std::string(const Variable& var)> lookup_var_name_fn = [](const Variable& var)
    { return ""; };

    void enterFrame() { env_stack.emplace_back(); }

    void leaveFrame() { env_stack.pop_back(); }

    void   setValue(const IValue& v, Value* value);
    void   delValue(const IValue& var);
    Value* getValue(const IValue& var);
    Value* getOutput(const IValue& var, size_t i);
    bool   hasValue(const IValue& var) const;

    Node* createNode(quarisma::Symbol op_name, size_t num_outputs);
    void  insertNode(Node* node);

private:
    using WeakIValue = quarisma::WeakIValue;

    struct WeakIValueHasher
    {
        size_t operator()(const WeakIValue& t) const { return t.hash(); }
    };

    struct WeakIValueEq
    {
        bool operator()(const WeakIValue& t1, const WeakIValue& t2) const
        {
            return t1.isSameIdentity(t2);
        }
    };

    using Frame = std::unordered_map<WeakIValue, Value*, WeakIValueHasher, WeakIValueEq>;
    std::vector<Frame> env_stack;
};

// This is meant to be used as a thread local place, where we can store extra
// info that gets lost when we call into Quarisma from Python bindings. One example
// for when this happens is when we get an IntArrayRef argument with e.g. sizes
// for view. When tracing, those might be tensors, which let us encode extra
// data dependencies, but once they get to the Quarisma call where we actually have
// the tracing logic, they get converted into a raw IntArrayRef, and we loose
// all information. To prevent this, we temporarily stash it in here.
struct ArgumentStash
{
    struct IntArrayRefTrace : std::vector<Value*>
    {
        IntArrayRefTrace(size_t size) : std::vector<Value*>(size, nullptr) {}
    };

    static bool empty() { return stash.intlists.empty(); }

    TORCH_API static void stashIntArrayRefElem(
        const std::string& arg_name, size_t size, size_t idx, const Variable& var);

    static bool hasIntArrayRef(const std::string& arg_name)
    {
        return stash.intlists.count(arg_name) > 0;
    }

    static IntArrayRefTrace popIntArrayRef(const std::string& arg_name)
    {
        auto info = std::move(stash.intlists.quarisma(arg_name));
        stash.intlists.erase(arg_name);
        return info;
    }

    // Value stashing: Use these methods to stash arguments which correspond
    // to regular Value*'s in the graph. i.e. they don't require special
    // handling like in the case of IntArrayRefs
    TORCH_API static void stashValue(
        const std::string&       arg_name,
        size_t                   idx,
        const Variable&          var,
        const quarisma::TypePtr& type = nullptr);

    static bool hasValue(const std::string& arg_name) { return stash.values.count(arg_name) > 0; }

    static Value* popValue(const std::string& arg_name)
    {
        auto info = stash.values.quarisma(arg_name);
        stash.values.erase(arg_name);
        return info;
    }

private:
    static thread_local ArgumentStash                 stash;
    std::unordered_map<std::string, IntArrayRefTrace> intlists;
    std::unordered_map<std::string, Value*>           values;
};

// Retrieve or set the current tracing state. Returns a nullptr if tracing is
// disabled.
TORCH_API const std::shared_ptr<TracingState>& getTracingState();
TORCH_API void                                 setTracingState(std::shared_ptr<TracingState> state);

inline bool isTracing()
{
    return static_cast<bool>(getTracingState());
}

using warn_fn_type = void (*)(const std::string& msg);
TORCH_API extern const char* WARN_PYTHON_DATAFLOW;
TORCH_API extern const char* WARN_CONSTRUCTOR;
TORCH_API extern const char* WARN_RESIZE;
TORCH_API extern const char* STRICT_TRACER_MSG;
TORCH_API void               _do_warn(const char* _reason, const char* _kind);
inline void                  warn(const char* _reason, const char* _kind = nullptr)
{
    if (const auto& state = getTracingState())
    {
        if (!state->warn)
            return;
        _do_warn(_reason, _kind);
    }
}
TORCH_API void setWarn(warn_fn_type fn);

struct TORCH_API NoWarn
{
    NoWarn() : state(getTracingState())
    {
        if (state)
        {
            prev        = state->warn;
            state->warn = false;
        }
    }
    ~NoWarn()
    {
        if (state)
        {
            state->warn = prev;
        }
    }
    std::shared_ptr<TracingState> state;
    bool                          prev{false};
};

struct WithNestedTracingFrame
{
    WithNestedTracingFrame() { getTracingState()->enterFrame(); }

    ~WithNestedTracingFrame() { getTracingState()->leaveFrame(); }
};
TORCH_API void recordSourceLocation(Node* n);
TORCH_API void setRecordSourceLocation(void (*v)(Node*));

TORCH_API std::vector<StackEntry> pythonCallstack();
TORCH_API void                    setPythonCallstack(std::vector<StackEntry> (*v)());

// Having finished adding a new 'node' to the graph IR 'setValueTrace'
// associates this node with an output variable, so that further operations
// involving this variable know which node in the IR to reference.
TORCH_API void setValueTrace(const IValue& v, Value* value);

TORCH_API void delValueTrace(const IValue& var);

TORCH_API std::function<void()> pauseTracing();

TORCH_API Value* getValueTrace(const IValue& var);

TORCH_API std::pair<std::shared_ptr<TracingState>, Stack> trace(
    Stack                                       inputs,
    const std::function<Stack(Stack)>&          traced_fn,
    std::function<std::string(const Variable&)> var_name_lookup_fn,
    bool                                        strict         = true,
    bool                                        force_outplace = false,
    Module*                                     self           = nullptr,
    const std::vector<std::string>&             argument_names = {});

TORCH_API void abandon();

// NB: those serve both as an intermediate steps in addInputs below,
// as well as the overloads that terminate template recursion
TORCH_API void addInputs(Node* n, const char* name, int64_t value);
TORCH_API void addInputs(Node* n, const char* name, const quarisma::SymInt& value);
TORCH_API void addInputs(Node* n, const char* name, std::optional<int64_t> value);
TORCH_API void addInputs(Node* n, const char* name, bool value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<bool>& value);
TORCH_API void addInputs(Node* n, const char* name, double value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<double>& value);
TORCH_API void addInputs(Node* n, const char* name, const quarisma::Scalar& value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<quarisma::Scalar>& value);
TORCH_API void addInputs(Node* n, const char* name, const quarisma::Tensor& value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<quarisma::Tensor>& value);
TORCH_API void addInputs(Node* n, const char* name, ArrayRef<int64_t> value);
TORCH_API void addInputs(Node* n, const char* name, quarisma::SymIntArrayRef value);
TORCH_API void addInputs(Node* n, const char* name, std::optional<quarisma::SymInt> value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<ArrayRef<int64_t>>& value);
TORCH_API void addInputs(Node* n, const char* name, const quarisma::OptionalIntArrayRef& opt_value);
TORCH_API void addInputs(
    Node* n, const char* name, const quarisma::OptionalSymIntArrayRef& opt_value);
TORCH_API void addInputs(
    Node* n, const char* name, ArrayRef<quarisma::Tensor> value, bool allow_undefined = false);
TORCH_API void addInputs(
    Node*                                n,
    const char*                          name,
    const std::vector<quarisma::Tensor>& value,
    bool                                 allow_undefined = false);
TORCH_API void addInputs(
    Node* n, const char* name, quarisma::ITensorListRef value, bool allow_undefined = false);
TORCH_API void addInputs(
    Node* n, const char* name, const List<std::optional<quarisma::Tensor>>& value);
TORCH_API void addInputs(
    Node*                                                       n,
    const char*                                                 name,
    ArrayRef<quarisma::intrusive_ptr<quarisma::ivalue::Object>> value,
    const quarisma::ClassTypePtr&                               class_type);
TORCH_API void addInputs(Node* n, const char* name, ArrayRef<double> value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<ArrayRef<double>>& value);
TORCH_API void addInputs(Node* n, const char* name, const std::string_view value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<std::string_view>& value);
TORCH_API void addInputs(Node* n, const char* name, quarisma::Device value);
TORCH_API void addInputs(Node* n, const char* name, quarisma::Stream stream);
TORCH_API void addInputs(Node* n, const char* name, quarisma::Layout value);
TORCH_API void addInputs(Node* n, const char* name, quarisma::ScalarType value);
TORCH_API void addInputs(
    Node* n, const char* name, const std::optional<quarisma::ScalarType>& value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<quarisma::Device>& value);
TORCH_API void addInputs(Node* n, const char* name, const std::optional<quarisma::Layout>& value);
TORCH_API void addInputs(Node* n, const char* name, quarisma::MemoryFormat value);
TORCH_API void addInputs(Node* n, const char* name, std::optional<quarisma::DimnameList> value);
TORCH_API void addInputs(
    Node* n, const char* name, const std::optional<quarisma::MemoryFormat>& value);
TORCH_API void addInputs(
    Node* n, const char* name, const std::optional<quarisma::Generator>& value);

inline void addInputs(Node* n, const char* name, const std::vector<bool>& value)
{
    QUARISMA_CHECK(false, "Tracing a list of bool type is currently not supported!");
}

template <typename T>
void addInputs(Node* n, const char* name, ArrayRef<T> value)
{
    QUARISMA_CHECK(false, "Tracing a list of arbitrary type is currently not supported!");
}
template <typename K, typename V>
void addInputs(Node* n, const char* name, const std::unordered_map<K, V>& value)
{
    QUARISMA_CHECK(false, "Tracing a dict of arbitrary types is currently not supported!");
}

template <size_t N>
void addInputs(Node* n, const char* name, std::array<bool, N> value)
{
    throw std::runtime_error(
        "Found an unsupported argument type in the JIT tracer. File a bug report.");
}

TORCH_API void addInputs(
    Node* n, const char* name, const quarisma::intrusive_ptr<quarisma::ivalue::Object>& obj);

TORCH_API void ensureUniqueIfOutOfPlaced(const char* name, const quarisma::Tensor& tensor);
TORCH_API void ensureUniqueIfOutOfPlaced(
    const char* name, const std::optional<quarisma::Tensor>& tensor);

template <
    typename T,
    typename = std::enable_if_t<
        (!std::is_convertible_v<std::decay_t<T>, quarisma::TensorList> &&
         !std::is_convertible_v<std::decay_t<T>, quarisma::List<quarisma::Tensor>> &&
         !std::is_convertible_v<std::decay_t<T>, quarisma::Tensor> &&
         !std::
             is_convertible_v<std::decay_t<T>, quarisma::intrusive_ptr<quarisma::ivalue::Object>>)>>
void addOutput(Node* node, T&& /*unused*/)
{
    QUARISMA_CHECK(
        false,
        "Found an unsupported argument type ",
        quarisma::demangle_type<T>(),
        " in the JIT tracer. File a bug report.");
}
TORCH_API void addOutput(Node* node, const quarisma::Tensor& tensor);
TORCH_API void setOutput(Value* value, const quarisma::Tensor& output);
TORCH_API void addOutput(Node* node, const std::vector<quarisma::Tensor>& list);
TORCH_API void addOutput(Node* node, const quarisma::List<quarisma::Tensor>& list);
TORCH_API void addOutput(
    Node* node, const quarisma::intrusive_ptr<quarisma::ivalue::Object>& output);

TORCH_API autograd::Variable getSizeOf(const autograd::Variable& var, int64_t dim);

TORCH_API autograd::Variable getNumelOf(const autograd::Variable& var);

}  // namespace tracer
}  // namespace torch::jit
