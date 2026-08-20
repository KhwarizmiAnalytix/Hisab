/*
 * Quarisma DLL Export/Import Header
 *
 * This file defines macros for symbol visibility control across different
 * platforms and build configurations (static vs shared libraries).
 *
 * Inspired by Google Benchmark's export system.
 *
 * Usage:
 * - QUARISMA_API: Use for functions and methods that need to be exported/imported
 * - QUARISMA_VISIBILITY: Use for class declarations that need to be exported
 * - QUARISMA_IMPORT: Explicit import decoration (rarely needed)
 * - QUARISMA_HIDDEN: Hide symbols from external visibility
 *
 * This header uses generic macro names and can be reused by any Quarisma library project.
 * Each project should define the appropriate macros in their CMakeLists.txt:
 * - QUARISMA_STATIC_DEFINE for static library builds
 * - QUARISMA_SHARED_DEFINE for shared library builds
 * - QUARISMA_BUILDING_DLL when building the shared library
 */

#ifndef QUARISMA_EXPORT_H_INCLUDED_
#define QUARISMA_EXPORT_H_INCLUDED_

#define QUARISMA_VISIBILITY_ENUM

// Platform and build configuration detection
#if defined(QUARISMA_STATIC_DEFINE)
// Static library - no symbol decoration needed
#define QUARISMA_API
#define QUARISMA_VISIBILITY
#define QUARISMA_IMPORT
#define QUARISMA_HIDDEN

#elif defined(QUARISMA_SHARED_DEFINE)
// Shared library - platform-specific symbol decoration
#if defined(_WIN32) || defined(__CYGWIN__)
// Windows DLL export/import
#ifdef QUARISMA_BUILDING_DLL
#define QUARISMA_API __declspec(dllexport)
#else
#define QUARISMA_API __declspec(dllimport)
#endif
#define QUARISMA_VISIBILITY
#define QUARISMA_IMPORT __declspec(dllimport)
#define QUARISMA_HIDDEN
#elif defined(__GNUC__) && __GNUC__ >= 4
// GCC 4+ visibility attributes
#define QUARISMA_API __attribute__((visibility("default")))
#define QUARISMA_VISIBILITY __attribute__((visibility("default")))
#define QUARISMA_IMPORT __attribute__((visibility("default")))
#define QUARISMA_HIDDEN __attribute__((visibility("hidden")))
#else
// Fallback for other compilers
#define QUARISMA_API
#define QUARISMA_VISIBILITY
#define QUARISMA_IMPORT
#define QUARISMA_HIDDEN
#endif

#else
// Default fallback - assume static linking
#define QUARISMA_API
#define QUARISMA_VISIBILITY
#define QUARISMA_IMPORT
#define QUARISMA_HIDDEN
#endif

#endif  // QUARISMA_EXPORT_H_INCLUDED_
