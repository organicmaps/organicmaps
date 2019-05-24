#include "partners_api/downloader_promo.hpp"

#include "partners_api/megafon_countries.hpp"

namespace promo
{
// static
DownloaderPromoBanner DownloaderPromo::GetBanner(storage::Storage const & storage,
                                                 std::string const & mwmId,
                                                 std::string const & currentLocale,
                                                 bool hasRemoveAdsSubscription)
{
  if (!hasRemoveAdsSubscription && ads::HasMegafonDownloaderBanner(storage, mwmId, currentLocale))
    return {DownloaderPromoType::Megafon, ads::GetMegafonDownloaderBannerUrl()};

  // TODO: add bookmark catalog banner.

  return {};
}
}  // namespace promo
