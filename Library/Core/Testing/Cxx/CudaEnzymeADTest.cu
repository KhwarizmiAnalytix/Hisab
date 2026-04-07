/**
 * @file CudaEnzymeADTest.cu
 * @brief CUDA device code and fixture method implementations for CudaEnzymeADTest.
 *
 * This file must be compiled as a CUDA translation unit (clang -x cuda or nvcc)
 * because it contains __device__ / __global__ code and kernel-launch syntax (<<<>>>).
 * The corresponding TEST_F test cases live in TestEnzymeAD.cpp and include only
 * the host-side header CudaEnzymeADTest.h.
 */

#if QUARISMA_HAS_ENZYME

#include <cuda_runtime.h>
#include <gtest/gtest.h>

#include "CudaEnzymeADTest.h"

// ============================================================================
// Device code under test: y = x^2  and its Enzyme-generated gradient
// ============================================================================

void __device__ square_impl(double* x_in, double* x_out)
{
    x_out[0] = x_in[0] * x_in[0];
}

int __device__ enzyme_dup;
int __device__ enzyme_out;
int __device__ enzyme_const;

extern "C" __device__ void __enzyme_autodiff(void*, ...);

void __global__ square_forward(double* x_in, double* x_out)
{
    square_impl(x_in, x_out);
}

void __global__ square_grad(double* x, double* d_x, double* y, double* d_y)
{
    __enzyme_autodiff((void*)square_impl,
                      enzyme_dup, x, d_x,
                      enzyme_dup, y, d_y);
}

// ============================================================================
// CUDA error-checking macro (ASSERT variant — aborts the current test on error)
// ============================================================================

#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t _err = (call);                                              \
        ASSERT_EQ(_err, cudaSuccess)                                            \
            << "CUDA error at " #call ": " << cudaGetErrorString(_err);         \
    } while (0)

// ============================================================================
// CudaEnzymeADTest fixture method implementations
// ============================================================================

void CudaEnzymeADTest::SetUp()
{
    // Other CUDA tests may leave the process on an invalid device ordinal.
    // Always (re)select a valid device before allocating or launching kernels.
    int device_count = 0;
    cudaError_t dc_err = cudaGetDeviceCount(&device_count);
    ASSERT_EQ(dc_err, cudaSuccess)
        << "CUDA error at cudaGetDeviceCount(): " << cudaGetErrorString(dc_err);
    ASSERT_GT(device_count, 0) << "No CUDA devices found.";

    int chosen = -1;
    for (int dev = 0; dev < device_count; ++dev)
    {
        cudaError_t sd_err = cudaSetDevice(dev);
        if (sd_err == cudaSuccess)
        {
            chosen = dev;
            break;
        }
    }
    ASSERT_GE(chosen, 0) << "Failed to select any usable CUDA device out of "
                         << device_count << " visible devices.";

    // Clear any stale error state from prior CUDA calls in the same process.
    (void)cudaGetLastError();

    CUDA_CHECK(cudaMalloc(&x,   sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_x, sizeof(double)));
    CUDA_CHECK(cudaMalloc(&y,   sizeof(double)));
    CUDA_CHECK(cudaMalloc(&d_y, sizeof(double)));
}

void CudaEnzymeADTest::TearDown()
{
    cudaFree(x);
    cudaFree(d_x);
    cudaFree(y);
    cudaFree(d_y);
}

void CudaEnzymeADTest::RunGrad(double hx, double hdx, double hy, double hdy,
                                double& out_x, double& out_dx,
                                double& out_y, double& out_dy)
{
    CUDA_CHECK(cudaMemcpy(x,   &hx,  sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_x, &hdx, sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(y,   &hy,  sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_y, &hdy, sizeof(double), cudaMemcpyHostToDevice));

    square_grad<<<1, 1>>>(x, d_x, y, d_y);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(&out_x,  x,   sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&out_dx, d_x, sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&out_y,  y,   sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&out_dy, d_y, sizeof(double), cudaMemcpyDeviceToHost));
}

void CudaEnzymeADTest::RunForward(double hx, double& out_x, double& out_y)
{
    double hy = 0.0;
    CUDA_CHECK(cudaMemcpy(x, &hx, sizeof(double), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(y, &hy, sizeof(double), cudaMemcpyHostToDevice));

    square_forward<<<1, 1>>>(x, y);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(&out_x, x, sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&out_y, y, sizeof(double), cudaMemcpyDeviceToHost));
}

#endif  // QUARISMA_HAS_ENZYME
