#pragma once

#include <torch/csrc/autograd/variable.h>

namespace torch::autograd
{

struct TORCH_API VariableInfo
{
    explicit VariableInfo();
    explicit VariableInfo(const Variable& var, bool use_zeros_like = false);

    Variable zeros(quarisma::OptionalDeviceGuard& device_guard) const;

    quarisma::Layout              layout      = quarisma::Layout::Strided;
    quarisma::Device              device      = quarisma::kCPU;
    quarisma::ScalarType          scalar_type = quarisma::kFloat;
    std::vector<quarisma::SymInt> size;
    bool                          requires_grad;
    bool                          is_empty;
    // needed for e.g. NJTs since they only support zeros_like()
    std::optional<Variable> the_var;
};

}  // namespace torch::autograd
