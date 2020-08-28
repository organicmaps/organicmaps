#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/skyeng_ads.hpp"

UNIT_TEST(Skyeng_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::Skyeng skyeng(delegate);
  m2::PointD point;

  {
    auto const banner = skyeng.GetBanner("Russia_Tambov Oblast", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("US_North Carolina_Raleigh", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("US_North Carolina_Raleigh", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Tambov Oblast", point, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("US_North Carolina_Raleigh", point, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Cote dIvoire", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    auto const banner = skyeng.GetBanner("Cote dIvoire", point, "en");
    TEST(banner.empty(), ());
  }
}
