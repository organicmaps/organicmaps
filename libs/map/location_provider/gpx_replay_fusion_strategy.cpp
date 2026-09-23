#include "map/location_provider/gpx_replay_fusion_strategy.hpp"

#ifdef DEBUG

#include "map/location_provider/gpx_replay_provider.hpp"

#include "platform/location_provider/location_provider_registry.hpp"

namespace location_provider
{
location::GpsInfo GpxReplayFusionStrategy::Resolve(
    location::GpsInfo const & native, std::vector<std::pair<std::string, location::GpsInfo>> const & candidates)
{
  for (auto const & candidate : candidates)
  {
    if (candidate.first == GpxReplayProvider::kSourceId)
      return candidate.second;
  }
  return native;
}

namespace
{
// Registered once, at static-init time, alongside GpxReplayProvider::Instance() — see
// design.md's "GpxReplayFusionStrategy registration model" Key Decision. Never re-registered by
// the ?mock-gpx:/?mock-gpx-stop debug commands, which only call Arm()/Disarm() on the provider.
GpxReplayFusionStrategy gGpxReplayFusionStrategy;

struct GpxReplayRegistrar
{
  GpxReplayRegistrar()
  {
    auto & registry = LocationProviderRegistry::Instance();
    registry.RegisterProvider(&GpxReplayProvider::Instance());
    registry.SetFusionStrategy(&gGpxReplayFusionStrategy);
  }
};

GpxReplayRegistrar const gGpxReplayRegistrar;
}  // namespace
}  // namespace location_provider

#endif  // DEBUG
