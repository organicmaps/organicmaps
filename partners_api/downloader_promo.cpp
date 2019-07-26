#include "partners_api/downloader_promo.hpp"

#include "partners_api/megafon_countries.hpp"
#include "partners_api/promo_api.hpp"

#include "storage/storage.hpp"

#include "base/string_utils.hpp"

namespace promo
{
// static
DownloaderPromo::Banner DownloaderPromo::GetBanner(storage::Storage const & storage,
                                                   Api const & promoApi, std::string const & mwmId,
                                                   std::string const & currentLocale,
                                                   bool hasRemoveAdsSubscription)
{
  if (!hasRemoveAdsSubscription && ads::HasMegafonDownloaderBanner(storage, mwmId, currentLocale))
    return {Type::Megafon, ads::GetMegafonDownloaderBannerUrl()};

  auto const & cities = storage.GetMwmTopCityGeoIds();
  auto const it = cities.find(mwmId);

  if (it != cities.cend())
  {
    auto const id = strings::to_string(it->second.GetEncodedId());
    return {Type::BookmarkCatalog, promoApi.GetLinkForDownloader(id)};
  }

  return {};
}
}  // namespace promo
