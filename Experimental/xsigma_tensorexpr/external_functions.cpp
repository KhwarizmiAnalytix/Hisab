#include <Quarisma/Functions.h>
#include <Quarisma/NativeFunctions.h>
#include <Quarisma/Parallel.h>
#include <Quarisma/Quarisma.h>
#include <Quarisma/core/Tensor.h>
#include <Quarisma/native/mkldnn/OpContext.h>
#include <Quarisma/native/quantized/PackedParams.h>
#include <Quarisma/native/quantized/cpu/BinaryOps.h>
#include <Quarisma/native/quantized/cpu/QuantUtils.h>
#include <Quarisma/native/quantized/cpu/QuantizedOps.h>
#include <Quarisma/native/quantized/cpu/conv_serialization.h>
#include <Quarisma/native/xnnpack/OpContext.h>
#include <Quarisma/quantized/QTensorImpl.h>
#include <quarisma/core/TensorImpl.h>
#include <quarisma/core/TensorOptions.h>
#include <quarisma/util/ArrayRef.h>
#include <quarisma/util/irange.h>
#include <torch/csrc/jit/serialization/import_source.h>
#include <torch/csrc/jit/serialization/pickle.h>
#include <torch/csrc/jit/tensorexpr/exceptions.h>
#include <torch/csrc/jit/tensorexpr/external_functions.h>
#include <torch/csrc/jit/tensorexpr/external_functions_registry.h>

#include <utility>

namespace torch::jit::tensorexpr
{

static quarisma::MemoryFormat deduce_memory_format(
    quarisma::IntArrayRef strides, quarisma::IntArrayRef dims)
{
    if (strides.size() == 4 && strides[3] == dims[1] && strides[1] == 1l)
    {
        return quarisma::MemoryFormat::ChannelsLast;
    }
    return quarisma::MemoryFormat::Contiguous;
}

static quarisma::MemoryFormat deduce_memory_format(
    const std::vector<int64_t>& strides, const std::vector<int64_t>& dims)
{
    return deduce_memory_format(quarisma::IntArrayRef(strides), quarisma::IntArrayRef(dims));
}

static quarisma::Tensor from_blob_quantized(
    void*                 data,
    quarisma::IntArrayRef sizes,
    quarisma::IntArrayRef strides,
    double                qscale,
    int64_t               qzero,
    quarisma::ScalarType  dtype)
{
    auto memory_format = deduce_memory_format(strides, sizes);
    auto qx            = quarisma::_empty_affine_quantized(
        sizes, dtype, quarisma::kStrided, quarisma::kCPU, false, qscale, qzero, memory_format);
    auto        qtensor_impl = static_cast<quarisma::QTensorImpl*>(qx.unsafeGetTensorImpl());
    auto        typeMeta     = quarisma::scalarTypeToTypeMeta(dtype);
    std::size_t size         = 1;
    for (std::int64_t s : sizes)
    {
        size *= static_cast<std::size_t>(s);
    }
    qtensor_impl->ShareExternalPointer(
        quarisma::InefficientStdFunctionContext::makeDataPtr(
            data, [](void*) {}, quarisma::kCPU),
        typeMeta,
        size * typeMeta.itemsize());
    qtensor_impl->set_sizes_and_strides(sizes, strides);
    return qx;
}

std::vector<quarisma::Tensor> constructTensors(
    int64_t                                               bufs_num,
    void**                                                buf_data,
    int64_t*                                              buf_ranks,
    int64_t*                                              buf_dims,
    int64_t*                                              buf_strides,
    int8_t*                                               buf_dtypes,
    std::optional<std::vector<std::pair<size_t, QIData>>> qdataArg)
{
    std::vector<void*>                buf_data_vec;
    std::vector<std::vector<int64_t>> buf_dims_vec;
    std::vector<std::vector<int64_t>> buf_strides_vec;
    std::vector<quarisma::ScalarType> buf_dtypes_vec;
    int64_t                           buf_dims_idx    = 0;
    int64_t                           buf_strides_idx = 0;
    for (const auto i : quarisma::irange(bufs_num))
    {
        buf_data_vec.push_back(buf_data[i]);
        buf_dims_vec.emplace_back();
        buf_strides_vec.emplace_back();
        for (const auto dim : quarisma::irange(buf_ranks[i]))
        {
            (void)dim;
            buf_dims_vec[i].push_back(buf_dims[buf_dims_idx++]);
            buf_strides_vec[i].push_back(buf_strides[buf_strides_idx++]);
        }
        buf_dtypes_vec.push_back(static_cast<quarisma::ScalarType>(buf_dtypes[i]));
    }

    std::vector<quarisma::Tensor> tensors;
    if (!qdataArg.has_value())
    {
        for (const auto i : quarisma::irange(buf_data_vec.size()))
        {
            auto options =
                quarisma::TensorOptions()
                    .dtype(buf_dtypes_vec[i])
                    .layout(quarisma::kStrided)
                    .device(quarisma::kCPU)  // TODO: support GPUs too
                    .memory_format(deduce_memory_format(buf_strides_vec[i], buf_dims_vec[i]))
                    .requires_grad(false);
            auto tensor =
                quarisma::from_blob(buf_data_vec[i], buf_dims_vec[i], buf_strides_vec[i], options);
            tensors.emplace_back(std::move(tensor));
        }
    }
    else
    {
        // handle quantized
        std::vector<std::optional<QIData>> qdata(bufs_num, std::nullopt);
        for (const auto& qd : *qdataArg)
        {
            qdata[qd.first] = qd.second;
        }
        for (const auto i : quarisma::irange(buf_data_vec.size()))
        {
            auto options =
                quarisma::TensorOptions()
                    .dtype(buf_dtypes_vec[i])
                    .layout(quarisma::kStrided)
                    .device(quarisma::kCPU)  // TODO: support GPUs too
                    .memory_format(deduce_memory_format(buf_strides_vec[i], buf_dims_vec[i]))
                    .requires_grad(false);
            if (auto qd = qdata[i])
            {
                // inplace tensor
                auto tensor = from_blob_quantized(
                    buf_data_vec[i],
                    buf_dims_vec[i],
                    buf_strides_vec[i],
                    qd->scale,
                    qd->zero,
                    qd->scalarType);
                tensors.emplace_back(std::move(tensor));
            }
            else
            {
                auto tensor = quarisma::from_blob(
                    buf_data_vec[i], buf_dims_vec[i], buf_strides_vec[i], options);
                tensors.emplace_back(std::move(tensor));
            }
        }
    }
    return tensors;
}

static std::vector<quarisma::Tensor> constructTensors(
    int64_t                                bufs_num,
    void**                                 buf_data,
    int64_t*                               buf_ranks,
    int64_t*                               buf_dims,
    int64_t*                               buf_strides,
    int8_t*                                buf_dtypes,
    std::vector<std::pair<size_t, QIData>> qdata)
{
    std::optional<std::vector<std::pair<size_t, QIData>>> opt = std::move(qdata);
    return constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes, opt);
}

std::vector<quarisma::Tensor> constructTensors2(
    int64_t                                               bufs_in_num,
    void**                                                buf_data,
    int64_t*                                              buf_ranks,
    int64_t*                                              buf_dims,
    int64_t*                                              buf_strides,
    int8_t*                                               buf_dtypes,
    std::optional<std::vector<std::pair<size_t, QIData>>> qdataArg,
    size_t                                                bufs_out_num)
{
    std::vector<void*>                buf_data_vec;
    std::vector<std::vector<int64_t>> buf_dims_vec;
    std::vector<std::vector<int64_t>> buf_strides_vec;
    std::vector<quarisma::ScalarType> buf_dtypes_vec;
    int64_t                           buf_dims_idx    = 0;
    int64_t                           buf_strides_idx = 0;
    for (const auto i : quarisma::irange(bufs_in_num))
    {
        buf_data_vec.push_back(buf_data[bufs_out_num + i]);
        buf_dims_vec.emplace_back();
        buf_strides_vec.emplace_back();
        for (const auto dim : quarisma::irange(buf_ranks[i]))
        {
            (void)dim;
            buf_dims_vec[i].push_back(buf_dims[buf_dims_idx++]);
            buf_strides_vec[i].push_back(buf_strides[buf_strides_idx++]);
        }
        buf_dtypes_vec.push_back(static_cast<quarisma::ScalarType>(buf_dtypes[i]));
    }

    std::vector<quarisma::Tensor> tensors;
    quarisma::Tensor              und;
    for (const auto i : quarisma::irange(bufs_out_num))
    {
        (void)i;
        tensors.emplace_back(und);
    }
    if (!qdataArg.has_value())
    {
        for (const auto i : quarisma::irange(buf_data_vec.size()))
        {
            auto options =
                quarisma::TensorOptions()
                    .dtype(buf_dtypes_vec[i])
                    .layout(quarisma::kStrided)
                    .device(quarisma::kCPU)  // TODO: support GPUs too
                    .memory_format(deduce_memory_format(buf_strides_vec[i], buf_dims_vec[i]))
                    .requires_grad(false);
            auto tensor =
                quarisma::from_blob(buf_data_vec[i], buf_dims_vec[i], buf_strides_vec[i], options);
            tensors.emplace_back(std::move(tensor));
        }
    }
    else
    {
        // handle quantized
        std::vector<std::optional<QIData>> qdata(bufs_in_num, std::nullopt);
        for (const auto& qd : *qdataArg)
        {
            qdata[qd.first - bufs_out_num] = qd.second;
        }
        for (const auto i : quarisma::irange(buf_data_vec.size()))
        {
            auto options =
                quarisma::TensorOptions()
                    .dtype(buf_dtypes_vec[i])
                    .layout(quarisma::kStrided)
                    .device(quarisma::kCPU)  // TODO: support GPUs too
                    .memory_format(deduce_memory_format(buf_strides_vec[i], buf_dims_vec[i]))
                    .requires_grad(false);
            if (auto qd = qdata[i])
            {
                // inplace tensor
                auto tensor = from_blob_quantized(
                    buf_data_vec[i],
                    buf_dims_vec[i],
                    buf_strides_vec[i],
                    qd->scale,
                    qd->zero,
                    qd->scalarType);
                tensors.emplace_back(std::move(tensor));
            }
            else
            {
                auto tensor = quarisma::from_blob(
                    buf_data_vec[i], buf_dims_vec[i], buf_strides_vec[i], options);
                tensors.emplace_back(std::move(tensor));
            }
        }
    }
    return tensors;
}

static std::vector<quarisma::Tensor> constructTensors2(
    int64_t                                bufs_in_num,
    void**                                 buf_data,
    int64_t*                               buf_ranks,
    int64_t*                               buf_dims,
    int64_t*                               buf_strides,
    int8_t*                                buf_dtypes,
    std::vector<std::pair<size_t, QIData>> qdata,
    size_t                                 bufs_out_num = 0u)
{
    std::optional<std::vector<std::pair<size_t, QIData>>> opt = std::move(qdata);
    return constructTensors2(
        bufs_in_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes, opt, bufs_out_num);
}

#ifndef _WIN32
static quarisma::Tensor quantized_add(
    const quarisma::Tensor& x1, const quarisma::Tensor& x2, double scale, int64_t zero)
{
    const auto qadd_op =
        quarisma::Dispatcher::singleton()
            .findSchemaOrThrow("quantized::add", "")
            .typed<quarisma::Tensor(quarisma::Tensor, quarisma::Tensor, double, int64_t)>();
    return qadd_op.call(x1, x2, scale, zero);
}

static quarisma::Tensor quantized_mul(
    const quarisma::Tensor& x1, const quarisma::Tensor& x2, double scale, int64_t zero)
{
    const auto op =
        quarisma::Dispatcher::singleton()
            .findSchemaOrThrow("quantized::mul", "")
            .typed<quarisma::Tensor(quarisma::Tensor, quarisma::Tensor, double, int64_t)>();
    return op.call(x1, x2, scale, zero);
}

static quarisma::Tensor quantized_mul_scalar(const quarisma::Tensor& x, double scalar)
{
    const auto op = quarisma::Dispatcher::singleton()
                        .findSchemaOrThrow("quantized::mul", "Scalar")
                        .typed<quarisma::Tensor(quarisma::Tensor, quarisma::Scalar const&)>();
    auto s = quarisma::Scalar(scalar);
    return op.call(x, s);
}

static quarisma::Tensor quantized_cat(
    const quarisma::List<quarisma::Tensor>& qxs,
    int64_t                                 dim,
    std::optional<double>                   scale,
    std::optional<int64_t>                  zero)
{
    const auto op = quarisma::Dispatcher::singleton()
                        .findSchemaOrThrow("quantized::cat", "")
                        .typed<quarisma::Tensor(
                            quarisma::List<quarisma::Tensor> const&,
                            int64_t,
                            std::optional<double>,
                            std::optional<int64_t>)>();
    return op.redispatch(
        quarisma::DispatchKeySet({quarisma::DispatchKey::QuantizedCPU}), qxs, dim, scale, zero);
}

#endif  // _WIN32

#ifdef QUARISMA_MOBILE
extern "C"
{
#endif

    void nnc_aten_conv2d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r = tensors[0];
        const quarisma::Tensor& x = tensors[1];
        const quarisma::Tensor& w = tensors[2];
        if (args_num > 0)
        {
            // Check that if the extra arguments are provided, then the bias tensor is
            // also present
            TORCH_INTERNAL_ASSERT(args_num == 7 && bufs_num == 4);
            const quarisma::Tensor& b = tensors[3];

            int64_t strideH   = extra_args[0];
            int64_t strideW   = extra_args[1];
            int64_t paddingH  = extra_args[2];
            int64_t paddingW  = extra_args[3];
            int64_t dilationH = extra_args[4];
            int64_t dilationW = extra_args[5];
            int64_t groups    = extra_args[6];

            try
            {
                r = quarisma::conv2d(
                    x,
                    w,
                    b,
                    {strideH, strideW},
                    {paddingH, paddingW},
                    {dilationH, dilationW},
                    groups);
            }
            catch (...)
            {
            }
        }
        else
        {
            try
            {
                r = quarisma::conv2d(x, w);
            }
            catch (...)
            {
            }
        }

        // TODO: can i haz an out version of the conv2d?
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_conv1d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          qx               = tensors[1].unsqueeze(quant_utils::kConv1dSqueezeDim + 2);
        auto          r                = convPackedParams->apply(qx, out_qscale, out_qzero);
        r                              = r.squeeze_(quant_utils::kConv1dSqueezeDim + 2);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_conv1d_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          qx               = tensors[1].unsqueeze(quant_utils::kConv1dSqueezeDim + 2);
        auto          r                = convPackedParams->apply(qx, out_qscale, out_qzero);
        r                              = r.squeeze_(quant_utils::kConv1dSqueezeDim + 2);
        buf_data[0]                    = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_conv2d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          r                = convPackedParams->apply(tensors[1], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_conv2d_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          r                = convPackedParams->apply(tensors[1], out_qscale, out_qzero);
        buf_data[0]                    = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_conv2d_relu(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          r = convPackedParams->apply_relu(tensors[1], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_conv2d_relu_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);
        auto          convPackedParams = reinterpret_cast<ConvPackedParamsBase<2>*>(buf_data[2]);
        const double  out_qscale       = ((double*)extra_args)[3];
        const int64_t out_qzero        = extra_args[4];
        auto          r = convPackedParams->apply_relu(tensors[1], out_qscale, out_qzero);
        buf_data[0]     = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_linear(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto          linearPackedParams = reinterpret_cast<LinearPackedParamsBase*>(buf_data[2]);
        const double  out_qscale         = ((double*)extra_args)[3];
        const int64_t out_qzero          = extra_args[4];
        auto          r = linearPackedParams->apply(tensors[1], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_linear_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);
        auto          linearPackedParams = reinterpret_cast<LinearPackedParamsBase*>(buf_data[2]);
        const double  out_qscale         = ((double*)extra_args)[3];
        const int64_t out_qzero          = extra_args[4];
        auto          r = linearPackedParams->apply(tensors[1], out_qscale, out_qzero);
        buf_data[0]     = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_linear_relu(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto          linearPackedParams = reinterpret_cast<LinearPackedParamsBase*>(buf_data[2]);
        const double  out_qscale         = ((double*)extra_args)[3];
        const int64_t out_qzero          = extra_args[4];
        auto          r = linearPackedParams->apply_relu(tensors[1], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

#ifndef _WIN32
    void nnc_aten_quantized_add(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        // TORCH_INTERNAL_ASSERT(tensors.size() == 3);

        const double               a_qscale = ((double*)extra_args)[0];
        const int64_t              a_qzero  = extra_args[1];
        const quarisma::ScalarType a_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        const double               b_qscale = ((double*)extra_args)[3];
        const int64_t              b_qzero  = extra_args[4];
        const quarisma::ScalarType b_qdtype = static_cast<quarisma::ScalarType>(extra_args[5]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {a_qscale, a_qzero, toQIntType(a_qdtype)}},
                                    {2u, {b_qscale, b_qzero, toQIntType(b_qdtype)}}});

        const double  out_qscale = ((double*)extra_args)[6];
        const int64_t out_qzero  = extra_args[7];
        auto          r          = quantized_add(tensors[1], tensors[2], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_mul(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               a_qscale = ((double*)extra_args)[0];
        const int64_t              a_qzero  = extra_args[1];
        const quarisma::ScalarType a_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        const double               b_qscale = ((double*)extra_args)[3];
        const int64_t              b_qzero  = extra_args[4];
        const quarisma::ScalarType b_qdtype = static_cast<quarisma::ScalarType>(extra_args[5]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {a_qscale, a_qzero, toQIntType(a_qdtype)}},
                                    {2u, {b_qscale, b_qzero, toQIntType(b_qdtype)}}});
        const double  out_qscale = ((double*)extra_args)[6];
        const int64_t out_qzero  = extra_args[7];
        auto          r          = quantized_mul(tensors[1], tensors[2], out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_mul_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               a_qscale     = ((double*)extra_args)[0];
        const int64_t              a_qzero      = extra_args[1];
        const quarisma::ScalarType a_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        const double               b_qscale     = ((double*)extra_args)[3];
        const int64_t              b_qzero      = extra_args[4];
        const quarisma::ScalarType b_qdtype     = static_cast<quarisma::ScalarType>(extra_args[5]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {a_qscale, a_qzero, toQIntType(a_qdtype)}},
                                        {2u, {b_qscale, b_qzero, toQIntType(b_qdtype)}}},
            1u);
        const double  out_qscale = ((double*)extra_args)[6];
        const int64_t out_qzero  = extra_args[7];
        auto          r          = quantized_mul(tensors[1], tensors[2], out_qscale, out_qzero);
        buf_data[0]              = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_mul_scalar(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        const double scalar = ((double*)extra_args)[3];
        auto         r      = quantized_mul_scalar(tensors[1], scalar);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_mul_scalar_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t               bufs_out_num = 1u;
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);
        const double scalar = ((double*)extra_args)[3];
        auto         r      = quantized_mul_scalar(tensors[1], scalar);
        buf_data[0]         = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_relu(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});
        auto r = quarisma::relu(tensors[1]);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_sigmoid(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale = ((double*)extra_args)[0];
        const int64_t              x_qzero  = extra_args[1];
        const quarisma::ScalarType x_qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       tensors  = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}});

        auto r = quarisma::sigmoid(tensors[1]);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantized_sigmoid_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double               x_qscale     = ((double*)extra_args)[0];
        const int64_t              x_qzero      = extra_args[1];
        const quarisma::ScalarType x_qdtype     = static_cast<quarisma::ScalarType>(extra_args[2]);
        const size_t               bufs_out_num = 1u;
        auto                       tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {x_qscale, x_qzero, toQIntType(x_qdtype)}}},
            bufs_out_num);

        auto r      = quarisma::sigmoid(tensors[1]);
        buf_data[0] = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantized_cat(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        std::vector<std::pair<size_t, QIData>> qdata;
        const auto                             in_bufs_num = bufs_num - 1;
        const double  out_qscale = ((double*)extra_args)[3 * in_bufs_num + 1];
        const int64_t out_qzero  = extra_args[3 * in_bufs_num + 2];
        qdata.emplace_back(
            0u, QIData{out_qscale, out_qzero, static_cast<quarisma::ScalarType>(extra_args[2])});
        for (const size_t i : quarisma::irange(in_bufs_num))
        {
            const double               qscale = ((double*)extra_args)[3 * i + 0];
            const int64_t              qzero  = extra_args[3 * i + 1];
            const quarisma::ScalarType qdtype =
                static_cast<quarisma::ScalarType>(extra_args[3 * i + 2]);
            qdata.emplace_back(i + 1u, QIData{qscale, qzero, qdtype});
        }
        auto tensors = constructTensors(
            bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes, qdata);
        const int64_t dim = extra_args[3 * in_bufs_num + 0];
        auto          qxs = quarisma::List<quarisma::Tensor>(
            std::vector<quarisma::Tensor>(tensors.begin() + 1, tensors.end()));
        auto r = quantized_cat(qxs, dim, out_qscale, out_qzero);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }
#endif  // _WIN32

    void nnc_aten_upsample_nearest2d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        // NOLINTNEXTLINE(facebook-hte-LocalUncheckedArrayBounds)
        const double                                          x_qscale = ((double*)extra_args)[0];
        const int64_t                                         x_qzero  = extra_args[1];
        const int64_t                                         x_qdtype = extra_args[2];
        const auto                                            is_quantized = x_qdtype != -1;
        std::optional<std::vector<std::pair<size_t, QIData>>> qdata;
        if (is_quantized)
        {
            qdata = {
                {1u,
                 {x_qscale,
                  x_qzero,
                  quarisma::toQIntType(static_cast<quarisma::ScalarType>(x_qdtype))}}};
        }
        auto tensors = constructTensors(
            bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes, qdata);
        const auto& x = tensors[1];

        int64_t output_size_h  = extra_args[3];
        int64_t output_size_w  = extra_args[4];
        double  scale_factor_h = ((double*)extra_args)[5];
        double  scale_factor_w = ((double*)extra_args)[6];

        auto r = quarisma::upsample_nearest2d(
            x,
            (output_size_h != -1)
                ? std::optional<quarisma::IntArrayRef>({output_size_h, output_size_w})
                : std::nullopt,
            (scale_factor_h != -1.f)
                ? std::optional<quarisma::ArrayRef<double>>({scale_factor_h, scale_factor_w})
                : std::nullopt);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_upsample_nearest2d_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t bufs_out_num = 1u;
        // NOLINTNEXTLINE(facebook-hte-LocalUncheckedArrayBounds)
        const double                                          x_qscale = ((double*)extra_args)[0];
        const int64_t                                         x_qzero  = extra_args[1];
        const int64_t                                         x_qdtype = extra_args[2];
        const auto                                            is_quantized = x_qdtype != -1;
        std::optional<std::vector<std::pair<size_t, QIData>>> qdata;
        if (is_quantized)
        {
            qdata = {
                {1u,
                 {x_qscale,
                  x_qzero,
                  quarisma::toQIntType(static_cast<quarisma::ScalarType>(x_qdtype))}}};
        }
        auto tensors = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            qdata,
            bufs_out_num);
        auto x = tensors[1];

        int64_t output_size_h  = extra_args[3];
        int64_t output_size_w  = extra_args[4];
        double  scale_factor_h = ((double*)extra_args)[5];
        double  scale_factor_w = ((double*)extra_args)[6];

        auto r = quarisma::upsample_nearest2d(
            x,
            (output_size_h != -1)
                ? std::optional<quarisma::IntArrayRef>({output_size_h, output_size_w})
                : std::nullopt,
            (scale_factor_h != -1.f)
                ? std::optional<quarisma::ArrayRef<double>>({scale_factor_h, scale_factor_w})
                : std::nullopt);
        buf_data[0] = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_quantize_per_tensor(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);
        // NOLINTNEXTLINE(facebook-hte-LocalUncheckedArrayBounds)
        quarisma::Tensor           x      = tensors[1];
        const double               qscale = ((double*)extra_args)[0];
        const int64_t              qzero  = extra_args[1];
        const quarisma::ScalarType qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       r      = quarisma::quantize_per_tensor(x, qscale, qzero, qdtype);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_quantize_per_tensor_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t bufs_out_num = 1u;
        auto         tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            std::nullopt,
            bufs_out_num);
        // NOLINTNEXTLINE(facebook-hte-LocalUncheckedArrayBounds)
        const quarisma::Tensor&    x      = tensors[1];
        const double               qscale = ((double*)extra_args)[0];
        const int64_t              qzero  = extra_args[1];
        const quarisma::ScalarType qdtype = static_cast<quarisma::ScalarType>(extra_args[2]);
        auto                       r      = quarisma::quantize_per_tensor(x, qscale, qzero, qdtype);
        buf_data[0]                       = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_dequantize(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const double  qscale  = ((double*)extra_args)[0];
        const int64_t qzero   = extra_args[1];
        const int64_t qdtype  = extra_args[2];
        auto          tensors = constructTensors(
            bufs_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {qscale, qzero, toQIntType(static_cast<quarisma::ScalarType>(qdtype))}}});
        auto r = quarisma::dequantize(tensors[1]);
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_dequantize_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        const size_t  bufs_out_num = 1u;
        const double  qscale       = ((double*)extra_args)[0];
        const int64_t qzero        = extra_args[1];
        const int64_t qdtype       = extra_args[2];
        auto          tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            {{1u, {qscale, qzero, toQIntType(static_cast<quarisma::ScalarType>(qdtype))}}},
            bufs_out_num);
        auto r      = quarisma::dequantize(tensors[1]);
        buf_data[0] = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_conv1d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r = tensors[0];
        const quarisma::Tensor& x = tensors[1];
        const quarisma::Tensor& w = tensors[2];
        if (args_num > 0)
        {
            // Check that if the extra arguments are provided, then the bias tensor is
            // also present
            TORCH_INTERNAL_ASSERT(args_num == 4 && bufs_num == 4);
            const quarisma::Tensor& b = tensors[3];

            int64_t stride   = extra_args[0];
            int64_t padding  = extra_args[1];
            int64_t dilation = extra_args[2];
            int64_t groups   = extra_args[3];

            try
            {
                r = quarisma::conv1d(x, w, b, {stride}, {padding}, {dilation}, groups);
            }
            catch (...)
            {
            }
        }
        else
        {
            try
            {
                r = quarisma::conv1d(x, w);
            }
            catch (...)
            {
            }
        }

        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_conv1d_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        const size_t bufs_out_num = 1u;
        auto         tensors      = constructTensors2(
            bufs_in_num,
            buf_data,
            buf_ranks,
            buf_dims,
            buf_strides,
            buf_dtypes,
            std::nullopt,
            bufs_out_num);

        quarisma::Tensor        r;
        const quarisma::Tensor& x = tensors[1];
        const quarisma::Tensor& w = tensors[2];
        if (args_num > 0)
        {
            // Check that if the extra arguments are provided, then the bias tensor is
            // also present
            TORCH_INTERNAL_ASSERT(args_num == 4 && bufs_in_num == 3);
            const quarisma::Tensor& b = tensors[3];

            int64_t stride   = extra_args[0];
            int64_t padding  = extra_args[1];
            int64_t dilation = extra_args[2];
            int64_t groups   = extra_args[3];

            try
            {
                r = quarisma::conv1d(x, w, b, {stride}, {padding}, {dilation}, groups);
            }
            catch (...)
            {
            }
        }
        else
        {
            try
            {
                r = quarisma::conv1d(x, w);
            }
            catch (...)
            {
            }
        }

        buf_data[0] = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_adaptive_avg_pool2d(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r = tensors[0];
        const quarisma::Tensor& x = tensors[1];
        int64_t                 H = extra_args[0];
        int64_t                 W = H;
        if (args_num > 1)
        {
            W = extra_args[1];
        }
        try
        {
            r = quarisma::adaptive_avg_pool2d(x, {H, W});
        }
        catch (...)
        {
        }
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_mean(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r = tensors[0];
        const quarisma::Tensor& x = tensors[1];
        std::vector<int64_t>    mean_dims(args_num - 1);
        bool                    keepdim = (bool)extra_args[args_num - 1];
        if (args_num > 1)
        {
            memcpy(mean_dims.data(), extra_args, sizeof(int64_t) * (args_num - 1));
        }
        try
        {
            quarisma::mean_out(r, x, mean_dims, keepdim);
        }
        catch (...)
        {
        }
    }

    void nnc_aten_max_red(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r        = tensors[0];
        const quarisma::Tensor& x        = tensors[1];
        int64_t                 max_dim  = extra_args[0];
        bool                    keep_dim = extra_args[1];
        try
        {
            r = std::get<0>(quarisma::max(x, max_dim, keep_dim));
        }
        catch (...)
        {
        }
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

    void nnc_aten_max_red_out(
        int64_t  bufs_in_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t /*unused*/,
        int64_t* extra_args)
    {
        size_t bufs_out_num = 1u;
        auto   tensors =
            constructTensors2(bufs_in_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor r;
        // @lint-ignore CLANGTIDY
        const quarisma::Tensor& x        = tensors[1];
        int64_t                 max_dim  = extra_args[0];
        bool                    keep_dim = extra_args[1];
        try
        {
            r = std::get<0>(quarisma::max(x, max_dim, keep_dim));
        }
        catch (...)
        {
        }
        buf_data[0] = r.data_ptr();
        quarisma::raw::intrusive_ptr::incref(r.getIntrusivePtr().get());
        buf_data[bufs_in_num + bufs_out_num] = r.getIntrusivePtr().get();
    }

    void nnc_aten_addmm(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r = tensors[0];
        const quarisma::Tensor& x = tensors[1];
        const quarisma::Tensor& y = tensors[2];
        const quarisma::Tensor& z = tensors[3];
        // TODO: handle other alpha and beta dtypes, e.g. alpha=0.6, beta=0.2
        int64_t beta = extra_args[0], alpha = extra_args[1];

        try
        {
            quarisma::addmm_out(r, x, y, z, beta, alpha);
        }
        catch (...)
        {
        }
    }

    // Only provides first output, the second output is just a copy of one of the
    // inputs
    void nnc_aten_triangular_solve(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);
        quarisma::Tensor&       r     = tensors[0];
        quarisma::Tensor        r2    = tensors[2].clone();
        const quarisma::Tensor& input = tensors[1];
        const quarisma::Tensor& A     = tensors[2];
        try
        {
            quarisma::triangular_solve_out(
                r, r2, input, A, extra_args[0], extra_args[2], extra_args[3]);
        }
        catch (...)
        {
        }
    }

#if AT_MKLDNN_ENABLED()

    void nnc_mkldnn_prepacked_conv_run(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        using namespace quarisma::native::mkldnn;

        auto tensors =
            constructTensors(bufs_num - 1, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        const quarisma::Tensor& x       = tensors[1];
        auto                    context = reinterpret_cast<ConvOpContext*>(buf_data[2]);

        context->run(x, buf_data[0]);
    }

#endif  // AT_MKLDNN_ENABLED()

#ifdef USE_XNNPACK

    void nnc_prepacked_linear_clamp_run(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        using namespace quarisma::native::xnnpack;

        auto tensors =
            constructTensors(bufs_num - 1, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        const quarisma::Tensor& x       = tensors[1];
        auto                    context = reinterpret_cast<LinearOpContext*>(buf_data[2]);
        quarisma::Tensor        output  = context->run(x);
        memcpy(buf_data[0], output.const_data_ptr(), output.element_size() * output.numel());
    }

    void nnc_prepacked_conv2d_clamp_run(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        using namespace quarisma::native::xnnpack;

        auto tensors =
            constructTensors(bufs_num - 1, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        const quarisma::Tensor& x       = tensors[1];
        auto                    context = reinterpret_cast<Conv2dOpContext*>(buf_data[2]);
        quarisma::Tensor        output  = context->run(x);
        memcpy(buf_data[0], output.const_data_ptr(), output.element_size() * output.numel());
    }

#endif  // USE_XNNPACK

    void nnc_aten_embedding(
        int64_t  bufs_num,
        void**   buf_data,
        int64_t* buf_ranks,
        int64_t* buf_dims,
        int64_t* buf_strides,
        int8_t*  buf_dtypes,
        int64_t  args_num,
        int64_t* extra_args)
    {
        auto tensors =
            constructTensors(bufs_num, buf_data, buf_ranks, buf_dims, buf_strides, buf_dtypes);

        quarisma::Tensor&       r       = tensors[0];
        const quarisma::Tensor& weight  = tensors[1];
        const quarisma::Tensor& indices = tensors[2];
        try
        {
            r = quarisma::embedding(weight, indices);
        }
        catch (...)
        {
        }
        // TODO: have to copy output because quarisma::embedding doesn't have an out
        // variant and NNC's external calls don't support allocations
        memcpy(buf_data[0], r.const_data_ptr(), r.element_size() * r.numel());
    }

#ifndef QUARISMA_MOBILE

    const static RegisterNNCExternalFunction nnc_conv2d("nnc_aten_conv2d", nnc_aten_conv2d);

    const static RegisterNNCExternalFunction nnc_quantized_conv1d(
        "nnc_aten_quantized_conv1d", nnc_aten_quantized_conv1d);
    const static RegisterNNCExternalFunction nnc_quantized_conv1d_out(
        "nnc_aten_quantized_conv1d_out", nnc_aten_quantized_conv1d_out);
    const static RegisterNNCExternalFunction nnc_quantized_conv2d(
        "nnc_aten_quantized_conv2d", nnc_aten_quantized_conv2d);
    const static RegisterNNCExternalFunction nnc_quantized_conv2d_out(
        "nnc_aten_quantized_conv2d_out", nnc_aten_quantized_conv2d_out);
    const static RegisterNNCExternalFunction nnc_quantized_conv2d_relu(
        "nnc_aten_quantized_conv2d_relu", nnc_aten_quantized_conv2d_relu);
    const static RegisterNNCExternalFunction nnc_quantized_conv2d_relu_out(
        "nnc_aten_quantized_conv2d_relu_out", nnc_aten_quantized_conv2d_relu_out);
    const static RegisterNNCExternalFunction nnc_quantized_linear(
        "nnc_aten_quantized_linear", nnc_aten_quantized_linear);
    const static RegisterNNCExternalFunction nnc_quantized_linear_out(
        "nnc_aten_quantized_linear_out", nnc_aten_quantized_linear_out);
#ifndef _WIN32
    const static RegisterNNCExternalFunction nnc_quantized_add(
        "nnc_aten_quantized_add", nnc_aten_quantized_add);
    const static RegisterNNCExternalFunction nnc_quantized_mul(
        "nnc_aten_quantized_mul", nnc_aten_quantized_mul);
    const static RegisterNNCExternalFunction nnc_quantized_mul_out(
        "nnc_aten_quantized_mul_out", nnc_aten_quantized_mul_out);
    const static RegisterNNCExternalFunction nnc_quantized_mul_scalar(
        "nnc_aten_quantized_mul_scalar", nnc_aten_quantized_mul_scalar);
    const static RegisterNNCExternalFunction nnc_quantized_mul_scalar_out(
        "nnc_aten_quantized_mul_scalar_out", nnc_aten_quantized_mul_scalar_out);
    const static RegisterNNCExternalFunction nnc_quantized_sigmoid(
        "nnc_aten_quantized_sigmoid", nnc_aten_quantized_sigmoid);
    const static RegisterNNCExternalFunction nnc_quantized_sigmoid_out(
        "nnc_aten_quantized_sigmoid_out", nnc_aten_quantized_sigmoid_out);
    const static RegisterNNCExternalFunction nnc_quantized_cat(
        "nnc_aten_quantized_cat", nnc_aten_quantized_cat);
    const static RegisterNNCExternalFunction nnc_quantized_relu(
        "nnc_aten_quantized_relu", nnc_aten_quantized_relu);
#endif  // _WIN32
    const static RegisterNNCExternalFunction nnc_quantize_per_tensor(
        "nnc_aten_quantize_per_tensor", nnc_aten_quantize_per_tensor);
    const static RegisterNNCExternalFunction nnc_quantize_per_tensor_out(
        "nnc_aten_quantize_per_tensor_out", nnc_aten_quantize_per_tensor_out);
    const static RegisterNNCExternalFunction nnc_dequantize(
        "nnc_aten_dequantize", nnc_aten_dequantize);
    const static RegisterNNCExternalFunction nnc_dequantize_out(
        "nnc_aten_dequantize_out", nnc_aten_dequantize_out);

    const static RegisterNNCExternalFunction nnc_upsample_nearest2d(
        "nnc_aten_upsample_nearest2d", nnc_aten_upsample_nearest2d);
    const static RegisterNNCExternalFunction nnc_upsample_nearest2d_out(
        "nnc_aten_upsample_nearest2d_out", nnc_aten_upsample_nearest2d_out);
    const static RegisterNNCExternalFunction nnc_conv1d("nnc_aten_conv1d", nnc_aten_conv1d);
    const static RegisterNNCExternalFunction nnc_conv1d_out(
        "nnc_aten_conv1d_out", nnc_aten_conv1d_out);
    const static RegisterNNCExternalFunction nnc_adaptive_avg_pool2d(
        "nnc_aten_adaptive_avg_pool2d", nnc_aten_adaptive_avg_pool2d);
    const static RegisterNNCExternalFunction nnc_mean("nnc_aten_mean", nnc_aten_mean);
    const static RegisterNNCExternalFunction nnc_max_red("nnc_aten_max_red", nnc_aten_max_red);
    const static RegisterNNCExternalFunction nnc_max_red_out(
        "nnc_aten_max_red_out", nnc_aten_max_red_out);
    const static RegisterNNCExternalFunction nnc_addmm("nnc_aten_addmm", nnc_aten_addmm);

    const static RegisterNNCExternalFunction nnc_triangular_solve(
        "nnc_aten_triangular_solve", nnc_aten_triangular_solve);

    const static RegisterNNCExternalFunction nnc_embedding(
        "nnc_aten_embedding", nnc_aten_embedding);

#if AT_MKLDNN_ENABLED()
    const static RegisterNNCExternalFunction reg_nnc_mkldnn_prepacked_conv_run(
        "nnc_mkldnn_prepacked_conv_run", nnc_mkldnn_prepacked_conv_run);
#endif  // AT_MKLDNN_ENABLED()

#ifdef USE_XNNPACK
    const static RegisterNNCExternalFunction reg_nnc_prepacked_linear_clamp_run(
        "nnc_prepacked_linear_clamp_run", nnc_prepacked_linear_clamp_run);
    const static RegisterNNCExternalFunction reg_nnc_prepacked_conv2d_clamp_run(
        "nnc_prepacked_conv2d_clamp_run", nnc_prepacked_conv2d_clamp_run);
#endif  // USE_XNNPACK

#endif  // QUARISMA_MOBILE

#ifdef QUARISMA_MOBILE
}  // extern "C"
#endif

}  // namespace torch::jit::tensorexpr
