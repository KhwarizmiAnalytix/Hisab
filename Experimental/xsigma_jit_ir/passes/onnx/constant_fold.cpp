#include <Quarisma/Functions.h>
#include <quarisma/util/irange.h>
#include <torch/csrc/jit/jit_log.h>
#include <torch/csrc/jit/passes/onnx/constant_fold.h>
#include <torch/csrc/jit/passes/onnx/helper.h>

#include <algorithm>
#include <optional>

#include "util/exception.h"

namespace torch::jit
{

namespace onnx
{
using namespace ::quarisma::onnx;
}

namespace onnx_constant_fold
{

enum OnnxType : int
{
    ONNX_FLOAT = 1,
    ONNX_UINT8,
    ONNX_INT8,
    ONNX_UINT16,
    ONNX_INT16,
    ONNX_INT32,
    ONNX_INT64,
    ONNX_FLOAT16 = 10,
    ONNX_DOUBLE,
    ONNX_UINT32,
};

static std::unordered_map<int, quarisma::ScalarType> onnxTypeToScalarTypeMap = {
    // Only conversion of ONNX numeric types is included here.
    // Unsigned ONNX types are mapped to the next higher signed
    // ScalarType type.
    {ONNX_FLOAT, quarisma::kFloat},
    {ONNX_UINT8, quarisma::kByte},
    {ONNX_INT8, quarisma::kChar},
    {ONNX_UINT16, quarisma::kInt},
    {ONNX_INT16, quarisma::kShort},
    {ONNX_INT32, quarisma::kInt},
    {ONNX_INT64, quarisma::kLong},
    {ONNX_FLOAT16, quarisma::kFloat},
    {ONNX_DOUBLE, quarisma::kDouble},
    {ONNX_UINT32, quarisma::kLong},
};

static void handleNegativeStartEndIndex(
    int64_t& start, int64_t& end, int64_t& axis, quarisma::IntArrayRef tensorSizes)
{
    if (start < 0)
    {
        start = tensorSizes[axis] + start;
    }
    if (end < 0)
    {
        end = tensorSizes[axis] + end;
    }
    // index higher than dimension is treated as the end.
    if (end > tensorSizes[axis])
    {
        end = tensorSizes[axis];
    }
}

static std::optional<quarisma::Tensor> runTorchSlice_opset9(
    const Node* node, std::vector<quarisma::Tensor>& inputTensorValues)
{
    assert(inputTensorValues.size() == 1);
    if (inputTensorValues.size() != 1)
    {
        TORCH_WARN(
            "Constant folding - Invalid number of inputs found for opset 9 "
            "onnx::Slice op. Constant folding not applied.");
        return std::nullopt;
    }
    if (!(node->hasAttributeS("starts") && node->hasAttributeS("ends")))
    {
        return std::nullopt;
    }
    auto const& startsAttr = node->is(attr::starts);
    auto const& endsAttr   = node->is(attr::ends);
    if (startsAttr.size() != endsAttr.size())
    {
        return std::nullopt;
    }
    std::vector<int64_t> axesAttr;
    if (node->hasAttributeS("axes"))
    {
        axesAttr = node->is(attr::axes);
    }
    else
    {
        axesAttr.resize(startsAttr.size());
        std::iota(axesAttr.begin(), axesAttr.end(), 0);
    }
    auto updated_val = inputTensorValues[0];
    for (const auto i : quarisma::irange(axesAttr.size()))
    {
        // ONNX slice accepts negative starts and ends values.
        int64_t axis = axesAttr[i], start = startsAttr[i], end = endsAttr[i];
        // ONNX slice accepts negative axis, fix this for aten op
        axis += axis < 0 ? inputTensorValues[0].sizes().size() : 0;
        handleNegativeStartEndIndex(start, end, axis, updated_val.sizes());
        int64_t length = end - start;
        if (length < 0 || start > updated_val.sizes()[axis] - length)
            return std::nullopt;
        updated_val = quarisma::narrow(updated_val, axis, start, length);
    }
    return std::optional<quarisma::Tensor>(updated_val);
}

static std::optional<quarisma::Tensor> runTorchSlice_opset10(
    const Node* node, std::vector<quarisma::Tensor>& inputTensorValues)
{
    const int maxSliceInputCount = 5;
    const int minSliceInputCount = 3;
    if (inputTensorValues.size() < minSliceInputCount ||
        inputTensorValues.size() > maxSliceInputCount)
    {
        TORCH_WARN(
            "Constant folding - Invalid number of inputs found for opset opset >= 10 onnx::Slice "
            "op. "
            "Constant folding not applied.");
        return std::nullopt;
    }
    // Checking validity of 'starts' and 'ends' input
    if (inputTensorValues[1].sizes().size() != 1 || inputTensorValues[2].sizes().size() != 1)
    {
        TORCH_WARN(
            "Constant folding - Invalid 'starts' or 'ends' inputs found for opset >= 10 "
            "onnx::Slice op. "
            "Constant folding not applied.");
        return std::nullopt;
    }
    if (inputTensorValues[1].sizes()[0] != inputTensorValues[2].sizes()[0])
    {
        // Number of elements of 'starts' and 'ends' 1-D input tensors should be the
        // same
        return std::nullopt;
    }
    // Checking 'axes' input, if available.
    std::vector<int64_t> axes;
    if (inputTensorValues.size() > 3)
    {
        if (inputTensorValues[3].sizes().size() != 1)
        {
            TORCH_WARN(
                "Constant folding - Invalid 'axes' input found for opset >= 10 onnx::Slice op. "
                "Constant folding not applied.");
            return std::nullopt;
        }
        if (inputTensorValues[3].sizes()[0] != inputTensorValues[1].sizes()[0])
        {
            // Number of elements of 'axes' and 'ends' 1-D input tensors should be the
            // same
            TORCH_WARN(
                "Constant folding - Invalid 'axes' or 'ends' inputs found for opset >= 10 "
                "onnx::Slice op. "
                "Constant folding not applied.");
            return std::nullopt;
        }
        auto axes_a = inputTensorValues[3].accessor<int64_t, 1>();
        axes.resize(inputTensorValues[3].sizes()[0]);
        // ONNX slice accepts negative axis, fix this for aten op
        for (const auto i : quarisma::irange(inputTensorValues[3].sizes()[0]))
        {
            axes[i] = axes_a[i] < 0 ? axes_a[i] + inputTensorValues[0].sizes().size() : axes_a[i];
        }
    }
    else
    {
        axes = std::vector<int64_t>(inputTensorValues[1].sizes()[0], 0);
    }
    // Checking 'steps' input, if available.
    if (inputTensorValues.size() > 4)
    {
        if (inputTensorValues[4].sizes().size() != 1)
        {
            TORCH_WARN(
                "Constant folding - Invalid 'steps' input found for opset >= 10 onnx::Slice op. "
                "Constant folding not applied.");
            return std::nullopt;
        }
        if (inputTensorValues[4].sizes()[0] != inputTensorValues[1].sizes()[0])
        {
            // Number of elements of 'steps' and 'ends' 1-D input tensors should be
            // the same
            TORCH_WARN(
                "Constant folding - Invalid 'steps' or 'ends' inputs found for opset >= 10 "
                "onnx::Slice op. "
                "Constant folding not applied.");
            return std::nullopt;
        }
        auto steps_a = inputTensorValues[4].accessor<int64_t, 1>();
        for (const auto i : quarisma::irange(inputTensorValues[4].sizes()[0]))
        {
            // Only steps == 1 are supported for constant-folding.
            if (steps_a[i] != 1)
            {
                TORCH_WARN(
                    "Constant folding - Only steps=1 can be constant folded for opset >= 10 "
                    "onnx::Slice op. "
                    "Constant folding not applied.");
                return std::nullopt;
            }
        }
    }
    auto starts_a    = inputTensorValues[1].accessor<int64_t, 1>();
    auto ends_a      = inputTensorValues[2].accessor<int64_t, 1>();
    auto updated_val = inputTensorValues[0];
    for (const auto i : quarisma::irange(inputTensorValues[1].sizes()[0]))
    {
        // ONNX slice accepts negative starts and ends values.
        int64_t start = starts_a[i], end = ends_a[i], axis = axes[i];
        handleNegativeStartEndIndex(start, end, axis, updated_val.sizes());
        int64_t length = end - start;
        if (length < 0 || start > updated_val.sizes()[axis] - length)
            return std::nullopt;
        updated_val = quarisma::narrow(updated_val, axis, start, length);
    }
    return std::optional<quarisma::Tensor>(updated_val);
}

// Refer to AT_FORALL_SCALAR_TYPES_WITH_COMPLEX_EXCEPT_COMPLEX_HALF
static quarisma::Tensor runTorchArange_opset11(
    const Node* node, const std::vector<quarisma::Tensor>& inputTensorValues)
{
    TORCH_INTERNAL_ASSERT(inputTensorValues.size() == 3);
    auto             dtype = inputTensorValues[0].scalar_type();
    quarisma::Tensor updated_val;
    switch (dtype)
    {
    case quarisma::ScalarType::Float:
    {
        auto start  = inputTensorValues[0].item<float>();
        auto end    = inputTensorValues[1].item<float>();
        auto step   = inputTensorValues[2].item<float>();
        updated_val = quarisma::arange(start, end, step);
        break;
    }
    case quarisma::ScalarType::Double:
    {
        auto start  = inputTensorValues[0].item<double>();
        auto end    = inputTensorValues[1].item<double>();
        auto step   = inputTensorValues[2].item<double>();
        updated_val = quarisma::arange(start, end, step);
        break;
    }
    case quarisma::ScalarType::Short:
    {
        auto start  = inputTensorValues[0].item<int16_t>();
        auto end    = inputTensorValues[1].item<int16_t>();
        auto step   = inputTensorValues[2].item<int16_t>();
        updated_val = quarisma::arange(start, end, step);
        break;
    }
    case quarisma::ScalarType::Int:
    {
        auto start  = inputTensorValues[0].item<int>();
        auto end    = inputTensorValues[1].item<int>();
        auto step   = inputTensorValues[2].item<int>();
        updated_val = quarisma::arange(start, end, step);
        break;
    }
    case quarisma::ScalarType::Long:
    {
        auto start  = inputTensorValues[0].item<int64_t>();
        auto end    = inputTensorValues[1].item<int64_t>();
        auto step   = inputTensorValues[2].item<int64_t>();
        updated_val = quarisma::arange(start, end, step);
        break;
    }
    default:
    {
        TORCH_WARN("Constant folding - ONNX Range type: ", dtype, " is not supported.");
    }
    }
    return updated_val;
}

quarisma::Tensor IntToTensor(int64_t value)
{
    auto options = quarisma::TensorOptions().dtype(quarisma::kLong).device(quarisma::kCPU);
    std::vector<int64_t> size_data = {value};
    auto f = quarisma::from_blob(size_data.data(), {1}, quarisma::kLong).to(quarisma::kCPU);
    // Need copy here
    quarisma::Tensor f_copy = quarisma::empty({1}, options);
    f_copy.copy_(f);
    return quarisma::squeeze(f_copy, 0);
}

std::optional<quarisma::Tensor> runTorchBackendForOnnx(
    const Node* node, std::vector<quarisma::Tensor>& inputTensorValues, int opset_version)
{
    quarisma::Tensor updated_val;
    if (node->kind() == onnx::Slice)
    {
        if (opset_version == ONNX_OPSET_9)
        {
            return runTorchSlice_opset9(node, inputTensorValues);
        }
        else if (opset_version >= ONNX_OPSET_10)
        {
            return runTorchSlice_opset10(node, inputTensorValues);
        }
        else
        {
            TORCH_WARN(
                "Constant folding - unsupported opset version. Constant folding not applied.");
            return std::nullopt;
        }
    }
    else if (node->kind() == onnx::Concat)
    {
        if (!node->hasAttributeS("axis"))
        {
            return std::nullopt;
        }
        updated_val = quarisma::cat(quarisma::TensorList(inputTensorValues), node->i(attr::axis));
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Sqrt)
    {
        updated_val = quarisma::sqrt(inputTensorValues[0]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Div)
    {
        // One example shows quarisma::div(CPULongType, CPULongType) = CPUFloatType,
        // So we add a cast below.
        updated_val = quarisma::div(inputTensorValues[0], inputTensorValues[1]);
        if (inputTensorValues[0].scalar_type() == inputTensorValues[1].scalar_type())
        {
            updated_val = updated_val.to(inputTensorValues[0].scalar_type());
        }
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Mul)
    {
        updated_val = quarisma::mul(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Sub)
    {
        updated_val = quarisma::sub(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Add)
    {
        updated_val = quarisma::add(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Unsqueeze)
    {
        if (opset_version >= ONNX_OPSET_13)
        {
            assert(inputTensorValues.size() == 2);
            // Checking validity of 'axes' input
            if (inputTensorValues[1].sizes().size() != 1)
            {
                TORCH_WARN(
                    "Constant folding - Invalid 'axes' inputs found for opset 13 onnx::Unsqueeze "
                    "op. "
                    "Constant folding not applied.");
                return std::nullopt;
            }
            auto                 axes_a = inputTensorValues[1].accessor<int64_t, 1>();
            std::vector<int64_t> axes;
            for (int64_t i = 0; i < inputTensorValues[1].sizes()[0]; ++i)
            {
                // ONNX unsqueeze accepts negative axes
                // From https://pytorch.org/docs/stable/generated/torch.unsqueeze.html
                // Negative dim will correspond to unsqueeze() applied quarisma dim = dim +
                // input.dim() + 1.
                axes_a[i] += axes_a[i] < 0 ? inputTensorValues[0].sizes().size() + 1 : 0;
                axes.push_back(axes_a[i]);
            }
            std::sort(axes.begin(), axes.end());
            updated_val = inputTensorValues[0];
            for (int64_t i = 0; i < inputTensorValues[1].sizes()[0]; ++i)
            {
                updated_val = quarisma::unsqueeze(updated_val, axes[i]);
            }
            return std::optional<quarisma::Tensor>(updated_val);
        }
        else if (opset_version >= ONNX_OPSET_9)
        {
            assert(inputTensorValues.size() == 1);
            if (!node->hasAttributeS("axes"))
            {
                return std::nullopt;
            }
            updated_val                   = inputTensorValues[0];
            std::vector<int64_t> axesAttr = node->is(attr::axes);
            std::sort(axesAttr.begin(), axesAttr.end());
            for (auto axis : axesAttr)
            {
                updated_val = quarisma::unsqueeze(updated_val, axis);
            }
            return std::optional<quarisma::Tensor>(updated_val);
        }
        else
        {
            TORCH_WARN(
                "Constant folding - unsupported opset version. "
                "Constant folding not applied.");
            return std::nullopt;
        }
    }
    else if (node->kind() == onnx::Squeeze)
    {
        assert(inputTensorValues.size() == 2 || inputTensorValues.size() == 1);
        if (opset_version >= ONNX_OPSET_13)
        {
            // Squeeze version 13 input axes is optional, inputTensorValues.size() ==
            // 1 means axes equal to None
            updated_val = inputTensorValues[0];
            if (inputTensorValues.size() == 2)
            {
                // Checking validity of 'axes' input
                if (inputTensorValues[1].sizes().size() != 1)
                {
                    TORCH_WARN(
                        "Constant folding - Invalid 'axes' inputs found for opset 13 onnx::Squeeze "
                        "op. "
                        "Constant folding not applied.");
                    return std::nullopt;
                }
                auto                 axes_a = inputTensorValues[1].accessor<int64_t, 1>();
                std::vector<int64_t> axes;
                for (int64_t i = 0; i < inputTensorValues[1].sizes()[0]; ++i)
                {
                    // ONNX Squeeze accepts negative axes
                    axes_a[i] += axes_a[i] < 0 ? inputTensorValues[0].sizes().size() : 0;
                    axes.push_back(axes_a[i]);
                }
                std::sort(axes.begin(), axes.end());
                for (int64_t i = 0; i < inputTensorValues[1].sizes()[0]; ++i)
                {
                    updated_val = quarisma::squeeze(updated_val, axes[i]);
                }
            }
            return std::optional<quarisma::Tensor>(updated_val);
        }
        else if (opset_version >= ONNX_OPSET_9)
        {
            assert(inputTensorValues.size() == 1);
            updated_val = inputTensorValues[0];
            if (node->hasAttributeS("axes"))
            {
                std::vector<int64_t> axesAttr = node->is(attr::axes);
                std::sort(axesAttr.begin(), axesAttr.end());
                for (auto axis : axesAttr)
                {
                    updated_val = quarisma::squeeze(updated_val, axis);
                }
            }
            return std::optional<quarisma::Tensor>(updated_val);
        }
        else
        {
            TORCH_WARN(
                "Constant folding - unsupported opset version. "
                "Constant folding not applied.");
            return std::nullopt;
        }
    }
    else if (node->kind() == onnx::Transpose)
    {
        assert(inputTensorValues.size() == 1);
        if (!node->hasAttributeS("perm"))
        {
            return std::nullopt;
        }
        updated_val = inputTensorValues[0].permute(node->is(attr::perm));
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Cast)
    {
        assert(inputTensorValues.size() == 1);
        if (node->hasAttributeS("to") && ONNXTypeToATenType(node->i(attr::to)))
        {
            updated_val = inputTensorValues[0].to(ONNXTypeToATenType(node->i(attr::to)).value());
            return std::optional<quarisma::Tensor>(updated_val);
        }
        return std::nullopt;
    }
    else if (node->kind() == onnx::Reshape)
    {
        assert(inputTensorValues.size() == 2);
        updated_val = inputTensorValues[0];
        std::vector<int64_t> shape(inputTensorValues[1].sizes()[0], 0);
        auto                 shape_a = inputTensorValues[1].accessor<int64_t, 1>();
        assert(inputTensorValues[1].sizes()[0] >= 0);
        // Set value of allowzero
        int64_t allowzero = 0;
        if (node->hasAttributeS("allowzero"))
        {
            allowzero = node->i(attr::allowzero);
        }
        for (size_t i = 0; i < (size_t)(inputTensorValues[1].sizes()[0]); ++i)
        {
            // All shape dim values should be >= -1
            // onnx::Reshape supports a shape dim value to be zero, in
            // which case the actual dim value remains unchanged. However,
            // quarisma::reshape does not support shape dim value to be zero
            assert(shape_a[i] >= -1);
            if (shape_a[i] == 0 && !allowzero)
            {
                if (i >= inputTensorValues[0].sizes().size())
                {
                    throw std::runtime_error(
                        "Dimension with value 0 exceeds the input size dimensions.");
                }
                shape[i] = inputTensorValues[0].sizes()[i];
            }
            else
            {
                shape[i] = shape_a[i];
            }
        }
        return std::optional<quarisma::Tensor>(quarisma::reshape(updated_val, shape));
    }
    else if (node->kind() == onnx::Shape)
    {
        TORCH_INTERNAL_ASSERT(inputTensorValues.size() == 1);
        updated_val = quarisma::_shape_as_tensor(inputTensorValues[0]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::ReduceL1 || node->kind() == onnx::ReduceL2)
    {
        assert(inputTensorValues.size() == 1);
        if (!node->hasAttributeS("axes"))
        {
            return std::nullopt;
        }
        if (!node->hasAttributeS("keepdims"))
        {
            return std::nullopt;
        }
        int p = node->kind() == onnx::ReduceL1 ? 1 : 2;
        updated_val =
            quarisma::norm(inputTensorValues[0], p, node->is(attr::axes), node->i(attr::keepdims));
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::ReduceProd)
    {
        int64_t              rank = inputTensorValues[0].sizes().size();
        std::vector<int64_t> axes;
        if (!node->hasAttributeS("axes"))
        {
            axes = std::vector<int64_t>(rank);
            std::iota(axes.rbegin(), axes.rend(), 0);
        }
        else
        {
            for (const auto& axis : node->is(attr::axes))
            {
                axes.emplace_back(axis < 0 ? axis + rank : axis);
            }
            std::sort(axes.begin(), axes.end(), std::greater<>());
        }

        bool keepdims = node->hasAttributeS("keepdims") ? node->i(attr::keepdims) : true;
        updated_val   = inputTensorValues[0];
        for (const auto& axis : axes)
        {
            updated_val = quarisma::prod(updated_val, axis, keepdims);
        }
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Gather)
    {
        assert(inputTensorValues.size() == 2);
        // default axis = 0
        int64_t axis = 0;
        if (node->hasAttributeS("axis"))
        {
            axis = node->i(attr::axis);
        }
        // If axis attribute for onnx::Gather has a value less than 0,
        // It needs to be adjusted (+= dim sizes) for aten op
        axis += axis < 0 ? inputTensorValues[0].sizes().size() : 0;
        quarisma::Tensor indices = inputTensorValues[1];
        auto             q       = indices.dim();
        // quarisma::index_select only supports indices with rank <= 1.
        // See https://pytorch.org/docs/main/generated/torch.index_select.html
        if (q > 1)
        {
            return std::nullopt;
        }
        // If the device of indices tensor is not the same with it of the input
        // tensor, move it to the device of the input tensor
        if (inputTensorValues[0].device() != indices.device())
        {
            indices = indices.to(inputTensorValues[0].device());
        }
        // If indices input for onnx::Gather has a value less than 0,
        // It needs to be adjusted (+= dim value) for aten op
        auto less_mask      = quarisma::lt(indices, 0);
        auto indices_corr   = quarisma::add(indices, inputTensorValues[0].sizes()[axis]);
        auto indices_masked = quarisma::where(less_mask, indices_corr, indices);
        updated_val         = quarisma::index_select(inputTensorValues[0], axis, indices_masked);
        // If rank of indices is 0, rank of output tensor should be
        // rank_of_input - 1.
        if (q < 1)
        {
            updated_val = updated_val.squeeze(axis);
        }
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Range)
    {
        updated_val = runTorchArange_opset11(node, inputTensorValues);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Where)
    {
        updated_val =
            quarisma::where(inputTensorValues[0], inputTensorValues[1], inputTensorValues[2]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Equal)
    {
        updated_val = quarisma::eq(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Greater)
    {
        updated_val = quarisma::greater(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Less)
    {
        updated_val = quarisma::less(inputTensorValues[0], inputTensorValues[1]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Neg)
    {
        updated_val = quarisma::neg(inputTensorValues[0]);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Not)
    {
        auto ones   = quarisma::ones(inputTensorValues[0].sizes(), inputTensorValues[0].dtype());
        updated_val = quarisma::ne(inputTensorValues[0], ones);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else if (node->kind() == onnx::Size)
    {
        int64_t total_size = 1;
        for (auto size : inputTensorValues[0].sizes())
        {
            total_size *= size;
        }
        return std::optional<quarisma::Tensor>(IntToTensor(total_size));
    }
    else if (node->kind() == onnx::Softmax)
    {
        int64_t axis = node->hasAttributeS("axis") ? node->i(attr::axis) : -1;
        updated_val  = quarisma::softmax(inputTensorValues[0], axis);
        return std::optional<quarisma::Tensor>(updated_val);
    }
    else
    {
        return std::nullopt;
    }
}

static bool isConstant(Value* val, const ValueToParamPairMap& valsToParamsMap)
{
    auto parentNode = val->node();
    return (parentNode->kind() == prim::Param &&
            valsToParamsMap.find(val) !=
                valsToParamsMap.end()) ||  // Checks val is a parameter and not a real input
           (parentNode->kind() == onnx::Constant && !parentNode->mustBeNone() &&
            parentNode->kindOf(attr::value) == AttributeKind::t);  // Check other types?
}

static bool hasParamInput(Node* n, const ValueToParamPairMap& valsToParamsMap)
{
    for (auto input : n->inputs())
    {
        if (valsToParamsMap.find(input) != valsToParamsMap.end())
        {
            return true;
        }
    }
    return false;
}

static std::vector<quarisma::Tensor> getValues(
    Node* node, const ValueToParamPairMap& valsToParamsMap)
{
    size_t                        numInputs = node->inputs().size();
    std::vector<quarisma::Tensor> inputTensorValues;
    inputTensorValues.reserve(numInputs);
    for (auto val : node->inputs())
    {
        if (val->node()->kind() == prim::Param)
        {
            auto itr = valsToParamsMap.find(val);
            if (itr == valsToParamsMap.end())
            {
                throw std::runtime_error(
                    "getValues: Input value not found amongst constant parameters.");
            }
            inputTensorValues.push_back(itr->second.second.toTensor());
        }
        else if (val->node()->kind() == onnx::Constant)
        {
            inputTensorValues.push_back(val->node()->t(attr::value));
        }
        else
        {
            throw std::runtime_error("getValues: Unsupported kind of constant node found.");
        }
    }
    TORCH_INTERNAL_ASSERT(inputTensorValues.size() == numInputs);
    return inputTensorValues;
}

static bool areNodeInputsConstant(Node* node, const ValueToParamPairMap& valsToParamsMap)
{
    return std::all_of(
        node->inputs().begin(),
        node->inputs().end(),
        [&valsToParamsMap](Value* v) { return isConstant(v, valsToParamsMap); });
}

static std::vector<Node*> getOnnxConstParentsToRemove(Node* node)
{
    std::vector<Node*> parentNodes;
    for (auto val : node->inputs())
    {
        // If the parent of 'node' is an onnx::Constant node,
        // and 'node' is the only downstream node it serves (this
        // is important), then push it in the list to remove.
        if (val->node()->kind() == onnx::Constant && val->uses().size() == 1)
        {
            parentNodes.push_back(val->node());
        }
    }
    return parentNodes;
}

}  // namespace onnx_constant_fold

// This method updates the block in-place to fold all the one-time
// constant-based computations/ops into an initializer node.
//
// NB: This is not constant folding in the traditional sense, as we
// don't try particularly hard to evaluate operations on constant nodes.
// This is more of a partial evaluation analysis, where operations on constant
// nodes can be lifted so we run them earlier, before the usual parameters are
// known.
static void ConstantFoldONNX(Block* b, ParamMap& paramsDict, int opset_version)
{
    if (opset_version < ONNX_OPSET_9)
    {
        TORCH_WARN(
            "Constant folding supported for only opsets >= 9. "
            "Constant folding not applied.");
        return;
    }
    TORCH_INTERNAL_ASSERT(b->param_node());
    auto valsToParamsMap = buildValueToParamsMap(b, paramsDict);
    // Only the root block is constant-folded. Folding nested blocks is
    // not supported for now.
    for (auto it = b->nodes().begin(), end = b->nodes().end(); it != end; ++it)
    {
        auto node = *it;
        if (node->outputs().size() > 1)
        {
            // Constant folding for multiple-output nodes not supported. Skip it.
            continue;
        }
        if (!onnx_constant_fold::areNodeInputsConstant(node, valsToParamsMap))
        {
            // If all the inputs to this node are not either parameter or
            // onnx::Constant, then skip this node.
            continue;
        }

        auto inputTensorValues = onnx_constant_fold::getValues(node, valsToParamsMap);
        if (inputTensorValues.empty())
        {
            // This is a terminal node with no inputs, such as onnx::Constant. Skip
            // it.
            continue;
        }
        auto updatedValWrapped =
            onnx_constant_fold::runTorchBackendForOnnx(node, inputTensorValues, opset_version);
        if (updatedValWrapped == std::nullopt)
        {
            // Constant folding is not supported for this op. Skip it.
            continue;
        }

        quarisma::Tensor updatedVal          = *updatedValWrapped;
        auto             newSourceNodeOutput = [&]() -> Value*
        {
            if (onnx_constant_fold::hasParamInput(node, valsToParamsMap))
            {
                // Create a new input to the block (prim::Param node output). Add a
                // corresponding entry in valToParamMap. Replace the downstream inputs
                // with this value, and disconnect all the input values of the folded
                // node.
                auto newSourceNodeOutput = b->addInput();
                valsToParamsMap.insert(
                    {newSourceNodeOutput,
                     std::make_pair(newSourceNodeOutput->debugName(), updatedVal)});
                return newSourceNodeOutput;
            }
            else
            {
                auto newSourceNode = createONNXConstant(node->owningGraph(), node, updatedVal);
                newSourceNode->copyMetadata(node);
                return newSourceNode->output();
            }
        }();
        newSourceNodeOutput->inferTypeFrom(updatedVal);
        node->outputs().quarisma(0)->replaceAllUsesWith(newSourceNodeOutput);
        // Next we remove the current node that has been replaced by
        // an initializer. But before we start de-wiring this node,
        // we check if any parents of this nodes were onnx::Constant
        // and remove them first, and then remove the current node.
        // If the parent was an initializer (not onnx::Constant) then
        // they are all removed by the eraseUnusedBlockInputs() call
        // (below) outside the loop.
        auto onnxConstParents = onnx_constant_fold::getOnnxConstParentsToRemove(node);
        node->removeAllInputs();
        for (auto* n : onnxConstParents)
        {
            n->destroy();
        }
        it.destroyCurrent();
    }
    eraseUnusedValuesFromMap(valsToParamsMap);
    eraseUnusedBlockInputs(b);
    buildParamsMapFromValueToParamsMap(valsToParamsMap, paramsDict);
    return;
}

void ConstantFoldONNX(std::shared_ptr<Graph>& g, ParamMap& paramsDict, int opset_version)
{
    ConstantFoldONNX(g->block(), paramsDict, opset_version);
    GRAPH_DUMP("After ConstantFoldONNX:", g);
}

}  // namespace torch::jit
