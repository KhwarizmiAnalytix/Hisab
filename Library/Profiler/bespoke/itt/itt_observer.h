#include "bespoke/common/api.h"

namespace quarisma::profiler::impl
{

void pushITTCallbacks(
    const ProfilerConfig& config, const std::unordered_set<quarisma::RecordScope>& scopes);

}  // namespace quarisma::profiler::impl
