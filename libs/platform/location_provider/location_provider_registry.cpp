#include "platform/location_provider/location_provider_registry.hpp"

namespace location_provider
{
LocationProviderRegistry & LocationProviderRegistry::Instance()
{
  static LocationProviderRegistry inst;
  return inst;
}

void LocationProviderRegistry::RegisterProvider(LocationProvider * provider)
{
  m_providers.push_back(provider);
}

void LocationProviderRegistry::SetFusionStrategy(LocationFusionStrategy * strategy)
{
  m_strategy = strategy;
}

location::GpsInfo LocationProviderRegistry::Resolve(location::GpsInfo const & native)
{
  if (m_strategy == nullptr)
    return native;

  std::vector<std::pair<std::string, location::GpsInfo>> candidates;
  candidates.reserve(m_providers.size());
  for (auto * provider : m_providers)
  {
    if (auto reading = provider->GetCurrentReading())
      candidates.emplace_back(provider->GetSourceId(), *reading);
  }

  return m_strategy->Resolve(native, candidates);
}
}  // namespace location_provider
