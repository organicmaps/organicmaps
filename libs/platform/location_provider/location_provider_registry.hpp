#pragma once

#include "platform/location.hpp"
#include "platform/location_provider/location_fusion_strategy.hpp"
#include "platform/location_provider/location_provider.hpp"

#include <vector>

namespace location_provider
{
// Sits upstream of Framework::OnLocationUpdate: resolves the platform's native GpsInfo reading
// through the registered LocationFusionStrategy (if any), fed by every registered
// LocationProvider's current candidate reading. With nothing registered, Resolve() is a pure
// passthrough returning the native reading unchanged.
//
// RegisterProvider()/SetFusionStrategy() take non-owning pointers to objects with static storage
// duration; the registry never allocates or frees them. Registration is expected only during
// static initialization, before any location update is processed, so there is no
// concurrent-registration case to guard against; Resolve() is only ever called from the single
// location-update thread, matching Framework::OnLocationUpdate's existing threading contract.
// A plain, independently-constructible class — not enforced as a singleton — so tests can build
// isolated instances. Production code (framework.cpp, the debug reference plugin) uses the
// process-wide instance via Instance().
class LocationProviderRegistry
{
public:
  static LocationProviderRegistry & Instance();

  void RegisterProvider(LocationProvider * provider);

  // Replaces any previously registered strategy — last call wins. There is intentionally no
  // error path for double-registration.
  void SetFusionStrategy(LocationFusionStrategy * strategy);

  // No exception handling around a registered strategy's Resolve(): a misbehaving strategy is a
  // developer bug, caught in code review and testing, not guarded against at runtime.
  location::GpsInfo Resolve(location::GpsInfo const & native);

private:
  std::vector<LocationProvider *> m_providers;
  LocationFusionStrategy * m_strategy = nullptr;
};
}  // namespace location_provider
