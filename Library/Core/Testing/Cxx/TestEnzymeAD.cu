/**
 * @file TestEnzymeAD.cu
 * @brief CUDA + Enzyme GPU kernel automatic differentiation test
 *
 * Demonstrates Enzyme AD applied to __device__ code when clang is used as the
 * CUDA compiler (not nvcc).  clang's -x cuda path runs the Enzyme LLVM pass on
 * the NVPTX IR so __global__ kernels can call __enzyme_autodiff.
 *
 * Compilation notes:
 *  - Requires -fpass-plugin=LLVMEnzyme-XX.so (or LLDEnzyme-XX.so)
 *  - The __enzyme_autodiff declaration MUST have C linkage (extern "C") so
 *    Enzyme can match the unmangled symbol name in the NVPTX IR.
 *  - Activity annotation globals (enzyme_dup, etc.) keep C++ linkage; simple
 *    C++ top-level variable names are not mangled by the Itanium ABI.
 *  - Compiled with CMAKE_LANGUAGE=CXX + -x cuda flags (not via nvcc) so that
 *    the Enzyme LLVM pass sees the NVPTX IR before ptxas.
 *
 * Test: f(x) = x^2
 *   forward:  y  = x^2        → at x=1.4 expect y  = 1.96
 *   backward: dx = 2x * dy_in → at x=1.4 expect dx = 2.8
 */

#include <cuda_runtime.h>
#include <math.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Primal device function to differentiate
// ---------------------------------------------------------------------------
void __device__ square_impl(double* x_in, double* x_out)
{
    x_out[0] = x_in[0] * x_in[0];
}

// ---------------------------------------------------------------------------
// Enzyme activity annotations.
// Must be __device__ so they live in the device symbol table and Enzyme can
// locate them by name in the NVPTX IR.
// ---------------------------------------------------------------------------
int __device__ enzyme_dup;
int __device__ enzyme_out;
int __device__ enzyme_const;

// ---------------------------------------------------------------------------
// __enzyme_autodiff — MUST be declared with C linkage so Enzyme matches the
// unmangled symbol "__enzyme_autodiff" in the LLVM/NVPTX IR.
// The first argument is void* (cast from the function pointer at each call
// site); Enzyme replaces the whole call with the differentiated code.
// ---------------------------------------------------------------------------
extern "C" __device__ void __enzyme_autodiff(void*, ...);

// ---------------------------------------------------------------------------
// Forward kernel (primal only)
// ---------------------------------------------------------------------------
void __global__ square_forward(double* x_in, double* x_out)
{
    square_impl(x_in, x_out);
}

// ---------------------------------------------------------------------------
// Backward kernel — Enzyme differentiates square_impl in reverse mode.
// enzyme_dup marks each (value, gradient) pair; the seed d_y=1 propagates
// the reverse sweep back to d_x = 2*x.
// ---------------------------------------------------------------------------
void __global__ square_grad(double* x, double* d_x, double* y, double* d_y)
{
    __enzyme_autodiff((void*)square_impl,
                      enzyme_dup, x, d_x,
                      enzyme_dup, y, d_y);
}

// ---------------------------------------------------------------------------
// Host entry point — returns 0 (success) or 1 (failure) for CTest
// ---------------------------------------------------------------------------
int main()
{
    double *x, *d_x, *y, *d_y;
    cudaMalloc(&x,   sizeof(double));
    cudaMalloc(&d_x, sizeof(double));
    cudaMalloc(&y,   sizeof(double));
    cudaMalloc(&d_y, sizeof(double));

    // Input: x=1.4; seed gradient d_y=1.0 (reverse mode "output" seed)
    double host_x   = 1.4;
    double host_d_x = 0.0;
    double host_y   = 0.0;
    double host_d_y = 1.0;

    cudaMemcpy(x,   &host_x,   sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_x, &host_d_x, sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(y,   &host_y,   sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_y, &host_d_y, sizeof(double), cudaMemcpyHostToDevice);

    square_grad<<<1, 1>>>(x, d_x, y, d_y);
    cudaDeviceSynchronize();

    cudaMemcpy(&host_x,   x,   sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host_d_x, d_x, sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host_y,   y,   sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(&host_d_y, d_y, sizeof(double), cudaMemcpyDeviceToHost);

    // Expected: y = 1.4^2 = 1.96,  dx = d/dx[x^2]*d_y = 2*1.4*1.0 = 2.8
    printf("y  = %.6f  (expect 1.960000)\n", host_y);
    printf("dx = %.6f  (expect 2.800000)\n", host_d_x);

    int const ok =
        (fabs(host_y - 1.96) < 1e-6) &&
        (fabs(host_d_x - 2.8) < 1e-6);

    if (!ok)
    {
        fprintf(stderr, "FAIL: Enzyme CUDA autodiff produced incorrect results\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
