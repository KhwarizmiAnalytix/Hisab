#include "bespoke/kineto/kineto_shim.h"

#include <type_traits>

#include "bespoke/common/collection.h"

#if QUARISMA_HAS_KINETO
#include <libkineto.h>
#endif

// TODO: Missing Quarisma dependencies - original includes were:
#include "util/env.h"
// These are Quarisma-specific headers not available in Quarisma

namespace quarisma
{

namespace profiler::impl::kineto
{

// Here lies pain and `#if QUARISMA_HAS_KINETO`

#if QUARISMA_HAS_KINETO
namespace
{
const std::set<libkineto::ActivityType> kCpuTypes{
    libkineto::ActivityType::CPU_OP,
    libkineto::ActivityType::CPU_INSTANT_EVENT,
    libkineto::ActivityType::USER_ANNOTATION,
    libkineto::ActivityType::EXTERNAL_CORRELATION,
    libkineto::ActivityType::XPU_RUNTIME,
    libkineto::ActivityType::CUDA_RUNTIME,
    libkineto::ActivityType::CUDA_DRIVER,
    libkineto::ActivityType::PYTHON_FUNCTION,
    libkineto::ActivityType::PRIVATEUSE1_RUNTIME,
    libkineto::ActivityType::PRIVATEUSE1_DRIVER,
};

const std::set<libkineto::ActivityType> kCudaTypes = {
    libkineto::ActivityType::GPU_MEMCPY,
    libkineto::ActivityType::GPU_MEMSET,
    libkineto::ActivityType::GPU_USER_ANNOTATION,
    libkineto::ActivityType::CONCURRENT_KERNEL,
    // CUDA_RUNTIME appears in both kCpuTypes and kCudaTypes.
    libkineto::ActivityType::CUDA_RUNTIME,
    libkineto::ActivityType::CUDA_DRIVER,
    libkineto::ActivityType::OVERHEAD,
};
const std::set<libkineto::ActivityType> kXpuTypes = {
    libkineto::ActivityType::GPU_MEMCPY,
    libkineto::ActivityType::GPU_MEMSET,
    libkineto::ActivityType::CONCURRENT_KERNEL,
    // XPU_RUNTIME appears in both kCpuTypes and kXpuTypes.
    libkineto::ActivityType::XPU_RUNTIME,
};
const std::set<libkineto::ActivityType> kMtiaTypes = {
    libkineto::ActivityType::MTIA_CCP_EVENTS,
    libkineto::ActivityType::MTIA_RUNTIME,
    libkineto::ActivityType::MTIA_INSIGHT,
};
const std::set<libkineto::ActivityType> hpuTypes = {
    libkineto::ActivityType::HPU_OP,
};
const std::set<libkineto::ActivityType> kPrivateUse1Types = {
    libkineto::ActivityType::GPU_MEMCPY,
    libkineto::ActivityType::GPU_MEMSET,
    libkineto::ActivityType::GPU_USER_ANNOTATION,
    libkineto::ActivityType::CONCURRENT_KERNEL,
    // PRIVATEUSE1_RUNTIME appears in both kCpuTypes and kPrivateUse1Types.
    libkineto::ActivityType::PRIVATEUSE1_RUNTIME,
    libkineto::ActivityType::PRIVATEUSE1_DRIVER,
};
}  // namespace
#endif  // QUARISMA_HAS_KINETO

static_assert(
    std::is_trivial_v<DeviceAndResource>, "Kineto specific details should be in `kineto_ids`.");

DeviceAndResource kineto_ids()
{
#if QUARISMA_HAS_KINETO
    return {
        /*device=*/libkineto::processId(),
        /*resource=*/libkineto::systemThreadId()};
#else
    return {};
#endif  // QUARISMA_HAS_KINETO
}

void addMetadata(activity_t* activity, const std::string& key, const std::string& value)
{
#if QUARISMA_HAS_KINETO
    // Suppress false positive from clang static analyzer in fmt library
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wanalyzer-optin.cplusplus.UninitializedObject"
#endif
    activity->addMetadata(key, value);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif  // QUARISMA_HAS_KINETO
}

TraceWrapper::TraceWrapper(const int64_t start_time, const std::string& name)
#if QUARISMA_HAS_KINETO
    : cpu_trace_(std::make_unique<libkineto::CpuTraceBuffer>())
{
    cpu_trace_->span.startTime = start_time;
    cpu_trace_->gpuOpCount     = -1;
    cpu_trace_->span.name      = name;
}
#else
{
}
#endif  // QUARISMA_HAS_KINETO

activity_t* TraceWrapper::addCPUActivity(
    const std::string&            name,
    const libkineto::ActivityType type,
    const DeviceAndResource       device_and_resource,
    const uint64_t                correlation_id,
    const int64_t                 start_time,
    const int64_t                 end_time)
{
#if QUARISMA_HAS_KINETO
    // QUARISMA_CHECK((bool)(*this), "Cannot add event to non-existent trace.");
    cpu_trace_->emplace_activity(cpu_trace_->span, type, name);
    auto& act     = libkineto::CpuTraceBuffer::toRef(cpu_trace_->activities.back());
    act.device    = device_and_resource.device;
    act.resource  = device_and_resource.resource;
    act.id        = static_cast<int32_t>(correlation_id);
    act.startTime = start_time;
    if (type != libkineto::ActivityType::CPU_INSTANT_EVENT)
    {
        act.endTime = end_time;
    }
    return cpu_trace_->activities.back().get();
#else
    return nullptr;
#endif  // QUARISMA_HAS_KINETO
}

void TraceWrapper::transferCpuTrace(int64_t end_time)
{
#if QUARISMA_HAS_KINETO
    cpu_trace_->span.endTime = end_time;
    libkineto::api().activityProfiler().transferCpuTrace(std::move(cpu_trace_));
#endif  // QUARISMA_HAS_KINETO
}

TraceWrapper::operator bool() const
{
#if QUARISMA_HAS_KINETO
    return cpu_trace_ != nullptr;
#else
    return false;
#endif  // QUARISMA_HAS_KINETO
}

ActivityTraceWrapper::ActivityTraceWrapper(std::unique_ptr<interface_trace_t>&& trace)
    : trace_(std::move(trace))
{
}

ActivityTraceWrapper::operator bool() const
{
#if QUARISMA_HAS_KINETO
    return trace_ != nullptr;
#else
    return false;
#endif  // QUARISMA_HAS_KINETO
}

void ActivityTraceWrapper::save(const std::string& path)
{
#if QUARISMA_HAS_KINETO
    // QUARISMA_CHECK(!saved_, "Trace is already saved.");
    // cppcheck-suppress unknownMacro
    // QUARISMA_CHECK(trace_ != nullptr, "Missing trace.")
    trace_->save(path);
    saved_ = true;
#else
    QUARISMA_THROW(
        "Saving a trace requires using quarisma.profiler with Kineto support "
        "(QUARISMA_HAS_KINETO=1)");
#endif  // QUARISMA_HAS_KINETO
}

namespace
{
// Handles processing of Experimental Config options for Kineto
class ExperimentalConfigWrapper
{
public:
    explicit ExperimentalConfigWrapper(const quarisma::profiler::impl::ExperimentalConfig& config)
        : config_(config)
    {
    }

    bool assertValid() { return !config_.profiler_metrics.empty(); }

    void prepareTraceWithExperimentalOptions(std::set<libkineto::ActivityType>&& enabled_activities)
    {
        std::set<libkineto::ActivityType> k_activities = std::move(enabled_activities);
#if QUARISMA_HAS_KINETO
        k_activities.insert(libkineto::ActivityType::CUDA_PROFILER_RANGE);

        // Add CPU activities if we are measuring per kernel ranges
        if (config_.profiler_measure_per_kernel)
        {
            k_activities.insert(kCpuTypes.begin(), kCpuTypes.end());
        }

        const size_t      num_metrics = config_.profiler_metrics.size();
        std::stringstream configss;

#if 0
        // Disabled: Logging macros not available in profiler-only build.
        LOG(INFO) << "CUPTI profiler metrics size = " << num_metrics;
#endif

        configss << "ACTIVITIES_WARMUP_PERIOD_SECS=0\n"
                 << "CUPTI_PROFILER_METRICS=";

        for (size_t i = 0; i < num_metrics; i++)
        {
            configss << config_.profiler_metrics[i];
            if (num_metrics > 1 && i < (num_metrics - 1))
            {
                configss << ",";
            }
        }
        configss << "\nCUPTI_PROFILER_ENABLE_PER_KERNEL="
                 << (config_.profiler_measure_per_kernel ? "true" : "false") << "\n";
        configss << "CUSTOM_CONFIG=" << config_.custom_profiler_config << "\n";
#if 0
        // Disabled: Logging macros not available in profiler-only build.
        LOG(INFO) << "Generated config = " << configss.str();
#endif

        libkineto::api().activityProfiler().prepareTrace(k_activities, configss.str());
#endif  // QUARISMA_HAS_KINETO
    }

private:
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    const quarisma::profiler::impl::ExperimentalConfig& config_;
};
}  // namespace

bool collectivesProfilerExists()
{
#ifdef KINETO_HAS_HCCL_PROFILER
    return true;
#endif
    const auto val = quarisma::utils::get_env("QUARISMA_PROFILER_ENABLE_COLLECTIVE_PROFILING");
    return val == "1";
}

#if QUARISMA_HAS_KINETO
static std::string setTraceID(const std::string& trace_id)
{
    if (trace_id.empty())
    {
        return "";
    }
    std::stringstream configss;
    configss << "REQUEST_TRACE_ID=" << trace_id << "\n";
    configss << "REQUEST_GROUP_TRACE_ID=" << trace_id << "\n";
    return configss.str();
}

static std::string appendCustomConfig(
    const std::string& config, const std::string& custom_profiler_config)
{
    if (custom_profiler_config.empty())
    {
        return config;
    }
    std::stringstream configss;
    configss << config;
    configss << "CUSTOM_CONFIG=" << custom_profiler_config << "\n";
    return configss.str();
}
#endif

void prepareTrace(
    const bool                                          cpuOnly,
    const ActivitySet&                                  activities,
    const quarisma::profiler::impl::ExperimentalConfig& config,
    const std::string&                                  trace_id)
{
#if QUARISMA_HAS_KINETO
    libkineto::api().resetKinetoTLS();
    if (!libkineto::api().isProfilerRegistered())
    {
        libkineto_init(/*cpuOnly=*/cpuOnly, /*logOnError=*/true);
        libkineto::api().suppressLogMessages();
    }

    if (!libkineto::api().isProfilerInitialized())
    {
        libkineto::api().initProfilerIfRegistered();
    }

    std::set<libkineto::ActivityType> k_activities;
    bool const                        has_cpu_activity =
        activities.count(quarisma::autograd::profiler::ActivityType::CPU) > 0;  //NOLINT

    if (has_cpu_activity)
    {
        k_activities.insert(kCpuTypes.begin(), kCpuTypes.end());
    }
    if (activities.count(quarisma::autograd::profiler::ActivityType::XPU) > 0)  //NOLINT
    {
        k_activities.insert(kXpuTypes.begin(), kXpuTypes.end());
    }
    if (activities.count(quarisma::autograd::profiler::ActivityType::MTIA) > 0)  //NOLINT
    {
        k_activities.insert(kMtiaTypes.begin(), kMtiaTypes.end());
    }
    if (activities.count(quarisma::autograd::profiler::ActivityType::HPU) > 0)  //NOLINT
    {
        k_activities.insert(hpuTypes.begin(), hpuTypes.end());
    }
    if (activities.count(quarisma::autograd::profiler::ActivityType::CUDA) > 0)  //NOLINT
    {
        k_activities.insert(kCudaTypes.begin(), kCudaTypes.end());
        if (config.enable_cuda_sync_events || get_cuda_sync_enabled())
        {
#if 0
            // Disabled: Logging macros not available in profiler-only build.
            LOG(INFO) << "Enabling CUDA Sync Events";
#endif
            k_activities.insert(libkineto::ActivityType::CUDA_SYNC);
        }
    }
    if (collectivesProfilerExists())
    {
        k_activities.insert(libkineto::ActivityType::COLLECTIVE_COMM);
    }
    if (activities.count(quarisma::autograd::profiler::ActivityType::PrivateUse1) > 0)  //NOLINT
    {
        k_activities.insert(kPrivateUse1Types.begin(), kPrivateUse1Types.end());
    }

    ExperimentalConfigWrapper configWrap(config);

    // Experimental Configuration options are present
    if (config && configWrap.assertValid())
    {
        configWrap.prepareTraceWithExperimentalOptions(std::move(k_activities));
        return;
    }

    const std::string traceIdStr = setTraceID(trace_id);
    const std::string configStr  = appendCustomConfig(traceIdStr, config.custom_profiler_config);

    libkineto::api().activityProfiler().prepareTrace(k_activities, configStr);
#endif  // QUARISMA_HAS_KINETO
}

void toggleCollectionDynamic(const bool enable)
{
#if QUARISMA_HAS_KINETO
    // TODO: We may want to consider adding another input arg for this function
    // if we want to support turning off certain devices and keeping others on.
    // For now, we can keep it simple at have it turn off all tracing of "CUDA"
    // devices
    libkineto::api().activityProfiler().toggleCollectionDynamic(enable);
#endif  // QUARISMA_HAS_KINETO
}

void startTrace()
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().startTrace();
#endif  // QUARISMA_HAS_KINETO
}

ActivityTraceWrapper stopTrace()
{
    return ActivityTraceWrapper{
#if QUARISMA_HAS_KINETO
        libkineto::api().activityProfiler().stopTrace()
#else
        std::make_unique<interface_trace_t>()
#endif  // QUARISMA_HAS_KINETO
    };
}

void pushCorrelationId(uint64_t correlation_id)
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().pushCorrelationId(correlation_id);
#endif  // QUARISMA_HAS_KINETO
}

void pushUserCorrelationId(uint64_t correlation_id)
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().pushUserCorrelationId(correlation_id);
#endif  // QUARISMA_HAS_KINETO
}

void popCorrelationId()
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().popCorrelationId();
#endif  // QUARISMA_HAS_KINETO
}

void popUserCorrelationId()
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().popUserCorrelationId();
#endif  // QUARISMA_HAS_KINETO
}

void recordThreadInfo()
{
#if QUARISMA_HAS_KINETO
    libkineto::api().activityProfiler().recordThreadInfo();
#endif  // QUARISMA_HAS_KINETO
}

void logInvariantViolation(
    const std::string& assertion,
    const std::string& error,
    const std::string& profile_id,
    const std::string& group_profile_id)
{
#if QUARISMA_HAS_KINETO
    if (libkineto::api().isProfilerInitialized())
    {
        libkineto::api().activityProfiler().logInvariantViolation(
            profile_id, assertion, error, group_profile_id);
    }
#endif  // QUARISMA_HAS_KINETO
}

}  // namespace profiler::impl::kineto

namespace autograd::profiler
{
quarisma::device_enum deviceTypeFromActivity(
    quarisma::profiler::impl::kineto::activity_type_t activity_type)
{
    // fallthrough
    switch (activity_type)
    {
    case libkineto::ActivityType::GPU_MEMCPY:
    case libkineto::ActivityType::GPU_MEMSET:
    case libkineto::ActivityType::CONCURRENT_KERNEL:
    case libkineto::ActivityType::CUDA_SYNC:
    case libkineto::ActivityType::GPU_USER_ANNOTATION:
    case libkineto::ActivityType::CUDA_PROFILER_RANGE:
    {
        // PrivateUse1 kineto backend reuse above ActivityTypes,
        // If PrivateUse1 backend enabled, this should return
        // quarisma::device_enum::PrivateUse1.
#if 0
        // Disabled: get_privateuse1_backend() not available in profiler-only build.
        quarisma::device_enum device_type = []()
        {
            if (quarisma::get_privateuse1_backend() != "privateuseone")
            {
                return quarisma::device_enum::PrivateUse1;
            }
            return quarisma::device_enum::CUDA;
        }();
#else
        quarisma::device_enum const device_type = quarisma::device_enum::CUDA;
#endif
        return device_type;
    }
    // TODO: T151322015
#if 0
    // Disabled: MTIA device type not available in profiler-only build.
    case libkineto::ActivityType::MTIA_CCP_EVENTS:
    case libkineto::ActivityType::MTIA_INSIGHT:
    {
        // PrivateUse1 kineto backend reuse above ActivityTypes,
        // If PrivateUse1 backend enabled, this should return
        // quarisma::device_enum::PrivateUse1.
        quarisma::device_enum device_type = []()
        {
            if (quarisma::get_privateuse1_backend() != "privateuseone")
            {
                return quarisma::device_enum::PrivateUse1;
            }
            return quarisma::device_enum::MTIA;
        }();
        return device_type;
    }
#endif
#if 0
    // Disabled: HPU device type not available in profiler-only build.
    case libkineto::ActivityType::HPU_OP:
        return quarisma::device_enum::HPU;
#endif
    case libkineto::ActivityType::CPU_OP:
    case libkineto::ActivityType::USER_ANNOTATION:
    case libkineto::ActivityType::EXTERNAL_CORRELATION:
    case libkineto::ActivityType::CUDA_RUNTIME:
    case libkineto::ActivityType::XPU_RUNTIME:
    case libkineto::ActivityType::CPU_INSTANT_EVENT:
    case libkineto::ActivityType::GLOW_RUNTIME:
    case libkineto::ActivityType::MTIA_RUNTIME:
    case libkineto::ActivityType::PYTHON_FUNCTION:
    case libkineto::ActivityType::CUDA_DRIVER:
    case libkineto::ActivityType::PRIVATEUSE1_RUNTIME:
    case libkineto::ActivityType::PRIVATEUSE1_DRIVER:
    case libkineto::ActivityType::OVERHEAD:
        return quarisma::device_enum::CPU;
    default:
    {
#if 0
        // Disabled: QUARISMA_LOG_WARNING macro not available in profiler-only build.
        QUARISMA_LOG_WARNING("Unknown activity type (", (uint8_t)activity_type, "), assuming CPU device");
#endif
        return quarisma::device_enum::CPU;
    }
    }
}

void addMetadataJson(const std::string& key, const std::string& value)
{
#if QUARISMA_HAS_KINETO
    if (libkineto::api().isProfilerInitialized())
    {
        libkineto::api().activityProfiler().addMetadata(key, value);
    }
    else
    {
        QUARISMA_LOG_WARNING("Profiler is not initialized: skipping profiling metadata");
    }
#else
    QUARISMA_LOG_WARNING(
        "Adding profiling metadata requires using "
        "quarisma.profiler with Kineto support (QUARISMA_HAS_KINETO=1)");
#endif  // QUARISMA_HAS_KINETO
}

void profilerStep()
{
#if QUARISMA_HAS_KINETO
    libkineto::api().initProfilerIfRegistered();

    if (libkineto::api().isProfilerInitialized())
    {
        libkineto::api().activityProfiler().step();
    }
    else
    {
        QUARISMA_LOG_WARNING("Profiler is not initialized: skipping step() invocation");
    }
#endif  // QUARISMA_HAS_KINETO
}

}  // namespace autograd::profiler

}  // namespace quarisma
