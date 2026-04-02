#include "bespoke/common/collection.h"

#include <fmt/format.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <queue>
#include <type_traits>
#include <utility>

#if PROFILER_HAS_KINETO
#include <libkineto.h>
#endif

#include "bespoke/common/data_flow.h"
#include "bespoke/common/ivalue.h"
#include "bespoke/common/record_function.h"
#include "bespoke/kineto/kineto_shim.h"
#include "common/flat_hash.h"
#include "common/irange.h"
#include "common/overloaded.h"

namespace quarisma::profiler_impl::impl
{
using result_ptr_t = std::shared_ptr<Result>;
using trace_ptr_t  = std::unique_ptr<quarisma::profiler_impl::impl::kineto::ActivityTraceWrapper>;

RawTensorMetadataBase::RawTensorMetadataBase(const quarisma::Tensor& t)
    : data_{nullptr},
      dtype_{t.scalar_type()},
      layout_{t.layout()},
      size_dim_{static_cast<uint32_t>(t.sizes().size())}
{
    // PROFILER_CHECK_DEBUG(
        // t.sizes().size() <= std::numeric_limits<uint32_t>::max(),
        // "Cannot profile Tensors of size > uint32 max. Got dim: ",
        // t.sizes().size());
    // PROFILER_CHECK_DEBUG(
        // t.sizes().size() == t.strides().size(),
        // "Tensor has mismatching sizes and strides. Sizes: ",
        // t.sizes().size(),
        // " Strides: ",
        // t.strides().size());
}

RawTensorMetadata::RawTensorMetadata(const quarisma::Tensor& t)
    : RawTensorMetadataBase(t),
      weak_self_{WeakTensor(t)},
      device_type_{t.device().type()},
      device_index_{t.device().index()}
{
}

TensorMetadata::TensorMetadata(
    const RawTensorMetadata& r, std::vector<int64_t> sizes, std::vector<int64_t> strides)
    // NOLINTNEXTLINE(cppcoreguidelines-slicing)
    : RawTensorMetadataBase(r),
      weak_self_{r.weak_self_.value_or(WeakTensor(quarisma::Tensor()))},
      device_{r.device_index_, r.device_type_},
      sizes_{std::move(sizes)},
      strides_{std::move(strides)}
{
    SOFT_ASSERT(r.weak_self_.has_value());
}

// ============================================================================
// == Quarisma Ops =============================================================
// ============================================================================

namespace
{
struct TagToIOType
{
    InputOutputEncoder::Tag    tag;
    InputOutputEncoder::IOType io_type;
};

constexpr int tagCount = ((int)InputOutputEncoder::Tag::TERMINATOR) + 1;
constexpr std::array<TagToIOType, tagCount> tag_map = {{
    {InputOutputEncoder::Tag::Tensor, InputOutputEncoder::IOType::Shapes},
    {InputOutputEncoder::Tag::UndefinedTensor, InputOutputEncoder::IOType::Shapes},
    {InputOutputEncoder::Tag::TensorListBegin, InputOutputEncoder::IOType::Shapes},
    {InputOutputEncoder::Tag::ScalarList, InputOutputEncoder::IOType::ConcreteInputs},
    {InputOutputEncoder::Tag::Scalar, InputOutputEncoder::IOType::Shapes},
    {InputOutputEncoder::Tag::Other, InputOutputEncoder::IOType::Shapes},
    {InputOutputEncoder::Tag::TERMINATOR, InputOutputEncoder::IOType::None},
}};

constexpr bool allTagsMapped(int idx = 0)
{
    return tag_map[idx].tag == InputOutputEncoder::Tag::TERMINATOR ||
           ((idx == (int)tag_map[idx].tag) && allTagsMapped(idx + 1));
}
static_assert(allTagsMapped(), "tag_map is out of order");

[[maybe_unused]] constexpr InputOutputEncoder::IOType tagToIOType(InputOutputEncoder::Tag tag)
{
    return tag_map[(int)tag].io_type;
}
}  // namespace

// ----------------------------
// |  Input / Output encoder  |
// ----------------------------
void InputOutputEncoder::push(quarisma::array_ref<const quarisma::IValue> values)
{
    // Disabled: IValue methods (isTensor, toTensor, isScalar, isTensorList, toTensorList) not available in profiler-only build.
    for (const auto& value : values)
    {
#if 0
        if (value.isTensor())
        {
            push(value.toTensor());
        }
        else if (value.isScalar())
        {
            tags_.emplace_back(Tag::Scalar);
            // Scalars are small enough that they are stored in ivalues without an
            // extra memory alloc
            // TODO: further optimize this by maybe giving Profiler access to the
            // guts of IValue.
            ivalues_.emplace_back(value);
        }
        else if (value.isTensorList())
        {
            tags_.emplace_back(Tag::TensorListBegin);
            for (const auto& t : value.toTensorList())
            {
                push(t);
            }
            tags_.emplace_back(Tag::TERMINATOR);
        }
        else if (isSupportedScalarList(value))
        {
            tags_.emplace_back(Tag::ScalarList);
            ivalues_.emplace_back(value);
        }
        else
        {
            tags_.emplace_back(Tag::Other);
        }
#else
        tags_.emplace_back(Tag::Scalar);
        ivalues_.emplace_back(value);
#endif
    }
    tags_.emplace_back(Tag::TERMINATOR);
}

void InputOutputEncoder::push(const quarisma::Tensor& t)
{
    // TODO fix nested and symbolic sizes
    if (t.defined() && !t.is_nested() && !t.unsafeGetTensorImpl()->has_symbolic_sizes_strides())
    {
        tags_.emplace_back(Tag::Tensor);
        tensor_metadata_.emplace_back(t);
        tensor_sizes_strides_.copy(t.sizes());
        if (t.layout() == quarisma::kStrided)
        {
            // Only Strided layout tensors have strides
            tensor_sizes_strides_.copy(t.strides());
        }
    }
    else
    {
        tags_.emplace_back(Tag::UndefinedTensor);
    }
}

#if 0
// Disabled: IValue methods (isList, toListRef, isScalar) not available in profiler-only build.
bool InputOutputEncoder::isSupportedScalarList(const quarisma::IValue& list_candidate)
{
    // Scalar list can be very long. If a list is too long, we shouldn't
    // collect it. This function checks whether the list is a scalar list
    // and whether its length is sufficiently short.

    if (!get_record_concrete_inputs_enabled())
    {
        return false;
    }

    if (!list_candidate.isList())
    {
        return false;
    }
    auto list_ref = list_candidate.toListRef();
    if PROFILER_UNLIKELY(list_ref.empty())
    {
        return true;
    }
    if PROFILER_UNLIKELY(!list_ref[0].isScalar())
    {
        return false;
    }
    if PROFILER_UNLIKELY(list_ref.size() > SCALAR_LIST_LENGTH_LIMIT)
    {
        return false;
    }
    return true;
}
#else
// Stub implementation when IValue methods are not available
bool InputOutputEncoder::isSupportedScalarList(const quarisma::IValue& /*list_candidate*/)
{
    return false;
}
#endif

#if 0
// Disabled: This function uses many IValue methods and quarisma::irange that are not available in profiler-only build.
// This function returns a lambda which is a custom-iterator-like getter.
// Each invocation of the lambda returns input values for one op.
//
// io_type is used to filter the ivalues between 'Shapes' and 'Concrete Args'.
// Shapes are used to represent the shapes of tensors. We save only the shapes
//   of the tensors because tensors can be large.
// Concrete args are separated to clarify that they are the actual values.
auto InputOutputEncoder::getIValueGenerator(const IOType& io_type)
{
    return [this,
            tag_it                 = tags_.begin(),
            tensor_metadata_it     = tensor_metadata_.begin(),
            tensor_size_strides_it = tensor_sizes_strides_.begin(),
            ivals_it               = ivalues_.begin(),
            io_type]() mutable
    {
        auto decode_tensor = [&]() -> TensorMetadata
        {
            std::vector<int64_t> sizes;
            std::vector<int64_t> strides;
            if (tensor_metadata_it.exhausted())
            {
                //LOG(WARNING)
                << "Tensor metadata exhausted prematurely. Reported shapes may be inaccurate!";
                return {RawTensorMetadata(), sizes, strides};
            }
            const auto& raw_metadata = *tensor_metadata_it++;
            for ([[maybe_unused]] const auto _ : quarisma::irange(raw_metadata.size_dim_))
            {
                if (tensor_size_strides_it.exhausted())
                {
                    //LOG(WARNING) << "Expected Tensor Size mismatch with raw Tensor metadata. "
                    "Reported shapes may be inaccurate!";
                    return {raw_metadata, sizes, strides};
                }
                sizes.push_back(*tensor_size_strides_it++);
            }
            if (raw_metadata.layout_ == quarisma::kStrided)
            {
                for ([[maybe_unused]] const auto _ : quarisma::irange(raw_metadata.size_dim_))
                {
                    if (tensor_size_strides_it.exhausted())
                    {
                        //LOG(WARNING) << "Expected Tensor Strides mismatch with raw Tensor "
                        "metadata. Reported shapes may be inaccurate!";
                        return {raw_metadata, sizes, strides};
                    }
                    strides.push_back(*tensor_size_strides_it++);
                }
            }
            return {raw_metadata, sizes, strides};
        };

        std::vector<op_input_t> out;
        auto                    push_value = [&out, io_type](const Tag& tag, op_input_t input)
        {
            if (io_type == tagToIOType(tag))
            {
                out.emplace_back(std::move(input));
            }
            else
            {
                out.emplace_back(std::nullopt);
            }
        };

        bool terminate = false;
        while (!terminate && tag_it != tags_.end())
        {
            switch (*tag_it)
            {
            case Tag::Tensor:
                push_value(*tag_it, decode_tensor());
                break;

            case Tag::TensorListBegin:
            {
                std::vector<TensorMetadata> arg;
                bool                        found_undefined = false;
                while (*(++tag_it) != Tag::TERMINATOR)
                {
                    if (*tag_it == Tag::UndefinedTensor)
                    {
                        found_undefined = true;
                        continue;
                    }
                    // PROFILER_CHECK(*tag_it == Tag::Tensor, (int)(*tag_it));
                    arg.emplace_back(decode_tensor());
                }
                if (found_undefined)
                {
                    push_value(*tag_it, std::nullopt);
                }
                else
                {
                    push_value(Tag::TensorListBegin, std::move(arg));
                }
            }
            break;

            case Tag::ScalarList:
            case Tag::Scalar:
                push_value(*tag_it, *ivals_it++);
                break;

            case Tag::UndefinedTensor:
            case Tag::Other:
                push_value(*tag_it, std::nullopt);
                break;

            case Tag::TERMINATOR:
                // This marks the end of this op.
                terminate = true;
                break;

            default:
                break;
            }
            ++tag_it;
        }
        return out;
    };
}
#else
// Stub implementation when IValue methods are not available
auto InputOutputEncoder::getIValueGenerator(const IOType& /*io_type*/)  //NOLINT
{
    return []() mutable { return std::nullopt; };
}
#endif

auto InputOutputEncoder::getInputShapeGenerator()
{
    return getIValueGenerator(IOType::Shapes);
}

auto InputOutputEncoder::getConcreteInputGenerator()
{
    return getIValueGenerator(IOType::ConcreteInputs);
}

void InputOutputEncoder::clear()
{
    tags_.clear();
    tensor_metadata_.clear();
    tensor_sizes_strides_.clear();
    ivalues_.clear();
}

// ---------------------------------------------------
// |  Correlation ID tracking (OpList & EventBlock)  |
// ---------------------------------------------------
template <typename T, size_t ChunkSize>
ThreadLocalSubqueue::TorchOpStorage::EventBlock<T, ChunkSize>::EventBlock()
{
    static std::atomic<uint64_t> counter_{0};
    id_start_ = 1 + (ChunkSize * counter_++);
}

template <class... Args>
std::pair<KinetoObserverContext::Event*, uint64_t>
ThreadLocalSubqueue::TorchOpStorage::OpList::emplace_back(Args&&... args)
{
    auto event_ptr = AppendOnlyList::emplace_back(std::forward<Args>(args)...);
    auto corr_id   = buffer_last_->correlation_id(event_ptr);
    return {event_ptr, corr_id};
}

uint64_t ThreadLocalSubqueue::TorchOpStorage::OpList::correlationID(const OpList::Iterator& e)
{
    return e.address().first->correlation_id(&*e);
}

template <typename T, size_t ChunkSize>
uint64_t ThreadLocalSubqueue::TorchOpStorage::EventBlock<T, ChunkSize>::correlation_id(
    const T* ptr) const
{
    // PROFILER_CHECK_DEBUG(ptr >= this->data() && ptr < this->data() + ChunkSize);
    return id_start_ + (ptr - this->data());
}

// ---------------------------------
// |  Collection (Observer logic)  |
// ---------------------------------
std::unique_ptr<KinetoObserverContext> ThreadLocalSubqueue::begin_op(
    const quarisma::RecordFunction& fn)
{
    const auto* overload_name =
        config_.experimental_config.capture_overload_names ? fn.overload_name() : "";
    auto [event, corr_id] =
        torch_ops_.op_events_.emplace_back(quarisma::profiler_impl::impl::TorchOpBasicFields{
            fn.seqNr(),
            fn.forwardThreadId(),
            fn.scope(),
            fn.isAsync(),
            fn.handle(),
            fn.debugHandle(),
            fn.name(),
            overload_name});

    if (config_.report_input_shapes)
    {
        torch_ops_.inputs_outputs_.push(fn.inputs());
        torch_ops_.kwinputs_.emplace_back(fn.kwinputs());
    }
    if (!config_.experimental_config.disable_external_correlation)
    {
        if (fn.scope() == quarisma::RecordScope::USER_SCOPE)
        {
            quarisma::profiler_impl::impl::kineto::pushUserCorrelationId(corr_id);
        }
        else
        {
            quarisma::profiler_impl::impl::kineto::pushCorrelationId(corr_id);
        }
    }

#if !defined BUILD_LITE_INTERPRETER && !defined PROFILER_MOBILE
    // backward nodes source range corresponds to the forward node
    // TODO: consider using C++ stack trace
#if 0
    // Disabled: jit::currentCallstack() and jit::currentModuleHierarchy() not available in profiler-only build
    if (config_.with_stack && fn.scope() != quarisma::RecordScope::BACKWARD_FUNCTION)
    {
        auto cs = quarisma::profiler_impl::impl::prepareCallstack(jit::currentCallstack());
        torch_ops_.jit_stack_.emplace_back(callstackStr(cs));
    }
    if (config_.with_modules && fn.scope() != quarisma::RecordScope::BACKWARD_FUNCTION)
    {
        torch_ops_.jit_modules_.emplace_back(jit::currentModuleHierarchy());
    }
#endif
#endif
    if (config_.with_flops)
    {
        torch_ops_.extra_args_.emplace_back(quarisma::profiler_impl::impl::saveExtraArgs(fn));
    }

    auto out = std::make_unique<KinetoObserverContext>(event);
    if (fn.isNcclMeta())
    {
        // Record NCCL metadata for specific CPU ops, switch off output
        // introspection in this begin_op callback, we will do that in exit callback
        // if needed.
        quarisma::profiler_impl::impl::SaveNcclMetaConfig const ncclMetaConfig{true, true, true, false};
        out->event_->extra_nccl_meta_ = torch_ops_.extra_meta_.emplace_back(
            quarisma::profiler_impl::impl::saveNcclMeta(fn, ncclMetaConfig));
    }
    else
    {
        out->event_->extra_nccl_meta_ = torch_ops_.extra_meta_.emplace_back();
    }

    if (config_.state == ProfilerState::KINETO_GPU_FALLBACK)
    {
        out->fallback_ = torch_ops_.device_fallback_.emplace_back();
        quarisma::profiler_impl::impl::cudaStubs()->record(
            nullptr, &out->fallback_->device_event_start_, nullptr);
    }
    else if (config_.state == ProfilerState::KINETO_PRIVATEUSE1_FALLBACK)
    {
        out->fallback_ = torch_ops_.device_fallback_.emplace_back();
        quarisma::profiler_impl::impl::privateuse1Stubs()->record(
            nullptr, &out->fallback_->device_event_start_, nullptr);
    }

    event->start_time_ = quarisma::getApproximateTime();
#if 0
    // Disabled: quarisma::globalContext() not available in profiler-only build
    event->allow_tf32_cublas_ = quarisma::globalContext().float32Precision(
                                    quarisma::Float32Backend::CUDA, quarisma::Float32Op::MATMUL) ==
                                quarisma::Float32Precision::TF32;
#else
    event->allow_tf32_cublas_ = false;
#endif
    if (!config_.experimental_config.performance_events.empty())
    {
        const size_t n   = config_.experimental_config.performance_events.size();
        event->counters_ = std::make_unique<perf_counters_t>(n, 0);
        perf_profiler_->Enable();
    }
    return out;
}

// ---------------
// |  Collation  |
// ---------------
namespace
{
template <typename T>
struct StealOrDefault
{
    explicit StealOrDefault(T& container) : container_{container}, it_{container.begin()} {}

    StealOrDefault(const StealOrDefault&)            = delete;
    StealOrDefault(StealOrDefault&&)                 = delete;
    StealOrDefault& operator=(const StealOrDefault&) = delete;
    StealOrDefault& operator=(StealOrDefault&&)      = delete;
    ~StealOrDefault() { container_.get().clear(); }

    typename T::Iterator::value_type operator()()
    {
        if (it_.exhausted())
        {
            return typename T::Iterator::value_type();
        }

        auto result = std::move(*it_);
        ++it_;
        return result;
    }

    std::reference_wrapper<T> container_;
    typename T::Iterator      it_;
};
}  // namespace

[[maybe_unused]] static constexpr std::string_view profilerStepString = "ProfilerStep#";

void ThreadLocalSubqueue::TorchOpStorage::materialize(
    std::vector<std::shared_ptr<Result>>& /*out*/,
    std::vector<ProfilerStepInfo>& /*step_info*/,
    const std::function<quarisma::time_t(quarisma::approx_time_t)>& /*time_converter*/,
    const uint64_t /*tid*/,
    const kineto::DeviceAndResource& /*kineto_info*/)
{
#if 0
    // Disabled: This loop uses irange and modifies event fields that may not be available
    // Plumb Autograd info to the top level annotation.
    auto it = op_events_.begin();
    for ([[maybe_unused]] const auto _ :
         quarisma::irange(static_cast<int64_t>(op_events_.size()) - 1))
    {
        auto& first  = it->basic_fields_;
        auto& second = (++it)->basic_fields_;
        if (first.scope_ == quarisma::RecordScope::FUNCTION &&
            second.scope_ == quarisma::RecordScope::BACKWARD_FUNCTION &&
            first.name_.rfind("autograd::engine::evaluate_function: ", 0) == 0)
        {
            first.sequence_number_ = second.sequence_number_;
            first.forward_tid_     = second.forward_tid_;
        }
    }
#endif

    // `AccumulateGrad` is an important marker for profile analysis; however the
    // annotation relies on `quarisma::demangle` which is platform dependent. In
    // particular, Windows will add a "struct " prefix.
    const std::string accumulate_grad = "quarisma::autograd::AccumulateGrad";
    const std::string windows_pattern = std::string("struct ") + accumulate_grad;
    for (auto& event : op_events_)
    {
        auto& name     = event.basic_fields_.name_;
        auto  position = name.find(windows_pattern);
        if (position != std::string::npos)
        {
            name.replace(position, windows_pattern.size(), accumulate_grad);
        }
    }

    // TODO: CTAD will take care of template args when we move to C++17
    auto jit_stack    = StealOrDefault<decltype(jit_stack_)>(jit_stack_);
    auto jit_module   = StealOrDefault<decltype(jit_modules_)>(jit_modules_);
    auto extra_args   = StealOrDefault<decltype(extra_args_)>(extra_args_);
    auto extra_meta   = StealOrDefault<decltype(extra_meta_)>(extra_meta_);
    auto kwinputs     = StealOrDefault<decltype(kwinputs_)>(kwinputs_);
    auto gpu_fallback = StealOrDefault<decltype(device_fallback_)>(device_fallback_);

#if 0
    // Disabled: ExtraFields constructor expects std::vector<op_input_t> but getters return std::nullopt
    // These variables are only used in the disabled code below
    auto input_shape_getter    = inputs_outputs_.getInputShapeGenerator();
    auto concrete_input_getter = inputs_outputs_.getConcreteInputGenerator();
    for (auto event = op_events_.begin(); event != op_events_.end(); ++event)
    {
        ExtraFields<EventType::TorchOp> e{
            std::move(event->basic_fields_),
            ThreadLocalSubqueue::TorchOpStorage::OpList::correlationID(event),
            time_converter(event->end_time_),
            input_shape_getter(),
            concrete_input_getter(),
            jit_stack(),
            jit_module(),
            extra_args(),
            extra_meta(),
            kwinputs(),
            gpu_fallback(),
            event->allow_tf32_cublas_,
            std::move(event->counters_)};

        if (e.name_.find(profilerStepString) != std::string::npos)
        {
            step_info.emplace_back(
                time_converter(event->start_time_), time_converter(event->end_time_), out.size());
        }
        out.emplace_back(
            Result::create(time_converter(event->start_time_), tid, kineto_info, std::move(e)));
    }
#endif

    op_events_.clear();
    inputs_outputs_.clear();
}

#if 0
// Disabled: quarisma::profiler_impl::impl::vulkan not available in profiler-only build
template <size_t BlockSize>
static void materialize_vulkan(
    std::vector<std::shared_ptr<Result>>&                                   out,
    AppendOnlyList<ExtraFields<EventType::Vulkan>::raw_event_t, BlockSize>& raw_events,
    const std::function<quarisma::time_t(quarisma::approx_time_t)>&             time_converter,
    const uint64_t                                                          tid,
    const kineto::DeviceAndResource&                                        kineto_info)
{
    for (const auto& i : raw_events)
    {
        const auto name_and_duration_ns =
            quarisma::profiler_impl::impl::vulkan::getShaderNameAndDurationNs(i.second);

        out.emplace_back(
            Result::create(
                /*start_time_ns_=*/time_converter(i.first),
                /*start_tid_=*/tid,
                /*kineto_info_=*/kineto_info,
                /*extra_fields_=*/
                ExtraFields<EventType::Vulkan>{
                    /*name_=*/std::get<0>(name_and_duration_ns),
                    /*duration_ns_=*/
                    static_cast<int64_t>(std::get<1>(name_and_duration_ns)),
                    /*in_tree_building_=*/false}));
    }
    raw_events.clear();
}
#else
// Stub implementation
template <size_t BlockSize>
static void materialize_vulkan(
    std::vector<std::shared_ptr<Result>>& /*out*/,
    AppendOnlyList<ExtraFields<EventType::Vulkan>::raw_event_t, BlockSize>& raw_events,
    const std::function<quarisma::time_t(quarisma::approx_time_t)>& /*time_converter*/,
    const uint64_t /*tid*/,
    const kineto::DeviceAndResource& /*kineto_info*/)
{
    raw_events.clear();
}
#endif

namespace
{
// See `RecordQueue::getSubqueue()` for an overview of this cache.
struct SubQueueThreadCache
{
    uint32_t             key_;
    ThreadLocalSubqueue* ref_;
};

// The astute observer will note that this leaves a dangling reference; nothing
// in the teardown of `RecordQueue` or `ThreadLocalSubqueue` clears this value.
// (And the raw pointer in `SubQueueThreadCache` will not extend the lifetime
// of `*ref_`.) This is safe, however, because `getSubqueue` will check
// `sub_queue_cache_.key_` before attempting to access `ref_`, and if `key_`
// does not match the RecordQueue's *unique* `id_` it will evict
// `sub_queue_cache_` and fall back to a different mechanism.
std::atomic<uint32_t>            queue_id_{0};
thread_local SubQueueThreadCache sub_queue_cache_{0, nullptr};

#if 0
// Disabled: ExtraFields<EventType::PyCall> is not defined in profiler-only build
std::string toString(const ExtraFields<EventType::PyCall>& e)
{
    if (e.module_.has_value())
    {
        return fmt::format("nn.Module: {}_{}", e.module_->cls_name_.str(), e.module_->id_);
    }
    return fmt::format(
        "{}({}): {}",
        e.callsite_.filename_.str(),
        e.callsite_.line_no_,
        e.callsite_.funcname_.str());
}
#else
// Stub implementation
[[maybe_unused]] std::string toString(const ExtraFields<EventType::PyCall>& /*e*/)
{
    return "";
}
#endif

auto scopeToType(quarisma::RecordScope scope)
{
    return scope == quarisma::RecordScope::USER_SCOPE ? libkineto::ActivityType::USER_ANNOTATION
                                                      : libkineto::ActivityType::CPU_OP;
}

int64_t torchOpEndNS(
    const ExtraFields<EventType::TorchOp>& e,
    const bool                             finished,
    const std::weak_ptr<Result>&           parent)
{
    if (finished && e.end_time_ns_ == std::numeric_limits<quarisma::time_t>::min())
    {
        auto p = parent.lock();
        if (p)
        {
            return p->endTimeNS();
        }
    }
    return e.end_time_ns_;
}

auto kinetoEventCorrelationID(
    const ExtraFields<EventType::Kineto>& e, const std::weak_ptr<Result>& parent)
{
    if (e.correlation_id_ != 0u)
    {
        return e.correlation_id_;
    }
    auto p = parent.lock();
    return p ? p->correlationID() : 0;
}
}  // namespace

#define ATTRIBUTE(event_type, expr)                  \
    [&](const ExtraFields<EventType::event_type>& e) \
    {                                                \
        (void)e;                                     \
        return expr;                                 \
    }

#if 0
// Disabled: PyCall and PyCCall ExtraFields not defined in profiler-only build
std::string Result::name() const
{
    return visit(
        quarisma::overloaded(
            ATTRIBUTE(Vulkan, std::string(e.name_)),
            ATTRIBUTE(Allocation, std::string("[memory]")),
            ATTRIBUTE(OutOfMemory, std::string("[OutOfMemory]")),
            ATTRIBUTE(PyCall, toString(e)),
            ATTRIBUTE(PyCCall, std::string(e.function_name_.str())),
            ATTRIBUTE(PythonGC, std::string("Python GC")),
            [](const auto& e) -> std::string { return e.name_; }));
}
#else
// Stub implementation
std::string Result::name() const
{
    return visit(quarisma::overloaded(
        ATTRIBUTE(Vulkan, std::string(e.name_)),
        ATTRIBUTE(Allocation, std::string("[memory]")),
        ATTRIBUTE(OutOfMemory, std::string("[OutOfMemory]")),
        ATTRIBUTE(PythonGC, std::string("Python GC")),
        [](const auto& e) -> std::string { return e.name_; }));
}
#endif

std::string Result::overload_name() const
{
    return visit(quarisma::overloaded(
        ATTRIBUTE(TorchOp, std::string(e.overload_name_)),
        [](const auto& /*e*/) -> std::string { return ""; }));
}

libkineto::ActivityType Result::kinetoType() const
{
    return visit(quarisma::overloaded(
        ATTRIBUTE(TorchOp, scopeToType(e.scope_)),
        ATTRIBUTE(Backend, scopeToType(e.scope_)),
        ATTRIBUTE(Vulkan, libkineto::ActivityType::CPU_OP),
        ATTRIBUTE(Allocation, libkineto::ActivityType::CPU_INSTANT_EVENT),
        ATTRIBUTE(OutOfMemory, libkineto::ActivityType::CPU_INSTANT_EVENT),
        ATTRIBUTE(PyCall, libkineto::ActivityType::PYTHON_FUNCTION),
        ATTRIBUTE(PyCCall, libkineto::ActivityType::PYTHON_FUNCTION),
        ATTRIBUTE(PythonGC, libkineto::ActivityType::PYTHON_FUNCTION),
        ATTRIBUTE(Kineto, e.activity_type_)));
}

uint64_t Result::correlationID() const
{
    return visit(quarisma::overloaded(
        ATTRIBUTE(TorchOp, e.correlation_id_),
        ATTRIBUTE(Kineto, kinetoEventCorrelationID(e, parent_)),
        [&](const auto&) -> uint64_t { return 0; }));
}

int64_t Result::endTimeNS() const
{
    auto end_time_ns = visit(quarisma::overloaded(
        ATTRIBUTE(TorchOp, torchOpEndNS(e, finished_, parent_)),
        ATTRIBUTE(Backend, e.end_time_us_ * 1000),
        ATTRIBUTE(Vulkan, start_time_ns_ + (e.in_tree_building_ ? 0 : e.duration_ns_)),
        ATTRIBUTE(Allocation, start_time_ns_),
        ATTRIBUTE(OutOfMemory, start_time_ns_),
        ATTRIBUTE(Kineto, start_time_ns_ + e.duration_ns_),
        ATTRIBUTE(PythonGC, start_time_ns_ + e.duration_ns_),
        [&](const auto& e) -> int64_t { return e.end_time_ns_; }));

    // In rare cases we're willing to tolerate ops which are missing an end time
    // so long as they can borrow their parent's end time. A consequence of this,
    // however, is that `endTimeNS` may not make sense until tree construction is
    // complete.
    auto end_time_is_valid = !finished_ || SOFT_ASSERT(end_time_ns >= start_time_ns_, name());
    return end_time_is_valid ? end_time_ns : start_time_ns_;
}

uint64_t Result::endTID() const
{
    return visit(quarisma::overloaded(
        ATTRIBUTE(TorchOp, e.end_tid_), [&](const auto&) -> uint64_t { return start_tid_; }));
}

quarisma::device_enum Result::deviceType() const
{
    using quarisma::autograd::profiler_impl::deviceTypeFromActivity;
    return visit(quarisma::overloaded(
        ATTRIBUTE(Allocation, e.device_type_),
        ATTRIBUTE(OutOfMemory, e.device_type_),
        ATTRIBUTE(Kineto, deviceTypeFromActivity(e.activity_type_)),
        [&](const auto&) { return quarisma::device_enum::CPU; }));
}
#undef ATTRIBUTE

ThreadLocalSubqueue::ThreadLocalSubqueue(const uint64_t tid, ProfilerConfig config)
    : tid_{tid}, config_{std::move(config)}, kineto_info_{kineto::kineto_ids()}
{
    quarisma::profiler_impl::impl::kineto::recordThreadInfo();
    if (!config_.experimental_config.performance_events.empty())
    {
        perf_profiler_ = std::make_unique<quarisma::profiler_impl::impl::linux_perf::PerfProfiler>();
        perf_profiler_->Configure(config_.experimental_config.performance_events);
    }
}

RecordQueue::RecordQueue(ProfilerConfig config, std::set<ActivityType> activities)
    : id_(++queue_id_), config_{std::move(config)}, activities_{std::move(activities)}
{
    if (tracePython())
    {
        python_tracer_ = python_tracer::PythonTracerBase::make(this);
        if (getPythonGcEvents())
        {
            python_tracer_->register_gc_callback();
        }
    }
}

bool RecordQueue::tracePython() const
{
    return config_.with_stack && (activities_.count(ActivityType::CPU) > 0);  //NOLINT
}

bool RecordQueue::getPythonGcEvents() const
{
    return config_.experimental_config.record_python_gc_info;
}

ThreadLocalSubqueue* RecordQueue::getSubqueue()
{
    // In the most common case, a thread will want to write to the same sub-queue
    // that it wrote to last call. The only time that isn't true is if:
    //  A) The profiler context has ended and we are in a new one.
    //  B) Two profilers are active in different TLS contexts, and this thread
    //     is a worker helping with intra-op parallelism.
    // Since we expect this to be the OVERWHELMINGLY common case (>99%), we add a
    // special thread_local cache so that we can skip the overall `flat_hash_map`
    // (and corresponding lock).
    if (id_ == sub_queue_cache_.key_)
    {
        return sub_queue_cache_.ref_;
    }

    const auto             tid = quarisma::RecordFunction::currentThreadId();
    std::scoped_lock const guard(sub_queue_mutex_);
    auto                   it = sub_queues_.find(tid);
    if (it == sub_queues_.end())
    {
        it = sub_queues_.emplace(tid, std::make_unique<ThreadLocalSubqueue>(tid, config_)).first;
    }

    sub_queue_cache_ = SubQueueThreadCache{id_, it->second.get()};
    return it->second.get();
}

void RecordQueue::stop()
{
    if (python_tracer_)
    {
        python_tracer_->stop();
    }
}

void RecordQueue::restart()
{
    if (python_tracer_)
    {
        python_tracer_->restart();
    }
}

namespace
{
void mark_finished(const std::shared_ptr<Result>& r)
{
    //PROFILER_CHECK(!r->finished_, r->name());
    r->finished_ = true;
    //PROFILER_CHECK(r->endTimeNS() >= r->start_time_ns_, r->name());
}

#if PROFILER_HAS_KINETO
// Assumption: Total threads number will not exceed 2^16-1, and total ops will
// not exceed 2^48 -1.
uint64_t getForwardThreadKey(uint64_t tid, uint64_t seqNr)
{
    return ((tid << 48) | (seqNr & (((uint64_t)1 << 48) - 1)));
}

void generateForwardBackwardLink(
    const Result&                                                   profiler_result,
    uint64_t&                                                       fwd_bwd_link_id,
    libkineto::GenericTraceActivity&                                activity,
    std::unordered_map<uint64_t, libkineto::GenericTraceActivity*>& tidSeq2activity)
{
    const auto& extra_fields =
        std::get<ExtraFields<EventType::TorchOp>>(profiler_result.extra_fields_);
    if (extra_fields.forward_tid_ > 0)
    {
        // act is backward op.
        uint64_t const key =
            getForwardThreadKey(extra_fields.forward_tid_, extra_fields.sequence_number_);
        auto iter = tidSeq2activity.find(key);
        if (iter != tidSeq2activity.end())
        {
            libkineto::GenericTraceActivity* fwd = iter->second;
            fwd->flow.start                      = true;
            activity.flow.id = fwd->flow.id = fwd_bwd_link_id;
            activity.flow.type = fwd->flow.type = libkineto::kLinkFwdBwd;
            ++fwd_bwd_link_id;

            // If there are multiple events that match this sequence/tid pair, we
            // should delete this entry in the map to avoid inserting multiple "end"
            // flow events.
            tidSeq2activity.erase(iter);
        }
    }
    else if (profiler_result.start_tid_ != 0)
    {
        // act is forward op.
        uint64_t const key =
            getForwardThreadKey(profiler_result.start_tid_, extra_fields.sequence_number_);
        // Assumption: Among all ops with same sequence number,
        // the one with biggest start time is most likely launching backward op.
        auto iter = tidSeq2activity.find(key);
        if (iter == tidSeq2activity.end())
        {
            tidSeq2activity[key] = &activity;
        }
        else
        {
            // Now the sequence number is only incremented on creating a "Node"
            // object for backward pass, by calling
            // "quarisma::sequence_number::get_and_increment()". Among all ops with same
            // sequence number, the one with biggest startTime is the one launching
            // backward op.
            if (activity.startTime >= iter->second->startTime)
            {
                tidSeq2activity[key] = &activity;
            }
        }
    }
}
#endif  // PROFILER_HAS_KINETO

void generateForwardBackwardLinks(
    const std::unique_ptr<quarisma::profiler_impl::impl::kineto::trace_t>& cpu_trace,
    const std::vector<std::shared_ptr<Result>>&                       results)
{
#ifndef PROFILER_HAS_KINETO
}
#else   // PROFILER_HAS_KINETO
    // PROFILER_CHECK(cpu_trace->activities.size() == results.size());

    // startThreadId_seqNum to pointer of activity.
    // Low-16bits of startThreadId and low-48bits seqNum are concatenated into
    // one uint64_t variable as key.

    std::unordered_map<uint64_t, libkineto::GenericTraceActivity*> tidSeq2activity;
    uint64_t                                                       fwd_bwd_link_id = 1;

    using result_activity_t = std::pair<Result*, libkineto::GenericTraceActivity*>;
    std::vector<result_activity_t> torch_events;

    for (const auto idx : quarisma::irange(cpu_trace->activities.size()))
    {
        const auto& profiler_result = results[idx];
        auto&       activity        = cpu_trace->activities[idx];

        // add information about an associated forward op, if a sequence number
        // is available (e.g. during training)

        profiler_result->visit_if_base<ExtraFields<EventType::TorchOp>>(
            [&](const auto& e)
            {
                if (e.sequence_number_ >= 0)
                {
                    torch_events.emplace_back(profiler_result.get(), activity.get());
                }
            });
    }

    // We need to visit the events in chronological order.
    // So we sort them by end_time_ns_ before processing.
    std::sort(
        torch_events.begin(),
        torch_events.end(),
        [](const result_activity_t& left, const result_activity_t& right)
        {
            auto left_end_time =
                std::get<ExtraFields<EventType::TorchOp>>(left.first->extra_fields_).end_time_ns_;
            auto right_end_time =
                std::get<ExtraFields<EventType::TorchOp>>(right.first->extra_fields_).end_time_ns_;
            return left_end_time < right_end_time;
        });

    for (auto& [profiler_result, activity] : torch_events)
    {
        generateForwardBackwardLink(*profiler_result, fwd_bwd_link_id, *activity, tidSeq2activity);
    }
}
#endif  // PROFILER_HAS_KINETO

constexpr const char* indexKey = "Ev Idx";

void passEventsToKineto(
    const std::vector<std::shared_ptr<Result>>& results,
    uint64_t                                    start_time_ns,
    uint64_t                                    end_time_ns,
    const ProfilerConfig&                       config)
{
    using namespace quarisma::profiler_impl::impl::kineto;
    TraceWrapper cpu_trace(static_cast<int64_t>(start_time_ns), "Quarisma Profiler");

    // Generate Kineto events for each event recorded by the Quarisma profiler.
    for (const auto i : quarisma::irange(results.size()))
    {
        const auto& e = results[i];
        // Here we are essentially setting the duration to -1 if the event never
        // ends. This way Kineto will extend the event to the end of the trace. This
        // is useful so that we can still have 0 duration events if necessary
        // without extension
        int64_t const act_end_time = std::max(e->endTimeNS(), e->start_time_ns_ - 1);
        std::string   name         = e->name();
        if (!e->overload_name().empty())
        {
            name = fmt::format("{}.{}", e->name(), e->overload_name());
        }
        auto* activity = cpu_trace.addCPUActivity(
            name,
            e->kinetoType(),
            e->kineto_info_,
            e->correlationID(),
            e->start_time_ns_,
            act_end_time);

        if (activity != nullptr)
        {
            addMetadata(activity, indexKey, std::to_string(i));

            // There is a longstanding regression for initializing
            // on-demand Kineto activity handling. Enabling this path
            // for Profiler API could cause side effects as much has changed since.
            // Make a surgical fix here until we holistically assess the on-demand
            // vs API path fragmentation, which has been snowballing in complexity
            // and thus flakiness.
            if (config.global())
            {
                e->kineto_activity_ = activity;
            }
        }
    }

    if (get_fwd_bwd_enabled())
    {
        generateForwardBackwardLinks(cpu_trace.get(), results);
    }

    // Kineto adds the events that it collected.
    cpu_trace.transferCpuTrace(static_cast<int64_t>(end_time_ns));
}

#if PROFILER_HAS_KINETO
// There are two mechanisms that we use to connect Profiler and Kineto events.
// The first is the correlation ID. The profiler pushes a unique integer at the
// start of an op and pops it at the end. Kineto then associates the events
// that it collects with that correlation ID and sets the linked activity of
// the events that it collected to point to the profiler op.
//
// However, this is not a sufficient description because it does not retain
// dependency information between kineto ops. Consider a call to `quarisma.add`.
// Three events will be collected:
//   `aten::add`          (TorchOp, collected by profiler)
//   `cudaLaunchKernel`   (CUDA runtime event, collected by Kineto)
//   `quarisma::vectorized_...` (GPU kernel, collected by Kineto)
// If we only relied on correlation IDs we would set both Kineto events as
// children of the `quarisma::add`, rather than the correct
//   `quarisma::add -> cudaLaunchKernel -> quarisma::vectorized_...`
//
// Kineto surfaces this information through a second concept called a "flow".
// In this example, the `cudaLaunchKernel` event is the start of a flow and the
// GPU kernel has the same flow id but is not a start event. Thus, when merging
// the Kineto events into the call tree we first add all events which are flow
// start nodes. We then merge the rest, trying to pair them with flow starts
// and falling back to correlation ID if necessary. For any nodes without
// linked events the caller is determined using the normal tree construction
// algorithm.
class TransferEvents
{
    using itrace_t   = libkineto::ITraceActivity;
    using activity_t = quarisma::profiler_impl::impl::kineto::activity_t;

public:
    TransferEvents(
        std::vector<std::shared_ptr<Result>>& results,
        trace_ptr_t&                          trace,
        const ProfilerConfig&                 config)
        : results_{results}, config_{config}
    {
        const auto* trace_activities_ptr = trace->get()->activities();
        // PROFILER_CHECK(trace_activities_ptr != nullptr);
        trace_activities_ = *trace_activities_ptr;
        reassociate();
        extractEventsFromTrace();
        setParents();
    }

private:
    static long long extractIndex(const std::string& metadata_json)
    {
        static const auto prefix = fmt::format("\"{}\": ", indexKey);
        auto              pos    = metadata_json.find(prefix);
        return (pos == std::string::npos) ? unmatchedIndex : [&]()
        {
            auto end = metadata_json.find(',', pos);
            end      = (end == std::string::npos) ? metadata_json.size() : end;
            return std::stoll(metadata_json.substr(pos + prefix.size(), end));
        }();
    }

    std::shared_ptr<Result> lookup(const itrace_t* key)
    {
        if (key == nullptr)
        {
            return nullptr;
        }

        // First check the map.
        auto it = kineto_events_.find(key);
        if (it != kineto_events_.end())
        {
            return it->second;
        }

        // Then fallback to the encoded metadata.
        const auto index = extractIndex((key != nullptr) ? key->metadataJson() : "");
        if (index != unmatchedIndex)
        {
            auto out            = results_.get().at(index);
            kineto_events_[key] = out;
            return out;
        }

        // And finally give up.
        return nullptr;
    }

    void reassociate()
    {
        // Match profiler events with the corresponding kineto events. Kineto may
        // have moved or copied the activities, so we have to recover the
        // relationship between `libkineto::ITraceActivity` and `Result`.
        for (const auto* activity : trace_activities_)
        {
            // PROFILER_CHECK(activity != nullptr);
            auto e = lookup(activity);
            if (e != nullptr)
            {
                // PROFILER_CHECK(e->kineto_activity_ == nullptr);
                e->kineto_activity_ = static_cast<const activity_t*>(activity);
            }
        }
        /*if (results_.get().size() != kineto_events_.size())
        {
            PROFILER_LOG_WARNING(
                fmt::format(
                    "Failed to recover relationship between all profiler and kineto events: "
                    "{} vs. {}  reassociated.",
                    results_.get().size(),
                    kineto_events_.size()));
        }*/
    }

    static bool isHiddenEvent(const itrace_t* activity)
    {
        // PROFILER_CHECK(activity != nullptr);
        // Kineto uses "hidden" metadata to mark events that should be hidden.
        return activity->getMetadataValue("hidden") == "1";
    }

    static std::shared_ptr<Result> resultFromActivity(const itrace_t* activity)
    {
        // PROFILER_CHECK(activity != nullptr);

        // Kineto is inconsistent with types, so we have to cast to int32.
        quarisma::profiler_impl::impl::kineto::DeviceAndResource const device_and_resource{
            static_cast<int32_t>(activity->deviceId()),
            static_cast<int32_t>(activity->resourceId())};

        auto event = Result::create(
            activity->timestamp(),
            noTID,  // Placeholder
            device_and_resource,
            ExtraFields<EventType::Kineto>{
                activity->name(),
                activity->duration(),
                static_cast<uint64_t>(activity->correlationId()),
                activity->type(),
                {/*id=*/static_cast<uint32_t>(activity->flowId()),
                 /*type=*/static_cast<uint32_t>(activity->flowType()),
                 /*start=*/static_cast<uint32_t>(activity->flowStart())}});
        event->hidden_ = isHiddenEvent(activity);
        // NB: It's tempting to set `event->kineto_activity_`; however we can only
        // guarantee that the events we passed to Kineto are of type
        // `GenericTraceActivity`. Others may derive from ITraceActivity and thus
        // are not safe to cast.
        return event;
    }

    std::shared_ptr<Result> toResult(const itrace_t* activity)
    {
        auto e = lookup(activity);

        // Until we are very sure that we can reassociate kineto and profiler
        // events we need to be very defensive.
        const auto type = activity->type();
        if (e == nullptr && (type == libkineto::ActivityType::CPU_OP ||
                             type == libkineto::ActivityType::CPU_INSTANT_EVENT ||
                             type == libkineto::ActivityType::USER_ANNOTATION ||
                             type == libkineto::ActivityType::PYTHON_FUNCTION))
        {
            /* PROFILER_LOG_WARNING(
                "Detected an event which was likely passed to kineto by the Quarisma "
                "profiler, but is not present in the set of known events: ",
                activity->name(),
                " This most likely means that Kineto has not "
                "maintained address stability for this event. Please report this to "
                "the Quarisma team.");
            */
            return nullptr;
        }

        if (e == nullptr)
        {
            e = resultFromActivity(activity);
            results_.get().push_back(e);
            kineto_events_[activity] = e;
        }
        return e;
    }

    void extractEventsFromTrace()
    {
        for (const auto* activity : trace_activities_)
        {
            auto e = toResult(activity);
            if (e)
            {
                if (config_.experimental_config.expose_kineto_event_metadata)
                {
                    e->visit(quarisma::overloaded(
                        [&](ExtraFields<EventType::TorchOp>& i)
                        { i.metadata_json_ = activity->metadataJson(); },
                        [&](ExtraFields<EventType::Kineto>& i)
                        { i.metadata_json_ = activity->metadataJson(); },
                        [](auto&) { return; }));
                }
                const auto* linked_activity = activity->linkedActivity();
                if (linked_activity != nullptr)
                {
                    e->visit(quarisma::overloaded(
                        [&](ExtraFields<EventType::Kineto>& i)
                        { i.linked_activity_ = toResult(linked_activity); },
                        [](auto&) { /* PROFILER_CHECK(false); */ }));
                }
            }
        }
    }

    void setKinetoTID(std::shared_ptr<Result>& r, std::shared_ptr<Result> parent)
    {
        r->visit(quarisma::overloaded(
            [&]([[maybe_unused]] ExtraFields<EventType::Kineto>& i)
            {
                // PROFILER_CHECK(r->start_tid_ == noTID);
                r->start_tid_ =
                    parent ? parent->start_tid_ : quarisma::RecordFunction::currentThreadId();
            },
            [](auto&) {}));

        for (auto& child : r->children_)
        {
            setKinetoTID(child, r);
        }
    }

    void setParents()
    {
        // First pass: Collect start events and set parent to linked event.
        quarisma::flat_hash_map<uint32_t, std::shared_ptr<Result>> flow_map;
        for (auto& e : results_.get())
        {
            // PROFILER_CHECK(e != nullptr);
            e->visit(quarisma::overloaded(
                [&](const ExtraFields<EventType::Kineto>& i)
                {
                    if (i.flow.type == libkineto::kLinkAsyncCpuGpu && i.flow.start)
                    {
                        auto inserted = flow_map.insert({i.flow.id, e});
#ifdef PROFILER_USE_ROCM
                        if (inserted.second)
                        {
                            /* PROFILER_LOG_WARNING(
                                "ROCTracer produced duplicate flow start: ", i.flow.id);
                            */
                        }
#else   // PROFILER_USE_ROCM
                        // PROFILER_CHECK(inserted.second);
#endif  // PROFILER_USE_ROCM
                    }
                    // PROFILER_CHECK(e->parent_.expired());
                    e->parent_ = i.linked_activity_;
                },
                [](const auto&) {}));
        }

        // Second pass
        for (auto& e : results_.get())
        {
            e->visit(quarisma::overloaded(
                [&](const ExtraFields<EventType::Kineto>& i)
                {
                    // Flow takes priority over linked event.
                    const auto it = flow_map.find(i.flow.id);
                    if (it != flow_map.end() && i.flow.type == libkineto::kLinkAsyncCpuGpu &&
                        !i.flow.start)
                    {
                        e->parent_ = it->second;
                    }

                    // If a parent was set we have to do some bookkeeping.
                    auto parent = e->parent_.lock();
                    if (parent)
                    {
                        parent->children_.push_back(e);
                        mark_finished(e);
                    }
                },
                [](const auto&) {}));
        }

        // Set TIDs now that we have established lineage.
        for (auto& e : results_.get())
        {
            if (e->parent_.expired())
            {
                setKinetoTID(e, nullptr);
            }
        }
    }

    static constexpr long long unmatchedIndex = -1;
    static constexpr auto      noTID          = std::numeric_limits<uint64_t>::max();
    std::reference_wrapper<std::vector<std::shared_ptr<Result>>>      results_;
    const ProfilerConfig&                                             config_;
    std::vector<const itrace_t*>                                      trace_activities_;
    quarisma::flat_hash_map<const itrace_t*, std::shared_ptr<Result>> kineto_events_;
};
#else
class TransferEvents
{
public:
    template <class... Args>
    TransferEvents(Args&&... /*unused*/)
    {
    }
};
#endif

trace_ptr_t addKinetoEvents(
    std::vector<std::shared_ptr<Result>>& results,
    uint64_t                              start_time_ns,
    uint64_t                              end_time_ns,
    const ProfilerConfig&                 config)
{
    using namespace quarisma::profiler_impl::impl::kineto;
    passEventsToKineto(results, start_time_ns, end_time_ns, config);

    // In on demand mode kineto is directly controlled by other machinery.
    if (config.global())
    {
        return nullptr;
    }

    auto trace = std::make_unique<ActivityTraceWrapper>(stopTrace());
    //PROFILER_CHECK(trace || !kKinetoAvailable);
    // TransferEvents constructor has side effects (transfers Kineto events to results)
    // cppcheck-suppress unreadVariable
    TransferEvents const transfer{results, trace, config};
    return trace;
}

struct ResultGreater
{
    bool operator()(const result_ptr_t& a, const result_ptr_t& b) const
    {
        return a->endTimeNS() > b->endTimeNS();
    }
};

void set_in_tree_building(const std::vector<result_ptr_t>& results, const bool value)
{
    for (result_ptr_t const& r : results)
    {
        r->visit(quarisma::overloaded(
            [value](ExtraFields<EventType::Vulkan>& i) { i.in_tree_building_ = value; },
            [&](auto&)
            {
                // pass
            }));
    }
}

void build_tree(std::vector<std::shared_ptr<Result>>& sorted_events)
{
    set_in_tree_building(sorted_events, true);

    using op_fields = ExtraFields<EventType::TorchOp>;
    quarisma::flat_hash_map<uint64_t, std::shared_ptr<Result>>                  stacks;
    std::priority_queue<result_ptr_t, std::vector<result_ptr_t>, ResultGreater> end_events_;

    auto push_event = [&stacks, &end_events_](std::shared_ptr<Result>& event)
    {
        // Kineto builds subtrees using correlation ids and flows, so some Kineto
        // events are already marked finished before the main tree building
        // algorithm. It's fine to ignore them; the root event of these subtrees
        // not a Kineto op and will be handled normally.
        if (std::holds_alternative<ExtraFields<EventType::Kineto>>(event->extra_fields_) &&
            event->finished_)
        {
            return;
        }

        // PROFILER_CHECK(event->parent_.expired());
        for (const auto& child : event->children_)
        {
            // PROFILER_CHECK(child->finished_);
        }
        // PROFILER_CHECK(!event->finished_);

        auto parent_it = stacks.find(event->start_tid_);
        if (parent_it == stacks.end())
        {
            auto fwd_tid = event->visit(quarisma::overloaded(
                [](const op_fields& i) { return i.forward_tid_; },
                [](const auto&) -> uint64_t { return 0; }));
            if (fwd_tid)
            {
                parent_it = stacks.find(fwd_tid);
            }
        }

        if (parent_it != stacks.end())
        {
            event->parent_ = parent_it->second;
            parent_it->second->children_.push_back(event);
        }

        if (event->endTimeNS() > event->start_time_ns_)
        {
            stacks[event->start_tid_] = event;
            end_events_.push(event);
        }
        else if (event->endTimeNS() == std::numeric_limits<quarisma::time_t>::min())
        {
            // We use min time to indicate the lack of a termination event, so if we
            // encounter such a case we don't push to `end_events_`.
            stacks[event->start_tid_] = event;
        }
        else
        {
            mark_finished(event);
        }
    };

    auto pop_event = [&stacks](std::shared_ptr<Result> event)
    {
        if (event->finished_)
        {
            // This event was marked finished by a previous `pop_event` call.
            return;
        }

        auto start_tid = event->start_tid_;
        auto frame     = stacks.at(start_tid);

        while (frame.get() != event.get())
        {
            // PROFILER_CHECK(frame != nullptr);
            mark_finished(frame);
            // PROFILER_CHECK(!frame->parent_.expired());
            frame = frame->parent_.lock();
        }

        mark_finished(event);
        stacks.erase(start_tid);
        auto new_frame = event->parent_.lock();
        if (new_frame != nullptr)
        {
            stacks[start_tid] = new_frame;
        }
    };

    // Stack replay loop.
    for (auto& event : sorted_events)
    {
        while (!end_events_.empty() && end_events_.top()->endTimeNS() < event->start_time_ns_)
        {
            pop_event(end_events_.top());
            end_events_.pop();
        }
        push_event(event);
    }

    // Cleanup remaining exit events.
    while (!end_events_.empty())
    {
        pop_event(end_events_.top());
        end_events_.pop();
    }

    set_in_tree_building(sorted_events, false);
}

/**
 * Adjust r's duration to be the max of its current duration and the sum of all
 * of its children's adjusted durations (keeping its start time the same)
 * (adjust all child durations recursively)
 */
int64_t adjust_durations_dfs(std::shared_ptr<Result>& r)
{
    if (SOFT_ASSERT(r != nullptr))
    {
        int64_t const original_duration       = r->endTimeNS() - r->start_time_ns_;
        int64_t       children_total_duration = std::accumulate(
            r->children_.begin(),
            r->children_.end(),
            0,
            [](int64_t acc, std::shared_ptr<Result>& child)
            { return acc + adjust_durations_dfs(child); });

        if (children_total_duration > original_duration)
        {
            r->visit(quarisma::overloaded(
                [&r, &children_total_duration](ExtraFields<EventType::TorchOp>& i)
                { i.end_time_ns_ = r->start_time_ns_ + children_total_duration; },
                [&children_total_duration](ExtraFields<EventType::Vulkan>& i)
                { i.duration_ns_ = children_total_duration; },
                []([[maybe_unused]] ExtraFields<EventType::Allocation>& _)
                {
                    // Pass- Allocation events can't have children
                },
                [&](auto&)
                {
                    SOFT_ASSERT(
                        false,
                        "unexpected event type in mobile profiler adjust_durations_dfs: ",
                        r->name());
                }));
            return children_total_duration;
        }

        return original_duration;
    }

    return 0;
}

/**
 * 1) Adjust r's start time to be [new_start_time] (also adjusting end time and
      keeping duration the same)
 * 2) Recursively adjust r's children's start times, making them line up such
      that the last one ends at the same time as r
 * 3) Return r's final end time
 */
int64_t adjust_timestamps_dfs(std::shared_ptr<Result>& r, int64_t new_start_time)
{
    if (SOFT_ASSERT(r != nullptr))
    {
        if (r->start_time_ns_ != new_start_time)
        {
            // Adjust start time (keeping duration constant)
            r->visit(quarisma::overloaded(
                [&r, &new_start_time](ExtraFields<EventType::TorchOp>& i)
                { i.end_time_ns_ = new_start_time + (i.end_time_ns_ - r->start_time_ns_); },
                []([[maybe_unused]] ExtraFields<EventType::Vulkan>& i)
                {
                    // Pass- We don't need to manually adjust end time for Vulkan events
                },
                []([[maybe_unused]] ExtraFields<EventType::Allocation>& _)
                {
                    // Pass- No duration or end time to adjust
                },
                [&](auto&)
                {
                    SOFT_ASSERT(
                        false,
                        "unexpected event type in mobile profiler adjust_timestamps_dfs: ",
                        r->name());
                }));
            r->start_time_ns_ = new_start_time;
        }
        int64_t const children_total_duration = std::accumulate(
            r->children_.begin(),
            r->children_.end(),
            0,
            [](int64_t acc, std::shared_ptr<Result>& child)
            { return acc + (child->endTimeNS() - child->start_time_ns_); });

        int64_t child_start_time = r->endTimeNS() - children_total_duration;
        for (std::shared_ptr<Result>& child : r->children_)
        {
            child_start_time = adjust_timestamps_dfs(child, child_start_time);
        }
    }
    return r->endTimeNS();
}

/**
 * Adjust timestamps and durations of nodes in [out] such that
 *  - Vulkan event timelines are synchronized with CPU event times
 *  - Parent event timelines fully contain their child timelines
 *  - No overlaps in timelines for nodes at the same depth
 */
void adjust_timestamps(std::vector<std::shared_ptr<Result>>& out)
{
    if (out.empty())
    {
        return;
    }

    int64_t min_start_time = out[0]->start_time_ns_;
    for (std::shared_ptr<Result>& r : out)
    {
        // Only begin traversal for root nodes.
        if (r->parent_.expired())
        {
            adjust_durations_dfs(r);
            min_start_time = adjust_timestamps_dfs(
                r,
                std::max(
                    r->tag() != EventType::Vulkan ? r->start_time_ns_
                                                  : std::numeric_limits<int64_t>::min(),
                    min_start_time));
        }
    }
}
}  // namespace

std::pair<
    std::vector<std::shared_ptr<Result>>,
    std::unique_ptr<quarisma::profiler_impl::impl::kineto::ActivityTraceWrapper>>
RecordQueue::getRecords(
    std::function<quarisma::time_t(quarisma::approx_time_t)> time_converter,
    uint64_t                                                 start_time_ns,
    uint64_t                                                 end_time_ns)
{
    auto converter = [&](quarisma::approx_time_t t)
    {
        return t == std::numeric_limits<quarisma::approx_time_t>::min()
                   ? std::numeric_limits<quarisma::time_t>::min()
                   : time_converter(t);
    };

    // Lambda that checks that only the right side of the base intersects with
    // ev_start and ev_end
    auto right_intersection_only = [&](ProfilerStepInfo base, int64_t ev_start, int64_t ev_end)
    {
        return (base.start_time_ns < ev_start) &&
               (base.end_time_ns <= ev_end && base.end_time_ns > ev_start);
    };
    std::vector<std::shared_ptr<Result>>        out;
    std::vector<python_tracer::CompressedEvent> python_enters;
    std::vector<ProfilerStepInfo>               step_info;
    long unsigned int                           step_idx = 0;
    for (auto& subqueue_it : sub_queues_)
    {
        auto& queue       = *subqueue_it.second;
        auto  materialize = [&](auto& events)
        {
            for (auto& i : events)
            {
                quarisma::time_t start_time_ns = 0;
                if constexpr (std::is_same_v<
                                  std::remove_reference_t<decltype(i)>,
                                  ExtraFields<EventType::Backend>>)
                {
                    start_time_ns = i.start_time_us_ * 1000;
                }
                else
                {
                    start_time_ns = converter(i.start_time_);
                }
                out.emplace_back(Result::create(
                    /*start_time_ns_=*/start_time_ns,
                    /*start_tid_=*/queue.tid(),
                    /*kineto_info_=*/queue.kineto_info(),
                    /*extra_fields_=*/std::move(i)));
            }
            events.clear();
        };

        queue.torch_ops_.materialize(out, step_info, converter, queue.tid(), queue.kineto_info());
        materialize(queue.backend_events_);
        materialize_vulkan(out, queue.vulkan_events_, converter, queue.tid(), queue.kineto_info());
        for (auto& i : queue.allocations_)
        {
            out.emplace_back(Result::create(
                /*start_time_ns_=*/converter(i.start_time_),
                /*start_tid_=*/queue.tid(),
                /*kineto_info_=*/queue.kineto_info(),
                /*extra_fields_=*/ExtraFields<EventType::Allocation>(i)));
        }
        queue.allocations_.clear();
        materialize(queue.ooms_);

#if 0
        // Disabled: PythonGC event type not in variant (commented out in collection.h)
        std::optional<int64_t> pending_start;
        for (auto& e : queue.pythongc_)
        {
            if (e.first.find("start") != std::string::npos)
            {
                pending_start = e.second;
            }
            else if (e.first.find("stop") != std::string::npos)
            {
                if (pending_start.has_value())
                {
                    out.emplace_back(
                        Result::create(
                            /*start_time_ns_=*/converter(pending_start.value()),
                            /*start_tid_=*/queue.tid(),
                            /*kineto_info_=*/queue.kineto_info(),
                            /*extra_fields_=*/
                            // NOLINTNEXTLINE
                            ExtraFields<EventType::PythonGC>{
                                e.first, converter(e.second) - converter(pending_start.value())}));
                    pending_start.reset();
                }
                else
                {
                    // Handle the case where "stop" is found without a matching "start"
                    // For example, you might want to log a warning or take other action:
                    //LOG(WARNING) << R"("stop" event found without a matching "start": )" << e.first;
                }
            }
        }

        for (auto& i : queue.py_calls_)
        {
            python_enters.push_back(
                {i.first, queue.tid(), queue.kineto_info(), converter(i.second)});
        }
#endif
    }

    if (python_tracer_)
    {
        std::vector<std::shared_ptr<quarisma::profiler_impl::impl::Result>> ev;
        try
        {
            ev = python_tracer_->getEvents(
                converter, python_enters, static_cast<quarisma::time_t>(end_time_ns));
        }
        catch (std::exception&)
        {
            // Normally addKinetoEvents() below will stop the trace - but if an
            // exception happens here then the events will never be stopped and future
            // runs will be broken - so make sure to stopTrace() if we see an
            // exception.
            quarisma::profiler_impl::impl::kineto::stopTrace();
            throw;
        }
        // Placeholder for if we run out of ProfilerStep annotations
        ProfilerStepInfo const defaultStep = {LLONG_MAX, LLONG_MAX, 0};
        ProfilerStepInfo step = step_idx < step_info.size() ? step_info[step_idx] : defaultStep;
        for (const auto& i : ev)
        {
            // Only adjust timestamps if experimental config is enabled
            if (config_.experimental_config.adjust_profiler_step)
            {
                // If event has start time after step end time we can continue to the
                // next step
                while (i->start_time_ns_ > step.end_time_ns)
                {
                    step_idx++;
                    step = step_idx < step_info.size() ? step_info[step_idx] : defaultStep;
                }
                // If Step annotation starts before event and ends before event ends
                // with intersection then we move the lefthand side of the step
                // annotation to the event start time
                if (right_intersection_only(step, i->start_time_ns_, i->endTimeNS()))
                {
                    // NOLINTNEXTLINE(facebook-hte-LocalUncheckedArrayBounds)
                    auto const& currStepRes     = out[step.out_idx];
                    currStepRes->start_time_ns_ = i->start_time_ns_ + 1;
                    step_idx++;
                    step = step_idx < step_info.size() ? step_info[step_idx] : defaultStep;
                }
            }
            out.push_back(i);
        }
        python_tracer_.reset();
    }

    if (config_.experimental_config.adjust_timestamps)
    {
        std::stable_sort(
            out.begin(),
            out.end(),
            [](const auto& a, const auto& b) { return a->start_time_ns_ < b->start_time_ns_; });
        build_tree(out);
        adjust_timestamps(out);
        for (auto& r : out)
        {
            r->parent_.reset();
            // Reset these so that second build_tree can happen
            r->finished_ = false;
            r->children_.clear();
        }
    }

    auto trace = addKinetoEvents(out, start_time_ns, end_time_ns, config_);

    std::stable_sort(
        out.begin(),
        out.end(),
        [](const auto& a, const auto& b) { return a->start_time_ns_ < b->start_time_ns_; });

    if (config_.report_input_shapes && config_.profile_memory)
    {
        calculateUniqueTensorIDs(out);
    }

    build_tree(out);
    return {out, std::move(trace)};
}

namespace
{
std::function<bool()>& record_concrete_inputs_enabled_fn()
{
    static std::function<bool()> fn = []() { return true; };
    return fn;
}
}  // namespace

bool get_record_concrete_inputs_enabled()
{
    return record_concrete_inputs_enabled_fn()();
}

void set_record_concrete_inputs_enabled_fn(std::function<bool()> fn)
{
    record_concrete_inputs_enabled_fn() = std::move(fn);
}

void set_record_concrete_inputs_enabled_val(bool val)
{
    record_concrete_inputs_enabled_fn() = [val]() { return val; };
}

namespace
{
std::function<bool()>& fwd_bwd_enabled_fn()
{
    static std::function<bool()> fn = []() { return true; };
    return fn;
}
}  // namespace

bool get_fwd_bwd_enabled()
{
    return fwd_bwd_enabled_fn()();
}

void set_fwd_bwd_enabled_fn(std::function<bool()> fn)
{
    fwd_bwd_enabled_fn() = std::move(fn);
}

void set_fwd_bwd_enabled_val(bool val)
{
    fwd_bwd_enabled_fn() = [val]() { return val; };
}

namespace
{
std::function<bool()>& cuda_sync_enabled_fn()
{
    static std::function<bool()> fn = []() { return false; };
    return fn;
}
}  // namespace

bool get_cuda_sync_enabled()
{
    return cuda_sync_enabled_fn()();
}

void set_cuda_sync_enabled_fn(std::function<bool()> fn)
{
    cuda_sync_enabled_fn() = std::move(fn);
}

void set_cuda_sync_enabled_val(bool val)
{
    cuda_sync_enabled_fn() = [val]() { return val; };
}

namespace
{
std::function<bool()>& record_tensor_addrs_enabled()
{
    static std::function<bool()> fn = []() { return false; };
    return fn;
}
}  // namespace

bool get_record_tensor_addrs_enabled()
{
    static std::optional<bool> cached_record_tensor_addrs_enabled;
    if (!cached_record_tensor_addrs_enabled.has_value())
    {
        cached_record_tensor_addrs_enabled = record_tensor_addrs_enabled()();
    }
    return cached_record_tensor_addrs_enabled.value();
}

void set_record_tensor_addrs_enabled_fn(std::function<bool()> fn)
{
    record_tensor_addrs_enabled() = std::move(fn);
}

void set_record_tensor_addrs_enabled_val(bool val)
{
    record_tensor_addrs_enabled() = [val]() { return val; };
}
}  // namespace quarisma::profiler_impl::impl
