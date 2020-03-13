#include "partners_api/ads/google_ads.hpp"

namespace
{
#if defined(OMIM_OS_IPHONE)
  auto const kSearchbannerId = "mobile-app-mapsme";
#elif defined(OMIM_OS_ANDROID)
  auto const kSearchbannerId = "mobile-app-mapsme";
#else
  auto const kSearchbannerId = "dummy";
#endif
}  // namespace

namespace ads
{
std::string Google::GetBanner() const
{
  return kSearchbannerId;
}

bool Google::HasBanner() const
{
  return true;
}
}  // namespace ads
