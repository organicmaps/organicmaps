#include "map/ads_engine_delegate.hpp"

#include "geometry/mercator.hpp"

namespace ads
{
AdsEngineDelegate::AdsEngineDelegate(storage::CountryInfoGetter const & infoGetter,
                                     storage::Storage const & storage, promo::Api const & promoApi,
                                     Purchase const & purchase, taxi::Engine const & taxiEngine)
  : m_countryInfoGetter(infoGetter)
  , m_storage(storage)
  , m_promoApi(promoApi)
  , m_purchase(purchase)
  , m_taxiEngine(taxiEngine)
{
}

bool AdsEngineDelegate::IsAdsRemoved() const
{
  return m_purchase.IsSubscriptionActive(SubscriptionType::RemoveAds);
}

storage::CountryId AdsEngineDelegate::GetCountryId(m2::PointD const & pos)
{
  return m_countryInfoGetter.GetRegionCountryId(pos);
}

storage::CountryId AdsEngineDelegate::GetTopmostParentFor(storage::CountryId const & countryId) const
{
  return m_storage.GetTopmostParentFor(countryId);
}

std::string AdsEngineDelegate::GetLinkForCountryId(storage::CountryId const & countryId) const
{
  auto const & cities = m_storage.GetMwmTopCityGeoIds();
  auto const it = cities.find(countryId);

  if (it == cities.cend())
    return {};

  auto const cityGeoId = strings::to_string(it->second.GetEncodedId());

  if (cityGeoId.empty())
    return {};

  return m_promoApi.GetLinkForDownloader(cityGeoId);
}

std::vector<taxi::Provider::Type> AdsEngineDelegate::GetTaxiProvidersAtPos(m2::PointD const & pos) const
{
  return m_taxiEngine.GetProvidersAtPos(mercator::ToLatLon(pos));
}
}  // namespace ads
