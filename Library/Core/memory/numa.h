#pragma once

#include <cstddef>

// IWYU pragma: keep
#include "common/export.h"

namespace quarisma
{
/**
 * Check whether NUMA is enabled
 */
QUARISMA_API bool IsNUMAEnabled();

/**
 * Bind to a given NUMA node
 */
QUARISMA_API void NUMABind(int numa_node_id);

/**
 * Get the NUMA id for a given pointer `ptr`
 */
QUARISMA_API int GetNUMANode(const void* ptr);

/**
 * Get number of NUMA nodes
 */
QUARISMA_API int GetNumNUMANodes();

/**
 * Move the memory pointed to by `ptr` of a given size to another NUMA node
 */
QUARISMA_API void NUMAMove(void* ptr, size_t size, int numa_node_id);

/**
 * Get the current NUMA node id
 */
QUARISMA_API int GetCurrentNUMANode();

}  // namespace quarisma
