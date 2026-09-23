#pragma once

#ifdef DEBUG

#include "platform/location.hpp"
#include "platform/location_provider/location_fusion_strategy.hpp"

#include <string>
#include <utility>
#include <vector>

namespace location_provider
{
// Debug-only reference LocationFusionStrategy: if a "gpx_replay" candidate is present, returns it
// verbatim; otherwise passes the native reading through unchanged. Deliberately holds no reference
// to GpxReplayProvider — see design.md's registration-model decision.
class GpxReplayFusionStrategy : public LocationFusionStrategy
{
public:
  location::GpsInfo Resolve(location::GpsInfo const & native,
                             std::vector<std::pair<std::string, location::GpsInfo>> const & candidates) override;
};
}  // namespace location_provider

#endif  // DEBUG
