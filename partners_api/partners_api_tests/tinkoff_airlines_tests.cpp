#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/tinkoff_allairlines_ads.hpp"

UNIT_TEST(TinkoffAirlines_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::TinkoffAllAirlines tinkoffAirlines(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Germany");
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Germany");
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Germany");
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Germany");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = tinkoffAirlines.GetBanner("", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent({"Norway"});
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent({"Norway"});
    delegate.SetCountryId("Ukraine");
    auto const banner = tinkoffAirlines.GetBanner("", point, "ru");
    TEST(banner.empty(), ());
  }
}
