#pragma once

#include <Quarisma/core/Tensor.h>
#include <quarisma/core/SafePyObject.h>

namespace torch::autograd
{

struct TORCH_API SavedVariableHooks
{
    virtual void             call_pack_hook(const quarisma::Tensor& tensor) = 0;
    virtual quarisma::Tensor call_unpack_hook()                             = 0;
    virtual ~SavedVariableHooks()                                           = default;
    virtual std::optional<std::pair<quarisma::SafePyObject, quarisma::SafePyObject>>
    retrieve_unpack_hook_data() const
    {
        QUARISMA_CHECK(false, "Compiled Autograd only supports python saved tensor hooks ");
    }
};

}  // namespace torch::autograd
