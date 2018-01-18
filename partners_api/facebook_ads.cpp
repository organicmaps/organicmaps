#include "partners_api/facebook_ads.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kSingleBannerIdForAllTypes = "185237551520383_1450324925011633";
  auto const kSearchbannerId = "185237551520383_1453784847998974";
#elif defined(OMIM_OS_ANDROID)
  auto const kSingleBannerIdForAllTypes = "185237551520383_1450325641678228";
  auto const kSearchbannerId = "185237551520383_1384653791578747";
#else
  auto const kSingleBannerIdForAllTypes = "dummy";
  auto const kSearchbannerId = "dummy";
#endif
}  // namespace

namespace ads
{
bool Facebook::HasSearchBanner() const { return true; }

std::string Facebook::GetBannerIdForOtherTypes() const
{
  return kSingleBannerIdForAllTypes;
}

std::string Facebook::GetSearchBannerId() const { return kSearchbannerId; }
}  // namespace ads
