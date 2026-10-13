#include "map/location_provider/gpx_replay_fusion_strategy.hpp"

#ifdef DEBUG

#include "map/location_provider/gpx_replay_provider.hpp"
#include "map/location_provider/gpx_replay_settings.hpp"

#include "platform/location_provider/location_provider_registry.hpp"
#include "platform/settings_contribution/settings_contribution_registry.hpp"

#include <mutex>

namespace location_provider
{
namespace
{
GpxReplayFusionStrategy gGpxReplayFusionStrategy;

void RegisterGpxReplayPluginOnce()
{
  static std::once_flag once;
  std::call_once(once, []
  {
    auto & locationRegistry = LocationProviderRegistry::Instance();
    locationRegistry.RegisterProvider(&GpxReplayProvider::Instance());
    locationRegistry.SetFusionStrategy(&gGpxReplayFusionStrategy);

    auto & settingsRegistry = settings_contribution::SettingsContributionRegistry::Instance();
    settingsRegistry.Register(&GpxReplayFileContribution::Instance());
    settingsRegistry.Register(&GpxReplayArmContribution::Instance());
  });
}
}  // namespace

// Must be called from a live code path (not a side-effect-only static). Xcode links with
// -dead_strip, which can drop static registrars whose only purpose is construction side effects.
void ForceLinkGpxReplayPlugin()
{
  RegisterGpxReplayPluginOnce();
}

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
}  // namespace location_provider

#endif  // DEBUG
