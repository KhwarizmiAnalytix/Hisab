#pragma once

#include <Quarisma/ExpandUtils.h>
#include <Quarisma/NestedTensorImpl.h>
#include <Quarisma/core/Tensor.h>
#include <quarisma/core/Device.h>
#include <quarisma/core/DeviceType.h>
#include <quarisma/core/Stream.h>
#include <quarisma/core/SymIntArrayRef.h>
#include <quarisma/core/TensorImpl.h>
#include <quarisma/core/impl/DeviceGuardImplInterface.h>
#include <quarisma/util/DimVector.h>
#include <quarisma/util/SmallVector.h>

#include "util/exception.h"

#ifndef AT_PER_OPERATOR_HEADERS
#include <Quarisma/Functions.h>
#else
#include <Quarisma/ops/zeros.h>
#endif

namespace torch::autograd
{

using SymIntSmallVec = quarisma::SmallVector<quarisma::SymInt, quarisma::kDimVectorStaticSize>;
using MetadataShape  = std::variant<SymIntSmallVec, quarisma::Tensor>;

/**
 * Records TensorOptions, shape of the tensor, whether or not the Python
 * dispatch key is set (tensor subclass), and, where applicable, the stream the
 * corresponding operation took place on.
 *
 * If is_valid() is false, then the corresponding input is not used and may be
 * an undefined tensor.
 */
struct TORCH_API InputMetadata
{
    InputMetadata() = default;
    InputMetadata(
        const quarisma::TensorOptions&      options,
        MetadataShape                       input_shape,
        bool                                is_tensor_subclass,
        bool                                is_nested,
        std::optional<quarisma::ScalarType> grad_dtype);
    InputMetadata(const quarisma::Tensor& t);

    const quarisma::TensorOptions& options() const { return options_; }

    caffe2::TypeMeta dtype() const { return options_.dtype(); }

    quarisma::Device device() const { return options_.device(); }

    quarisma::Layout layout() const { return options_.layout(); }

    quarisma::Stream stream() const { return stream_; }

    bool is_tensor_subclass() const { return is_tensor_subclass_; }

    quarisma::Tensor zeros_like() const;

    bool is_same_shape(const quarisma::Tensor& grad) const;

    bool is_expandable_to_shape(const quarisma::Tensor& grad) const;

    quarisma::Tensor reduce_grad(quarisma::Tensor& grad) const;

    quarisma::Tensor maybe_reduce(
        const size_t                                          index,
        quarisma::Tensor                                      grad,
        const std::function<std::string(const std::string&)>& format_error) const;

    std::stringstream incompatible_shape_error_message(
        const size_t index, const quarisma::Tensor& grad) const;

    bool was_default_constructed() const { return was_default_constructed_; }

    bool is_cpp_nested_tensor() const;

    bool is_nested_tensor() const { return is_nested_; }

    quarisma::SymIntArrayRef shape_as_dim_vector() const;

    // Danger: not thread safe, caller must protect with lock
    SymIntSmallVec& mutable_shape_as_dim_vector();

    std::optional<quarisma::ScalarType> grad_dtype() const
    {
        TORCH_INTERNAL_ASSERT(!was_default_constructed_);
        return grad_dtype_;
    }

    void set_grad_dtype(const std::optional<quarisma::ScalarType>& grad_dtype)
    {
        TORCH_INTERNAL_ASSERT(!was_default_constructed_);
        grad_dtype_ = grad_dtype;
    }

private:
    quarisma::Tensor shape_as_tensor() const;
    bool             is_nestedness_same(const quarisma::Tensor& grad) const;
    bool             maybe_expandable_to(const quarisma::Tensor& grad) const;

    // NB: The engine does not use the dtype from the options, but rather the
    //     grad_dtype_ field to validate grad_output dtype.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const quarisma::TensorOptions options_;
    MetadataShape                 shape_;
    quarisma::Stream stream_ = quarisma::Stream(quarisma::Stream::Default::DEFAULT, device());
    bool             is_tensor_subclass_      = false;
    bool             is_nested_               = false;
    bool             was_default_constructed_ = true;

    // The grad_dtype_ field is the dtype that the engine expects the grad to be.
    // When nullopt, grad_dtype_ is allowed to be any dtype.
    // This field is mutated if THPVariable_set_grad_dtype is called
    // and the AccumulateGrad has already been created.
    std::optional<quarisma::ScalarType> grad_dtype_;
};
}  // namespace torch::autograd
