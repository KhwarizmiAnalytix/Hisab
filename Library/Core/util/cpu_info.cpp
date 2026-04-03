#include "util/cpu_info.h"

#include <cpuinfo.h>
#include <fmt/core.h>

#include <cstddef>

namespace quarisma
{
bool cpu_info::initialize()
{
    return cpuinfo_initialize();
}

int cpu_info::number_of_cores()
{
    return cpuinfo_get_cores_count();
}

int cpu_info::number_of_threads()
{
    return cpuinfo_get_processors_count();
}

void cpu_info::info()
{
    const struct cpuinfo_package* package = cpuinfo_get_package(0);
    if (package != nullptr)
    {
        fmt::print("-- Running on {} CPU", package->name);
    }
    fmt::print("-- support f16c:     {}\n", cpuinfo_has_x86_f16c() ? "True" : "False");
    fmt::print("-- support sse:      {}\n", cpuinfo_has_x86_sse() ? "True" : "False");
    fmt::print("-- support sse2:     {}\n", cpuinfo_has_x86_sse2() ? "True" : "False");
    fmt::print("-- support sse3:     {}\n", cpuinfo_has_x86_sse3() ? "True" : "False");
    fmt::print("-- support sse4a:    {}\n", cpuinfo_has_x86_sse4a() ? "True" : "False");
    fmt::print("-- support sse4_1:   {}\n", cpuinfo_has_x86_sse4_1() ? "True" : "False");
    fmt::print("-- support sse4_2:   {}\n", cpuinfo_has_x86_sse4_2() ? "True" : "False");
    fmt::print("-- support avx:      {}\n", cpuinfo_has_x86_avx() ? "True" : "False");
    fmt::print("-- support avx2:     {}\n", cpuinfo_has_x86_avx2() ? "True" : "False");
    fmt::print("-- support axv512:   {}\n", cpuinfo_has_x86_avx512f() ? "True" : "False");
    fmt::print("-- support arm neon: {}\n", cpuinfo_has_arm_neon() ? "True" : "False");

    fmt::print("-- size of double {}\n", static_cast<int>(sizeof(double)));
    fmt::print("-- size of float {}\n", static_cast<int>(sizeof(float)));
    fmt::print("==================================================\n");
    fmt::print("                  cache info                      \n");
    fmt::print("==================================================\n");

    const struct cpuinfo_cache* l1d_cache = cpuinfo_get_l1d_caches();
    if (l1d_cache != nullptr)
    {
        fmt::print(
            "-- Cache L1d:\n size={},\n associativity={},\n sets={},\n partitions={},\n  "
            "line_size={},\n "
            "flags={}\n, processor_start={},\n processor_count={}.\n",
            l1d_cache->size,
            l1d_cache->associativity,
            l1d_cache->sets,
            l1d_cache->partitions,
            l1d_cache->line_size,
            l1d_cache->flags,
            l1d_cache->processor_start,
            l1d_cache->processor_count);
    }

    const struct cpuinfo_cache* l1i_cache = cpuinfo_get_l1i_caches();
    if (l1i_cache != nullptr)
    {
        fmt::print(
            "-- Cache L1i:\n size={},\n associativity={},\n sets={},\n partitions={},  "
            "line_size={},\n "
            "flags={},\n processor_start={},\n processor_count={}.\n",
            l1i_cache->size,
            l1i_cache->associativity,
            l1i_cache->sets,
            l1i_cache->partitions,
            l1i_cache->line_size,
            l1i_cache->flags,
            l1i_cache->processor_start,
            l1i_cache->processor_count);
    }

    const struct cpuinfo_cache* l2_cache = cpuinfo_get_l2_caches();
    if (l2_cache != nullptr)
    {
        fmt::print(
            "-- Cache L2:\n size={},\n associativity={},\n sets={},\n partitions={},\n  "
            "line_size={},\n "
            "flags={},\n processor_start={},\n processor_count={}.\n",
            l2_cache->size,
            l2_cache->associativity,
            l2_cache->sets,
            l2_cache->partitions,
            l2_cache->line_size,
            l2_cache->flags,
            l2_cache->processor_start,
            l2_cache->processor_count);
    }

    const struct cpuinfo_cache* l3_cache = cpuinfo_get_l3_caches();
    if (l3_cache != nullptr)
    {
        fmt::print(
            "-- Cache L3:\n size={},\n associativity={},\n sets={},\n partitions={},\n  "
            "line_size={},\n "
            "flags={},\n processor_start={},\n processor_count={}.\n",
            l3_cache->size,
            l3_cache->associativity,
            l3_cache->sets,
            l3_cache->partitions,
            l3_cache->line_size,
            l3_cache->flags,
            l3_cache->processor_start,
            l3_cache->processor_count);
    }

    cpuinfo_deinitialize();

    fmt::print("==================================================\n");
    fmt::print("                     Flags                        \n");
    fmt::print("==================================================\n");

#if QUARISMA_HAS_MKL
    fmt::print("MKL is enabled!\n");
#endif  // QUARISMA_HAS_MKL
#if QUARISMA_HAS_TBB
    fmt::print("TBB is enabled!\n");
#endif  // QUARISMA_HAS_MKL
}

void cpu_info::cpuinfo_cach(
    std::ptrdiff_t& l1, std::ptrdiff_t& l2, std::ptrdiff_t& l3, std::ptrdiff_t& l3_count)
{
    const struct cpuinfo_cache* l1d_cache = cpuinfo_get_l1d_caches();
    const struct cpuinfo_cache* l2_cache  = cpuinfo_get_l2_caches();
    const struct cpuinfo_cache* l3_cache  = cpuinfo_get_l3_caches();

    l1       = (l1d_cache != nullptr) ? l1d_cache->size : 0;
    l2       = (l2_cache != nullptr) ? l2_cache->size : 0;
    l3       = (l3_cache != nullptr) ? l3_cache->size : 0;
    l3_count = (l3_cache != nullptr) ? l3_cache->processor_count : 0;

    cpuinfo_deinitialize();
}
};  // namespace quarisma
