#pragma once

#include <Quarisma/Quarisma.h>
#include <quarisma/cuda/CUDAStream.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/autograd/function.h>
#include <torch/csrc/autograd/variable.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace torch::autograd
{

struct TORCH_CUDA_CU_API Scatter : public Node
{
    explicit Scatter(
        std::vector<quarisma::Device>       devices,
        std::optional<std::vector<int64_t>> chunk_sizes = std::nullopt,
        int64_t                             dim         = 0,
        std::optional<std::vector<std::optional<quarisma::cuda::CUDAStream>>> streams =
            std::nullopt,
        bool unsqueeze_scalars = false);
    ~Scatter() override;

    variable_list apply(variable_list&& inputs) override;

    std::vector<quarisma::Device>                                         devices_;
    std::optional<std::vector<int64_t>>                                   chunk_sizes_;
    int64_t                                                               dim_;
    std::optional<std::vector<std::optional<quarisma::cuda::CUDAStream>>> streams_;
    bool                                                                  unsqueeze_scalars_;
};

struct TORCH_CUDA_CU_API Gather : public Node
{
    explicit Gather(const quarisma::Device& destination_device, int64_t dim = 0);
    ~Gather() override;

    variable_list apply(variable_list&& inputs) override;

    quarisma::Device destination_device_;
    int64_t          dim_;
};

}  // namespace torch::autograd
