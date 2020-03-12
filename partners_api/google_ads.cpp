#include "partners_api/google_ads.hpp"

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
bool Google::HasBanner() const
{
  return true;
}

std::string Google::GetBanner() const
{
  return kSearchbannerId;
}
}  // namespace ads
