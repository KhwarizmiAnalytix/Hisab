#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * Comprehensive test suite for XPlane Schema
 * Tests schema definitions, enums, and utility functions
 */

#include <string>

#include "Testing/baseTest.h"
#include "native/exporters/xplane/xplane_schema.h"

using namespace quarisma;

// ============================================================================
// ContextType Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneSchema, context_type_string_conversion)
{
    // Test kLegacy - returns empty string
    const char* result = GetContextTypeString(ContextType::kLegacy);
    EXPECT_STREQ(result, "");

    // Test kGeneric - returns empty string
    result = GetContextTypeString(ContextType::kGeneric);
    EXPECT_STREQ(result, "");

    // Test kThreadpoolEvent
    result = GetContextTypeString(ContextType::kThreadpoolEvent);
    EXPECT_STREQ(result, "threadpool_event");

    // Test kGpuLaunch
    result = GetContextTypeString(ContextType::kGpuLaunch);
    EXPECT_STREQ(result, "gpu_launch");

    // Test kTfExecutor
    result = GetContextTypeString(ContextType::kTfExecutor);
    EXPECT_STREQ(result, "tf_exec");

    // Test kTfrtExecutor
    result = GetContextTypeString(ContextType::kTfrtExecutor);
    EXPECT_STREQ(result, "tfrt_exec");

    // Test kSharedBatchScheduler
    result = GetContextTypeString(ContextType::kSharedBatchScheduler);
    EXPECT_STREQ(result, "batch_sched");

    // Test kPjRt
    result = GetContextTypeString(ContextType::kPjRt);
    EXPECT_STREQ(result, "PjRt");

    // Test kAdaptiveSharedBatchScheduler
    result = GetContextTypeString(ContextType::kAdaptiveSharedBatchScheduler);
    EXPECT_STREQ(result, "as_batch_sched");

    // Test kTfrtTpuRuntime
    result = GetContextTypeString(ContextType::kTfrtTpuRuntime);
    EXPECT_STREQ(result, "tfrt_rt");

    // Test kTpuEmbeddingEngine
    result = GetContextTypeString(ContextType::kTpuEmbeddingEngine);
    EXPECT_STREQ(result, "tpu_embed");

    // Test kBatcher
    result = GetContextTypeString(ContextType::kBatcher);
    EXPECT_STREQ(result, "batcher");

    // Test kTpuStream
    result = GetContextTypeString(ContextType::kTpuStream);
    EXPECT_STREQ(result, "tpu_stream");

    // Test kTpuLaunch
    result = GetContextTypeString(ContextType::kTpuLaunch);
    EXPECT_STREQ(result, "tpu_launch");

    // Test kPathwaysExecutor
    result = GetContextTypeString(ContextType::kPathwaysExecutor);
    EXPECT_STREQ(result, "pathways_exec");

    // Test kPjrtLibraryCall
    result = GetContextTypeString(ContextType::kPjrtLibraryCall);
    EXPECT_STREQ(result, "pjrt_library_call");
}

QUARISMATEST(XPlaneSchema, context_type_safe_conversion)
{
    // Test valid context type conversion
    ContextType result = GetSafeContextType(static_cast<uint32_t>(ContextType::kGpuLaunch));
    EXPECT_EQ(result, ContextType::kGpuLaunch);

    // Test invalid context type conversion - should default to Generic
    result = GetSafeContextType(999);
    EXPECT_EQ(result, ContextType::kGeneric);
}

// ============================================================================
// HostEventType Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneSchema, host_event_type_operations)
{
    // Test getting valid host event type string
    std::string_view result = GetHostEventTypeStr(HostEventType::kFirstHostEventType);
    EXPECT_FALSE(result.empty());

    // Test finding valid host event type
    std::optional<int32_t> found = FindHostEventType("UnknownHostEventType");
    EXPECT_TRUE(found.has_value());
    if (found.has_value())
    {
        EXPECT_EQ(found.value(), static_cast<int32_t>(HostEventType::kUnknownHostEventType));
    }

    // Test finding invalid host event type
    found = FindHostEventType("NonExistentEventType");
    EXPECT_FALSE(found.has_value());

    // Test finding with empty string
    found = FindHostEventType("");
    EXPECT_FALSE(found.has_value());
}

// ============================================================================
// StatType Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneSchema, stat_type_operations)
{
    // Test getting valid stat type string
    std::string_view result = GetStatTypeStr(StatType::kFirstStatType);
    EXPECT_FALSE(result.empty());

    // Test finding valid stat type
    std::optional<int32_t> found = FindStatType("UnknownStatType");
    EXPECT_TRUE(found.has_value());
    if (found.has_value())
    {
        EXPECT_EQ(found.value(), static_cast<int32_t>(StatType::kUnknownStatType));
    }

    // Test finding invalid stat type
    found = FindStatType("NonExistentStatType");
    EXPECT_FALSE(found.has_value());

    // Test finding with empty string
    found = FindStatType("");
    EXPECT_FALSE(found.has_value());
}

// ============================================================================
// XFlow Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneSchema, xflow_construction_and_properties)
{
    // Test construction and basic properties
    uint64_t             flow_id   = 12345;
    XFlow::FlowDirection direction = XFlow::kFlowIn;
    ContextType          context   = ContextType::kGpuLaunch;

    XFlow flow(flow_id, direction, context);

    EXPECT_EQ(flow.Id(), flow_id);
    EXPECT_EQ(flow.Direction(), direction);
    EXPECT_EQ(flow.Category(), context);
}

QUARISMATEST(XPlaneSchema, xflow_encoding_and_decoding)
{
    // Test encoding and decoding with all flow directions
    uint64_t flow_id = 54321;

    std::vector<XFlow::FlowDirection> directions = {
        XFlow::kFlowIn, XFlow::kFlowOut, XFlow::kFlowInOut};

    for (auto direction : directions)
    {
        ContextType context = ContextType::kGpuLaunch;
        XFlow       original(flow_id, direction, context);
        uint64_t    encoded = original.ToStatValue();
        XFlow       decoded = XFlow::FromStatValue(encoded);

        EXPECT_EQ(decoded.Id(), flow_id);
        EXPECT_EQ(decoded.Direction(), direction);
        EXPECT_EQ(decoded.Category(), context);
    }

    // Test with different flow_id and verify all directions work
    flow_id = 99999;
    for (auto direction : directions)
    {
        XFlow    flow(flow_id, direction);
        uint64_t encoded = flow.ToStatValue();
        XFlow    decoded = XFlow::FromStatValue(encoded);

        EXPECT_EQ(decoded.Id(), flow_id);
        EXPECT_EQ(decoded.Direction(), direction);
    }
}

QUARISMATEST(XPlaneSchema, xflow_unique_id_generation)
{
    // Test unique ID generation - should be monotonically increasing
    uint64_t id1 = XFlow::GetUniqueId();
    uint64_t id2 = XFlow::GetUniqueId();
    uint64_t id3 = XFlow::GetUniqueId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
    EXPECT_LT(id1, id2);  // Should be monotonically increasing
    EXPECT_LT(id2, id3);
}

QUARISMATEST(XPlaneSchema, xflow_hash_based_id_generation)
{
    // Test that GetFlowId generates consistent hashes
    std::string key1 = "test_key_1";
    std::string key2 = "test_key_2";

    uint64_t hash1a = XFlow::GetFlowId(key1);
    uint64_t hash1b = XFlow::GetFlowId(key1);  // Same key
    uint64_t hash2  = XFlow::GetFlowId(key2);  // Different key

    EXPECT_EQ(hash1a, hash1b);  // Same input should give same hash
    EXPECT_NE(hash1a, hash2);   // Different inputs should give different hashes
}

// ============================================================================
// Hash Function Tests - Consolidated
// ============================================================================

QUARISMATEST(XPlaneSchema, hash_basic_types)
{
    // Test HashOf with int64_t
    int64_t int_value = 12345;
    size_t  int_hash  = HashOf(int_value);
    EXPECT_NE(int_hash, 0);  // Hash should be non-zero for non-zero input

    // Test HashOf with uint64_t
    uint64_t uint_value = 67890;
    size_t   uint_hash  = HashOf(uint_value);
    EXPECT_NE(uint_hash, 0);

    // Test HashOf with string
    std::string string_value = "test_string";
    size_t      string_hash  = HashOf(string_value);
    EXPECT_NE(string_hash, 0);

    // Test HashOf with string_view
    std::string_view sv_value = "test_string";
    size_t           sv_hash  = HashOf(sv_value);
    EXPECT_NE(sv_hash, 0);

    // Test HashOf with empty string (just verify it doesn't crash)
    std::string empty_value = "";
    size_t      empty_hash  = HashOf(empty_value);
    (void)empty_hash;  // Empty string hash may be zero or non-zero
    EXPECT_TRUE(true);

    // Test string and string_view consistency
    std::string      str = "test";
    std::string_view sv  = str;
    EXPECT_EQ(HashOf(str), HashOf(sv));
}

QUARISMATEST(XPlaneSchema, hash_consistency_and_uniqueness)
{
    // Test that same values produce same hashes
    int64_t value1 = 12345;
    int64_t value2 = 12345;
    size_t  hash1  = HashOf(value1);
    size_t  hash2  = HashOf(value2);
    EXPECT_EQ(hash1, hash2);

    // Test that different values produce different hashes
    int64_t value3 = 54321;
    size_t  hash3  = HashOf(value3);
    EXPECT_NE(hash1, hash3);
}

QUARISMATEST(XPlaneSchema, hash_combine_operations)
{
    // Test basic hash combination
    size_t hash1    = HashOf(int64_t(123));
    size_t hash2    = HashOf(int64_t(456));
    size_t combined = hash1;
    HashCombine(combined, hash2);
    EXPECT_NE(combined, hash1);
    EXPECT_NE(combined, hash2);

    // Test that order matters in hash combination
    size_t combined1 = hash1;
    HashCombine(combined1, hash2);

    size_t combined2 = hash2;
    HashCombine(combined2, hash1);

    EXPECT_NE(combined1, combined2);  // Order should matter

    // Test multiple hash combinations
    size_t multi_hash = 0;
    HashCombine(multi_hash, HashOf(int64_t(1)));
    HashCombine(multi_hash, HashOf(int64_t(2)));
    HashCombine(multi_hash, HashOf(int64_t(3)));
    EXPECT_NE(multi_hash, 0);

    // Test hash combination with zero (just verify it doesn't crash)
    size_t zero_hash = 0;
    HashCombine(zero_hash, HashOf(int64_t(0)));
    (void)zero_hash;  // Result may or may not be zero
    EXPECT_TRUE(true);
}

#endif  // PROFILER_HAS_NATIVE_PROFILER
