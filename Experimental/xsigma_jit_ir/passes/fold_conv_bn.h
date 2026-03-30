#pragma once

#include <torch/csrc/jit/api/module.h>

namespace torch::jit
{

/** \brief Fold Conv2d-BatchNorm2d into Conv2d in all methods of this
 * module and all its submodules, forward is included by default.
 *
 * The weight and bias of the Conv2d are correspondingly updated. Should only be
 * used on modules in eval mode.
 */
TORCH_API Module FoldConvBatchNorm(const Module& module);

struct TORCH_API ConvBNParameters
{
    quarisma::Tensor conv_w;
    quarisma::Tensor conv_b;
    quarisma::Tensor bn_rm;
    quarisma::Tensor bn_rv;
    double           bn_eps = 0.0;
    quarisma::Tensor bn_w;
    quarisma::Tensor bn_b;
};

/**
 * Given the current weight and bias tensors of a Conv module and parameters
 * of the BatchNorm module we're folding with, compute the updated values
 * for the weight and bias.
 *
 * The function is basically copied from torch/nn/utils/fusion.py
 */
TORCH_API std::tuple<quarisma::Tensor, quarisma::Tensor> computeUpdatedConvWeightAndBias(
    const ConvBNParameters& p);

}  // namespace torch::jit
