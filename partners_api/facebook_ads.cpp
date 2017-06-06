#include "partners_api/facebook_ads.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kSingleBannerIdForAllTypes = "185237551520383_1450324925011633";
#else
  auto const kSingleBannerIdForAllTypes = "185237551520383_1450325641678228";
#endif
}  // namespace

namespace ads
{
std::string Facebook::GetBannerIdForOtherTypes() const
{
  return kSingleBannerIdForAllTypes;
}
}  // namespace ads
