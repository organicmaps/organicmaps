#include "platform/marketing_service.hpp"

namespace marketing
{

// Tags.
char const * const kMapVersion = "map_version";
char const * const kMapListing = "map_listing";
char const * const kMapDownloadDiscovered = "map_download_discovered";
char const * const kMapLastDownloaded = "last_map_downloaded";
char const * const kMapLastDownloadedTimestamp = "last_map_downloaded_time";
char const * const kRoutingP2PVehicleDiscovered = "routing_p2p_vehicle_discovered";
char const * const kRoutingP2PPedestrianDiscovered = "routing_p2p_pedestrian_discovered";
char const * const kRoutingP2PBicycleDiscovered = "routing_p2p_bicycle_discovered";
char const * const kRoutingP2PTaxiDiscovered = "routing_p2p_taxi_discovered";
char const * const kRoutingVehicleDiscovered = "routing_vehicle_discovered";
char const * const kRoutingPedestrianDiscovered = "routing_pedestrian_discovered";
char const * const kRoutingBicycleDiscovered = "routing_bicycle_discovered";
char const * const kRoutingTaxiDiscovered = "routing_taxi_discovered";
char const * const kEditorAddDiscovered = "editor_add_discovered";
char const * const kEditorEditDiscovered = "editor_edit_discovered";
char const * const kTrafficDiscovered = "traffic_discovered";

// Events.
char const * const kDownloaderMapActionFinished = "Downloader_Map_action_finished";
char const * const kSearchEmitResultsAndCoords = "searchEmitResultsAndCoords";
char const * const kRoutingCalculatingRoute = "Routing_CalculatingRoute";
char const * const kBookmarksBookmarkAction = "Bookmarks_Bookmark_action";
char const * const kPlacepageHotelBook = "Placepage_Hotel_book";
char const * const kEditorAddStart = "EditorAdd_start";
char const * const kEditorEditStart = "EditorEdit_start";

}  // marketing

void MarketingService::ProcessFirstLaunch()
{
  // Send initial value for "discovered" tags.
  using namespace marketing;
  vector<string> tags =
  {
    kMapDownloadDiscovered,

    kRoutingP2PVehicleDiscovered, kRoutingP2PPedestrianDiscovered,
    kRoutingP2PBicycleDiscovered, kRoutingP2PTaxiDiscovered,
    kRoutingVehicleDiscovered, kRoutingPedestrianDiscovered,
    kRoutingBicycleDiscovered, kRoutingTaxiDiscovered,

    kEditorAddDiscovered, kEditorEditDiscovered,

    kTrafficDiscovered
  };

  for (auto const & tag : tags)
    SendPushWooshTag(tag, vector<string>{"0"});
}
