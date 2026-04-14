/*
 * Quarisma DLL Export/Import Header
 *
 * This file defines macros for symbol visibility control across different
 * platforms and build configurations (static vs shared libraries).
 *
 * Inspired by Google Benchmark's export system.
 *
 * Usage:
 * - PARALLEL_API: Use for functions and methods that need to be exported/imported
 * - PARALLEL_VISIBILITY: Use for class declarations that need to be exported
 * - PARALLEL_IMPORT: Explicit import decoration (rarely needed)
 * - PARALLEL_HIDDEN: Hide symbols from external visibility
 *
 * This header uses generic macro names and can be reused by any Quarisma library project.
 * Each project should define the appropriate macros in their CMakeLists.txt:
 * - PARALLEL_STATIC_DEFINE for static library builds
 * - PARALLEL_SHARED_DEFINE for shared library builds
 * - PARALLEL_BUILDING_DLL when building the shared library
 */

#pragma once

#define PARALLEL_VISIBILITY_ENUM

// Platform and build configuration detection
#if defined(PARALLEL_STATIC_DEFINE)
// Static library - no symbol decoration needed
#define PARALLEL_API
#define PARALLEL_VISIBILITY
#define PARALLEL_IMPORT
#define PARALLEL_HIDDEN

#elif defined(PARALLEL_SHARED_DEFINE)
// Shared library - platform-specific symbol decoration
#if defined(_WIN32) || defined(__CYGWIN__)
// Windows DLL export/import
#ifdef PARALLEL_BUILDING_DLL
#define PARALLEL_API __declspec(dllexport)
#else
#define PARALLEL_API __declspec(dllimport)
#endif
#define PARALLEL_VISIBILITY
#define PARALLEL_IMPORT __declspec(dllimport)
#define PARALLEL_HIDDEN
#elif defined(__GNUC__) && __GNUC__ >= 4
// GCC 4+ visibility attributes
#define PARALLEL_API __attribute__((visibility("default")))
#define PARALLEL_VISIBILITY __attribute__((visibility("default")))
#define PARALLEL_IMPORT __attribute__((visibility("default")))
#define PARALLEL_HIDDEN __attribute__((visibility("hidden")))
#else
// Fallback for other compilers
#define PARALLEL_API
#define PARALLEL_VISIBILITY
#define PARALLEL_IMPORT
#define PARALLEL_HIDDEN
#endif

#else
// Default fallback - assume static linking
#define PARALLEL_API
#define PARALLEL_VISIBILITY
#define PARALLEL_IMPORT
#define PARALLEL_HIDDEN
#endif

