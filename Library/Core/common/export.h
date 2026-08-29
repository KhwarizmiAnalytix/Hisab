/*
 * XSigma DLL Export/Import Header
 *
 * This file defines macros for symbol visibility control across different
 * platforms and build configurations (static vs shared libraries).
 *
 * Inspired by Google Benchmark's export system.
 *
 * Usage:
 * - XSIGMA_API: Use for functions and methods that need to be exported/imported
 * - XSIGMA_VISIBILITY: Use for class declarations that need to be exported
 * - XSIGMA_IMPORT: Explicit import decoration (rarely needed)
 * - XSIGMA_HIDDEN: Hide symbols from external visibility
 *
 * This header uses generic macro names and can be reused by any XSigma library project.
 * Each project should define the appropriate macros in their CMakeLists.txt:
 * - XSIGMA_STATIC_DEFINE for static library builds
 * - XSIGMA_SHARED_DEFINE for shared library builds
 * - XSIGMA_BUILDING_DLL when building the shared library
 */

#ifndef XSIGMA_EXPORT_H_INCLUDED_
#define XSIGMA_EXPORT_H_INCLUDED_

#define XSIGMA_VISIBILITY_ENUM

// Platform and build configuration detection
#if defined(XSIGMA_STATIC_DEFINE)
// Static library - no symbol decoration needed
#define XSIGMA_API
#define XSIGMA_VISIBILITY
#define XSIGMA_IMPORT
#define XSIGMA_HIDDEN

#elif defined(XSIGMA_SHARED_DEFINE)
// Shared library - platform-specific symbol decoration
#if defined(_WIN32) || defined(__CYGWIN__)
// Windows DLL export/import
#ifdef XSIGMA_BUILDING_DLL
#define XSIGMA_API __declspec(dllexport)
#else
#define XSIGMA_API __declspec(dllimport)
#endif
#define XSIGMA_VISIBILITY
#define XSIGMA_IMPORT __declspec(dllimport)
#define XSIGMA_HIDDEN
#elif defined(__GNUC__) && __GNUC__ >= 4
// GCC 4+ visibility attributes
#define XSIGMA_API __attribute__((visibility("default")))
#define XSIGMA_VISIBILITY __attribute__((visibility("default")))
#define XSIGMA_IMPORT __attribute__((visibility("default")))
#define XSIGMA_HIDDEN __attribute__((visibility("hidden")))
#else
// Fallback for other compilers
#define XSIGMA_API
#define XSIGMA_VISIBILITY
#define XSIGMA_IMPORT
#define XSIGMA_HIDDEN
#endif

#else
// Default fallback - assume static linking
#define XSIGMA_API
#define XSIGMA_VISIBILITY
#define XSIGMA_IMPORT
#define XSIGMA_HIDDEN
#endif

#endif  // XSIGMA_EXPORT_H_INCLUDED_
