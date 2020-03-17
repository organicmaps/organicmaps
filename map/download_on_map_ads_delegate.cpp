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

storage::CountriesVec DownloadOnMapDelegate::GetTopmostNodesFor(storage::CountryId const & mwmId) const
{
  storage::CountriesVec countries;
  m_storage.GetTopmostNodesFor(mwmId, countries);
  return countries;
}

std::string DownloadOnMapDelegate::GetMwmTopCityGeoId(storage::CountryId const & mwmId) const
{
  auto const & cities = m_storage.GetMwmTopCityGeoIds();
  auto const it = cities.find(mwmId);

  if (it == cities.cend())
    return {};

  return strings::to_string(it->second.GetEncodedId());
}

std::string DownloadOnMapDelegate::GetLinkForGeoId(std::string const & id) const
{
  return m_promoApi.GetLinkForDownloader(id);
}

bool DownloadOnMapDelegate::IsAdsRemoved() const
{
  return m_purchase.IsSubscriptionActive(SubscriptionType::RemoveAds);
}
}  // namespace ads
