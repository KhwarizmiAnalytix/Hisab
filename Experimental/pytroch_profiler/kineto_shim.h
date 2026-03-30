#pragma once

#include <ActivityType.h>

#include <memory>
#include <string>

#include "api.h"
#include "common/export.h"

#if QUARISMA_HAS_KINETO
// Forward declarations so we don't have to include `libkineto.h` in a header.
namespace libkineto
{
class GenericTraceActivity;
struct CpuTraceBuffer;
class ActivityTraceInterface;
}  // namespace libkineto
#endif

namespace quarisma
{
namespace profiler
{

#if QUARISMA_HAS_KINETO
constexpr bool kKinetoAvailable{true};
#else
constexpr bool kKinetoAvailable{false};
#endif

namespace impl::kineto
{

// ----------------------------------------------------------------------------
// -- Interface (Does not require Kineto) -------------------------------------
// ----------------------------------------------------------------------------
struct DeviceAndResource
{
    int32_t device;
    int32_t resource;
};
const DeviceAndResource kineto_ids();

#if QUARISMA_HAS_KINETO
using trace_t           = libkineto::CpuTraceBuffer;
using interface_trace_t = libkineto::ActivityTraceInterface;
using activity_t        = libkineto::GenericTraceActivity;
#else
struct DummyTraceBuffer
{
};
struct DummyTraceInterface
{
};

using trace_t           = DummyTraceBuffer;
using interface_trace_t = DummyTraceBuffer;
struct activity_t;
#endif  // QUARISMA_HAS_KINETO

void addMetadata(activity_t* activity, const std::string& key, const std::string& value);

// Wraps: libkineto::CpuTraceBuffer
struct TraceWrapper
{
    TraceWrapper(const int64_t start_time, const std::string& name);

    // The caller is expected to hold a mutex when calling `addCPUActivity`.
    activity_t* addCPUActivity(
        const std::string&            name,
        const libkineto::ActivityType type,
        const DeviceAndResource       device_and_resource,
        const uint64_t                correlation_id,
        const int64_t                 start_time,
        const int64_t                 end_time);

    void transferCpuTrace(int64_t end_time);

    explicit operator bool() const;

    std::unique_ptr<trace_t>& get() { return cpu_trace_; }

private:
    std::unique_ptr<trace_t> cpu_trace_;
};

// Wraps libkineto::ActivityTraceInterface
struct ActivityTraceWrapper
{
    explicit ActivityTraceWrapper(std::unique_ptr<interface_trace_t>&& trace);
    ActivityTraceWrapper() = default;
    explicit operator bool() const;
    void     save(const std::string& path);

    const std::unique_ptr<interface_trace_t>& get() { return trace_; }

private:
    std::unique_ptr<interface_trace_t> trace_;
#if QUARISMA_HAS_KINETO
    bool saved_ = false;  // Kineto's save is destructive
#endif
};

using ActivitySet = std::set<quarisma::autograd::profiler::ActivityType>;
void prepareTrace(
    const bool                                          cpuOnly,
    const ActivitySet&                                  activities,
    const quarisma::profiler::impl::ExperimentalConfig& config,
    const std::string&                                  trace_id = "");

void                 toggleCollectionDynamic(const bool enable);
void                 startTrace();
ActivityTraceWrapper stopTrace();
void                 pushCorrelationId(uint64_t correlation_id);
void                 pushUserCorrelationId(uint64_t correlation_id);
void                 popCorrelationId();
void                 popUserCorrelationId();
void                 recordThreadInfo();
bool                 collectivesProfilerExists();

void logInvariantViolation(
    const std::string& assertion,
    const std::string& error,
    const std::string& profile_id,
    const std::string& group_profile_id);

}  // namespace impl::kineto

}  // namespace profiler

namespace autograd::profiler
{
quarisma::device_enum deviceTypeFromActivity(libkineto::ActivityType activity_type);

QUARISMA_API void addMetadataJson(const std::string& key, const std::string& value);

QUARISMA_API void profilerStep();

}  // namespace autograd::profiler

}  // namespace quarisma
