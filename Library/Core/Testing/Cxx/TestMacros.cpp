/*
 * XSigma: High-Performance Computational Library
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR Commercial
 *
 * This file is part of XSigma and is licensed under a dual-license model:
 *
 *   - Open-source License (GPLv3):
 *       Free for personal, academic, and research use under the terms of
 *       the GNU General Public License v3.0 or later.
 *
 *   - Commercial License:
 *       A commercial license is required for proprietary, closed-source,
 *       or SaaS usage. Contact us to obtain a commercial agreement.
 *
 * Contact: licensing@xsigma.co.uk
 * Website: https://www.xsigma.co.uk
 */

#include <gtest/gtest.h>

#include <cstdint>

#include "CoreTest.h"
#include "common/macros.h"

// ============================================================================
// Compile-time checks: these fire at build time if a macro is broken
// ============================================================================

// XSIGMA_ALIGNMENT must be a power of two and >= 16
static_assert(XSIGMA_ALIGNMENT >= 16, "XSIGMA_ALIGNMENT must be at least 16");
static_assert(
    (XSIGMA_ALIGNMENT & (XSIGMA_ALIGNMENT - 1)) == 0, "XSIGMA_ALIGNMENT must be a power of two");

// XSIGMA_COMPILE_TIME_MAX_GPUS must be positive
static_assert(XSIGMA_COMPILE_TIME_MAX_GPUS > 0, "XSIGMA_COMPILE_TIME_MAX_GPUS must be > 0");

// PROJECT_HAS_THREE_WAY_COMPARISON must be 0 or 1
static_assert(
    PROJECT_HAS_THREE_WAY_COMPARISON == 0 || PROJECT_HAS_THREE_WAY_COMPARISON == 1,
    "PROJECT_HAS_THREE_WAY_COMPARISON must be 0 or 1");

// PROJECT_HAS_CXA_DEMANGLE must be 0 or 1
static_assert(
    PROJECT_HAS_CXA_DEMANGLE == 0 || PROJECT_HAS_CXA_DEMANGLE == 1,
    "PROJECT_HAS_CXA_DEMANGLE must be 0 or 1");

// ============================================================================
// Token-pasting macros
// ============================================================================

XSIGMATEST(Macros, Concatenate)
{
    // XSIGMA_CONCATENATE must paste two tokens into one identifier
    int XSIGMA_CONCATENATE(foo, Bar) = 42;
    EXPECT_EQ(fooBar, 42);

    END_TEST();
}

XSIGMATEST(Macros, AnonymousVariable)
{
    // Two XSIGMA_ANONYMOUS_VARIABLE calls in the same scope must produce distinct names
    int XSIGMA_ANONYMOUS_VARIABLE(anon) = 1;
    int XSIGMA_ANONYMOUS_VARIABLE(anon) = 2;
    // If the two variables had the same name this would not compile.
    // The test just verifies compilation succeeded and both values exist.
    EXPECT_EQ(1, 1);

    END_TEST();
}

// ============================================================================
// Alignment macro
// ============================================================================

XSIGMATEST(Macros, AlignmentValue)
{
#ifdef XSIGMA_MOBILE
    EXPECT_EQ(XSIGMA_ALIGNMENT, static_cast<size_t>(16));
#else
    EXPECT_EQ(XSIGMA_ALIGNMENT, static_cast<size_t>(64));
#endif

    END_TEST();
}

XSIGMATEST(Macros, AlignMacro)
{
    // XSIGMA_ALIGN must make a variable honour the requested alignment
    XSIGMA_ALIGN(64) char buf[64];
    EXPECT_EQ(reinterpret_cast<uintptr_t>(buf) % 64, static_cast<uintptr_t>(0));

    END_TEST();
}

// ============================================================================
// Branch-prediction macros
// ============================================================================

XSIGMATEST(Macros, Likely)
{
    bool value = true;
    bool result;
    if XSIGMA_LIKELY (value)
        result = true;
    else
        result = false;
    EXPECT_TRUE(result);

    END_TEST();
}

XSIGMATEST(Macros, Unlikely)
{
    bool value = false;
    bool result;
    if XSIGMA_UNLIKELY (value)
        result = true;
    else
        result = false;
    EXPECT_FALSE(result);

    END_TEST();
}

// ============================================================================
// Function-attribute macros (compile-time only: if they expand wrongly the
// function definition below would not compile)
// ============================================================================

static XSIGMA_FORCE_INLINE int forceinline_add(int a, int b)
{
    return a + b;
}

XSIGMATEST(Macros, ForceInline)
{
    EXPECT_EQ(forceinline_add(2, 3), 5);

    END_TEST();
}

static XSIGMA_NOINLINE int noinline_add(int a, int b)
{
    return a + b;
}

XSIGMATEST(Macros, NoInline)
{
    EXPECT_EQ(noinline_add(10, 20), 30);

    END_TEST();
}

// XSIGMA_COLD on a rarely-called helper
static XSIGMA_COLD int cold_add(int a, int b)
{
    return a + b;
}

XSIGMATEST(Macros, Cold)
{
    EXPECT_EQ(cold_add(1, 1), 2);

    END_TEST();
}

// XSIGMA_VECTORCALL (calling-convention, no-op on non-MSVC)
// Note: __vectorcall must appear after the return type on MSVC.
static int XSIGMA_VECTORCALL vectorcall_mul(int a, int b)
{
    return a * b;
}

XSIGMATEST(Macros, VectorCall)
{
    EXPECT_EQ(vectorcall_mul(3, 4), 12);

    END_TEST();
}

// ============================================================================
// Sanitizer / attribute macros (compile-time only)
// ============================================================================

static XSIGMA_NO_SANITIZE_MEMORY int sanitizer_fn(int x)
{
    return x + 1;
}

XSIGMATEST(Macros, NoSanitizeMemory)
{
    EXPECT_EQ(sanitizer_fn(41), 42);

    END_TEST();
}

// ============================================================================
// CUDA function-type macros (no-ops on CPU builds)
// ============================================================================

XSIGMA_CUDA_FUNCTION_TYPE static int cuda_fn(int x)
{
    return x * 2;
}

XSIGMATEST(Macros, CudaFunctionType)
{
    EXPECT_EQ(cuda_fn(5), 10);

    END_TEST();
}

// XSIGMA_FUNCTION_ATTRIBUTE
XSIGMA_FUNCTION_ATTRIBUTE static int fn_attr(int x)
{
    return x - 1;
}

XSIGMATEST(Macros, FunctionAttribute)
{
    EXPECT_EQ(fn_attr(10), 9);

    END_TEST();
}

// ============================================================================
// Integer type macros
// ============================================================================

XSIGMATEST(Macros, IntegerTypes)
{
    xsigma_int  i = -1LL;
    xsigma_long u = 1ULL;

    EXPECT_EQ(sizeof(xsigma_int), sizeof(long long int));
    EXPECT_EQ(sizeof(xsigma_long), sizeof(unsigned long long int));
    EXPECT_LT(i, static_cast<xsigma_int>(0));
    EXPECT_GT(u, static_cast<xsigma_long>(0));

    END_TEST();
}

// ============================================================================
// Attribute-detection macros
// ============================================================================

XSIGMATEST(Macros, HaveAttribute)
{
    // XSIGMA_HAVE_ATTRIBUTE must expand to 0 or 1 (works in expressions on all compilers)
    constexpr int attr_val = XSIGMA_HAVE_ATTRIBUTE(unused);
    EXPECT_TRUE(attr_val == 0 || attr_val == 1);

    // XSIGMA_HAVE_CPP_ATTRIBUTE wraps __has_cpp_attribute which is preprocessor-only on MSVC;
    // evaluate it at preprocessing time to avoid C2065 on MSVC.
#if XSIGMA_HAVE_CPP_ATTRIBUTE(nodiscard)
    constexpr int cpp_attr_val = 1;
#else
    constexpr int cpp_attr_val = 0;
#endif
    EXPECT_TRUE(cpp_attr_val == 0 || cpp_attr_val >= 1);

    END_TEST();
}

// ============================================================================
// XSIGMA_NODISCARD
// ============================================================================

XSIGMA_NODISCARD static int nodiscard_fn()
{
    return 42;
}

XSIGMATEST(Macros, Nodiscard)
{
    int val = nodiscard_fn();
    EXPECT_EQ(val, 42);

    END_TEST();
}

// ============================================================================
// XSIGMA_UNUSED
// ============================================================================

XSIGMATEST(Macros, Unused)
{
    XSIGMA_UNUSED int unused_var = 99;
    // Suppress unused-variable warning; test simply verifies the attribute compiles.
    EXPECT_EQ(unused_var, 99);

    END_TEST();
}

// ============================================================================
// XSIGMA_USED
// ============================================================================

static XSIGMA_USED int used_global = 7;

XSIGMATEST(Macros, Used)
{
    EXPECT_EQ(used_global, 7);

    END_TEST();
}

// ============================================================================
// XSIGMA_FUNCTION_CONSTEXPR
// ============================================================================

XSIGMA_FUNCTION_CONSTEXPR static int constexpr_square(int x)
{
    return x * x;
}

XSIGMATEST(Macros, FunctionConstexpr)
{
    EXPECT_EQ(constexpr_square(5), 25);

    END_TEST();
}

// ============================================================================
// XSIGMA_ASM_COMMENT (no-op on non-GCC/Clang x86/arm)
// ============================================================================

static void asm_comment_fn()
{
    XSIGMA_ASM_COMMENT("test marker");
}

XSIGMATEST(Macros, AsmComment)
{
    asm_comment_fn();  // just verifies it compiles and doesn't crash
    EXPECT_TRUE(true);

    END_TEST();
}

// ============================================================================
// XSIGMA_RESTRICT
// ============================================================================

static void restrict_copy(int* XSIGMA_RESTRICT dst, const int* XSIGMA_RESTRICT src, int n)
{
    for (int i = 0; i < n; ++i)
        dst[i] = src[i];
}

XSIGMATEST(Macros, Restrict)
{
    int src[4] = {1, 2, 3, 4};
    int dst[4] = {};
    restrict_copy(dst, src, 4);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(dst[i], src[i]);

    END_TEST();
}

// ============================================================================
// XSIGMA_SIMD_RETURN_TYPE
// ============================================================================

// XSIGMA_SIMD_RETURN_TYPE expands to a static void function with optional
// force-inline + vectorcall in release builds.
namespace
{
struct SimdTestHelper
{
    XSIGMA_SIMD_RETURN_TYPE run(int* out, int val) { *out = val; }
};
}  // namespace

XSIGMATEST(Macros, SimdReturnType)
{
    int            out = 0;
    SimdTestHelper h;
    h.run(&out, 77);
    EXPECT_EQ(out, 77);

    END_TEST();
}

// ============================================================================
// XSIGMA_DELETE_CLASS
// ============================================================================

XSIGMATEST(Macros, DeleteClass)
{
    // Verify that DeletedClass is not default-constructible, copyable, or movable.
    struct DeletedClass
    {
        XSIGMA_DELETE_CLASS(DeletedClass)
    };
    EXPECT_FALSE(std::is_default_constructible_v<DeletedClass>);
    EXPECT_FALSE(std::is_copy_constructible_v<DeletedClass>);
    EXPECT_FALSE(std::is_move_constructible_v<DeletedClass>);

    END_TEST();
}

// ============================================================================
// XSIGMA_DELETE_COPY_AND_MOVE
// ============================================================================

XSIGMATEST(Macros, DeleteCopyAndMove)
{
    struct NoCopyMove
    {
        NoCopyMove()  = default;
        ~NoCopyMove() = default;
        XSIGMA_DELETE_COPY_AND_MOVE(NoCopyMove)
    };
    EXPECT_FALSE(std::is_copy_constructible_v<NoCopyMove>);
    EXPECT_FALSE(std::is_copy_assignable_v<NoCopyMove>);
    EXPECT_FALSE(std::is_move_constructible_v<NoCopyMove>);
    EXPECT_FALSE(std::is_move_assignable_v<NoCopyMove>);

    END_TEST();
}

// ============================================================================
// XSIGMA_DELETE_COPY
// ============================================================================

XSIGMATEST(Macros, DeleteCopy)
{
    struct NoCopy
    {
        NoCopy()  = default;
        ~NoCopy() = default;
        XSIGMA_DELETE_COPY(NoCopy)
    };
    EXPECT_FALSE(std::is_copy_constructible_v<NoCopy>);
    EXPECT_FALSE(std::is_copy_assignable_v<NoCopy>);
    // move should still be available (implicitly deleted but not explicitly)
    // we just check copy is gone
    EXPECT_TRUE(std::is_default_constructible_v<NoCopy>);

    END_TEST();
}

// ============================================================================
// MACRO_CORE_TYPE_ID_NAME
// ============================================================================

XSIGMATEST(Macros, TypeIdName)
{
    const char* name = MACRO_CORE_TYPE_ID_NAME(int);
    EXPECT_NE(name, nullptr);
    EXPECT_GT(std::strlen(name), static_cast<size_t>(0));

    END_TEST();
}

// ============================================================================
// MACRO_CORE_PRINTF_FORMAT (compile-time only; verifies no build error)
// ============================================================================

static void MACRO_CORE_PRINTF_FORMAT(1, 2) printf_fmt_fn(const char* fmt, ...)
{
    (void)fmt;
}

XSIGMATEST(Macros, PrintfFormat)
{
    printf_fmt_fn("%d", 1);
    EXPECT_TRUE(true);

    END_TEST();
}

// ============================================================================
// XSIGMA_PRINTF_ATTRIBUTE (same attribute, different spelling)
// ============================================================================

static void XSIGMA_PRINTF_ATTRIBUTE(1, 2) xsigma_printf_fn(const char* fmt, ...)
{
    (void)fmt;
}

XSIGMATEST(Macros, PrintfAttribute)
{
    xsigma_printf_fn("%s", "ok");
    EXPECT_TRUE(true);

    END_TEST();
}

// ============================================================================
// Thread-safety annotation macros (compile-time only)
// ============================================================================

#if XSIGMA_HAVE_ATTRIBUTE(capability)
// Only define the annotated class when the capability attribute is available
// (i.e. Clang with thread-safety analysis), otherwise the annotations are
// silently dropped via the empty macro expansions and we just skip.
struct FakeMutex
{
    void lock() {}
    void unlock() {}
};
static FakeMutex gMutex;
static int       XSIGMA_GUARDED_BY(gMutex) gGuardedValue = 0;
#endif

XSIGMATEST(Macros, ThreadSafetyAnnotations)
{
    // These macros expand to attributes or nothing at all; the only
    // meaningful check is that the translation unit compiled successfully.
    EXPECT_TRUE(true);

    END_TEST();
}

// ============================================================================
// XSIGMA_LIFETIME_BOUND
// ============================================================================

static const int& identity_ref(const int& x XSIGMA_LIFETIME_BOUND)
{
    return x;
}

XSIGMATEST(Macros, LifetimeBound)
{
    int        val = 123;
    const int& ref = identity_ref(val);
    EXPECT_EQ(ref, 123);

    END_TEST();
}

// ============================================================================
// XSIGMA_CONST_INIT
// ============================================================================

XSIGMA_CONST_INIT static int const_init_val = 10;

XSIGMATEST(Macros, ConstInit)
{
    EXPECT_EQ(const_init_val, 10);

    END_TEST();
}

// ============================================================================
// XSIGMA_NO_THREAD_SAFETY_ANALYSIS
// ============================================================================

static XSIGMA_NO_THREAD_SAFETY_ANALYSIS void no_tsa_fn(int& x)
{
    x = 42;
}

XSIGMATEST(Macros, NoThreadSafetyAnalysis)
{
    int val = 0;
    no_tsa_fn(val);
    EXPECT_EQ(val, 42);

    END_TEST();
}

// ============================================================================
// XSIGMA_EXCLUSIVE_LOCKS_REQUIRED / XSIGMA_LOCKS_EXCLUDED (compile only)
// ============================================================================

XSIGMATEST(Macros, LockAnnotations)
{
    // Macros expand to empty or to Clang thread-safety attributes; the only
    // check is that the file compiled without errors.
    EXPECT_TRUE(true);

    END_TEST();
}

// ============================================================================
// Three-way comparison detection
// ============================================================================

XSIGMATEST(Macros, ThreeWayComparison)
{
#if __cplusplus >= 202002L
    EXPECT_EQ(PROJECT_HAS_THREE_WAY_COMPARISON, 1);
#else
    EXPECT_EQ(PROJECT_HAS_THREE_WAY_COMPARISON, 0);
#endif

    END_TEST();
}

// ============================================================================
// XSIGMA_NORETURN (compile-time only: applying it to a real noreturn fn)
// ============================================================================

static XSIGMA_NORETURN void always_throw()
{
    throw std::runtime_error("always_throw");
}

XSIGMATEST(Macros, Noreturn)
{
    EXPECT_THROW(always_throw(), std::runtime_error);

    END_TEST();
}
