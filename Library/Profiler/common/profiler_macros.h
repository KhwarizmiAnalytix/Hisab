//----------------------------------------------------------------------------
#if __cplusplus >= 201703L
#define PROFILER_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__)
// For GCC or Clang: use __attribute__
#define PROFILER_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
// For MSVC
#define PROFILER_UNUSED __pragma(warning(suppress : 4100))
#else
// Fallback for other compilers
#define PROFILER_UNUSED
#endif

//----------------------------------------------------------------------------
#if defined(__cplusplus) && defined(__has_cpp_attribute)
#define PROFILER_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define PROFILER_HAVE_CPP_ATTRIBUTE(x) 0
#endif

//------------------------------------------------------------------------
// C++20 [[likely]] and [[unlikely]] attributes are only available in C++20 and later
#if __cplusplus >= 202002L && PROFILER_HAVE_CPP_ATTRIBUTE(likely) && \
    PROFILER_HAVE_CPP_ATTRIBUTE(unlikely)
#define PROFILER_LIKELY(expr) (expr) [[likely]]
#define PROFILER_UNLIKELY(expr) (expr) [[unlikely]]
#elif defined(__GNUC__) || defined(__ICL) || defined(__clang__)
#define PROFILER_LIKELY(expr) (__builtin_expect(static_cast<bool>(expr), 1))
#define PROFILER_UNLIKELY(expr) (__builtin_expect(static_cast<bool>(expr), 0))
#else
#define PROFILER_LIKELY(expr) (expr)
#define PROFILER_UNLIKELY(expr) (expr)
#endif



//----------------------------------------------------------------------------
#if PROFILER_HAVE_CPP_ATTRIBUTE(clang::lifetimebound)
#define PROFILER_LIFETIME_BOUND [[clang::lifetimebound]]
#elif PROFILER_HAVE_CPP_ATTRIBUTE(msvc::lifetimebound)
#define PROFILER_LIFETIME_BOUND [[msvc::lifetimebound]]
#elif PROFILER_HAVE_ATTRIBUTE(lifetimebound)
#define PROFILER_LIFETIME_BOUND __attribute__((lifetimebound))
#else
#define PROFILER_LIFETIME_BOUND
#endif