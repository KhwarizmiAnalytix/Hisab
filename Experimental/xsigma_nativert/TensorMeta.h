#pragma once

#include <quarisma/core/Device.h>
#include <quarisma/core/Layout.h>
#include <quarisma/core/MemoryFormat.h>
#include <quarisma/core/ScalarType.h>
#include <quarisma/core/TensorOptions.h>
#include <quarisma/util/ArrayRef.h>
#include <quarisma/util/Logging.h>
#include <torch/csrc/utils/generated_serialization_types.h>
#include <torch/nativert/executor/Placement.h>

namespace torch::nativert
{

quarisma::ScalarType   convertJsonScalarType(const torch::_export::ScalarType& scalarType);
quarisma::MemoryFormat convertJsonMemoryFormat(const torch::_export::MemoryFormat& memoryFormat);
quarisma::Layout       convertJsonLayout(const torch::_export::Layout& layout);
quarisma::Device       convertJsonDevice(const torch::_export::Device& device);

class TensorMeta
{
public:
    explicit TensorMeta(const torch::_export::TensorMeta& tensorMeta);

    quarisma::IntArrayRef sizes() const
    {
        QUARISMA_CHECK(!hasSymbolicShape_, "TensorMeta has symbolic shape");
        return sizes_;
    }

    quarisma::IntArrayRef strides() const
    {
        QUARISMA_CHECK(!hasSymbolicShape_, "TensorMeta has symbolic shape");
        return strides_;
    }

    quarisma::Layout layout() const { return layout_; }

    quarisma::ScalarType dtype() const { return dtype_; }

    bool requires_grad() const { return requiresGrad_; }

    int64_t storage_offset() const { return storage_offset_; }

    int64_t dim() const { return sizes_.size(); }

    int64_t numel() const
    {
        QUARISMA_CHECK(!hasSymbolicShape_, "TensorMeta has symbolic shape");
        return numel_;
    }

    quarisma::Device device() const { return device_; }

    // override device according to placement
    void setDevice(quarisma::Device device) { device_ = device; }

    quarisma::TensorOptions asTensorOptions() const
    {
        return quarisma::TensorOptions().dtype(dtype_).layout(layout_).requires_grad(requiresGrad_);
    }

    // override device according to placement
    void applyDevicePlacement(const Placement& placement)
    {
        device_ = placement.getMappedDevice(device_);
    }

    // NYI
    // quarisma::SymIntArrayRef sym_sizes() const {}
    // quarisma::SymIntArrayRef sym_strides() const {}
    // quarisma::SymInt sym_storage_offset() const {}
    // quarisma::SymInt sym_numel() const {}

private:
    bool hasSymbolicShape_ = false;

    std::vector<int64_t> sizes_;
    std::vector<int64_t> strides_;
    int64_t              storage_offset_ = 0;
    int64_t              numel_          = 1;

    quarisma::ScalarType dtype_;
    quarisma::Layout     layout_;
    bool                 requiresGrad_;

    quarisma::Device device_;
};

}  // namespace torch::nativert
