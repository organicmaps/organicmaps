#include "testing/testing.hpp"

#include "partners_api/ads/google_ads.hpp"

UNIT_TEST(Google_BannerInSearch)
{
  ads::Google google;
  auto result = google.GetBanner();
  TEST_EQUAL(result, "dummy", ());
}
