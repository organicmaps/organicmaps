#include "map/download_on_map_ads_delegate.hpp"

namespace ads
{
DownloadOnMapDelegate::DownloadOnMapDelegate(storage::CountryInfoGetter const & infoGetter,
                                             storage::Storage const & storage,
                                             promo::Api const & promoApi, Purchase const & purchase)
  : m_countryInfoGetter(infoGetter)
  , m_storage(storage)
  , m_promoApi(promoApi)
  , m_purchase(purchase)
{
}

storage::CountryId DownloadOnMapDelegate::GetCountryId(m2::PointD const & pos)
{
  return m_countryInfoGetter.GetRegionCountryId(pos);
}

storage::CountryId DownloadOnMapDelegate::GetTopmostParentFor(storage::CountryId const & countryId) const
{
  return m_storage.GetTopmostParentFor(countryId);
}

std::string DownloadOnMapDelegate::GetLinkForCountryId(storage::CountryId const & countryId) const
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

bool DownloadOnMapDelegate::IsAdsRemoved() const
{
  return m_purchase.IsSubscriptionActive(SubscriptionType::RemoveAds);
}
}  // namespace ads
