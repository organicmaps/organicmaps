#pragma once

#include "platform/location.hpp"

#include <optional>
#include <string>

namespace location_provider
{
// Supplies a candidate location reading, independent of the platform's native GPS, to a
// LocationFusionStrategy via LocationProviderRegistry.
class LocationProvider
{
public:
  virtual ~LocationProvider() = default;

  // Identifies this provider among the candidates passed to LocationFusionStrategy::Resolve().
  // Must be non-empty and stable for the lifetime of the provider.
  virtual std::string GetSourceId() const = 0;

  // Called synchronously on every location update; must not block. std::nullopt is simply
  // omitted from the candidates passed to the fusion strategy.
  virtual std::optional<location::GpsInfo> GetCurrentReading() = 0;
};
}  // namespace location_provider
