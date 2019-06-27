#pragma once

#include "base/url_helpers.hpp"

#include <cstdint>
#include <string>

enum class UTM : uint8_t
{
  None = 0,
  BookmarksPageCatalogButton,
  ToolbarButton,
  DownloadMwmBanner,
  PlacepageGallery,
  DiscoveryPageGallery,
  TipsAndTricks
};

inline std::string InjectUTM(std::string const & url, UTM utm)
{
  base::url::Params params;
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
  case UTM::PlacepageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "placepage_gallery");
    break;
  case UTM::DiscoveryPageGallery:
    params.emplace_back("utm_medium", "gallery");
    params.emplace_back("utm_campaign", "discovery_button_gallery");
    break;
  case UTM::TipsAndTricks:
    params.emplace_back("utm_medium", "button");
    params.emplace_back("utm_campaign", "tips_and_tricks");
    break;
  case UTM::None:
    return url;
  }
  return base::url::Make(url, params);
}
