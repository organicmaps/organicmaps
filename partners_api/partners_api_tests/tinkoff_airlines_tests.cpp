#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/tinkoff_allairlines_ads.hpp"

UNIT_TEST(TinkoffAirlines_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::TinkoffAllAirlines tinkoffAirlines(delegate);

  {
    delegate.SetTopmostNodes({"Germany", "Russian Federation"});
    auto const banner = tinkoffAirlines.GetBanner("", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostNodes({"Germany", "Russian Federation"});
    auto const banner = tinkoffAirlines.GetBanner("", {}, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostNodes({"Germany", "Cote dIvoire"});
    auto const banner = tinkoffAirlines.GetBanner("", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostNodes({"Norway"});
    delegate.SetCountryId("Russian Federation");
    auto const banner = tinkoffAirlines.GetBanner("", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostNodes({"Norway"});
    delegate.SetCountryId("Ukraine");
    auto const banner = tinkoffAirlines.GetBanner("", {}, "ru");
    TEST(banner.empty(), ());
  }
}
