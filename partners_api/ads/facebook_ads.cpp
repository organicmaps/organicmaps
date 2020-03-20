#include "partners_api/ads/facebook_ads.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kSingleBannerIdForAllTypes = "185237551520383_3000232483354195";
  auto const kSearchBannerId = "185237551520383_3000236913353752";
#elif defined(OMIM_OS_ANDROID)
  auto const kSingleBannerIdForAllTypes = "185237551520383_1450325641678228";
  auto const kSearchBannerId = "185237551520383_1384653791578747";
#else
  auto const kSingleBannerIdForAllTypes = "dummy";
  auto const kSearchBannerId = "dummy";
#endif
}  // namespace

namespace ads
{
std::string FacebookPoi::GetBannerForOtherTypes() const
{
  return kSingleBannerIdForAllTypes;
}

std::string FacebookSearch::GetBanner() const
{
  return kSearchBannerId;
}

bool FacebookSearch::HasBanner() const
{
  return true;
}
}  // namespace ads
