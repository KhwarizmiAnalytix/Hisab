#pragma once

#include <Quarisma/Quarisma.h>
#include <Quarisma/core/ivalue.h>
#include <Quarisma/core/jit_type.h>
#include <Quarisma/core/stack.h>
#include <quarisma/util/sparse_bitset.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/jit/ir/ir.h>

#include <list>
#include <unordered_map>
#include <vector>

namespace torch::jit
{

using SparseBitVector = ::quarisma::SparseBitVector<256>;

// BuildLivenessSets computes "bailout" liveness which is equivalent to
// "{LIVE_IN} or {GEN}" or "{LIVE_OUT} - {KILL}"
TORCH_API std::unordered_map<Node*, std::vector<Value*>> BuildLivenessSets(
    std::shared_ptr<Graph> graph);
}  // namespace torch::jit
