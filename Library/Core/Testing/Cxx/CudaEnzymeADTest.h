/**
 * @file CudaEnzymeADTest.h
 * @brief Declaration of the CudaEnzymeADTest Google Test fixture.
 *
 * Provides a fixture that allocates four device scalars (x, d_x, y, d_y) and
 * exposes RunForward / RunGrad helpers so that individual TEST_F bodies can be
 * written in plain C++ (TestEnzymeAD.cpp) while the CUDA kernel launches are
 * implemented in CudaEnzymeADTest.cu.
 */

#pragma once

#if CORE_HAS_ENZYME

#include <gtest/gtest.h>

class CudaEnzymeADTest : public ::testing::Test
{
protected:
    double* x   = nullptr;
    double* d_x = nullptr;
    double* y   = nullptr;
    double* d_y = nullptr;

    void SetUp() override;
    void TearDown() override;

    /** Upload four host scalars → device, run the gradient kernel, download. */
    void RunGrad(double hx, double hdx, double hy, double hdy,
                 double& out_x, double& out_dx, double& out_y, double& out_dy);

    /** Upload two host scalars → device, run the forward kernel, download. */
    void RunForward(double hx, double& out_x, double& out_y);
};

#endif  // CORE_HAS_ENZYME
