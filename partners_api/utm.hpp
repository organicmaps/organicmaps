#pragma once

#include "coding/url.hpp"

#include <cstdint>
#include <string>

enum class UTM : uint8_t
{
  None = 0,
  BookmarksPageCatalogButton,
  ToolbarButton,
  DownloadMwmBanner,
  LargeToponymsPlacepageGallery,
  SightseeingsPlacepageGallery,
  DiscoveryPageGallery,
  TipsAndTricks,
  BookingPromo,
  DiscoverCatalogOnboarding,
  FreeSamplesOnboading,
  OutdoorPlacepageGallery,
  GuidesOnMapGallery,
};

enum class UTMContent : uint8_t
{
  Description = 0,
  View,
  Details,
  More,
};

inline std::string InjectUTM(std::string const & url, UTM utm)
{
  if (url.empty())
    return {};

  url::Params params;
  params.emplace_back("utm_source", "maps.me");
  switch (utm)
  {
  case UTM::BookmarksPageCatalogButton:
    params.emplace_back("utm_medium", "button");
    params.emplace_back("utm_campaign", "bookmarks_downloaded");
    break;
  case UTM::ToolbarButton:
    params.emplace_back("utm_medium", "button");
    params.emplace_back("utm_campaign", "toolbar_menu");
    break;
  case UTM::DownloadMwmBanner:
    params.emplace_back("utm_medium", "banner");
    params.emplace_back("utm_campaign", "download_map_popup");
    break;
  case UTM::LargeToponymsPlacepageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "large_toponyms_placepage_gallery");
    break;
  case UTM::SightseeingsPlacepageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "sightseeings_placepage_gallery");
    break;
  case UTM::DiscoveryPageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "discovery_button_gallery");
    break;
  case UTM::TipsAndTricks:
    params.emplace_back("utm_medium", "button");
    params.emplace_back("utm_campaign", "tips_and_tricks");
    break;
  case UTM::BookingPromo:
    params.emplace_back("utm_medium", "popup");
    params.emplace_back("utm_campaign", "bookingcom");
    break;
  case UTM::DiscoverCatalogOnboarding:
    params.emplace_back("utm_medium", "onboarding_button");
    params.emplace_back("utm_campaign", "catalog_discovery");
    break;
  case UTM::FreeSamplesOnboading:
    params.emplace_back("utm_medium", "onboarding_button");
    params.emplace_back("utm_campaign", "sample_discovery");
    break;
  case UTM::OutdoorPlacepageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "outdoor_placepage_gallery");
    break;
  case UTM::GuidesOnMapGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "map");
    break;
  case UTM::None:
    return url;
  }
  return url::Make(url, params);
}

inline std::string InjectUTMContent(std::string const & url, UTMContent content)
{
  url::Params params;
  switch (content)
  {
  case UTMContent::Description:
    params.emplace_back("utm_content", "description");
    break;
  case UTMContent::View:
    params.emplace_back("utm_content", "view");
    break;
  case UTMContent::Details:
    params.emplace_back("utm_content", "details");
    break;
  case UTMContent::More:
    params.emplace_back("utm_content", "more");
    break;
  }
  return url::Make(url, params);
}

inline std::string InjectUTMTerm(std::string const & url, std::string const & value)
{
  return url::Make(url, {{"utm_term", value}});
}
