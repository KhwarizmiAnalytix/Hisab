#include <quarisma/util/irange.h>
#include <torch/csrc/jit/jit_log.h>
#include <torch/csrc/jit/passes/dead_code_elimination.h>
#include <torch/csrc/jit/passes/onnx/helper.h>
#include <torch/csrc/jit/passes/onnx/scalar_type_analysis.h>

namespace torch::jit
{

namespace onnx
{
using namespace ::quarisma::onnx;
}

namespace
{
const int ONNX_OPSET_14 = 14;

static const std::unordered_map<quarisma::ScalarType, int, ScalarTypeHashFunction>
    scalarTypeToONNXTypeMap = {
        {quarisma::kFloat, 1},
        {quarisma::kByte, 2},
        {quarisma::kChar, 3},
        {quarisma::kShort, 5},
        {quarisma::kInt, 6},
        {quarisma::kLong, 7},
        {quarisma::kBool, 9},
        {quarisma::kHalf, 10},
        {quarisma::kDouble, 11},
        {quarisma::kQInt8, 12},
        {quarisma::kQUInt8, 13},
        {quarisma::kQInt32, 14},
        {quarisma::kBFloat16, 15},
        {quarisma::kFloat8_e4m3fn, 16},
        {quarisma::kFloat8_e5m2, 17},
        {quarisma::kFloat8_e4m3fnuz, 18},
        {quarisma::kFloat8_e5m2fnuz, 19},
};

static int64_t ScalarTypeToONNXType(const quarisma::ScalarType& st)
{
    int64_t    onnx_type = -1;
    const auto it        = scalarTypeToONNXTypeMap.find(st);
    if (it != scalarTypeToONNXTypeMap.end())
    {
        onnx_type = it->second;
    }
    return onnx_type;
}

// For these operators, all inputs and outputs share the same scalar type.
// There is no operator-wise special case handling needed.
static const std::unordered_set<NodeKind> standardOps = {
    onnx::Add,
    onnx::Concat,
    onnx::Div,
    onnx::Gemm,
    onnx::Min,
    onnx::Max,
    onnx::Mod,
    onnx::Mul,
    onnx::Pow,
    onnx::Sub,
    onnx::MatMul,
    onnx::Conv,
};

// For these operators, all inputs share the same scalar type.
// The output scalar type is always Bool.
static const std::unordered_set<NodeKind> comparisonOps = {
    onnx::Greater,
    onnx::Less,
    onnx::Equal,
    onnx::GreaterOrEqual,
    onnx::LessOrEqual,
};

static const std::unordered_set<NodeKind> selectorOps = {onnx::Where};

static bool IsStandardOp(const NodeKind& nkind)
{
    return standardOps.find(nkind) != standardOps.end();
}

static bool IsComparisonOp(const NodeKind& nkind)
{
    return comparisonOps.find(nkind) != comparisonOps.end();
}

static bool IsSelectorOp(const NodeKind& nkind)
{
    return selectorOps.find(nkind) != selectorOps.end();
}

static TensorTypePtr CreateProfiledTensorTypeWithScalarType(
    const TensorTypePtr& typePtr, const quarisma::ScalarType& scalar_type)
{
    TORCH_INTERNAL_ASSERT(typePtr != nullptr);
    return typePtr->withScalarType({scalar_type});
}

static bool IsImplicitCastSupported(const NodeKind& nodeKind)
{
    return IsStandardOp(nodeKind) || IsComparisonOp(nodeKind) || IsSelectorOp(nodeKind);
}

static std::optional<quarisma::ScalarType> PromoteScalarTypes(
    const std::vector<quarisma::ScalarType>& types)
{
    if (types.empty())
    {
        return std::nullopt;
    }
    auto st = types[0];
    for (const auto i : quarisma::irange(1, types.size()))
    {
        st = quarisma::promoteTypes(st, types[i]);
    }
    return st;
}

// Type promotion between scalars and tensors
// per logic here
// https://pytorch.org/docs/main/tensor_attributes.html#tensor-attributes
static std::optional<quarisma::ScalarType> PromoteScalarTypesWithCategory(
    const std::vector<quarisma::ScalarType>& typesFromTensors,
    const std::vector<quarisma::ScalarType>& typesFromScalars)
{
    auto typeFromTensor = PromoteScalarTypes(typesFromTensors);
    auto typeFromScalar = PromoteScalarTypes(typesFromScalars);

    auto getTypeCategory = [](quarisma::ScalarType t)
    {
        if (quarisma::kBool == t)
        {
            return 1;
        }
        if (quarisma::isIntegralType(t, /*includeBool=*/false))
        {
            return 2;
        }
        if (quarisma::isFloatingType(t))
        {
            return 3;
        }
        return 0;
    };

    if (std::nullopt == typeFromScalar)
    {
        return typeFromTensor;
    }
    else if (std::nullopt == typeFromTensor)
    {
        return typeFromScalar;
    }

    auto typeCategoryFromTensor = getTypeCategory(typeFromTensor.value());
    auto typeCategoryFromScalar = getTypeCategory(typeFromScalar.value());

    if (typeCategoryFromScalar > typeCategoryFromTensor)
    {
        return typeFromScalar;
    }
    return typeFromTensor;
}

static std::optional<quarisma::ScalarType> InferExpectedScalarType(const Node* n)
{
    std::vector<quarisma::ScalarType> typesFromTensors;
    std::vector<quarisma::ScalarType> typesFromScalars;

    auto get_scalar_type = [](const Value* input) -> std::optional<quarisma::ScalarType>
    {
        if (auto* tensor_type = input->type()->castRaw<TensorType>())
        {
            return tensor_type->scalarType();
        }
        return std::nullopt;
    };
    auto emplace_type_from_scalar =
        [&typesFromTensors, &typesFromScalars](quarisma::ScalarType scalar_type)
    {
        // Mimic PyTorch scalar type promotion logic
        // from https://github.com/pytorch/pytorch/issues/9515
        // Quoting:
        //    A Tensor is a considered a "wrapped number" if it is
        //    auto-wrapped from a C++ or Python number type. Integer types are
        //    wrapped as 0-dim int64 tensors and floating-point types are
        //    wrapped as 0-dim double tensors.
        auto default_scalar_type = quarisma::typeMetaToScalarType(quarisma::get_default_dtype());
        switch (scalar_type)
        {
        case quarisma::kDouble:
        case quarisma::kFloat:
            // floating-point numbers wrapped as float32/float64 tensors are
            // considered to have default type, instead of double.
            typesFromScalars.emplace_back(default_scalar_type);
            break;
        case quarisma::kLong:
        case quarisma::kBool:
            // bool and integer numbers remain the same type.
            typesFromScalars.emplace_back(scalar_type);
            break;
        default:
            // other types are not from wrapped numbers,
            // track them as types from tensors.
            typesFromTensors.emplace_back(scalar_type);
            break;
        }
    };

    size_t input_idx = 0;
    std::for_each(
        n->inputs().begin(),
        n->inputs().end(),
        [&](const Value* input)
        {
            // We skip the 'condition' input (i.e., the first input) in case of
            // onnx::Where operator.
            if (IsSelectorOp(n->kind()) && input_idx == 0)
            {
                input_idx++;
                return;
            }

            auto nkind = input->node()->kind();
            if (nkind == onnx::Gather && input->node()->input(0)->node()->kind() == onnx::Shape)
            {
                // This is a special pattern generated by code like `dim_size =
                // x.size(0)`. It gets converted to the below ONNX IR graph
                //    %1 : Long() = onnx::Constant[value={0}]()
                //    %2 : Tensor = onnx::Shape(%x)
                //    %dim_size : Long() = onnx::Gather(%2, %1)
                // `dim_size` is treated in PyTorch as Scalar.
                // However, in the ONNX IR graph, it is an output of onnx::Gather,
                // which is by default considered as a tensor.
                typesFromScalars.emplace_back(quarisma::kLong);
            }
            else if (nkind == onnx::Constant)
            {
                auto tensor      = input->node()->t(attr::value);
                auto rank        = tensor.dim();
                auto scalar_type = tensor.scalar_type();

                if (rank == 0)
                {
                    emplace_type_from_scalar(scalar_type);
                }
                else
                {
                    typesFromTensors.emplace_back(scalar_type);
                }
            }
            else if (auto scalar_type = get_scalar_type(input))
            {
                auto tensor_type = input->type()->castRaw<TensorType>();
                // get_scalar_type returns non-null value already guarantees
                // that the input has a valid tensor_type.
                TORCH_INTERNAL_ASSERT(nullptr != tensor_type);
                // ONNX model track shape related computes that were done in pytorch
                // by python numbers as tensor computes. This is the only way for ONNX
                // to track them properly since ONNX only has tensor type, otherwise
                // the computation result will be tracked statically as constant, and
                // the model won't work for another input that differs in shape.

                // Now for type promotion logic, scalars should be treated differently
                // with tensors. More info regarding type promotion logic commented quarisma
                // `emplace_type_from_scalar`. Here we filter out rank 0 tensors and
                // run it with `emplace_type_from_scalar` to determine if they are
                // considered scalars for type promotion.

                // NOTE that this might introduce regression that a REAL 0-rank tensor
                // is now being recognized as scalar. The downside is the model will
                // drop in accuracy for these cases as certain computations will
                // happen in lower precision data types.
                auto rank = tensor_type->dim();
                if (rank && rank.value() == 0)
                {
                    emplace_type_from_scalar(scalar_type.value());
                }
                else
                {
                    typesFromTensors.emplace_back(scalar_type.value());
                }

                input_idx++;
            }
        });

    std::optional<quarisma::ScalarType> st        = std::nullopt;
    const auto                          output_st = get_scalar_type(n->output());

    if (IsComparisonOp(n->kind()))
    {
        // For comparison ops, always promote scalar type to highest among inputs,
        // regardless if that input is a tensor or scalar.
        typesFromScalars.insert(
            typesFromScalars.end(), typesFromTensors.begin(), typesFromTensors.end());
        st = PromoteScalarTypes(typesFromScalars);
    }
    else
    {
        if (output_st)
        {
            // If output scalar type is available, use that.
            st = output_st;
        }
        else
        {
            // PyTorch now does implicit type promotion regardless whether the inputs
            // are tensors or scalars. (Previously only scalars support implicit
            // casting).
            // Per logic here
            // https://pytorch.org/docs/main/tensor_attributes.html#tensor-attributes
            st = PromoteScalarTypesWithCategory(typesFromTensors, typesFromScalars);
        }
    }

    return st;
}

static std::optional<quarisma::ScalarType> LowPrecisionCastForStandardOps(
    const Node* n, const quarisma::ScalarType& scalar_type)
{
    // Some of standardOps do not support uint8\int8\int16 type for ONNX
    // opset version < 14.
    // Fix in this ONNX PR:
    // https://github.com/onnx/onnx/pull/3334
    if (n->kind() != onnx::Gemm && IsStandardOp(n->kind()) &&
        (scalar_type == quarisma::kByte || scalar_type == quarisma::kChar ||
         scalar_type == quarisma::kShort))
    {
        return quarisma::kLong;
    }
    return scalar_type;
}

static void UpdateScalarTypeForInputs(Node* n, const quarisma::ScalarType& scalar_type)
{
    const int64_t onnx_type = ScalarTypeToONNXType(scalar_type);
    if (onnx_type < 0)
    {
        TORCH_WARN(
            "ONNX Scalar Type Analysis - Scalar type: ",
            quarisma::toString(scalar_type),
            " of input tensor in operator: ",
            n->kind().toDisplayString(),
            " not supported in ONNX. ");
        return;
    }

    size_t input_idx = 0;
    for (auto input : n->inputs())
    {
        auto input_tensor_type = input->type()->cast<TensorType>();
        auto input_scalar_type = input_tensor_type ? input_tensor_type->scalarType() : std::nullopt;

        // We skip the 'condition' input (i.e., the first input) in case of
        // onnx:Where operator.
        if (IsSelectorOp(n->kind()) && input_idx == 0)
        {
            input_idx++;
            continue;
        }

        if ((input->node()->kind() == onnx::Constant) ||
            (input_scalar_type && (*input_scalar_type != scalar_type)))
        {
            if (input->node()->kind() == onnx::Constant)
            {
                // Fix up the scalar directly instead of inserting a cast operator.
                // TODO: Keep only the else branch once constant_folding is enabled by
                // default.
                quarisma::Tensor val        = input->node()->t(attr::value);
                quarisma::Tensor new_val    = val.to(scalar_type);
                Node*            const_node = n->owningGraph()->create(onnx::Constant);
                const_node->t_(attr::value, new_val);
                const_node->insertBefore(n);
                const_node->output()->setType(TensorType::create(new_val));
                const_node->copyMetadata(n);
                n->replaceInputWith(input, const_node->output());
            }
            else
            {
                Node* cast_node = n->owningGraph()->create(onnx::Cast);
                cast_node->addInput(input);
                cast_node->i_(attr::to, onnx_type);
                cast_node->insertBefore(n);
                cast_node->output()->setType(
                    CreateProfiledTensorTypeWithScalarType(input_tensor_type, scalar_type));
                cast_node->copyMetadata(n);
                n->replaceInputWith(input, cast_node->output());
            }
        }

        input_idx++;
    }
}

static void UpdateScalarTypeForOutput(Node* n, const quarisma::ScalarType& scalar_type)
{
    if (auto output_tensor_type = n->output()->type()->cast<TensorType>())
    {
        n->output()->setType(
            CreateProfiledTensorTypeWithScalarType(output_tensor_type, scalar_type));
    }
}

static void RecoverScalarTypeForOutput(Value* out, const quarisma::ScalarType& scalar_type)
{
    Node* n = out->node();
    TORCH_INTERNAL_ASSERT(nullptr != n);
    const int64_t onnx_type = ScalarTypeToONNXType(scalar_type);
    Node*         cast_node = n->owningGraph()->create(onnx::Cast, 1);
    cast_node->addInput(out);
    cast_node->i_(attr::to, onnx_type);
    cast_node->insertAfter(n);
    cast_node->copyMetadata(n);
    out->replaceAllUsesAfterNodeWith(cast_node, cast_node->output());
}

// This example error found when exports transfo_xl model using add op in uint8
// type, as below:
// if self.same_length:
//     all_ones = word_emb.new_ones((qlen, klen), dtype=torch.uint8)
//     mask_len = klen - self.mem_len
//     if mask_len > 0:
//         mask_shift_len = qlen - mask_len
//     else:
//         mask_shift_len = qlen
//     dec_attn_mask = (torch.triu(all_ones, 1 + mlen) + torch.tril(all_ones,
//     -mask_shift_len))[:, :, None]  # -1
//
// `all_ones is` an uint8 tensor, But the calculation of `dec_attn_mask` using
// add(+) op to get the uint8 result. Reference Link:
// https://github.com/huggingface/transformers/blob/b020a736c374460af1b34267283f957988350630/src/transformers/models/transfo_xl/modeling_transfo_xl.py#L936
static void LowPrecisionCastNodeForStandardOps(Node* n, int opset_version)
{
    TORCH_INTERNAL_ASSERT(n->outputs().size() == 1);
    if (n->output()->type()->cast<TensorType>() == nullptr ||
        n->output()->type()->cast<TensorType>()->scalarType() == std::nullopt)
    {
        // skip LowPrecisionCast if op output type is null.
        return;
    }
    auto output_scalar_type = n->output()->type()->cast<TensorType>()->scalarType().value();
    for (size_t i = 0; i < n->inputs().size(); ++i)
    {
        if (n->input(i)->type()->cast<TensorType>() == nullptr ||
            n->input(i)->type()->cast<TensorType>()->scalarType() == std::nullopt)
        {
            // skip LowPrecisionCast if any op input type node is null.
            return;
        }
        auto input_tensor_type = n->input(i)->type()->cast<TensorType>()->scalarType().value();
        TORCH_INTERNAL_ASSERT(output_scalar_type == input_tensor_type);
    }

    // The LowPrecision problem will be fixed in ONNX opset 14.
    if (opset_version < ONNX_OPSET_14)
    {
        auto expected_scalar_type_cast = LowPrecisionCastForStandardOps(n, output_scalar_type);
        UpdateScalarTypeForInputs(n, *expected_scalar_type_cast);
        if (output_scalar_type != *expected_scalar_type_cast)
        {
            // If input type is changed, convert it to the original type.
            RecoverScalarTypeForOutput(n->output(), output_scalar_type);
        }
    }
}

static void ImplicitCastNodeForONNX(Node* n)
{
    if (IsImplicitCastSupported(n->kind()))
    {
        auto expected_scalar_type = InferExpectedScalarType(n);
        if (expected_scalar_type)
        {
            UpdateScalarTypeForInputs(n, *expected_scalar_type);
            if (!IsComparisonOp(n->kind()))
            {
                UpdateScalarTypeForOutput(n, *expected_scalar_type);
            }
        }
    }
}

static void ImplicitCastForONNX(Block* block)
{
    for (auto it = block->nodes().begin(); it != block->nodes().end(); ++it)
    {
        for (auto sub : it->blocks())
        {
            ImplicitCastForONNX(sub);
        }

        ImplicitCastNodeForONNX(*it);
    }
    EliminateDeadCode(block, true, DCESideEffectPolicy::ALLOW_DELETING_NODES_WITH_SIDE_EFFECTS);
}

static void LowPrecisionCastForStandardOpsONNX(Block* block, int opset_version)
{
    for (auto it = block->nodes().begin(); it != block->nodes().end(); ++it)
    {
        for (auto sub : it->blocks())
        {
            LowPrecisionCastForStandardOpsONNX(sub, opset_version);
        }

        if (IsStandardOp(it->kind()))
        {
            LowPrecisionCastNodeForStandardOps(*it, opset_version);
        }
    }
    EliminateDeadCode(block, true, DCESideEffectPolicy::ALLOW_DELETING_NODES_WITH_SIDE_EFFECTS);
}
}  // anonymous namespace

void ScalarTypeAnalysisForONNX(
    const std::shared_ptr<Graph>& graph, bool lowprecision_cast, int opset_version)
{
    GRAPH_DUMP("Before ScalarTypeAnalysisForONNX: ", graph);
    ImplicitCastForONNX(graph->block());
    if (lowprecision_cast)
    {
        LowPrecisionCastForStandardOpsONNX(graph->block(), opset_version);
    }
    GRAPH_DUMP("After ScalarTypeAnalysisForONNX: ", graph);
}

void ScalarTypeAnalysisNodeForONNX(Node* n)
{
    ImplicitCastNodeForONNX(n);
}

}  // namespace torch::jit
