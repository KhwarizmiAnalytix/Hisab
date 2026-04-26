/*
 * In-place transpose for row-major packed storage (reference implementation).
 */
#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace vectorization
{

template <typename T>
void matrix_transpose(std::size_t rows, std::size_t cols, T* data)
{
    if (rows <= 1 || cols <= 1)
    {
        return;
    }
    if (rows == cols)
    {
        for (std::size_t i = 0; i < rows; ++i)
        {
            for (std::size_t j = i + 1; j < cols; ++j)
            {
                std::swap(data[i * cols + j], data[j * cols + i]);
            }
        }
        return;
    }
    std::vector<T> tmp(rows * cols);
    for (std::size_t i = 0; i < rows; ++i)
    {
        for (std::size_t j = 0; j < cols; ++j)
        {
            tmp[j * rows + i] = data[i * cols + j];
        }
    }
    std::copy(tmp.begin(), tmp.end(), data);
}

}  // namespace vectorization
