#include "bespoke/common/api.h"

namespace quarisma::profiler::impl
{

void pushNVTXCallbacks(
    const ProfilerConfig& config, const std::unordered_set<quarisma::RecordScope>& scopes);

}  // namespace quarisma::profiler::impl
