#pragma once

#include "common/macros.h"

namespace xsigma
{
class XSIGMA_VISIBILITY cpu_info
{
    XSIGMA_DELETE_CLASS(cpu_info);

public:
    XSIGMA_API static bool initialize();

    XSIGMA_API static int number_of_cores();

    XSIGMA_API static int number_of_threads();

    XSIGMA_API static void info();

    XSIGMA_API static void cpuinfo_cach(
        std::ptrdiff_t& l1, std::ptrdiff_t& l2, std::ptrdiff_t& l3, std::ptrdiff_t& l3_count);
};
}  // namespace xsigma