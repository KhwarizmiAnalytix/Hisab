/*
 * Reference (scalar) matrix multiply for row-major storage. SIMD kernels can replace this later.
 */
#pragma once

#include <cstddef>

namespace vectorization
{

template <typename T>
void matrix_multiplication(
    bool          transpose_lhs,
    bool          transpose_rhs,
    std::size_t   m,
    std::size_t   n,
    std::size_t   k,
    const T*      lhs,
    std::size_t   ldlhs,
    const T*      rhs,
    std::size_t   ldrhs,
    T*            c,
    std::size_t   ldc)
{
    auto a_at = [&](std::size_t i, std::size_t p) -> T
    {
        return transpose_lhs ? lhs[p * ldlhs + i] : lhs[i * ldlhs + p];
    };
    auto b_at = [&](std::size_t p, std::size_t j) -> T
    {
        return transpose_rhs ? rhs[j * ldrhs + p] : rhs[p * ldrhs + j];
    };

    for (std::size_t i = 0; i < m; ++i)
    {
        for (std::size_t j = 0; j < n; ++j)
        {
            T sum = 0;
            for (std::size_t p = 0; p < k; ++p)
            {
                sum += a_at(i, p) * b_at(p, j);
            }
            c[i * ldc + j] = sum;
        }
    }
}

}  // namespace vectorization
