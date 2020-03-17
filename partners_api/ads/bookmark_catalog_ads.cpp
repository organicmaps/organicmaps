#include "partners_api/ads/bookmark_catalog_ads.hpp"

namespace ads
{
BookmarkCatalog::BookmarkCatalog(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
}

std::string BookmarkCatalog::GetBanner(storage::CountryId const & mwmId, m2::PointD const & userPos,
                                       std::string const & userLanguage) const
{
  auto const cityGeoId = m_delegate.GetMwmTopCityGeoId(mwmId);

  if (!cityGeoId.empty())
    return m_delegate.GetLinkForGeoId(cityGeoId);

  return {};
}
}  // namespace ads
