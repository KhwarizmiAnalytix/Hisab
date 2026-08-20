#pragma once

#include <memory>

#include "common/strong_type.h"

namespace profiler::profiler_impl::impl
{

// Identity is a complex concept in Profiler. A recorded value might not have
// an associated storage, multiple values might share the same underlying
// storage, the storage of a value might change over time, etc.
//
// For the purpose of profiling we're mostly interested in data flow
// analysis. As a result, we can take an expansive view of identity: values
// share an ID if they share an object address or storage data.
//
// This identity equality is transitive; If values V0 and V1 share a storage
// S0 and V1 later points to a different storage S1 then all values which
// point to either S0 or S1 are considered to have the same identity. (Since
// profiler cannot reason beyond that.)
//
// The profiler will handle lifetime analysis to ensure that identities do
// not run afoul of the ABA problem. This does, however, mean that identities
// can only be assigned when memory profiling is enabled.
using TensorID = strong::type<size_t, struct TensorID_, strong::regular>;

// Uniquely identifies an allocation. (Generally a StorageImpl's data ptr.)
using AllocationID =
    strong::type<size_t, struct StorageID_, strong::ordered, strong::regular, strong::hashable>;

// Opaque identity key for a recorded value's owning object. XSigma has no
// tensor type to actually take a weak/owning reference on, so this is never
// dereferenced -- only used for address-identity comparison/hashing (see
// calculateUniqueTensorIDs), wrapped in a strong type to prevent direct
// access.
using TensorImplAddress = strong::type<
    const void*,
    struct TensorImplAddress_,
    strong::regular,
    strong::hashable,
    strong::boolean>;

using StorageImplData = strong::
    type<const void*, struct StorageImplData_, strong::regular, strong::hashable, strong::boolean>;

struct Result;

void calculateUniqueTensorIDs(std::vector<std::shared_ptr<Result>>& sorted_results);

}  // namespace profiler::profiler_impl::impl
