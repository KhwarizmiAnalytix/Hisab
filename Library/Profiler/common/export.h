/*
 * Profiler DLL export/import header
 *
 * Defines PROFILER_API, PROFILER_VISIBILITY, etc. for the Profiler library.
 * Build flags (set in Library/Profiler/CMakeLists.txt):
 * - PROFILER_STATIC_DEFINE — static library
 * - PROFILER_SHARED_DEFINE — shared library
 * - PROFILER_BUILDING_DLL — building the shared library (Windows)
 */

#ifndef __profiler_export_h__
#define __profiler_export_h__

#define PROFILER_VISIBILITY_ENUM

#if defined(PROFILER_STATIC_DEFINE)
#define PROFILER_API
#define PROFILER_VISIBILITY
#define PROFILER_IMPORT
#define PROFILER_HIDDEN

#elif defined(PROFILER_SHARED_DEFINE)
#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef PROFILER_BUILDING_DLL
#define PROFILER_API __declspec(dllexport)
#else
#define PROFILER_API __declspec(dllimport)
#endif
#define PROFILER_VISIBILITY
#define PROFILER_IMPORT __declspec(dllimport)
#define PROFILER_HIDDEN
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PROFILER_API __attribute__((visibility("default")))
#define PROFILER_VISIBILITY __attribute__((visibility("default")))
#define PROFILER_IMPORT __attribute__((visibility("default")))
#define PROFILER_HIDDEN __attribute__((visibility("hidden")))
#else
#define PROFILER_API
#define PROFILER_VISIBILITY
#define PROFILER_IMPORT
#define PROFILER_HIDDEN
#endif

#else
#define PROFILER_API
#define PROFILER_VISIBILITY
#define PROFILER_IMPORT
#define PROFILER_HIDDEN
#endif

#endif  // __profiler_export_h__
