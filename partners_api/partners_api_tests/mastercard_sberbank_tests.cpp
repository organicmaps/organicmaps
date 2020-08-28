#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/mastercard_sber_ads.hpp"

UNIT_TEST(MastercardSberbank_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::MastercardSberbank masterSber(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    auto const banner = masterSber.GetBanner("Russia_Tambov Oblast", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    auto const banner = masterSber.GetBanner("Russia_Tambov Oblast", {}, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("United States of America");
    auto const banner = masterSber.GetBanner("US_North Carolina_Raleigh", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    auto const banner = masterSber.GetBanner("Russia_Tambov Oblast", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("United States of America");
    auto const banner = masterSber.GetBanner("US_North Carolina_Raleigh", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    auto const banner = masterSber.GetBanner("Cote dIvoire", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    auto const banner = masterSber.GetBanner("Cote dIvoire", point, "en");
    TEST(banner.empty(), ());
  }
}
