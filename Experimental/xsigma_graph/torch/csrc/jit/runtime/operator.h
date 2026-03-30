// in memory description of all Quarisma Ops similar to Caffe2 schema
// once C10 exists this can be removed, or stubbed out, but we need
// it now to implement correct semantic checking for script
#pragma once

#include <Quarisma/core/dispatch/Dispatcher.h>
#include <Quarisma/core/dispatch/OperatorOptions.h>
#include <Quarisma/core/function_schema.h>
#include <Quarisma/core/op_registration/op_allowlist.h>
#include <Quarisma/core/stack.h>
#include <Quarisma/core/symbol.h>
#include <quarisma/util/overloaded.h>
#include <torch/csrc/jit/frontend/function_schema_parser.h>
#include <torch/csrc/jit/runtime/operator_options.h>
#include <torch/library.h>

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "util/exception.h"

namespace torch::jit
{

struct Node;
using ::quarisma::Argument;
using ::quarisma::FunctionSchema;
using ::quarisma::Symbol;

using OperationCreator = Operation (*)(const Node*);

namespace
{
const std::array<quarisma::Tag, 1> kJitOnlyOperatorTags = {quarisma::Tag::pt2_compliant_tag};
}

/*
 * Note: JIT relies on Operator instances having static lifetime, because
 * it for example stores a non-owning FunctionSchema* pointer in the Node class,
 * which points to the function schema stored in the Operator instance.
 * Also, jit::Operator is meant to store more operator related information like
 * symbolic derivatives, which also requires them to have static lifetime
 * so that changes to symbolic derivatives are remembered.
 *
 * Currently, the JIT operator library contains a jit::Operator instance
 * with a wrapper for each quarisma operator. The quarisma operator library registers
 * those wrappers using listeners in register_c10_ops.cpp.
 * TODO Instead of doing it this way, we should only have pure-jit ops in
 * the jit library but have the JIT operator lookup look into the quarisma library
 * too.
 */

// An Operator is a thin wrapper around either a pure JIT operator (e.g. prim
// ops) or a quarisma operator, allowing some common operations and abstracting away
// the concrete operator nature.
struct TORCH_API Operator
{
private:
    struct C10Operator final
    {
        quarisma::OperatorHandle handle_;
        Operation                op_;
    };
    struct UnparsedFunctionSchema final
    {
        std::string                                        schema_string_;
        mutable std::optional<quarisma::AliasAnalysisKind> alias_analysis_;
    };
    struct JitOnlyOperator final
    {
        // The only valid transition for schema_ is from right->left, i.e.
        // when the schema gets parsed.
        mutable std::variant<FunctionSchema, UnparsedFunctionSchema> schema_;

        std::variant<Operation, OperationCreator> op_;
    };

public:
    Operator(quarisma::OperatorHandle opHandle, Operation operation)
        : op_(C10Operator{std::move(opHandle), std::move(operation)})
    {
    }

    Operator(std::string schema, Operation op, quarisma::AliasAnalysisKind alias_analysis)
        : op_(JitOnlyOperator{
              UnparsedFunctionSchema{std::move(schema), alias_analysis}, Operation(std::move(op))})
    {
    }

    Operator(
        std::string                 name,
        std::string                 overload_name,
        std::vector<Argument>       arguments,
        std::vector<Argument>       returns,
        Operation                   op,
        quarisma::AliasAnalysisKind alias_analysis)
        : op_(JitOnlyOperator{
              FunctionSchema(varArgSchemaWithName(
                  std::move(name),
                  std::move(overload_name),
                  std::move(arguments),
                  std::move(returns),
                  alias_analysis)),
              std::move(op)})
    {
    }

    Operator(
        std::string schema, OperationCreator op_creator, quarisma::AliasAnalysisKind alias_analysis)
        : op_(JitOnlyOperator{
              UnparsedFunctionSchema{std::move(schema), alias_analysis}, op_creator})
    {
    }

    // Helper constructor to register `op` to run
    // run for _every_ IR Node where n.kind() == name, regardless of arguments.
    // This is accomplished by marking the schema varargs and having no required
    // arguments.
    Operator(Symbol name, OperationCreator op_creator, quarisma::AliasAnalysisKind alias_analysis)
        : op_(JitOnlyOperator{
              FunctionSchema(varArgSchemaWithName(name, alias_analysis)), op_creator})
    {
    }

    Operation getOperation(const Node* node = nullptr) const
    {
        return std::visit(
            quarisma::overloaded(
                [](const C10Operator& op) { return op.op_; },
                [node](const JitOnlyOperator& op)
                {
                    return std::visit(
                        quarisma::overloaded(
                            [](const Operation& op) { return op; },
                            [node](const OperationCreator& op_creator)
                            { return op_creator(node); }),
                        op.op_);
                }),
            op_);
    }

    Operation getOperationForDispatchKey(quarisma::DispatchKey dk) const
    {
        // TODO: some sort of caching mechanism?
        return std::visit(
            quarisma::overloaded(
                [dk](const C10Operator& op)
                {
                    return Operation([op, dk](Stack& stack)
                                     { op.handle_.callBoxedForDispatchKey(dk, stack); });
                },
                [](const JitOnlyOperator& op)
                {
                    QUARISMA_CHECK(
                        false, "calling a JIT operator for dispatch key is not supported");
                    return Operation(nullptr);
                }),
            op_);
    }

    const FunctionSchema& schema() const
    {
        return std::visit(
            quarisma::overloaded(
                [](const C10Operator& op) -> const FunctionSchema& { return op.handle_.schema(); },
                [](const JitOnlyOperator& op) -> const FunctionSchema&
                {
                    // we lazily parse schema initialized from strings so that
                    // we do less work during static operator registration
                    if (op.schema_.index() == 1)
                    {
                        auto& unmaterializedSchema = std::get<UnparsedFunctionSchema>(op.schema_);
                        FunctionSchema schema = parseSchema(unmaterializedSchema.schema_string_);
                        if (unmaterializedSchema.alias_analysis_.has_value())
                        {
                            // TODO What if it gets set later?
                            schema.setAliasAnalysis(*unmaterializedSchema.alias_analysis_);
                        }
                        op.schema_ = std::move(schema);
                    }
                    return std::get<FunctionSchema>(op.schema_);
                }),
            op_);
    }

    quarisma::ArrayRef<quarisma::Tag> getTags() const
    {
        return std::visit(
            quarisma::overloaded(
                [](const C10Operator& op) { return op.handle_.getTags(); },
                [](const JitOnlyOperator& op)
                {
                    // JitOnlyOperators don't have an quarisma::OperatorHandle or a way to
                    // specify tags. We're grandfathering them all into
                    // pt2_compliant_tag, but for anything else, please just stop
                    // using JitOnlyOperator.
                    return quarisma::ArrayRef<quarisma::Tag>(kJitOnlyOperatorTags);
                }),
            op_);
    }

    bool isC10Op() const { return op_.index() == 0; }

    quarisma::AliasAnalysisKind aliasAnalysisKind() const
    {
        const FunctionSchema&       schemaRef      = schema();
        quarisma::AliasAnalysisKind alias_analysis = schemaRef.aliasAnalysis();

        QUARISMA_CHECK(
            alias_analysis == AliasAnalysisKind::FROM_SCHEMA || !schemaRef.hasAnyAliasInfo(),
            "In operator registration: Tried to register operator ",
            schemaRef,
            " with aliasing information in the schema but without AliasAnalysisKind::FROM_SCHEMA.");
        return alias_analysis;
    }

    bool hasOperation() const
    {
        return std::visit(
            quarisma::overloaded(
                [](const C10Operator&) { return true; },
                [](const JitOnlyOperator& op) { return op.op_.index() == 0; }),
            op_);
    }

private:
    static FunctionSchema varArgSchemaWithName(Symbol name, AliasAnalysisKind alias_analysis)
    {
        auto result = FunctionSchema(
            name,
            "",
            {},
            {},
            /*is_vararg*/ true,
            /*is_varret*/ true);
        result.setAliasAnalysis(alias_analysis);
        return result;
    }

    static FunctionSchema varArgSchemaWithName(
        std::string           name,
        std::string           overload_name,
        std::vector<Argument> arguments,
        std::vector<Argument> returns,
        AliasAnalysisKind     alias_analysis)
    {
        auto result = FunctionSchema(
            std::move(name),
            std::move(overload_name),
            std::move(arguments),
            std::move(returns),
            /*is_vararg*/ false,
            /*is_varret*/ false);
        result.setAliasAnalysis(alias_analysis);
        return result;
    }

    std::variant<C10Operator, JitOnlyOperator> op_;
};

TORCH_API std::string canonicalSchemaString(const FunctionSchema& schema);

TORCH_API const std::vector<std::shared_ptr<Operator>> getAllOperators();
TORCH_API const std::vector<std::shared_ptr<Operator>>& getAllOperatorsFor(Symbol name);
// Returns operators in the order which OpOverloadPacket resolves them.
TORCH_API std::vector<std::shared_ptr<Operator>> getAllSortedOperatorsFor(Symbol name);

// given a operator with an overload name, find the specific operator related to
// it, may return nullptr if no operator exists.
TORCH_API std::shared_ptr<Operator> findOperatorFor(const quarisma::OperatorName& full_name);

TORCH_API std::vector<Symbol> findSimilarOperators(Symbol input_op);

TORCH_API void registerOperator(Operator&& op);
TORCH_API void deregisterOperator(const FunctionSchema& schema);

// XXX: this function is meant to be used with string literals only!
TORCH_API std::shared_ptr<Operator> getOperatorForLiteral(const char* signature);

// Ensure the thing that registers quarisma ops is defined.
// Otherwise, our registry will not have quarisma ops. You can run into this
// scenario if you're querying registered ops during static init.
//
// This fn is defined in register_c10_ops.cpp
TORCH_API void ensure_c10_registerer_defined();

// Used to assert that unschematized operators have an analysis method written
TORCH_API bool aliasAnalysisHasSpecialCaseFor(quarisma::Symbol sym);

// A factory function to generate an optional operator. It has two
// instantiations depending on the template bool arg value. The arg can be a
// compile-time function for the selective op registration based on schema
// string.
template <typename Func>
std::optional<Operator> OperatorGenerator(
    const char* schema_str, Func&& op, AliasAnalysisKind alias_analysis)
{
    return std::optional<Operator>(
        Operator(std::string(schema_str), std::forward<Func>(op), alias_analysis));
}

template <typename Func>
std::optional<Operator> OperatorGenerator(
    torch::detail::SelectiveStr<true> schema_str, Func&& op, AliasAnalysisKind alias_analysis)
{
    return OperatorGenerator(
        static_cast<const char*>(schema_str), std::forward<Func>(op), alias_analysis);
}

template <typename Func>
std::optional<Operator> OperatorGenerator(
    torch::detail::SelectiveStr<false> schema_str, Func&& op, AliasAnalysisKind alias_analysis)
{
    return std::nullopt;
}

template <typename Func>
std::optional<Operator> OperatorGenerator(
    const std::string                     name,
    const std::string                     overload_name,
    const std::vector<quarisma::Argument> arguments,
    const std::vector<quarisma::Argument> returns,
    Func&&                                op,
    AliasAnalysisKind                     alias_analysis)
{
    return std::optional<Operator>(
        Operator(name, overload_name, arguments, returns, std::forward<Func>(op), alias_analysis));
}

}  // namespace torch::jit
