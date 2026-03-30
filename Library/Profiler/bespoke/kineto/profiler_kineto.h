#pragma once

#include <set>
#include <string>
#include <vector>

#include "bespoke/base/base.h"
#include "bespoke/common/api.h"
#include "bespoke/common/events.h"
#include "bespoke/common/util.h"

namespace quarisma
{
constexpr bool hasCUDA()
{
    return QUARISMA_HAS_CUDA == 1;
}

namespace profiler::impl
{
struct Result;
namespace kineto
{
struct ActivityTraceWrapper;
}  // namespace kineto
}  // namespace profiler::impl

namespace autograd::profiler
{
using experimental_event_t = std::shared_ptr<quarisma::profiler::impl::Result>;
using extra_meta_t         = std::unordered_map<std::string, std::string>;

struct PROFILER_VISIBILITY KinetoEvent
{
    PROFILER_API KinetoEvent(
        const std::shared_ptr<const quarisma::profiler::impl::Result>& /*result*/,
        const bool verbose);

    PROFILER_API uint64_t startThreadId() const;
    PROFILER_API uint64_t endThreadId() const;
    PROFILER_API uint8_t  activityType() const;
    PROFILER_API uint64_t fwdThreadId() const;
    PROFILER_API bool     hasShapes() const;
    PROFILER_API quarisma::array_ref<std::vector<int64_t>> shapes() const;
    PROFILER_API bool                                      hasTypes() const;
    PROFILER_API quarisma::array_ref<std::string> dtypes() const;
    PROFILER_API bool                             hasConcreteInputs() const;
    PROFILER_API quarisma::array_ref<quarisma::IValue> concreteInputs() const;
    PROFILER_API bool                                  hasKwinputs() const;
    PROFILER_API bool                                  isHiddenEvent() const;
    PROFILER_API std::unordered_map<std::string, quarisma::IValue> kwinputs() const;
    PROFILER_API uint64_t                                          flops() const;
    PROFILER_API int64_t                                           sequenceNr() const;
    PROFILER_API bool                                              hasStack() const;
    PROFILER_API quarisma::array_ref<std::string> stack() const;
    PROFILER_API uint8_t                          scope() const;
    PROFILER_API bool                             hasModuleHierarchy() const;
    PROFILER_API quarisma::array_ref<std::string> moduleHierarchy() const;
    PROFILER_API int64_t                          debugHandle() const;
    PROFILER_API std::string name() const;
    PROFILER_API std::string overload_name() const;
    PROFILER_API quarisma::device_enum deviceType() const;
    PROFILER_API int                   deviceIndex() const;
    PROFILER_API int64_t               nBytes() const;
    PROFILER_API uint64_t              startNs() const;
    PROFILER_API uint64_t              endNs() const;
    PROFILER_API uint64_t              durationNs() const;
    PROFILER_API bool                  isAsync() const;
    PROFILER_API uint64_t              correlationId() const;
    PROFILER_API uint64_t              linkedCorrelationId() const;
    PROFILER_API int64_t               deviceResourceId() const;
    PROFILER_API std::string backend() const;
    static PROFILER_API bool isPythonFunction();
    PROFILER_API int64_t     cudaElapsedUs() const;
    PROFILER_API int64_t     privateuse1ElapsedUs() const;
    PROFILER_API void getPerfEventCounters(quarisma::profiler::perf_counters_t& /*in*/) const;
    PROFILER_API extra_meta_t extraMeta() const;
    PROFILER_API std::string metadataJson() const;

private:
    quarisma::profiler::impl::ProfilerVoidEventStub fallbackStart() const;
    quarisma::profiler::impl::ProfilerVoidEventStub fallbackEnd() const;

    std::shared_ptr<const quarisma::profiler::impl::Result> result_;
    std::vector<std::string>                                python_stack_;

    // Copy fields from result so we can return ArrayRefs.
    std::vector<std::vector<int64_t>>                 shapes_;
    std::vector<std::string>                          dtypes_;
    std::vector<quarisma::IValue>                     concrete_inputs_;
    std::unordered_map<std::string, quarisma::IValue> kwinputs_;
};

// Consolidating events returned directly from Kineto
// with events manually created by us (e.g. start/stop marks,
// memory allocation events)
struct PROFILER_VISIBILITY ProfilerResult
{
    PROFILER_API ProfilerResult();
    PROFILER_API ProfilerResult(
        uint64_t                                                                  start_time,
        std::vector<KinetoEvent>                                                  events,
        std::unique_ptr<quarisma::profiler::impl::kineto::ActivityTraceWrapper>&& trace,
        std::vector<experimental_event_t>&&                                       event_tree);
    PROFILER_API ~ProfilerResult();

    uint64_t trace_start_ns() const { return trace_start_ns_; }

    const std::vector<KinetoEvent>& events() const { return events_; }

    const std::vector<experimental_event_t>& event_tree() const { return event_tree_; }

    PROFILER_API void save(const std::string& path);

private:
    uint64_t                                                                trace_start_ns_ = 0;
    std::vector<KinetoEvent>                                                events_;
    std::unique_ptr<quarisma::profiler::impl::kineto::ActivityTraceWrapper> trace_;
    std::vector<experimental_event_t>                                       event_tree_;
};

/*
 * This API is used by backends to record latency of events that
 * happened in the backend but were not visible to pytorch runtime.
 * For example, if part of the model is lowered to a dsp backend, then
 * the execution of that part of the model is delegated to the backend.
 * When backend finishes execution it has an option to provide profiling
 * information (latency only at the moment) corresponding to different operators
 * that were executed in the backend.
 * When such events are recorded by backend using this API, the event
 * records will be collected by active kineto profiler. If no kineto profiler
 * is active then the event is ignored.
 * This provides us with a way to generate all the profiling information
 * for a model regardless of where model (or part of it) executed.
 * @param start_time_us: start time in us of the event
 * @param end_time_us: end time in us of the event
 * @param debug_handle: debug handle to correlate this event/op with
 * model level module/source information
 * @param scope: scope of the event, e.g. LITE_INTERPRETER, RECORD_FN etc.
 * @param event_name: name of the event, e.g. op name
 * @param backend_name: name of the backend where the event took place.
 */
PROFILER_API void reportBackendEventToActiveKinetoProfiler(
    const int64_t               start_time_us,
    const int64_t               end_time_us,
    const int64_t               debug_handle,
    const quarisma::RecordScope scope,
    const std::string&          event_name,
    const std::string&          backend_name);

PROFILER_API void enableProfiler(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities,
    const std::unordered_set<quarisma::RecordScope>&        scopes = {});

/*
 * Same as enableProfiler but with callback to do post-processing of
 * KinetoEvents.
 * enableProfilerWithEventPostProcess enables profiler to capture
 * specified activities, with specified RecordFunction scope, if any.
 * Additionally, it takes a functor that does in-place post processing of
 * events, e.g. populate stack trace or module hierarchy information lazily
 * using debug_handle.
 * Example usage is with lite interpreter that has recording scope of
 * LITE_INTERPRETER. In this case lite interpreter runtime, records debug
 * handles in RecordFunction, along with other information. Debug handles are
 * eventually passed down to KinetoEvent and recorded as part of the event.
 * KinetoEdgeCPUProfiler, in quarisma/csrc/jit/mobile/profiler_edge.cpp, enables
 * profiler using post-processing callback, via
 * enableProfilerWithEventPostProcess, that takes these debug handles and
 * generates stack trace and module hierarchy information, once profiling is
 * done.
 */
using post_process_t = std::function<void(
    /*debug_handle */ int64_t,
    /*jit_stack    */ std::vector<std::string>&,
    /*jit_modules  */ std::vector<std::string>&)>;
PROFILER_API void enableProfilerWithEventPostProcess(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities,
    post_process_t&&                                        cb,
    const std::unordered_set<quarisma::RecordScope>&        scopes = {});

PROFILER_API std::unique_ptr<ProfilerResult> disableProfiler();

PROFILER_API void prepareProfiler(
    const quarisma::profiler::impl::ProfilerConfig&         config,
    const std::set<quarisma::profiler::impl::ActivityType>& activities);

PROFILER_API void toggleCollectionDynamic(
    const bool enable, const std::set<quarisma::profiler::impl::ActivityType>& activities);

PROFILER_API void startMemoryProfile();
PROFILER_API void stopMemoryProfile();
PROFILER_API void exportMemoryProfile(const std::string& path);

/**
 * When a C++ thread really has no control over how the profiler was enabled,
 * for example, by some unreachable Python code, it can call these functions
 * to test/join/unjoin itself into the collection set of a profiler, if any.
 * Without calling these functions, the symptom may be "not seeing GPU events
 * from some child C++ threads". This is an example on how to use them,
 *
 *    using namespace quarisma::autograd::profiler;
 *    bool enabled = isProfilerEnabledInMainThread();
 *    if (enabled != saved_enabled_state) {
 *      if (enabled) {
 *        enableProfilerInChildThread();
 *      } else {
 *        disableProfilerInChildThread();
 *      }
 *      saved_enabled_state = enabled;
 *    }
 */
PROFILER_API bool isProfilerEnabledInMainThread();
PROFILER_API void enableProfilerInChildThread();
PROFILER_API void disableProfilerInChildThread();

}  // namespace autograd::profiler

namespace profiler::impl
{

// Experimental.
PROFILER_API void _reportVulkanEventToProfiler(vulkan_id_t id);

}  // namespace profiler::impl

}  // namespace quarisma
