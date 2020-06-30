#include "partners_api/ads/bookmark_catalog_ads.hpp"

namespace ads
{
BookmarkCatalog::BookmarkCatalog(Delegate & delegate)
  : DownloadOnMapContainer(delegate)
{
}

std::string BookmarkCatalog::GetBanner(storage::CountryId const & countryId,
                                       std::optional<m2::PointD> const & userPos,
                                       std::string const & userLanguage) const
{
  return m_delegate.GetLinkForCountryId(countryId);
}
}  // namespace ads
