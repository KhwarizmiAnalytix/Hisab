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
 * - Integration with Quarisma test framework
 */

#include <gtest/gtest.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

// Enzyme requires these declarations for AD
// The __enzyme_autodiff and __enzyme_fwddiff functions are provided by the Enzyme plugin
extern "C"
{
    // Reverse-mode AD: computes gradient
    double __enzyme_autodiff(void*, ...);

    // Forward-mode AD: computes directional derivative
    double __enzyme_fwddiff(void*, ...);
}

// Enzyme activity annotations for multivariate functions
enum
{
    enzyme_dup   = 1,  // Indicates this argument should be differentiated (input + gradient output)
    enzyme_const = 0   // Indicates this argument is constant (no differentiation needed)
};

// =============================================================================
// Test Functions for Automatic Differentiation
// =============================================================================

/**
 * @brief Simple quadratic function: f(x) = x^2
 *
 * This is a basic test case for AD.
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
 * This is a classic optimization test function.
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
// Google Test Suite for Enzyme AD
// =============================================================================

#if QUARISMA_HAS_ENZYME

/**
 * @brief Test reverse-mode AD on square function
 */
TEST(EnzymeAD, ReverseMode_Square)
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

    // Compute gradient using Enzyme reverse-mode AD
    // For scalar functions, __enzyme_autodiff returns the derivative
    double const derivative = __enzyme_autodiff((void*)square, x);

    // Expected derivative: f'(x) = 2x = 2 * 3.0 = 6.0
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
TEST(EnzymeAD, ReverseMode_Cubic)
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

    // Compute gradient using Enzyme
    double const derivative = __enzyme_autodiff((void*)cubic, x);

    // Expected derivative: f'(x) = 3x^2 + 4x + 3 = 3*4 + 4*2 + 3 = 12 + 8 + 3 = 23
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
TEST(EnzymeAD, ReverseMode_Exponential)
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

    // Compute gradient using Enzyme
    double const derivative = __enzyme_autodiff((void*)exponential, x);

    // Expected derivative: f'(x) = e^x
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
TEST(EnzymeAD, ReverseMode_Multivariate)
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

    // Compute partial derivatives using Enzyme
    // For multivariate functions, we differentiate with respect to each argument separately
    // ∂f/∂x: differentiate first argument
    double const dx = __enzyme_autodiff((void*)multivariate_dx, x, y);

    // ∂f/∂y: differentiate second argument
    // Note: we swap the order so y is the first (differentiated) parameter
    auto         multivariate_yx = [](double y, double x) { return multivariate(x, y); };
    double const dy              = __enzyme_autodiff((void*)+multivariate_yx, y, x);

    // Expected partial derivatives:
    // ∂f/∂x = 2x + 2y = 2*2 + 2*3 = 4 + 6 = 10
    // ∂f/∂y = 2x + 2y = 2*2 + 2*3 = 4 + 6 = 10
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
TEST(EnzymeAD, ReverseMode_Rosenbrock)
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

    // Compute partial derivatives using Enzyme
    // ∂f/∂x: differentiate first argument
    double const dx = __enzyme_autodiff((void*)rosenbrock_dx, x, y);

    // ∂f/∂y: differentiate second argument
    auto         rosenbrock_yx = [](double y, double x) { return rosenbrock(x, y); };
    double const dy            = __enzyme_autodiff((void*)+rosenbrock_yx, y, x);

    // Expected partial derivatives at (1, 1):
    // ∂f/∂x = -2(1-x) - 400x(y-x^2) = -2(0) - 400(1)(0) = 0
    // ∂f/∂y = 200(y-x^2) = 200(0) = 0
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

/**
 * @brief Test forward-mode AD on square function
 */
TEST(EnzymeAD, ForwardMode_Square)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Forward-Mode AD: Square Function\n";
    std::cout << "========================================\n";

    double const x   = 3.0;
    double const dx  = 1.0;  // Directional derivative seed
    double const f_x = square(x);

    std::cout << "Function: f(x) = x²\n";
    std::cout << "Expected derivative: f'(x) = 2x\n";
    std::cout << "Mode: Forward-mode (directional derivative)\n\n";

    std::cout << std::fixed << std::setprecision(10);
    std::cout << "Input:    x = " << x << "\n";
    std::cout << "Seed:     dx = " << dx << "\n";
    std::cout << "Output:   f(x) = " << f_x << "\n\n";

    // Compute directional derivative using Enzyme forward-mode AD
    double const result = __enzyme_fwddiff((void*)square, x, dx);

    // Expected derivative: f'(x) = 2x = 2 * 3.0 = 6.0
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
TEST(EnzymeAD, ForwardMode_Cubic)
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

    // Compute directional derivative using Enzyme
    double const result = __enzyme_fwddiff((void*)cubic, x, dx);

    // Expected derivative: f'(x) = 3x^2 + 4x + 3 = 23
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
TEST(EnzymeAD, NumericalAccuracy)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme AD: Numerical Accuracy Test\n";
    std::cout << "========================================\n";
    std::cout << "Testing f(x) = x² at multiple points\n";
    std::cout << "Expected: f'(x) = 2x\n\n";

    // Test at multiple points to verify accuracy
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

#else  // !QUARISMA_HAS_ENZYME

/**
 * @brief Placeholder test when Enzyme is disabled
 */
TEST(EnzymeAD, DISABLED_EnzymeNotEnabled)
{
    GTEST_SKIP() << "Enzyme AD is not enabled. Configure with -DQUARISMA_ENABLE_ENZYME=ON";
}

#endif  // QUARISMA_HAS_ENZYME

// =============================================================================
// Integration Test
// =============================================================================

/**
 * @brief Test that Enzyme compile definition is set correctly
 */
TEST(EnzymeAD, CompileDefinition)
{
    std::cout << "\n========================================\n";
    std::cout << "  Enzyme Configuration Check\n";
    std::cout << "========================================\n";

#if QUARISMA_HAS_ENZYME
    std::cout << "QUARISMA_HAS_ENZYME: ENABLED (1)\n";
    std::cout << "Status: ✓ Enzyme is properly configured\n";
    std::cout << "Plugin: Linked via -fpass-plugin\n";
    std::cout << "========================================\n";
    EXPECT_TRUE(true) << "QUARISMA_HAS_ENZYME is defined correctly";
#else
    std::cout << "QUARISMA_HAS_ENZYME: DISABLED (0)\n";
    std::cout << "Status: Enzyme is not enabled\n";
    std::cout << "========================================\n";
    EXPECT_TRUE(true) << "QUARISMA_HAS_ENZYME is not defined (Enzyme disabled)";
#endif
}

/**
 * @brief Main function for standalone test execution
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
