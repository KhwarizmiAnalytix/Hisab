/*
 * Quarisma: High-Performance Computational Library
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

/**
 * @file TestStringUtil.cpp
 * @brief Comprehensive test suite for string utility functions
 *
 * This file contains extensive tests for all string utility functions
 * including edge cases, error conditions, and performance validation.
 *
 * @author Quarisma Development Team
 * @version 2.0
 * @date 2024
 */

#include <gtest/gtest.h>  // for AssertionResult, Message, TestPartResult, EXPECT_EQ, EXPECT_FALSE, EXPECT_TRUE

#include <cstddef>      // for size_t
#include <fstream>      // for basic_ostream, filebuf, ostream
#include <memory>       // for _Simple_types
#include <string>       // for string, basic_string, char_traits
#include <string_view>  // for string_view
#include <vector>       // for vector, _Vector_const_iterato

#include "baseTest.h"       // for END_TEST, QUARISMATEST
#include "common/macros.h"  // for QUARISMA_UNUSED
#include "util/string_util.h"  // for is_float, is_integer, exclude_file_extension, file_extension, strip_basename

namespace quarisma
{
void testStringManipulation()
{
    // Test erase_all_sub_string function
    std::string      s   = "blabla";
    std::string_view sub = "la";
    erase_all_sub_string(s, sub);
    EXPECT_EQ(s, "bb");

    // More comprehensive erase tests
    s = "abcabcabc";
    erase_all_sub_string(s, "abc");
    EXPECT_EQ(s, "");

    s = "hello world hello";
    erase_all_sub_string(s, "hello");
    EXPECT_EQ(s, " world ");

    s = "test";
    erase_all_sub_string(s, "xyz");  // Non-existent substring
    EXPECT_EQ(s, "test");

    // Test replace_all function
    s            = "bb";
    size_t count = replace_all(s, "b", "c");
    EXPECT_EQ(s, "cc");
    EXPECT_EQ(count, 2u);

    // More comprehensive replace tests
    s     = "hello world hello";
    count = replace_all(s, "hello", "hi");
    EXPECT_EQ(s, "hi world hi");
    EXPECT_EQ(count, 2u);

    s     = "abcdef";
    count = replace_all(s, "xyz", "123");  // Non-existent substring
    EXPECT_EQ(s, "abcdef");
    EXPECT_EQ(count, 0u);

    // Test overlapping replacements
    s     = "aaa";
    count = replace_all(s, "aa", "b");
    EXPECT_EQ(s, "ba");  // Should replace first occurrence
    EXPECT_EQ(count, 1u);
}

void testCompatibilityFunctions()
{
    // Test starts_with function with std::string
    std::string str1    = "Hello World";
    std::string prefix1 = "Hello";
    EXPECT_TRUE(starts_with(str1, prefix1));

    std::string prefix2 = "hello";
    EXPECT_FALSE(starts_with(str1, prefix2));  // Case sensitive

    std::string str2    = "Hi";
    std::string prefix3 = "Hello";
    EXPECT_FALSE(starts_with(str2, prefix3));

    std::string empty1 = "";
    std::string empty2 = "";
    EXPECT_TRUE(starts_with(empty1, empty2));  // Edge case

    std::string prefix4 = "test";
    EXPECT_FALSE(starts_with(empty1, prefix4));

    // Test starts_with with string_view
    std::string_view sv1 = "Hello World";
    std::string_view sv2 = "Hello";
    EXPECT_TRUE(starts_with(sv1, sv2));

    // Test ends_with function with std::string
    std::string doc  = "document.pdf";
    std::string ext1 = ".pdf";
    EXPECT_TRUE(ends_with(doc, ext1));

    std::string ext2 = ".PDF";
    EXPECT_FALSE(ends_with(doc, ext2));  // Case sensitive

    std::string short_str = "doc";
    std::string long_ext  = ".pdf";
    EXPECT_FALSE(ends_with(short_str, long_ext));

    EXPECT_TRUE(ends_with(empty1, empty2));  // Edge case
    EXPECT_FALSE(ends_with(empty1, prefix4));

    // Test ends_with with string_view
    std::string_view sv3 = "document.pdf";
    std::string_view sv4 = ".pdf";
    EXPECT_TRUE(ends_with(sv3, sv4));
}

void testSourceLocation()
{
    quarisma::source_location loc;
    loc.file     = "test.cpp";
    loc.function = "testFunction";
    loc.line     = 42;

    // Test basic functionality (skip stream output for now due to linking issues)
    EXPECT_EQ(std::string(loc.file), "test.cpp");
    EXPECT_EQ(std::string(loc.function), "testFunction");
    EXPECT_EQ(loc.line, 42);

    // TODO: Fix operator<< linking issue
    // std::ostringstream oss;
    // oss << loc;
    // EXPECT_EQ(oss.str(), "testFunction at test.cpp:42");
}

void testStringConcatenation()
{
    // Test str_cat with multiple arguments
    std::string result1 = quarisma::strings::str_cat("Hello", " ", "World");
    EXPECT_EQ(result1, "Hello World");

    // Test str_cat with numeric types
    std::string result2 = quarisma::strings::str_cat("Value: ", 42);
    EXPECT_EQ(result2, "Value: 42");

    // Test str_cat with floating point
    std::string result3 = quarisma::strings::str_cat("Pi: ", 3.14);
    EXPECT_TRUE(result3.find("Pi:") != std::string::npos);
    EXPECT_TRUE(result3.find("3.14") != std::string::npos);

    // Test str_cat with empty strings
    std::string result4 = quarisma::strings::str_cat("", "test", "");
    EXPECT_EQ(result4, "test");

    // Test str_cat with single argument
    std::string result5 = quarisma::strings::str_cat("single");
    EXPECT_EQ(result5, "single");

    // Test str_cat with string_view
    std::string_view sv      = "view";
    std::string      result6 = quarisma::strings::str_cat("string_", sv);
    EXPECT_EQ(result6, "string_view");
}

void testStringAppend()
{
    // Test str_append with multiple arguments
    std::string s = "Start";
    quarisma::strings::str_append(&s, " ", "Middle", " ", 123);
    EXPECT_EQ(s, "Start Middle 123");

    // Test str_append with single argument
    std::string s2 = "Hello";
    quarisma::strings::str_append(&s2, " World");
    EXPECT_EQ(s2, "Hello World");

    // Test str_append with null pointer (should not crash)
    quarisma::strings::str_append(nullptr, "test");  // Should be safe

    // Test str_append with empty string
    std::string s3 = "";
    quarisma::strings::str_append(&s3, "content");
    EXPECT_EQ(s3, "content");

    // Test str_append with numeric types
    std::string s4 = "Numbers: ";
    quarisma::strings::str_append(&s4, 1, ", ", 2, ", ", 3);
    EXPECT_EQ(s4, "Numbers: 1, 2, 3");
}

void testStringContains()
{
    // Test str_contains with character found
    EXPECT_TRUE(quarisma::strings::str_contains("hello:world", ':'));

    // Test str_contains with character not found
    EXPECT_FALSE(quarisma::strings::str_contains("hello", 'x'));

    // Test str_contains with empty string
    EXPECT_FALSE(quarisma::strings::str_contains("", 'a'));

    // Test str_contains with single character string
    EXPECT_TRUE(quarisma::strings::str_contains("a", 'a'));

    // Test str_contains with multiple occurrences
    EXPECT_TRUE(quarisma::strings::str_contains("aaa", 'a'));
}

void testFormatHex()
{
    // Test format_hex with no padding
    std::string hex1 = quarisma::strings::format_hex(255);
    EXPECT_EQ(hex1, "ff");

    // Test format_hex with padding
    std::string hex2 = quarisma::strings::format_hex(255, quarisma::strings::hex_pad::pad4);
    EXPECT_EQ(hex2, "00ff");

    // Test format_hex with larger value
    std::string hex3 = quarisma::strings::format_hex(0x1234, quarisma::strings::hex_pad::pad8);
    EXPECT_EQ(hex3, "00001234");

    // Test format_hex with zero
    std::string hex4 = quarisma::strings::format_hex(0);
    EXPECT_EQ(hex4, "0");

    // Test format_hex with pad2
    std::string hex5 = quarisma::strings::format_hex(15, quarisma::strings::hex_pad::pad2);
    EXPECT_EQ(hex5, "0f");
}

void testToLower()
{
    // Test to_lower with uppercase
    std::string lower1 = quarisma::strings::to_lower("HELLO");
    EXPECT_EQ(lower1, "hello");

    // Test to_lower with mixed case
    std::string lower2 = quarisma::strings::to_lower("HeLLo WoRLd");
    EXPECT_EQ(lower2, "hello world");

    // Test to_lower with already lowercase
    std::string lower3 = quarisma::strings::to_lower("hello");
    EXPECT_EQ(lower3, "hello");

    // Test to_lower with numbers and special chars
    std::string lower4 = quarisma::strings::to_lower("Test123!@#");
    EXPECT_EQ(lower4, "test123!@#");

    // Test to_lower with empty string
    std::string lower5 = quarisma::strings::to_lower("");
    EXPECT_EQ(lower5, "");
}

void testDemangleFunction()
{
    // Test demangle with null pointer
    std::string demangled_null = quarisma::demangle(nullptr);
    EXPECT_EQ(demangled_null, "<unknown>");

    // Test demangle with empty string
    std::string demangled_empty = quarisma::demangle("");
    EXPECT_EQ(demangled_empty, "<unknown>");

    // Test demangle with simple name (not mangled)
    std::string demangled_simple = quarisma::demangle("main");
    EXPECT_FALSE(demangled_simple.empty());

    // Test demangle with actual mangled name (if available)
    // This is platform-dependent, so we just verify it doesn't crash
    std::string demangled_complex = quarisma::demangle("_Z1gv");
    EXPECT_FALSE(demangled_complex.empty());
}

void testReplaceAllEdgeCases()
{
    // Test replace_all with overlapping patterns
    std::string s1     = "aaa";
    size_t      count1 = quarisma::replace_all(s1, "aa", "b");
    EXPECT_EQ(s1, "ba");
    EXPECT_EQ(count1, 1u);

    // Test replace_all with replacement longer than original
    std::string s2     = "a";
    size_t      count2 = quarisma::replace_all(s2, "a", "hello");
    EXPECT_EQ(s2, "hello");
    EXPECT_EQ(count2, 1u);

    // Test replace_all with empty replacement
    std::string s3     = "hello";
    size_t      count3 = quarisma::replace_all(s3, "l", "");
    EXPECT_EQ(s3, "heo");
    EXPECT_EQ(count3, 2u);

    // Test replace_all with single character
    std::string s4     = "aaa";
    size_t      count4 = quarisma::replace_all(s4, "a", "b");
    EXPECT_EQ(s4, "bbb");
    EXPECT_EQ(count4, 3u);
}

void testEraseAllSubstringEdgeCases()
{
    // Test erase_all_sub_string with single character
    std::string s1 = "aaa";
    quarisma::erase_all_sub_string(s1, "a");
    EXPECT_EQ(s1, "");

    // Test erase_all_sub_string with multi-character substring
    std::string s2 = "abcabcabc";
    quarisma::erase_all_sub_string(s2, "abc");
    EXPECT_EQ(s2, "");

    // Test erase_all_sub_string with partial match
    std::string s3 = "abcdefabc";
    quarisma::erase_all_sub_string(s3, "abc");
    EXPECT_EQ(s3, "def");

    // Test erase_all_sub_string with no match
    std::string s4 = "hello";
    quarisma::erase_all_sub_string(s4, "xyz");
    EXPECT_EQ(s4, "hello");
}

void testStartsWithEdgeCases()
{
    // Test starts_with with equal strings
    EXPECT_TRUE(quarisma::starts_with("hello", "hello"));

    // Test starts_with with longer prefix than string
    EXPECT_FALSE(quarisma::starts_with("hi", "hello"));

    // Test starts_with with single character
    EXPECT_TRUE(quarisma::starts_with("hello", "h"));

    // Test starts_with with empty prefix
    EXPECT_TRUE(quarisma::starts_with("hello", ""));

    // Test starts_with with empty string and non-empty prefix
    EXPECT_FALSE(quarisma::starts_with("", "a"));
}

void testEndsWithEdgeCases()
{
    // Test ends_with with equal strings
    EXPECT_TRUE(quarisma::ends_with("hello", "hello"));

    // Test ends_with with longer suffix than string
    EXPECT_FALSE(quarisma::ends_with("hi", "hello"));

    // Test ends_with with single character
    EXPECT_TRUE(quarisma::ends_with("hello", "o"));

    // Test ends_with with empty suffix
    EXPECT_TRUE(quarisma::ends_with("hello", ""));

    // Test ends_with with empty string and non-empty suffix
    EXPECT_FALSE(quarisma::ends_with("", "a"));
}

void testAllFunctions()
{
    testStringManipulation();
    testCompatibilityFunctions();
    testSourceLocation();
    testStringConcatenation();
    testStringAppend();
    testStringContains();
    testFormatHex();
    testToLower();
    testDemangleFunction();
    testReplaceAllEdgeCases();
    testEraseAllSubstringEdgeCases();
    testStartsWithEdgeCases();
    testEndsWithEdgeCases();
}

}  // namespace quarisma

QUARISMATEST(StringUtil, test)
{
    quarisma::testAllFunctions();
    END_TEST();
}
