#pragma once

#include <torch/csrc/jit/api/module.h>

namespace torch::jit
{

struct TORCH_API LinearBNParameters
{
    quarisma::Tensor linear_w;
    quarisma::Tensor linear_b;
    quarisma::Tensor bn_rm;
    quarisma::Tensor bn_rv;
    double           bn_eps = 0.0;
    quarisma::Tensor bn_w;
    quarisma::Tensor bn_b;
};

/**
 * Given the current weight and bias tensors of a Linear module and parameters
 * of the BatchNorm module we're folding with, compute the updated values
 * for the weight and bias.
 *
 * The function is basically copied from torch/nn/utils/fusion.py
 */
TORCH_API std::tuple<quarisma::Tensor, quarisma::Tensor> computeUpdatedLinearWeightAndBias(
    const LinearBNParameters& p);

}  // namespace torch::jit
