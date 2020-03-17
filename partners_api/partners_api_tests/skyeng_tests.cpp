#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/skyeng_ads.hpp"

UNIT_TEST(Skyeng_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::Skyeng skyeng(delegate);

  {
    auto const banner = skyeng.GetBanner("Russia_Tambov Oblast", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("US_North Carolina_Raleigh", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Tambov Oblast", {}, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("US_North Carolina_Raleigh", {}, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Moscow", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Cote dIvoire", {}, "ru");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Moscow", {}, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Cote dIvoire", {}, "en");
    TEST(banner.empty(), ());
  }
}
