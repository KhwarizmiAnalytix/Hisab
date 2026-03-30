#include <quarisma/util/Logging.h>
#include <torch/nativert/graph/TensorMeta.h>

namespace torch::nativert
{

quarisma::ScalarType convertJsonScalarType(const torch::_export::ScalarType& scalarType)
{
    switch (scalarType)
    {
    case torch::_export::ScalarType::UNKNOWN:
        QUARISMA_CHECK(false, "scalar type is not properly set");
    case torch::_export::ScalarType::BYTE:
        return quarisma::ScalarType::Byte;
    case torch::_export::ScalarType::CHAR:
        return quarisma::ScalarType::Char;
    case torch::_export::ScalarType::SHORT:
        return quarisma::ScalarType::Short;
    case torch::_export::ScalarType::INT:
        return quarisma::ScalarType::Int;
    case torch::_export::ScalarType::LONG:
        return quarisma::ScalarType::Long;
    case torch::_export::ScalarType::HALF:
        return quarisma::ScalarType::Half;
    case torch::_export::ScalarType::FLOAT:
        return quarisma::ScalarType::Float;
    case torch::_export::ScalarType::DOUBLE:
        return quarisma::ScalarType::Double;
    case torch::_export::ScalarType::COMPLEXHALF:
        return quarisma::ScalarType::ComplexHalf;
    case torch::_export::ScalarType::COMPLEXFLOAT:
        return quarisma::ScalarType::ComplexFloat;
    case torch::_export::ScalarType::COMPLEXDOUBLE:
        return quarisma::ScalarType::ComplexDouble;
    case torch::_export::ScalarType::BOOL:
        return quarisma::ScalarType::Bool;
    case torch::_export::ScalarType::BFLOAT16:
        return quarisma::ScalarType::BFloat16;
    case torch::_export::ScalarType::UINT16:
        return quarisma::ScalarType::UInt16;
    case torch::_export::ScalarType::FLOAT8E4M3FN:
        return quarisma::ScalarType::Float8_e4m3fn;
    case torch::_export::ScalarType::FLOAT8E5M2:
        return quarisma::ScalarType::Float8_e5m2;
    case torch::_export::ScalarType::FLOAT8E4M3FNUZ:
        return quarisma::ScalarType::Float8_e4m3fnuz;
    case torch::_export::ScalarType::FLOAT8E5M2FNUZ:
        return quarisma::ScalarType::Float8_e5m2fnuz;
    default:
        QUARISMA_CHECK(false, "unknown scalar type", static_cast<int>(scalarType));
    }
}

quarisma::MemoryFormat convertJsonMemoryFormat(const torch::_export::MemoryFormat& memoryFormat)
{
    switch (memoryFormat)
    {
    case torch::_export::MemoryFormat::Unknown:
        QUARISMA_CHECK(false, "got unknown scalar type");
    case torch::_export::MemoryFormat::ContiguousFormat:
        return quarisma::MemoryFormat::Contiguous;
    case torch::_export::MemoryFormat::ChannelsLast:
        return quarisma::MemoryFormat::ChannelsLast;
    case torch::_export::MemoryFormat::ChannelsLast3d:
        return quarisma::MemoryFormat::ChannelsLast3d;
    case torch::_export::MemoryFormat::PreserveFormat:
        return quarisma::MemoryFormat::Preserve;
    default:
        QUARISMA_CHECK(false, "unknown memory format", static_cast<int>(memoryFormat));
    }
}

quarisma::Layout convertJsonLayout(const torch::_export::Layout& layout)
{
    switch (layout)
    {
    case torch::_export::Layout::Unknown:
        QUARISMA_CHECK(false, "got unknown layout");
    case torch::_export::Layout::SparseCoo:
        // TODO is this the right translation
        return quarisma::Layout::Sparse;
    case torch::_export::Layout::SparseCsr:
        return quarisma::Layout::SparseCsr;
    case torch::_export::Layout::SparseCsc:
        return quarisma::Layout::SparseCsc;
    case torch::_export::Layout::SparseBsr:
        return quarisma::Layout::SparseBsr;
    case torch::_export::Layout::SparseBsc:
        return quarisma::Layout::SparseBsc;
    case torch::_export::Layout::_mkldnn:
        return quarisma::Layout::Mkldnn;
    case torch::_export::Layout::Strided:
        return quarisma::Layout::Strided;
    default:
        QUARISMA_CHECK(false, "unknown layout", static_cast<int>(layout));
    }
}

quarisma::Device convertJsonDevice(const torch::_export::Device& device)
{
    quarisma::Device d(device.get_type());
    if (auto index = device.get_index())
    {
        d.set_index(static_cast<quarisma::DeviceIndex>(*index));
    }
    return d;
}

TensorMeta::TensorMeta(const torch::_export::TensorMeta& tensorMeta)
    : dtype_(convertJsonScalarType(tensorMeta.get_dtype())),
      layout_(convertJsonLayout(tensorMeta.get_layout())),
      requiresGrad_(tensorMeta.get_requires_grad()),
      device_(convertJsonDevice(tensorMeta.get_device()))
{
    const auto& storageOffset = tensorMeta.get_storage_offset();
    if (storageOffset.tag() == torch::_export::SymInt::Tag::AS_INT)
    {
        storage_offset_ = tensorMeta.get_storage_offset().get_as_int();
    }
    else if (storageOffset.tag() == torch::_export::SymInt::Tag::AS_EXPR)
    {
        // TODO: it's still unclear how SymInt shape should be used in runtime
        // setting the storage offset to 0 for now
        hasSymbolicShape_ = true;
        storage_offset_   = 0;
    }

    for (const auto& size : tensorMeta.get_sizes())
    {
        if (size.tag() == torch::_export::SymInt::Tag::AS_INT)
        {
            int64_t val = size.get_as_int();
            sizes_.emplace_back(val);
            numel_ *= val;
        }
        else if (size.tag() == torch::_export::SymInt::Tag::AS_EXPR)
        {
            // TODO: it's still unclear how SymInt shape should be used in runtime
            // One potential use cases is for verifying inputs shape matches constrain
            // This would require unpacking the serialized constrain, which is NYI
            //
            // For the time being, we just set the symbolic dim to -1
            hasSymbolicShape_ = true;
            sizes_.emplace_back(-1);
            numel_ = -1;
        }
    }

    for (const auto& stride : tensorMeta.get_strides())
    {
        if (stride.tag() == torch::_export::SymInt::Tag::AS_INT)
        {
            strides_.emplace_back(stride.get_as_int());
        }
        else if (stride.tag() == torch::_export::SymInt::Tag::AS_EXPR)
        {
            // TODO: it's still unclear how SymInt shape should be used in runtime
            // Setting symbolic shape to -1 for now
            hasSymbolicShape_ = true;
            strides_.emplace_back(-1);
        }
    }
}

}  // namespace torch::nativert
