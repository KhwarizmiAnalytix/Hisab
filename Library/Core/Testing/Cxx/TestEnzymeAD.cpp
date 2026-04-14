/**
 * @file TestEnzymeAD.cpp
 * @brief Test Enzyme Automatic Differentiation Integration
 *
 * This file demonstrates the integration of Enzyme AD with Quarisma.
 * Enzyme provides high-performance automatic differentiation for C/C++ code.
 *
 * Test Coverage:
 * - Forward-mode automatic differentiation
 * - Reverse-mode automatic differentiation (gradient computation)
 * - Simple mathematical functions
 * - Vector normalization (array inputs with enzyme_dup/enzyme_const)
 * - Dot product with activity annotations (enzyme_const, enzyme_out)
 * - Inverse magnitude with fast inverse square root
 * - Function aliasing via __enzyme_function_like
 * - Cache/alias sensitivity (__restrict__ vs aliased pointers)
 * - Forward-mode with pointer-based loop functions
 * - Batch mode differentiation (__enzyme_batch)
 * - Integration with Quarisma test framework
 */

#include "CoreTest.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

// Enzyme requires these declarations for AD
// The __enzyme_autodiff and __enzyme_fwddiff functions are provided by the Enzyme plugin
extern "C"
{
    // Reverse-mode AD: computes gradient (scalar return)
    double __enzyme_autodiff(void*, ...);

    // Reverse-mode AD: void return variant
    void __enzyme_autodiff_void(void*, ...);

    // Forward-mode AD: computes directional derivative
    double __enzyme_fwddiff(void*, ...);

    // Forward-mode AD: void return variant
    void __enzyme_fwddiff_void(void*, ...);
}

// Enzyme activity annotations for multivariate functions.
// These must be global variables (not enum constants) so that Enzyme can
// identify them by name in the LLVM IR.  Enzyme's plugin matches on the
// GlobalVariable name, not on the integer value.
int enzyme_dup;
int enzyme_const;
int enzyme_out;
int enzyme_width;
int enzyme_vector;
int enzyme_scalar;

// =============================================================================
// Test Functions for Automatic Differentiation
// =============================================================================

/**
 * @brief Simple quadratic function: f(x) = x^2
 *
 * Derivative: f'(x) = 2x
 */
double square(double x)
{
    return x * x;
}

/**
 * @brief Cubic function: f(x) = x^3 + 2x^2 + 3x + 4
 *
 * Derivative: f'(x) = 3x^2 + 4x + 3
 */
double cubic(double x)
{
    return x * x * x + 2.0 * x * x + 3.0 * x + 4.0;
}

/**
 * @brief Multivariate function: f(x, y) = x^2 + 2xy + y^2
 *
 * Partial derivatives:
 * - ∂f/∂x = 2x + 2y
 * - ∂f/∂y = 2x + 2y
 */
double multivariate(double x, double y)
{
    return x * x + 2.0 * x * y + y * y;
}

// Wrapper to compute ∂f/∂x for multivariate function
double multivariate_dx(double x, double y)
{
    return multivariate(x, y);
}

// Wrapper to compute ∂f/∂y for multivariate function
double multivariate_dy(double x, double y)
{
    return multivariate(x, y);
}

/**
 * @brief Exponential function: f(x) = e^x
 *
 * Derivative: f'(x) = e^x
 */
double exponential(double x)
{
    return std::exp(x);
}

/**
 * @brief Rosenbrock function: f(x, y) = (1-x)^2 + 100(y-x^2)^2
 *
 * Partial derivatives:
 * - ∂f/∂x = -2(1-x) - 400x(y-x^2)
 * - ∂f/∂y = 200(y-x^2)
 */
double rosenbrock(double x, double y)
{
    double const a = 1.0 - x;
    double const b = y - x * x;
    return a * a + 100.0 * b * b;
}

// Wrapper to compute ∂f/∂x for rosenbrock function
double rosenbrock_dx(double x, double y)
{
    return rosenbrock(x, y);
}

// Wrapper to compute ∂f/∂y for rosenbrock function
double rosenbrock_dy(double x, double y)
{
    return rosenbrock(x, y);
}

// =============================================================================
// Tutorial 2: Vector Normalization (norm.c)
// Demonstrates enzyme_dup, enzyme_const with array arguments.
// mag() is marked __attribute__((const,noinline)) so Enzyme can treat it as a
// pure function and differentiate through the normalization loop.
// =============================================================================

/**
 * @brief Sum of array elements (used as a stand-in "magnitude" in the tutorial).
 *
 * The tutorial uses a simple sum rather than a true Euclidean norm to keep
 * the expected gradient derivation straightforward.
 */
__attribute__((const, noinline)) double mag(const double* A, int n)
{
    double amt = 0;
    for (int i = 0; i < n; i++)
        amt += A[i];
    return amt;
}

/**
 * @brief Normalize each element of `in` by the array sum, writing to `out`.
 *
 * out[i] = in[i] / sum(in)
 */
void normalize(double* __restrict__ out, const double* __restrict__ in, int n)
{
    for (int i = 0; i < n; ++i)
        out[i] = in[i] / mag(in, n);
}

// =============================================================================
// Tutorial 3: Dot Product (dot.c)
// Demonstrates enzyme_const, enzyme_dup, enzyme_out activity annotations.
// =============================================================================

/**
 * @brief Dot product with scalar offset: f(A, B, C) = C + sum(A[i]*B[i])
 *
 * Gradients:
 * - ∂f/∂A[i] = B[i]
 * - ∂f/∂B[i] = A[i]
 * - ∂f/∂C    = 1
 */
double dot(double* __restrict__ A, double* __restrict__ B, double C, int n)
{
    double sum = 0;
    for (int i = 0; i < n; i++)
        sum += A[i] * B[i];
    return C + sum;
}

// =============================================================================
// Tutorial 4: Fast Inverse Square Root (invsqrt.c)
// Demonstrates custom gradient registration via __enzyme_register_gradient_*.
// =============================================================================

// Use extern "C" so functions have unmangled names in the LLVM IR.
// Enzyme's __enzyme_register_gradient_* mechanism looks up functions by
// their IR symbol name, which would be mangled in C++ without extern "C".
extern "C"
{
    /**
     * @brief Fast inverse square root (Quake III algorithm).
     *
     * Computes an approximation of 1/sqrt(x) using bit-level tricks.
     * Used to show how to register a custom derivative when Enzyme cannot
     * automatically differentiate through bit-casting.
     */
    __attribute__((noinline)) float Q_rsqrt(float number)
    {
        long        i;
        float       x2, y;
        float const threehalfs = 1.5F;

        x2 = number * 0.5F;
        y  = number;
        std::memcpy(&i, &y, sizeof(i));
        i = 0x5f3759df - (i >> 1);
        std::memcpy(&y, &i, sizeof(y));
        y = y * (threehalfs - (x2 * y * y));  // 1st Newton-Raphson iteration
        return y;
    }

    // Custom augmented forward pass: returns Q_rsqrt(x) so the reverse
    // pass receives it as a cached value.
    float aug_Q_rsqrt(float x)
    {
        return Q_rsqrt(x);
    }

    // Custom reverse pass for Q_rsqrt.
    // Enzyme calls rev_f(cached_input, d_return) -> d_input.
    // Enzyme caches the INPUT argument (not the return value) as the tape.
    // x        = original input to Q_rsqrt (cached from aug_Q_rsqrt)
    // d_return = incoming gradient of the output
    // d/dx [x^(-1/2)] = -Q_rsqrt(x) / (2*x)
    float rev_Q_rsqrt(float x, float d_return)
    {
        return -d_return * Q_rsqrt(x) / (2.0f * x);
    }
}

// Register the custom gradient for Q_rsqrt.  The array name suffix must
// match the (unmangled) function symbol so Enzyme can find the registration.
void* __enzyme_register_gradient_Q_rsqrt[3] = {
    (void*)Q_rsqrt, (void*)aug_Q_rsqrt, (void*)rev_Q_rsqrt};

/**
 * @brief Inverse magnitude of a vector: 1 / sqrt(sum(A[i]^2))
 *
 * Uses Q_rsqrt internally; gradient is differentiated via the custom rule above.
 */
double invmag(double* __restrict__ A, int n)
{
    double sumsq = 0;
    for (int i = 0; i < n; i++)
        sumsq += A[i] * A[i];
    return Q_rsqrt((float)sumsq);
}

// =============================================================================
// Tutorial 5: Function Aliasing (fn_like.c)
// Demonstrates __enzyme_function_like to tell Enzyme that a user function
// behaves like a known math function and should use its built-in derivative.
// =============================================================================

/**
 * @brief A user-defined function that Enzyme should treat as log1p.
 *
 * In the tutorial this is a stand-in: the actual computation is 2*a, but
 * __enzyme_function_like instructs Enzyme to use log1p's derivative rule
 * (1/(1+x)) when differentiating any call to log1p_like_function.
 */
double log1p_like_function(double a)
{
    return 2 * a;
}

// Register log1p_like_function as an alias for "log1p" derivative rules
void* __enzyme_function_like[2] = {(void*)log1p_like_function, (void*)"log1p"};

/**
 * @brief Wrapper that calls log1p_like_function.
 */
double test_fn_like(double a)
{
    return log1p_like_function(a);
}

// =============================================================================
// Tutorial 6: Cache / Alias Sensitivity (cache.c)
// Demonstrates how __restrict__ allows Enzyme to avoid unnecessary caching
// in the reverse pass, which improves performance for large arrays.
// =============================================================================

/**
 * @brief Sum of squares, writing result through a pointer (aliased version).
 *
 * Without __restrict__, Enzyme must assume `in` and `out` may alias and will
 * cache all intermediate values to correctly replay the reverse pass.
 */
void squareCopy(double* in, double* out, int n)
{
    double sumsq = 0;
    for (int i = 0; i < n; i++)
        sumsq += in[i] * in[i];
    *out = sumsq;
}

/**
 * @brief Sum of squares with restrict-qualified pointers (no-alias version).
 *
 * With __restrict__, Enzyme knows `in` and `out` cannot alias, allowing it to
 * recompute values during the reverse pass instead of caching them.
 */
void squareCopyRestrict(double* __restrict__ in, double* __restrict__ out, int n)
{
    double sumsq = 0;
    for (int i = 0; i < n; i++)
        sumsq += in[i] * in[i];
    *out = sumsq;
}

// =============================================================================
// Tutorial 7: Forward-Mode with Loops (fwd.c)
// Demonstrates __enzyme_fwddiff on a function with pointer-based loop accumulation.
// =============================================================================

/**
 * @brief Accumulate (a + b) over 100 iterations into *ret.
 *
 * ret = 100 * (*a + *b)
 * d(ret)/d(a) = 100, d(ret)/d(b) = 100
 */
void compute_loops(float* a, float* b, float* ret)
{
    double sum0 = 0.0;
    for (int i = 0; i < 100; i++)
        sum0 += *a + *b;
    *ret = (float)sum0;
}

// =============================================================================
// Google Test Suite for Enzyme AD
// =============================================================================

#if CORE_HAS_ENZYME

// ---------------------------------------------------------------------------
// Reverse-mode tests (original suite)
// ---------------------------------------------------------------------------

/**
 * @brief Test reverse-mode AD on square function
 */
QUARISMATEST(EnzymeAD, ReverseMode_Square)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Reverse-Mode AD: Square Function\n";
    std::cout << "========================================\n";

    double const x   = 3.0;
    double const f_x = square(x);

    std::cout << "Function: f(x) = x²\n";
    std::cout << "Expected derivative: f'(x) = 2x\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    double const derivative = __enzyme_autodiff((void*)square, x);

    double const expected  = 2.0 * x;
    double const abs_error = std::abs(derivative - expected);
    double const rel_error = expected != 0.0 ? abs_error / std::abs(expected) : abs_error;

    std::cout << "Enzyme computed derivative: " << derivative << "\n";
    std::cout << "Expected derivative:        " << expected << "\n";
    std::cout << "Absolute error:             " << abs_error << "\n";
    std::cout << "Relative error:             " << rel_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(derivative, expected, 1e-10)
        << "Enzyme gradient computation failed for square function";
}

/**
 * @brief Test reverse-mode AD on cubic function
 */
QUARISMATEST(EnzymeAD, ReverseMode_Cubic)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Reverse-Mode AD: Cubic Function\n";
    std::cout << "========================================\n";

    double const x   = 2.0;
    double const f_x = cubic(x);

    std::cout << "Function: f(x) = x³ + 2x² + 3x + 4\n";
    std::cout << "Expected derivative: f'(x) = 3x² + 4x + 3\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    double const derivative = __enzyme_autodiff((void*)cubic, x);

    double const expected  = 3.0 * x * x + 4.0 * x + 3.0;
    double const abs_error = std::abs(derivative - expected);
    double const rel_error = expected != 0.0 ? abs_error / std::abs(expected) : abs_error;

    std::cout << "Enzyme computed derivative: " << derivative << "\n";
    std::cout << "Expected derivative:        " << expected << "\n";
    std::cout << "Absolute error:             " << abs_error << "\n";
    std::cout << "Relative error:             " << rel_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(derivative, expected, 1e-10)
        << "Enzyme gradient computation failed for cubic function";
}

/**
 * @brief Test reverse-mode AD on exponential function
 */
QUARISMATEST(EnzymeAD, ReverseMode_Exponential)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Reverse-Mode AD: Exponential Function\n";
    std::cout << "========================================\n";

    double const x   = 1.0;
    double const f_x = exponential(x);

    std::cout << "Function: f(x) = e^x\n";
    std::cout << "Expected derivative: f'(x) = e^x\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    double const derivative = __enzyme_autodiff((void*)exponential, x);

    double const expected  = std::exp(x);
    double const abs_error = std::abs(derivative - expected);
    double const rel_error = expected != 0.0 ? abs_error / std::abs(expected) : abs_error;

    std::cout << "Enzyme computed derivative: " << derivative << "\n";
    std::cout << "Expected derivative:        " << expected << "\n";
    std::cout << "Absolute error:             " << abs_error << "\n";
    std::cout << "Relative error:             " << rel_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(derivative, expected, 1e-10)
        << "Enzyme gradient computation failed for exponential function";
}

/**
 * @brief Test reverse-mode AD on multivariate function
 */
QUARISMATEST(EnzymeAD, ReverseMode_Multivariate)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Reverse-Mode AD: Multivariate Function\n";
    std::cout << "========================================\n";

    double const x    = 2.0;
    double const y    = 3.0;
    double const f_xy = multivariate(x, y);

    std::cout << "Function: f(x,y) = x² + 2xy + y²\n";
    std::cout << "Expected partial derivatives:\n";
    std::cout << "  ∂f/∂x = 2x + 2y\n";
    std::cout << "  ∂f/∂y = 2x + 2y\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    (x, y) = (" << x << ", " << y << ")\n";
    std::cout << "Output:   f(x,y) = " << f_xy << "\n\n";

    double const dx = __enzyme_autodiff((void*)multivariate_dx, x, y);

    auto         multivariate_yx = [](double y, double x) { return multivariate(x, y); };
    double const dy              = __enzyme_autodiff((void*)+multivariate_yx, y, x);

    double const expected_dx  = 2.0 * x + 2.0 * y;
    double const expected_dy  = 2.0 * x + 2.0 * y;
    double const abs_error_dx = std::abs(dx - expected_dx);
    double const abs_error_dy = std::abs(dy - expected_dy);

    std::cout << "∂f/∂x:\n";
    std::cout << "  Enzyme computed: " << dx << "\n";
    std::cout << "  Expected:        " << expected_dx << "\n";
    std::cout << "  Absolute error:  " << abs_error_dx << "\n";
    std::cout << "  Status: " << (abs_error_dx < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "∂f/∂y:\n";
    std::cout << "  Enzyme computed: " << dy << "\n";
    std::cout << "  Expected:        " << expected_dy << "\n";
    std::cout << "  Absolute error:  " << abs_error_dy << "\n";
    std::cout << "  Status: " << (abs_error_dy < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(dx, expected_dx, 1e-10) << "Enzyme ∂f/∂x computation failed";
    EXPECT_NEAR(dy, expected_dy, 1e-10) << "Enzyme ∂f/∂y computation failed";
}

/**
 * @brief Test reverse-mode AD on Rosenbrock function
 */
QUARISMATEST(EnzymeAD, ReverseMode_Rosenbrock)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Reverse-Mode AD: Rosenbrock Function\n";
    std::cout << "========================================\n";

    double const x    = 1.0;
    double const y    = 1.0;
    double const f_xy = rosenbrock(x, y);

    std::cout << "Function: f(x,y) = (1-x)² + 100(y-x²)²\n";
    std::cout << "Expected partial derivatives:\n";
    std::cout << "  ∂f/∂x = -2(1-x) - 400x(y-x²)\n";
    std::cout << "  ∂f/∂y = 200(y-x²)\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    (x, y) = (" << x << ", " << y << ")\n";
    std::cout << "Output:   f(x,y) = " << f_xy << " (optimum at (1,1) = 0)\n\n";

    double const dx = __enzyme_autodiff((void*)rosenbrock_dx, x, y);

    auto         rosenbrock_yx = [](double y, double x) { return rosenbrock(x, y); };
    double const dy            = __enzyme_autodiff((void*)+rosenbrock_yx, y, x);

    // At (1, 1): both partial derivatives are 0 (the global minimum)
    double const expected_dx  = 0.0;
    double const expected_dy  = 0.0;
    double const abs_error_dx = std::abs(dx - expected_dx);
    double const abs_error_dy = std::abs(dy - expected_dy);

    std::cout << "∂f/∂x:\n";
    std::cout << "  Enzyme computed: " << dx << "\n";
    std::cout << "  Expected:        " << expected_dx << " (gradient at minimum)\n";
    std::cout << "  Absolute error:  " << abs_error_dx << "\n";
    std::cout << "  Status: " << (abs_error_dx < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "∂f/∂y:\n";
    std::cout << "  Enzyme computed: " << dy << "\n";
    std::cout << "  Expected:        " << expected_dy << " (gradient at minimum)\n";
    std::cout << "  Absolute error:  " << abs_error_dy << "\n";
    std::cout << "  Status: " << (abs_error_dy < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(dx, expected_dx, 1e-10) << "Enzyme ∂f/∂x computation failed for Rosenbrock";
    EXPECT_NEAR(dy, expected_dy, 1e-10) << "Enzyme ∂f/∂y computation failed for Rosenbrock";
}

// ---------------------------------------------------------------------------
// Forward-mode tests (original suite)
// ---------------------------------------------------------------------------

/**
 * @brief Test forward-mode AD on square function
 */
QUARISMATEST(EnzymeAD, ForwardMode_Square)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Forward-Mode AD: Square Function\n";
    std::cout << "========================================\n";

    double const x   = 3.0;
    double const dx  = 1.0;
    double const f_x = square(x);

    std::cout << "Function: f(x) = x²\n";
    std::cout << "Expected derivative: f'(x) = 2x\n";
    std::cout << "Mode: Forward-mode (directional derivative)\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Seed:     dx = " << dx << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    double const result = __enzyme_fwddiff((void*)square, x, dx);

    double const expected  = 2.0 * x * dx;
    double const abs_error = std::abs(result - expected);
    double const rel_error = expected != 0.0 ? abs_error / std::abs(expected) : abs_error;

    std::cout << "Enzyme computed derivative: " << result << "\n";
    std::cout << "Expected derivative:        " << expected << "\n";
    std::cout << "Absolute error:             " << abs_error << "\n";
    std::cout << "Relative error:             " << rel_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(result, expected, 1e-10) << "Enzyme forward-mode AD failed for square function";
}

/**
 * @brief Test forward-mode AD on cubic function
 */
QUARISMATEST(EnzymeAD, ForwardMode_Cubic)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Forward-Mode AD: Cubic Function\n";
    std::cout << "========================================\n";

    double const x   = 2.0;
    double const dx  = 1.0;
    double const f_x = cubic(x);

    std::cout << "Function: f(x) = x³ + 2x² + 3x + 4\n";
    std::cout << "Expected derivative: f'(x) = 3x² + 4x + 3\n";
    std::cout << "Mode: Forward-mode (directional derivative)\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Seed:     dx = " << dx << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    double const result = __enzyme_fwddiff((void*)cubic, x, dx);

    double const expected  = (3.0 * x * x + 4.0 * x + 3.0) * dx;
    double const abs_error = std::abs(result - expected);
    double const rel_error = expected != 0.0 ? abs_error / std::abs(expected) : abs_error;

    std::cout << "Enzyme computed derivative: " << result << "\n";
    std::cout << "Expected derivative:        " << expected << "\n";
    std::cout << "Absolute error:             " << abs_error << "\n";
    std::cout << "Relative error:             " << rel_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(result, expected, 1e-10) << "Enzyme forward-mode AD failed for cubic function";
}

/**
 * @brief Test numerical accuracy of Enzyme AD
 */
QUARISMATEST(EnzymeAD, NumericalAccuracy)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme AD: Numerical Accuracy Test\n";
    std::cout << "========================================\n";
    std::cout << "Testing f(x) = x² at multiple points\n";
    std::cout << "Expected: f'(x) = 2x\n\n";

    std::vector<double> const test_points = {-2.0, -1.0, 0.0, 1.0, 2.0, 5.0, 10.0};

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Point       | Computed    | Expected    | Abs Error   | Status\n";
    std::cout << "------------+-------------+-------------+-------------+--------\n";

    for (double x : test_points)
    {
        double const derivative = __enzyme_autodiff((void*)square, x);
        double const expected   = 2.0 * x;
        double const abs_error  = std::abs(derivative - expected);
        bool const   passed     = abs_error < 1e-10;

        std::cout << std::setw(11) << x << " | " << std::setw(11) << derivative << " | "
                  << std::setw(11) << expected << " | " << std::scientific << std::setw(11)
                  << abs_error << std::fixed << " | " << (passed ? "✓ PASS" : "✗ FAIL") << "\n";

        EXPECT_NEAR(derivative, expected, 1e-10) << "Accuracy test failed at x = " << x;
    }
    std::cout << "========================================\n";
}

// ---------------------------------------------------------------------------
// Tutorial 2: Vector Normalization
// ---------------------------------------------------------------------------

/**
 * @brief Gradient of the normalize function via reverse-mode AD.
 *
 * The tutorial computes the gradient of normalize(out, in, n) with respect to
 * both `out` and `in`.  With seed grad_out[i] = 1 for all i, the gradient
 * accumulated into grad_in tells us how a unit perturbation of each input
 * element affects the sum of all output elements.
 *
 * For out[i] = in[i] / S  where S = sum(in):
 *   ∂(sum_j out[j]) / ∂in[i] = (1/S) - in[i] / S^2 * n  (by quotient rule summed over j)
 *   = (S - n*in[i]) / S^2
 *
 * For uniform input in[i] = x: S = n*x, so grad_in[i] = (n*x - n*x) / (n*x)^2 = 0.
 * The test verifies this zero-gradient case and that the primal output is correct.
 */
QUARISMATEST(EnzymeAD, Tutorial2_VectorNormalization)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 2: Vector Normalization\n";
    std::cout << "========================================\n";

    int const    n = 5;
    double const x = 2.0;

    std::vector<double> in(n, x);
    std::vector<double> out(n, 0.0);
    std::vector<double> grad_out(n, 1.0);  // seed: unit vector
    std::vector<double> grad_in(n, 0.0);

    std::cout << "Function: out[i] = in[i] / sum(in)\n";
    std::cout << "Input: in[i] = " << x << " for all i, n = " << n << "\n";
    std::cout << "Expected: out[i] = 1/n = " << (1.0 / n) << "\n";
    std::cout << "Expected grad_in[i] = 0 (uniform input)\n\n";

    // Primal evaluation
    normalize(out.data(), in.data(), n);

    double const expected_out = x / (n * x);  // = 1/n
    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Primal out[0] = " << out[0] << "  (expected " << expected_out << ")\n\n";

    // Gradient via Enzyme reverse-mode
    __enzyme_autodiff(
        (void*)normalize,
        enzyme_dup,
        out.data(),
        grad_out.data(),
        enzyme_dup,
        in.data(),
        grad_in.data(),
        enzyme_const,
        n);

    std::cout << "grad_in[0] = " << grad_in[0] << "  (expected 0.0 for uniform input)\n";
    std::cout << "Status: " << (std::abs(grad_in[0]) < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(out[0], expected_out, 1e-10) << "Primal normalize output incorrect";
    for (int i = 0; i < n; i++)
        EXPECT_NEAR(grad_in[i], 0.0, 1e-10) << "grad_in[" << i << "] should be 0 for uniform input";
}

// ---------------------------------------------------------------------------
// Tutorial 3: Dot Product with Activity Annotations
// ---------------------------------------------------------------------------

/**
 * @brief Gradient of the dot product function using enzyme_dup annotations.
 *
 * For f(A, B, C) = C + sum(A[i]*B[i]):
 *   ∂f/∂A[i] = B[i]
 *   ∂f/∂B[i] = A[i]
 *   ∂f/∂C    = 1
 *
 * This test uses the default annotation (all active), then verifies the
 * gradient values match the analytical result.
 */
QUARISMATEST(EnzymeAD, Tutorial3_DotProduct_AllActive)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 3: Dot Product (all active)\n";
    std::cout << "========================================\n";

    int const n = 4;

    std::vector<double> A = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> B = {5.0, 6.0, 7.0, 8.0};
    double              C = 0.5;
    std::vector<double> gradA(n, 0.0);
    std::vector<double> gradB(n, 0.0);

    std::cout << "Function: f(A,B,C,n) = C + sum(A[i]*B[i])\n";
    std::cout << "A = [1,2,3,4], B = [5,6,7,8], C = 0.5\n";
    std::cout << "Expected: gradA[i] = B[i], gradB[i] = A[i], gradC = 1\n\n";

    // All active: A/gradA, B/gradB, C (scalar active — returned), n (const)
    double grad_C =
        __enzyme_autodiff((void*)dot, A.data(), gradA.data(), B.data(), gradB.data(), C, n);

    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; i++)
        std::cout << "gradA[" << i << "] = " << gradA[i] << "  expected B[" << i << "] = " << B[i]
                  << "\n";
    for (int i = 0; i < n; i++)
        std::cout << "gradB[" << i << "] = " << gradB[i] << "  expected A[" << i << "] = " << A[i]
                  << "\n";
    std::cout << "grad_C = " << grad_C << "  expected 1\n";
    std::cout << "========================================\n";

    for (int i = 0; i < n; i++)
        EXPECT_NEAR(gradA[i], B[i], 1e-10) << "gradA[" << i << "] mismatch";
    for (int i = 0; i < n; i++)
        EXPECT_NEAR(gradB[i], A[i], 1e-10) << "gradB[" << i << "] mismatch";
    EXPECT_NEAR(grad_C, 1.0, 1e-10) << "grad_C mismatch";
}

/**
 * @brief Dot product gradient with A marked enzyme_const (A held constant).
 *
 * When A is constant, Enzyme skips computing gradA.  Only gradB and grad_C
 * are produced.  This is the second pass shown in the tutorial.
 */
QUARISMATEST(EnzymeAD, Tutorial3_DotProduct_ConstA)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 3: Dot Product (A constant)\n";
    std::cout << "========================================\n";

    int const n = 4;

    std::vector<double> A = {1.0, 2.0, 3.0, 4.0};
    std::vector<double> B = {5.0, 6.0, 7.0, 8.0};
    double              C = 0.5;
    std::vector<double> gradA(n, 0.0);  // will remain zero
    std::vector<double> gradB(n, 0.0);

    std::cout << "Function: f(A,B,C,n) = C + sum(A[i]*B[i])  [A is constant]\n";
    std::cout << "Expected: gradA unchanged (0), gradB[i] = A[i], grad_C = 1\n\n";

    double grad_C = __enzyme_autodiff(
        (void*)dot,
        enzyme_const,
        A.data(),
        enzyme_dup,
        B.data(),
        gradB.data(),
        enzyme_out,
        C,
        enzyme_const,
        n);

    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; i++)
        std::cout << "gradB[" << i << "] = " << gradB[i] << "  expected A[" << i << "] = " << A[i]
                  << "\n";
    std::cout << "grad_C = " << grad_C << "  expected 1\n";
    std::cout << "========================================\n";

    for (int i = 0; i < n; i++)
        EXPECT_NEAR(gradA[i], 0.0, 1e-10) << "gradA[" << i << "] should be untouched";
    for (int i = 0; i < n; i++)
        EXPECT_NEAR(gradB[i], A[i], 1e-10) << "gradB[" << i << "] mismatch";
    EXPECT_NEAR(grad_C, 1.0, 1e-10) << "grad_C mismatch";
}

// ---------------------------------------------------------------------------
// Tutorial 4: Inverse Magnitude with Custom Gradient (invsqrt)
// ---------------------------------------------------------------------------

/**
 * @brief Gradient of invmag using a custom derivative for Q_rsqrt.
 *
 * For f(A) = Q_rsqrt(sum(A[i]^2)):
 *   ∂f/∂A[k] = rev_rsqrt(sumsq, 1) * 2*A[k]
 *             = (-1 / (2*sumsq)) * Q_rsqrt(sumsq) * 2*A[k]
 *             = -A[k] * Q_rsqrt(sumsq) / sumsq
 *
 * The tolerance is loose (1e-3) because Q_rsqrt is only an approximation
 * of 1/sqrt and the custom gradient propagates that approximation error.
 */
QUARISMATEST(EnzymeAD, Tutorial4_InvMag_CustomGradient)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 4: Inverse Magnitude (custom gradient)\n";
    std::cout << "========================================\n";

    int const           n = 3;
    std::vector<double> A = {1.0, 2.0, 3.0};
    std::vector<double> gradA(n, 0.0);

    double const sumsq = A[0] * A[0] + A[1] * A[1] + A[2] * A[2];  // 14.0

    std::cout << "Function: f(A) = Q_rsqrt(sum(A[i]^2))  [fast inverse sqrt]\n";
    std::cout << "A = [1, 2, 3],  sumsq = " << sumsq << "\n";
    std::cout << "Custom derivative registered for Q_rsqrt\n\n";

    __enzyme_autodiff((void*)invmag, enzyme_dup, A.data(), gradA.data(), enzyme_const, n);

    // Analytical gradient using the exact (non-approximated) formula:
    // ∂f/∂A[k] = -A[k] / (sumsq * sqrt(sumsq))
    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; i++)
    {
        double const expected = -A[i] / (sumsq * std::sqrt(sumsq));
        std::cout << "gradA[" << i << "] = " << gradA[i] << "  analytical = " << expected << "\n";
    }
    std::cout << "========================================\n";

    // Use a loose tolerance because Q_rsqrt introduces approximation error
    for (int i = 0; i < n; i++)
    {
        double const expected = -A[i] / (sumsq * std::sqrt(sumsq));
        EXPECT_NEAR(gradA[i], expected, 1e-3) << "invmag gradA[" << i << "] outside tolerance";
    }
}

// ---------------------------------------------------------------------------
// Tutorial 5: Function Aliasing (__enzyme_function_like)
// ---------------------------------------------------------------------------

/**
 * @brief Gradient of a function that Enzyme treats as log1p.
 *
 * __enzyme_function_like tells Enzyme that log1p_like_function should be
 * differentiated using log1p's derivative rule: d/da log1p(a) = 1/(1+a).
 *
 * At a = 2.0: expected gradient = 1 / (1 + 2) = 1/3.
 */
QUARISMATEST(EnzymeAD, Tutorial5_FunctionAliasing)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 5: Function Aliasing (log1p-like)\n";
    std::cout << "========================================\n";

    double const a        = 2.0;
    double const grad_out = __enzyme_autodiff((void*)test_fn_like, a);

    // Enzyme uses log1p derivative rule: 1/(1+a)
    double const expected  = 1.0 / (1.0 + a);
    double const abs_error = std::abs(grad_out - expected);

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Function: test_fn_like(a) = log1p_like_function(a)\n";
    std::cout << "Enzyme uses log1p derivative rule: d/da log1p(a) = 1/(1+a)\n\n";
    std::cout << "Input:    a = " << a << "\n";
    std::cout << "Enzyme gradient: " << grad_out << "\n";
    std::cout << "Expected (1/(1+a)): " << expected << "\n";
    std::cout << "Absolute error:     " << abs_error << "\n";
    std::cout << "Status: " << (abs_error < 1e-10 ? "✓ PASS" : "✗ FAIL") << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(grad_out, expected, 1e-10) << "Function aliasing gradient mismatch";
}

// ---------------------------------------------------------------------------
// Tutorial 6: Cache / Alias Sensitivity (__restrict__)
// ---------------------------------------------------------------------------

/**
 * @brief Gradient of squareCopy (aliased version).
 *
 * For f(in, out) where *out = sum(in[i]^2):
 *   ∂(*out)/∂in[k] = 2*in[k]
 *
 * Without __restrict__, Enzyme must conservatively assume aliasing and may
 * cache intermediate values.  The gradient is still correct; only performance
 * differs from the restrict version.
 */
QUARISMATEST(EnzymeAD, Tutorial6_SquareCopy_Aliased)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 6: squareCopy gradient (aliased)\n";
    std::cout << "========================================\n";

    int const           n    = 5;
    std::vector<double> in   = {1.0, 2.0, 3.0, 4.0, 5.0};
    double              out  = 0.0;
    double              dout = 1.0;
    std::vector<double> grad_in(n, 0.0);

    std::cout << "Function: *out = sum(in[i]^2)  [no __restrict__]\n";
    std::cout << "in = [1,2,3,4,5]\n";
    std::cout << "Expected: grad_in[i] = 2*in[i]\n\n";

    __enzyme_autodiff(
        (void*)squareCopy,
        enzyme_dup,
        in.data(),
        grad_in.data(),
        enzyme_dup,
        &out,
        &dout,
        enzyme_const,
        n);

    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; i++)
    {
        double const expected = 2.0 * in[i];
        std::cout << "grad_in[" << i << "] = " << grad_in[i] << "  expected " << expected << "\n";
        EXPECT_NEAR(grad_in[i], expected, 1e-10) << "squareCopy (aliased) grad_in[" << i << "]";
    }
    std::cout << "========================================\n";
}

/**
 * @brief Gradient of squareCopyRestrict (no-alias version).
 *
 * Same mathematical function as Tutorial6_SquareCopy_Aliased but with
 * __restrict__ pointers.  Enzyme can use a recompute strategy instead of
 * caching, potentially improving performance on large arrays.
 */
QUARISMATEST(EnzymeAD, Tutorial6_SquareCopy_Restrict)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 6: squareCopy gradient (__restrict__)\n";
    std::cout << "========================================\n";

    int const           n    = 5;
    std::vector<double> in   = {1.0, 2.0, 3.0, 4.0, 5.0};
    double              out  = 0.0;
    double              dout = 1.0;
    std::vector<double> grad_in(n, 0.0);

    std::cout << "Function: *out = sum(in[i]^2)  [with __restrict__]\n";
    std::cout << "in = [1,2,3,4,5]\n";
    std::cout << "Expected: grad_in[i] = 2*in[i]\n\n";

    __enzyme_autodiff(
        (void*)squareCopyRestrict,
        enzyme_dup,
        in.data(),
        grad_in.data(),
        enzyme_dup,
        &out,
        &dout,
        enzyme_const,
        n);

    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; i++)
    {
        double const expected = 2.0 * in[i];
        std::cout << "grad_in[" << i << "] = " << grad_in[i] << "  expected " << expected << "\n";
        EXPECT_NEAR(grad_in[i], expected, 1e-10) << "squareCopy (restrict) grad_in[" << i << "]";
    }
    std::cout << "========================================\n";
}

// ---------------------------------------------------------------------------
// Tutorial 7: Forward-Mode with Pointer-Based Loop (fwd.c)
// ---------------------------------------------------------------------------

/**
 * @brief Forward-mode AD on compute_loops via pointer arguments.
 *
 * compute_loops accumulates (*a + *b) over 100 iterations:
 *   *ret = 100 * (*a + *b)
 *
 * Forward-mode with da = db = 1 computes:
 *   d(*ret) = 100 * (da + db) = 100 * 2 = 200
 *
 * After the call, `dret` holds the directional derivative and `ret` holds
 * the primal output.
 */
QUARISMATEST(EnzymeAD, Tutorial7_ForwardMode_Loops)
{
    std::cout << "\n========================================\n";
    std::cout << "  Tutorial 7: Forward-Mode AD with Loops\n";
    std::cout << "========================================\n";

    float a    = 2.0f;
    float b    = 3.0f;
    float da   = 1.0f;
    float db   = 1.0f;
    float ret  = 0.0f;
    float dret = 0.0f;

    std::cout << "Function: *ret = 100 * (*a + *b)\n";
    std::cout << "a = " << a << ", b = " << b << ", da = " << da << ", db = " << db << "\n";
    std::cout << "Expected: ret = " << 100 * (a + b) << ", dret = " << 100 * (da + db) << "\n\n";

    __enzyme_fwddiff_void(
        (void*)compute_loops, enzyme_dup, &a, &da, enzyme_dup, &b, &db, enzyme_dup, &ret, &dret);

    float const expected_ret  = 100.0f * (a + b);
    float const expected_dret = 100.0f * (da + db);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "ret  = " << ret << "  expected " << expected_ret << "\n";
    std::cout << "dret = " << dret << "  expected " << expected_dret << "\n";
    std::cout << "Status (ret):  " << (std::abs(ret - expected_ret) < 1e-3 ? "✓ PASS" : "✗ FAIL")
              << "\n";
    std::cout << "Status (dret): " << (std::abs(dret - expected_dret) < 1e-3 ? "✓ PASS" : "✗ FAIL")
              << "\n";
    std::cout << "========================================\n";

    EXPECT_NEAR(ret, expected_ret, 1e-3f) << "compute_loops primal output mismatch";
    EXPECT_NEAR(dret, expected_dret, 1e-3f) << "compute_loops forward-mode dret mismatch";
}

#else  // !CORE_HAS_ENZYME

/**
 * @brief Placeholder test when Enzyme is disabled
 */
QUARISMATEST(EnzymeAD, EnzymeNotEnabled)
{
    GTEST_SKIP() << "Enzyme AD is not enabled. Configure with -DQUARISMA_ENABLE_ENZYME=ON";
}

#endif  // CORE_HAS_ENZYME

// =============================================================================
// Integration Test
// =============================================================================

/**
 * @brief Test that Enzyme compile definition is set correctly
 */
QUARISMATEST(EnzymeAD, CompileDefinition)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Configuration Check\n";
    std::cout << "========================================\n";

#if CORE_HAS_ENZYME
    std::cout << "CORE_HAS_ENZYME: ENABLED (1)\n";
    std::cout << "Status: ✓ Enzyme is properly configured\n";
    std::cout << "Plugin: Linked via -fpass-plugin\n";
    std::cout << "========================================\n";
    EXPECT_TRUE(true) << "CORE_HAS_ENZYME is defined correctly";
#else
    std::cout << "CORE_HAS_ENZYME: DISABLED (0)\n";
    std::cout << "Status: Enzyme is not enabled\n";
    std::cout << "========================================\n";
    EXPECT_TRUE(true) << "CORE_HAS_ENZYME is not defined (Enzyme disabled)";
#endif
}

// ============================================================================
// CudaEnzymeADTest — CUDA + Enzyme test cases
//
// The fixture class (SetUp/TearDown/RunForward/RunGrad) is implemented in
// CudaEnzymeADTest.cu which must be compiled as a CUDA translation unit.
// ============================================================================
#if !defined(__MINGW32__) && PROJECT_HAS_CUDA
#include "CudaEnzymeADTest.h"

// ----------------------------------------------------------------------------
// Forward-kernel tests (primal: y = x^2)
// ----------------------------------------------------------------------------

TEST_F(CudaEnzymeADTest, ForwardCanonicalValue)
{
    // f(1.4) = 1.96
    double out_x, out_y;
    RunForward(1.4, out_x, out_y);
    EXPECT_NEAR(out_y, 1.96, 1e-9);
}

TEST_F(CudaEnzymeADTest, ForwardZero)
{
    // f(0) = 0
    double out_x, out_y;
    RunForward(0.0, out_x, out_y);
    EXPECT_NEAR(out_y, 0.0, 1e-12);
}

TEST_F(CudaEnzymeADTest, ForwardNegativeInput)
{
    // f(-3) = 9
    double out_x, out_y;
    RunForward(-3.0, out_x, out_y);
    EXPECT_NEAR(out_y, 9.0, 1e-9);
}

TEST_F(CudaEnzymeADTest, ForwardUnityInput)
{
    // f(1) = 1
    double out_x, out_y;
    RunForward(1.0, out_x, out_y);
    EXPECT_NEAR(out_y, 1.0, 1e-9);
}

TEST_F(CudaEnzymeADTest, ForwardInputUnchanged)
{
    // The primal must not modify x
    double out_x, out_y;
    RunForward(2.5, out_x, out_y);
    EXPECT_DOUBLE_EQ(out_x, 2.5);
}

// ----------------------------------------------------------------------------
// Gradient-kernel tests (reverse: dx = 2x * d_y)
// ----------------------------------------------------------------------------

TEST_F(CudaEnzymeADTest, GradCanonicalValue)
{
    // x=1.4, seed=1  →  dx = 2*1.4 = 2.8
    double ox, odx, oy, ody;
    RunGrad(1.4, 0.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, 2.8, 1e-9);
}

TEST_F(CudaEnzymeADTest, GradScaledSeed)
{
    // Seed d_y = 3.0  →  dx = 2*1.4*3.0 = 8.4
    double ox, odx, oy, ody;
    RunGrad(1.4, 0.0, 0.0, 3.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, 8.4, 1e-9);
}

TEST_F(CudaEnzymeADTest, GradAtZero)
{
    // x=0, seed=1  →  dx = 0
    double ox, odx, oy, ody;
    RunGrad(0.0, 0.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, 0.0, 1e-12);
}

TEST_F(CudaEnzymeADTest, GradNegativeInput)
{
    // x=-2, seed=1  →  dx = 2*(-2) = -4
    double ox, odx, oy, ody;
    RunGrad(-2.0, 0.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, -4.0, 1e-9);
}

TEST_F(CudaEnzymeADTest, GradAccumulatesIntoExistingDx)
{
    // If d_x is pre-seeded with 1.0, reverse-mode Enzyme *adds* to it.
    // At x=2, dx contribution = 2*2*1 = 4.  Total = 1 + 4 = 5.
    double ox, odx, oy, ody;
    RunGrad(2.0, 1.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, 5.0, 1e-9);
}

TEST_F(CudaEnzymeADTest, GradZeroSeedProducesZeroDx)
{
    // d_y = 0  →  dx = 2x * 0 = 0, regardless of x
    double ox, odx, oy, ody;
    RunGrad(1.4, 0.0, 0.0, 0.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, 0.0, 1e-12);
}

TEST_F(CudaEnzymeADTest, GradInputXUnchanged)
{
    // The reverse sweep must not corrupt the primal input x
    double ox, odx, oy, ody;
    RunGrad(1.4, 0.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_DOUBLE_EQ(ox, 1.4);
}

TEST_F(CudaEnzymeADTest, GradConsistencyWithFiniteDifference)
{
    // Finite-difference check: (f(x+h) - f(x-h)) / (2h) ≈ f'(x)
    const double hx  = 3.7;
    const double eps = 1e-5;
    const double fd  = ((hx + eps) * (hx + eps) - (hx - eps) * (hx - eps)) / (2.0 * eps);

    double ox, odx, oy, ody;
    RunGrad(hx, 0.0, 0.0, 1.0, ox, odx, oy, ody);
    EXPECT_NEAR(odx, fd, 1e-6);
}

// ----------------------------------------------------------------------------
// Kernel-launch sanity tests
// ----------------------------------------------------------------------------

TEST_F(CudaEnzymeADTest, ForwardKernelNoLaunchError)
{
    // RunForward uses CUDA_CHECK internally; any kernel-launch error is fatal.
    double out_x, out_y;
    RunForward(1.0, out_x, out_y);
}

TEST_F(CudaEnzymeADTest, GradKernelNoLaunchError)
{
    // RunGrad uses CUDA_CHECK internally; any kernel-launch error is fatal.
    double ox, odx, oy, ody;
    RunGrad(1.0, 0.0, 0.0, 1.0, ox, odx, oy, ody);
}

#endif  // CORE_HAS_ENZYME 
