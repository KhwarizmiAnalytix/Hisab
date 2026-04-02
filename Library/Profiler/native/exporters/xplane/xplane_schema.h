/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of Quarisma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@quarisma.co.uk
 * Website: https://www.quarisma.co.uk
 */

/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "common/profiler_macros.h"
//#include "logging/logger.h"
#include "util/string_util.h"

//#include "tsl/profiler/lib/context_types.h"

namespace quarisma
{

inline void HashCombine(std::size_t& seed, std::size_t hash)
{
    // From Boost's hash_combine
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
std::size_t HashOf(const T& val)
{
    return std::hash<T>{}(val);
}

// Variadic template for multiple arguments
template <typename T, typename... Rest>
std::size_t HashOf(const T& val, const Rest&... rest)
{
    std::size_t seed = HashOf(val);
    (HashCombine(seed, HashOf(rest)), ...);
    return seed;
}

// Specialization for pairs
template <typename T1, typename T2>
struct PairHasher
{
    std::size_t operator()(const std::pair<T1, T2>& p) const { return HashOf(p.first, p.second); }
};

// Specialization for vectors
template <typename T>
struct VectorHasher
{
    std::size_t operator()(const std::vector<T>& vec) const
    {
        std::size_t seed = vec.size();
        for (const auto& item : vec)
        {
            HashCombine(seed, HashOf(item));
        }
        return seed;
    }
};

// Specialization for strings and string_views
template <>
inline std::size_t HashOf(const std::string_view& str)
{
    return std::hash<std::string_view>{}(str);
}

template <>
inline std::size_t HashOf(const std::string& str)
{
    return std::hash<std::string>{}(str);
}
enum class ContextType : int
{
    kGeneric = 0,
    kLegacy,
    kTfExecutor,
    kTfrtExecutor,
    kSharedBatchScheduler,
    kPjRt,
    kAdaptiveSharedBatchScheduler,
    kTfrtTpuRuntime,
    kTpuEmbeddingEngine,
    kGpuLaunch,
    kBatcher,
    kTpuStream,
    kTpuLaunch,
    kPathwaysExecutor,
    kPjrtLibraryCall,
    kThreadpoolEvent,
    kLastContextType = ContextType::kTpuLaunch,
};

// In XFlow we encode context type as flow category as 6 bits.
static_assert(
    static_cast<int>(ContextType::kLastContextType) < 64, "Should have less than 64 categories.");

inline const char* GetContextTypeString(ContextType context_type)
{
    switch (context_type)
    {
    case ContextType::kGeneric:
    case ContextType::kLegacy:
        return "";
    case ContextType::kTfExecutor:
        return "tf_exec";
    case ContextType::kTfrtExecutor:
        return "tfrt_exec";
    case ContextType::kSharedBatchScheduler:
        return "batch_sched";
    case ContextType::kPjRt:
        return "PjRt";
    case ContextType::kAdaptiveSharedBatchScheduler:
        return "as_batch_sched";
    case ContextType::kTfrtTpuRuntime:
        return "tfrt_rt";
    case ContextType::kTpuEmbeddingEngine:
        return "tpu_embed";
    case ContextType::kGpuLaunch:
        return "gpu_launch";
    case ContextType::kBatcher:
        return "batcher";
    case ContextType::kTpuStream:
        return "tpu_stream";
    case ContextType::kTpuLaunch:
        return "tpu_launch";
    case ContextType::kPathwaysExecutor:
        return "pathways_exec";
    case ContextType::kPjrtLibraryCall:
        return "pjrt_library_call";
    case ContextType::kThreadpoolEvent:
        return "threadpool_event";
    }
    return "unknown";  // Fallback for any unhandled enum values
}

inline ContextType GetSafeContextType(uint32_t context_type)
{
    if (context_type > static_cast<uint32_t>(ContextType::kLastContextType))
    {
        return ContextType::kGeneric;
    }
    return static_cast<ContextType>(context_type);
}
constexpr std::string_view kHostThreadsPlaneName      = "/host:CPU";
constexpr std::string_view kGpuPlanePrefix            = "/device:GPU:";
constexpr std::string_view kTpuPlanePrefix            = "/device:TPU:";
constexpr std::string_view kTpuNonCorePlaneNamePrefix = "#Chip";
constexpr char             kTpuPlaneRegex[]           = {"/device:TPU:([0-9]*)$"};
constexpr char             kSparseCorePlaneRegex[]    = {"/device:TPU:[0-9]+ SparseCore ([0-9]+)$"};
// TODO(b/195582092): change it to /device:custom once all literals are
// migrated.
constexpr std::string_view kCustomPlanePrefix = "/device:CUSTOM:";

constexpr std::string_view kTpuRuntimePlaneName     = "/host:TPU-runtime";
constexpr std::string_view kCuptiDriverApiPlaneName = "/host:CUPTI";
constexpr std::string_view kRoctracerApiPlaneName   = "/host:ROCTRACER";
constexpr std::string_view kMetadataPlaneName       = "/host:metadata";
constexpr std::string_view kTFStreamzPlaneName      = "/host:tfstreamz";
constexpr std::string_view kPythonTracerPlaneName   = "/host:python-tracer";
constexpr std::string_view kHostCpusPlaneName       = "Host CPUs";
constexpr std::string_view kSyscallsPlaneName       = "Syscalls";

constexpr std::string_view kStepLineName                = "Steps";
constexpr std::string_view kSparseCoreStepLineName      = "Sparse Core Steps";
constexpr std::string_view kTensorFlowNameScopeLineName = "Framework Name Scope";
constexpr std::string_view kTensorFlowOpLineName        = "Framework Ops";
constexpr std::string_view kXlaModuleLineName           = "XLA Modules";
constexpr std::string_view kXlaOpLineName               = "XLA Ops";
constexpr std::string_view kXlaAsyncOpLineName          = "Async XLA Ops";
constexpr std::string_view kKernelLaunchLineName        = "Launch Stats";
constexpr std::string_view kSourceLineName              = "Source code";
constexpr std::string_view kHostOffloadOpLineName       = "Host Offload Ops";
constexpr std::string_view kCounterEventsLineName       = "_counters_";

constexpr std::string_view kDeviceVendorNvidia = "Nvidia";
constexpr std::string_view kDeviceVendorAMD    = "AMD";

constexpr std::string_view kTaskEnvPlaneName = "Task Environment";

// Max collectives to display per TPU.
// Since in most cases there will be more than 9 collectives, the last line
// contains all collectives that did not qualify to get their own line.
static constexpr uint32_t kMaxCollectivesToDisplay = 9;

// Interesting event types (i.e., TraceMe names).
enum HostEventType
{
    kFirstHostEventType   = 0,
    kUnknownHostEventType = kFirstHostEventType,
    kTraceContext,
    kSessionRun,
    kFunctionRun,
    kRunGraph,
    kRunGraphDone,
    kTfOpRun,
    kEagerKernelExecute,
    kExecutorStateProcess,
    kExecutorDoneCallback,
    kMemoryAllocation,
    kMemoryDeallocation,
    // Performance counter related.
    kRemotePerf,
    // tf.data captured function events.
    kTfDataCapturedFunctionRun,
    kTfDataCapturedFunctionRunWithBorrowedArgs,
    kTfDataCapturedFunctionRunInstantiated,
    kTfDataCapturedFunctionRunAsync,
    // Loop ops.
    kParallelForOp,
    kForeverOp,
    kWhileOpEvalCond,
    kWhileOpStartBody,
    kForOp,
    // tf.data related.
    kIteratorGetNextOp,
    kIteratorGetNextAsOptionalOp,
    kIterator,
    kDeviceInputPipelineSecondIterator,
    kPrefetchProduce,
    kPrefetchConsume,
    kParallelInterleaveProduce,
    kParallelInterleaveConsume,
    kParallelInterleaveInitializedInput,
    kParallelMapProduce,
    kParallelMapConsume,
    kMapAndBatchProduce,
    kMapAndBatchConsume,
    kParseExampleProduce,
    kParseExampleConsume,
    kParallelBatchProduce,
    kParallelBatchConsume,
    // Batching related.
    kBatchingSessionRun,
    kProcessBatch,
    kBrainSessionRun,
    kConcatInputTensors,
    kMergeInputTensors,
    kScheduleWithoutSplit,
    kScheduleWithSplit,
    kScheduleWithEagerSplit,
    kASBSQueueSchedule,
    // TFRT related.
    kTfrtModelRun,
    // Serving related.
    kServingModelRun,
    // GPU related.
    kKernelLaunch,
    kKernelExecute,
    // TPU related
    kEnqueueRequestLocked,
    kRunProgramRequest,
    kHostCallbackRequest,
    kTransferH2DRequest,
    kTransferPreprocessedH2DRequest,
    kTransferD2HRequest,
    kOnDeviceSendRequest,
    kOnDeviceRecvRequest,
    kOnDeviceSendRecvLocalRequest,
    kCustomWait,
    kOnDeviceSendRequestMulti,
    kOnDeviceRecvRequestMulti,
    kPjrtAsyncWait,
    kDoEnqueueProgram,
    kDoEnqueueContinuationProgram,
    kWriteHbm,
    kReadHbm,
    kTpuExecuteOp,
    kCompleteCallbacks,
    kTransferToDeviceIssueEvent,
    kTransferToDeviceDone,
    kTransferFromDeviceIssueEvent,
    kTransferFromDeviceDone,
    kTpuSystemExecute,
    kTpuPartitionedCallOpInitializeVarOnTpu,
    kTpuPartitionedCallOpExecuteRemote,
    kTpuPartitionedCallOpExecuteLocal,
    kLinearize,
    kDelinearize,
    kTransferBufferFromDeviceFastPath,
    kLastHostEventType = kTransferBufferFromDeviceFastPath,
};

enum StatType
{
    kFirstStatType   = 0,
    kUnknownStatType = kFirstStatType,
    // TraceMe arguments.
    kStepId,
    kDeviceOrdinal,
    kChipOrdinal,
    kNodeOrdinal,
    kModelId,
    kQueueId,
    kQueueAddr,
    kRequestId,
    kRunId,
    kReplicaId,
    kGraphType,
    kStepNum,
    kIterNum,
    kIndexOnHost,
    kAllocatorName,
    kBytesReserved,
    kBytesAllocated,
    kBytesAvailable,
    kFragmentation,
    kPeakBytesInUse,
    kRequestedBytes,
    kAllocationBytes,
    kAddress,
    kRegionType,
    kDataType,
    kTensorShapes,
    kTensorLayout,
    kKpiName,
    kKpiValue,
    kElementId,
    kParentId,
    kCoreType,
    // XPlane semantics related.
    kProducerType,
    kConsumerType,
    kProducerId,
    kConsumerId,
    kIsRoot,
    kIsAsync,
    // device_option trace arguments.
    kDeviceId,
    kDeviceTypeString,
    kContextId,
    kCorrelationId,
    // TODO(b/176137043): These "details" should differentiate between activity
    // and API event sources.
    kMemcpyDetails,
    kMemallocDetails,
    kMemFreeDetails,
    kMemsetDetails,
    kMemoryResidencyDetails,
    kNVTXRange,
    kKernelDetails,
    kStream,
    // Stats added when processing traces.
    kGroupId,
    kFlow,
    kStepName,
    kTfOp,
    kHloOp,
    kDeduplicatedName,
    kHloCategory,
    kHloModule,
    kProgramId,
    kEquation,
    kIsEager,
    kIsFunc,
    kTfFunctionCall,
    kTfFunctionTracingCount,
    kFlops,
    kModelFlops,
    kBytesAccessed,
    kMemoryAccessBreakdown,
    kSourceInfo,
    kModelName,
    kModelVersion,
    kBytesTransferred,
    kDmaQueue,
    kDcnCollectiveInfo,
    // Performance counter related.
    kRawValue,
    kScaledValue,
    kThreadId,
    kMatrixUnitUtilizationPercent,
    // XLA metadata map related.
    kHloProto,
    // device_option capability related.
    kDevCapClockRateKHz,
    // For GPU, this is the number of SMs.
    kDevCapCoreCount,
    kDevCapMemoryBandwidth,
    kDevCapMemorySize,
    kDevCapComputeCapMajor,
    kDevCapComputeCapMinor,
    kDevCapPeakTeraflopsPerSecond,
    kDevCapPeakHbmBwGigabytesPerSecond,
    kDevCapPeakSramRdBwGigabytesPerSecond,
    kDevCapPeakSramWrBwGigabytesPerSecond,
    kDevVendor,
    // Batching related.
    kBatchSizeAfterPadding,
    kPaddingAmount,
    kBatchingInputTaskSize,
    // GPU occupancy metrics
    kTheoreticalOccupancyPct,
    kOccupancyMinGridSize,
    kOccupancySuggestedBlockSize,
    // Aggregated Stats
    kSelfDurationPs,
    kMinDurationPs,
    kTotalProfileDurationPs,
    kMaxIterationNum,
    kDeviceType,
    kUsesMegaCore,
    kSymbolId,
    kTfOpName,
    kDmaStallDurationPs,
    kKey,
    kPayloadSizeBytes,
    kDuration,
    kBufferSize,
    kTransfers,
    // Dcn message Stats
    kDcnLabel,
    kDcnSourceSliceId,
    kDcnSourcePerSliceDeviceId,
    kDcnDestinationSliceId,
    kDcnDestinationPerSliceDeviceId,
    kDcnChunk,
    kDcnLoopIndex,
    kEdgeTpuModelInfo,
    kEdgeTpuModelProfileInfo,
    kEdgeTpuMlir,
    kDroppedTraces,
    kCudaGraphId,
    // Many events have kCudaGraphId, such as graph sub events when tracing is in
    // node level. Yet kCudaGraphExecId is used only for CudaGraphExecution events
    // on the GPU device when tracing is in graph level.
    kCudaGraphExecId,
    kCudaGraphOrigId,
    kStepIdleTimePs,
    kGpuDeviceName,
    kSourceStack,
    kDeviceOffsetPs,
    kDeviceDurationPs,
    kLastStatType = kDeviceDurationPs,
};

enum MegaScaleStatType : uint8_t
{
    kMegaScaleGraphKey,
    kFirstMegaScaleStatType = kMegaScaleGraphKey,
    kMegaScaleLocalDeviceId,
    kMegaScaleNumActions,
    kMegaScaleCollectiveType,
    kMegaScaleInputSize,
    kMegaScaleSlackUs,
    kMegaScaleActionType,
    kMegaScaleStartEndType,
    kMegaScaleActionIndex,
    kMegaScaleActionDurationNs,
    kMegaScaleActionInputs,
    kMegaScaleTransferSource,
    kMegaScaleTransferDestinations,
    kMegaScaleBufferSizes,
    kMegaScaleComputeOperation,
    kMegaScaleChunk,
    kMegaScaleLaunchId,
    kMegaScaleLoopIteration,
    kMegaScaleGraphProtos,
    kMegaScaleNetworkTransportLatency,
    kMegaScaleTransmissionBudgetUs,
    kMegaScaleDelayBudgetUs,
    kLastMegaScaleStatType = kMegaScaleDelayBudgetUs,
};

enum TaskEnvStatType
{
    kFirstTaskEnvStatType = 1,
    kEnvProfileStartTime  = kFirstTaskEnvStatType,
    kEnvProfileStopTime,
    kLastTaskEnvStatType = kEnvProfileStopTime,
};

static constexpr uint32_t kLineIdOffset = 10000;

enum LineIdType
{
    kFirstLineIdType   = kLineIdOffset,
    kUnknownLineIdType = kFirstLineIdType,
    // DCN Traffic
    kDcnHostTraffic,
    kDcnCollectiveTraffic,
    // kDcnCollectiveTrafficMax reserves id's from kDcnCollectiveTraffic to
    // (kDcnCollectiveTraffic + kMaxCollectivesToDisplay) for DcnCollective lines.
    kDcnCollectiveTrafficMax = kDcnCollectiveTraffic + kMaxCollectivesToDisplay,
    kLastLineIdType          = kDcnCollectiveTrafficMax,
};

inline std::string TpuPlaneName(int32_t device_ordinal)
{
    return strings::str_cat(kTpuPlanePrefix, device_ordinal);
}

inline std::string GpuPlaneName(int32_t device_ordinal)
{
    return strings::str_cat(kGpuPlanePrefix, device_ordinal);
}

PROFILER_API std::string_view GetHostEventTypeStr(HostEventType event_type);

bool IsHostEventType(HostEventType event_type, std::string_view event_name);

inline bool IsHostEventType(HostEventType event_type, std::string_view event_name)
{
    return GetHostEventTypeStr(event_type) == event_name;
}

PROFILER_API std::optional<int64_t> FindHostEventType(std::string_view event_name);

PROFILER_API std::optional<int64_t> FindTfOpEventType(std::string_view event_name);

PROFILER_API std::string_view GetStatTypeStr(StatType stat_type);

PROFILER_API bool IsStatType(StatType stat_type, std::string_view stat_name);

inline bool IsStatType(StatType stat_type, std::string_view stat_name)
{
    return GetStatTypeStr(stat_type) == stat_name;
}

PROFILER_API std::optional<int64_t> FindStatType(std::string_view stat_name);

PROFILER_API std::string_view GetMegaScaleStatTypeStr(MegaScaleStatType stat_type);

inline bool IsMegaScaleStatType(MegaScaleStatType stat_type, std::string_view stat_name)
{
    return GetMegaScaleStatTypeStr(stat_type) == stat_name;
}

PROFILER_API std::optional<int64_t> FindMegaScaleStatType(std::string_view stat_name);

// Returns true if the given event shouldn't be shown in the trace viewer.
PROFILER_API bool IsInternalEvent(std::optional<int64_t> event_type);

// Returns true if the given stat shouldn't be shown in the trace viewer.
PROFILER_API bool IsInternalStat(std::optional<int64_t> stat_type);

PROFILER_API std::string_view GetTaskEnvStatTypeStr(TaskEnvStatType stat_type);

PROFILER_API std::optional<int64_t> FindTaskEnvStatType(std::string_view stat_name);

// Support for flow events:
// This class enables encoding/decoding the flow id and direction, stored as
// XStat value. The flow id are limited to 56 bits.
class PROFILER_VISIBILITY XFlow
{
public:
    enum FlowDirection
    {
        kFlowUnspecified = 0x0,
        kFlowIn          = 0x1,
        kFlowOut         = 0x2,
        kFlowInOut       = 0x3,
    };

    XFlow(uint64_t flow_id, FlowDirection direction, ContextType category = ContextType::kGeneric)
    {
        // PROFILER_CHECK_DEBUG(direction != kFlowUnspecified);
        encoded_.parts.direction = direction;
        encoded_.parts.flow_id   = flow_id;
        encoded_.parts.category  = static_cast<uint64_t>(category);
    }

    // Encoding
    uint64_t ToStatValue() const { return encoded_.whole; }

    // Decoding
    static XFlow FromStatValue(uint64_t encoded) { return XFlow(encoded); }

    /* NOTE: std::HashOf is not consistent across processes (some process level
   * salt is added), even different executions of the same program.
   * However we are not tracking cross-host flows, i.e. A single flow's
   * participating events are from the same XSpace. On the other hand,
   * events from the same XSpace is always processed in the same profiler
   * process. Flows from different hosts are unlikely to collide because of
   * 2^56 hash space. Therefore, we can consider this is good for now. We should
   * revisit the hash function when cross-hosts flows became more popular.
   */
    template <typename... Args>
    static uint64_t GetFlowId(Args&&... args)
    {
        return HashOf(std::forward<Args>(args)...) & kFlowMask;
    }

    uint64_t      Id() const { return encoded_.parts.flow_id; }
    ContextType   Category() const { return GetSafeContextType(encoded_.parts.category); }
    FlowDirection Direction() const { return FlowDirection(encoded_.parts.direction); }

    static uint64_t GetUniqueId()
    {  // unique in current process.
        return next_flow_id_.fetch_add(1);
    }

private:
    explicit XFlow(uint64_t encoded) { encoded_.whole = encoded; }
    static constexpr uint64_t                 kFlowMask = (1ULL << 56) - 1;
    PROFILER_API static std::atomic<uint64_t> next_flow_id_;

    union
    {
        // Encoded representation.
        uint64_t whole;
        struct
        {
            uint64_t direction : 2;
            uint64_t flow_id : 56;
            uint64_t category : 6;
        } parts;
    } encoded_;

    static_assert(sizeof(encoded_) == sizeof(uint64_t), "Must be 64 bits.");
};
// String constants for XProf TraceMes for DCN Messages.
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDcnReceive;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDcnSend;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDcnSendFinished;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDcnMemAllocate;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDcnMemCopy;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleTopologyDiscovery;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleBarrier;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleHostCommand;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleD2HTransferStart;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleD2HTransferFinished;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleH2DTransferStart;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleH2DTransferFinished;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleReductionStart;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleReductionFinished;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleCompressionStart;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleCompressionFinished;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDecompressionStart;
PROFILER_CONST_INIT extern const std::string_view kMegaScaleDecompressionFinished;
PROFILER_CONST_INIT extern const char             kXProfMetadataKey[];
PROFILER_CONST_INIT extern const char             kXProfMetadataFlow[];
PROFILER_CONST_INIT extern const char             kXProfMetadataTransfers[];
PROFILER_CONST_INIT extern const char             kXProfMetadataBufferSize[];

// String constants for threadpool_listener events
PROFILER_CONST_INIT extern const std::string_view kThreadpoolListenerRecord;
PROFILER_CONST_INIT extern const std::string_view kThreadpoolListenerStartRegion;
PROFILER_CONST_INIT extern const std::string_view kThreadpoolListenerStopRegion;
PROFILER_CONST_INIT extern const std::string_view kThreadpoolListenerRegion;
}  // namespace quarisma
