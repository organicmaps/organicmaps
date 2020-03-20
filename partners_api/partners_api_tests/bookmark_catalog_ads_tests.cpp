#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

#include "partners_api/ads/bookmark_catalog_ads.hpp"

UNIT_TEST(BokmarkCatalogAds_GetBanner)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::BookmarkCatalog catalogAds(delegate);

  {
    delegate.SetLinkForCountryId("123");
    auto const banner = catalogAds.GetBanner("", {}, "");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetLinkForCountryId("");
    auto const banner = catalogAds.GetBanner("", {}, "");
    TEST(banner.empty(), ());
  }
}
