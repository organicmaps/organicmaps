#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/mts_ads.hpp"

UNIT_TEST(Mts_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::Mts mts(delegate);

  {
    delegate.SetTopmostParent("France");
    auto const banner = mts.GetBanner("", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("France");
    auto const banner = mts.GetBanner("", {}, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("France");
    delegate.SetCountryId("Russian Federation");
    auto const banner = mts.GetBanner("", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Thailand");
    delegate.SetCountryId("Russian Federation");
    auto const banner = mts.GetBanner("", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Thailand");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = mts.GetBanner("", {}, "ru");
    TEST(!banner.empty(), ());
  }
}
