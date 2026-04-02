#if PROFILER_HAS_NATIVE_PROFILER
/*
 * Quarisma: High-Performance Quantitative Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 */

#include <string_view>

#include "Testing/baseTest.h"
#include "native/utils/parse_annotation.h"

using namespace quarisma::profiler_impl;
using namespace std::literals;

// ============================================================================
// parse_annotation tests
// ============================================================================

QUARISMATEST(Profiler, parse_annotation_parses_metadata_pairs)
{
    const auto anno = parse_annotation("matrix_multiply#rows=128,cols=256#");

    EXPECT_EQ(anno.name, "matrix_multiply"sv);
    ASSERT_EQ(anno.metadata.size(), 2u);
    EXPECT_EQ(anno.metadata[0].key, "rows"sv);
    EXPECT_EQ(anno.metadata[0].value, "128"sv);
    EXPECT_EQ(anno.metadata[1].key, "cols"sv);
    EXPECT_EQ(anno.metadata[1].value, "256"sv);
}

QUARISMATEST(Profiler, parse_annotation_without_metadata_returns_name_only)
{
    const auto anno = parse_annotation("simple_operation");

    EXPECT_EQ(anno.name, "simple_operation"sv);
    EXPECT_TRUE(anno.metadata.empty());
}

QUARISMATEST(Profiler, parse_annotation_trims_whitespace_and_skips_invalid_pairs)
{
    const auto anno = parse_annotation(" compute # threads = 4 , invalid , depth=  3 #");

    EXPECT_EQ(anno.name, "compute"sv);
    ASSERT_EQ(anno.metadata.size(), 2u);
    EXPECT_EQ(anno.metadata[0].key, "threads"sv);
    EXPECT_EQ(anno.metadata[0].value, "4"sv);
    EXPECT_EQ(anno.metadata[1].key, "depth"sv);
    EXPECT_EQ(anno.metadata[1].value, "3"sv);
}

QUARISMATEST(Profiler, parse_annotation_preserves_nested_metadata_tokens)
{
    const auto anno = parse_annotation("kernel#attr={a:1,b:2},shape=[1,2,3],label=\"conv2d\"#");

    EXPECT_EQ(anno.name, "kernel"sv);
    ASSERT_EQ(anno.metadata.size(), 3u);
    EXPECT_EQ(anno.metadata[0].key, "attr"sv);
    EXPECT_EQ(anno.metadata[0].value, "{a:1,b:2}"sv);
    EXPECT_EQ(anno.metadata[1].key, "shape"sv);
    EXPECT_EQ(anno.metadata[1].value, "[1,2,3]"sv);
    EXPECT_EQ(anno.metadata[2].key, "label"sv);
    EXPECT_EQ(anno.metadata[2].value, "\"conv2d\""sv);
}

QUARISMATEST(Profiler, parse_annotation_handles_missing_metadata_after_marker)
{
    const auto anno = parse_annotation("name_with_marker#");

    EXPECT_EQ(anno.name, "name_with_marker"sv);
    EXPECT_TRUE(anno.metadata.empty());
}

// ============================================================================
// parse_annotation_stack tests
// ============================================================================

QUARISMATEST(Profiler, parse_annotation_stack_parses_multiple_entries)
{
    const auto annos =
        parse_annotation_stack("outer_scope#level=1#::inner_scope#level=2,phase=forward#");

    ASSERT_EQ(annos.size(), 2u);
    EXPECT_EQ(annos[0].name, "outer_scope"sv);
    ASSERT_EQ(annos[0].metadata.size(), 1u);
    EXPECT_EQ(annos[0].metadata[0].key, "level"sv);
    EXPECT_EQ(annos[0].metadata[0].value, "1"sv);

    EXPECT_EQ(annos[1].name, "inner_scope"sv);
    ASSERT_EQ(annos[1].metadata.size(), 2u);
    EXPECT_EQ(annos[1].metadata[0].key, "level"sv);
    EXPECT_EQ(annos[1].metadata[0].value, "2"sv);
    EXPECT_EQ(annos[1].metadata[1].key, "phase"sv);
    EXPECT_EQ(annos[1].metadata[1].value, "forward"sv);
}

QUARISMATEST(Profiler, parse_annotation_stack_ignores_empty_segments)
{
    const auto annos = parse_annotation_stack("::outer#id=1#::::inner#id=2#::");

    ASSERT_EQ(annos.size(), 2u);
    EXPECT_EQ(annos[0].name, "outer"sv);
    EXPECT_EQ(annos[1].name, "inner"sv);
}

// ============================================================================
// has_metadata tests
// ============================================================================

QUARISMATEST(Profiler, has_metadata_detects_trailing_marker)
{
    EXPECT_TRUE(has_metadata("kernel#"));
    EXPECT_FALSE(has_metadata("kernel"));
    EXPECT_FALSE(has_metadata(""));
}
#endif  // PROFILER_HAS_NATIVE_PROFILER
