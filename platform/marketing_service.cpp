#include "platform/marketing_service.hpp"

#include "base/gmtime.hpp"

namespace marketing
{
// Tags.
char const * const kMapVersionMin = "map_version_min";
char const * const kMapVersionMax = "map_version_max";
char const * const kMapListing = "map_listing";
char const * const kMapDownloadDiscovered = "map_download_discovered";
char const * const kMapLastDownloaded = "last_map_downloaded";
char const * const kMapLastDownloadedTimestamp = "last_map_downloaded_time";
char const * const kRoutingP2PVehicleDiscovered = "routing_p2p_vehicle_discovered";
char const * const kRoutingP2PPedestrianDiscovered = "routing_p2p_pedestrian_discovered";
char const * const kRoutingP2PBicycleDiscovered = "routing_p2p_bicycle_discovered";
char const * const kRoutingP2PTaxiDiscovered = "routing_p2p_taxi_discovered";
char const * const kRoutingP2PTransitDiscovered = "routing_p2p_transit_discovered";
char const * const kRoutingVehicleDiscovered = "routing_vehicle_discovered";
char const * const kRoutingPedestrianDiscovered = "routing_pedestrian_discovered";
char const * const kRoutingBicycleDiscovered = "routing_bicycle_discovered";
char const * const kRoutingTaxiDiscovered = "routing_taxi_discovered";
char const * const kRoutingTransitDiscovered = "routing_transit_discovered";
char const * const kEditorAddDiscovered = "editor_add_discovered";
char const * const kEditorEditDiscovered = "editor_edit_discovered";
char const * const kTrafficDiscovered = "traffic_discovered";
char const * const kDiscoveryButtonDiscovered = "discovery_button_discovered";
char const * const kBookHotelOnBookingComDiscovered = "hotel_book_bcom_discovered";
char const * const kSubscriptionBookmarksAllEnabled =
  "bookmark_catalog_subscription_city_outdoor_enabled";
char const * const kSubscriptionBookmarksAllDisabled =
  "bookmark_catalog_subscription_city_outdoor_disabled";
char const * const kSubscriptionBookmarksSightsEnabled =
  "bookmark_catalog_subscription_city_enabled";
char const * const kSubscriptionBookmarksSightsDisabled =
  "bookmark_catalog_subscription_city_disabled";
char const * const kRemoveAdsSubscriptionEnabled = "remove_ads_subscription_enabled";
char const * const kRemoveAdsSubscriptionDisabled = "remove_ads_subscription_disabled";
char const * const kSubscriptionContentDeleted = "bookmark_catalog_subscription_content_deleted";
char const * const kSubscriptionBookmarksAllTrialEnabled =
  "bookmark_catalog_subscription_city_outdoor_trial_enabled";

// Events.
char const * const kDownloaderMapActionFinished = "Downloader_Map_action_finished";
char const * const kSearchEmitResultsAndCoords = "searchEmitResultsAndCoords";
char const * const kBookmarksBookmarkAction = "Bookmarks_Bookmark_action";
char const * const kPlacepageHotelBook = "Placepage_Hotel_book";
char const * const kEditorAddStart = "EditorAdd_start";
char const * const kEditorEditStart = "EditorEdit_start";

// Settings.
char const * const kFrom = "utm_source";
char const * const kType = "utm_medium";
char const * const kName = "utm_campaign";
char const * const kContent = "utm_content";
char const * const kKeyword = "utm_term";
}  // marketing

void MarketingService::ProcessFirstLaunch()
{
  // Send initial value for "discovered" tags.
  using namespace marketing;
  std::vector<std::string> tags =
  {
    kMapDownloadDiscovered,

    kRoutingP2PVehicleDiscovered, kRoutingP2PPedestrianDiscovered,
    kRoutingP2PBicycleDiscovered, kRoutingP2PTaxiDiscovered,
    kRoutingVehicleDiscovered, kRoutingPedestrianDiscovered,
    kRoutingBicycleDiscovered, kRoutingTaxiDiscovered,
    kRoutingP2PTransitDiscovered, kRoutingTransitDiscovered,

    kEditorAddDiscovered, kEditorEditDiscovered,

    kTrafficDiscovered,
    kDiscoveryButtonDiscovered,
    kBookHotelOnBookingComDiscovered
  };

  for (auto const & tag : tags)
    SendPushWooshTag(tag, std::vector<std::string>{"0"});
}

std::string MarketingService::GetPushWooshTimestamp()
{
  char nowStr[18]{};
  auto const now = base::GmTime(time(nullptr));
  strftime(nowStr, sizeof(nowStr), "%Y-%m-%d %H:%M", &now);
  return std::string(nowStr);
}
