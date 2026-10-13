#pragma once

#include "platform/location.hpp"

#include <string>
#include <utility>
#include <vector>

namespace location_provider
{
// Resolves the platform's native GpsInfo reading plus every registered LocationProvider's
// current candidate reading into a single GpsInfo, via LocationProviderRegistry.
class LocationFusionStrategy
{
public:
  virtual ~LocationFusionStrategy() = default;

  // candidates holds one (GetSourceId(), reading) pair per registered LocationProvider whose
  // GetCurrentReading() returned a value this update. Must return within the caller's
  // location-update cadence (no blocking I/O) — see design.md's fail-fast error-handling note.
  virtual location::GpsInfo Resolve(location::GpsInfo const & native,
                                     std::vector<std::pair<std::string, location::GpsInfo>> const & candidates) = 0;
};
}  // namespace location_provider
