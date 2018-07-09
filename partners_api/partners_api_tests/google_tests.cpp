#include "testing/testing.hpp"

#include "partners_api/google_ads.hpp"

UNIT_TEST(Google_BannerInSearch)
{
  ads::Google google;
  TEST(google.HasSearchBanner(), ());
  auto result = google.GetSearchBannerId();
  TEST_EQUAL(result, "dummy", ());
}
