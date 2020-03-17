#pragma once

#include "partners_api/ads/ads_base.hpp"

namespace ads
{
class BookmarkCatalog : public DownloadOnMapContainer
{
public:
  BookmarkCatalog(Delegate & delegate);

  std::string GetBanner(storage::CountryId const & mwmId, m2::PointD const & userPos,
                        std::string const & userLanguage) const override;
};
}  // namespace ads
