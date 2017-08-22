#pragma once

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace marketing
{
// Tags.
extern char const * const kMapVersion;
extern char const * const kMapListing;
extern char const * const kMapDownloadDiscovered;
extern char const * const kMapLastDownloaded;
extern char const * const kMapLastDownloadedTimestamp;
extern char const * const kRoutingP2PVehicleDiscovered;
extern char const * const kRoutingP2PPedestrianDiscovered;
extern char const * const kRoutingP2PBicycleDiscovered;
extern char const * const kRoutingP2PTaxiDiscovered;
extern char const * const kRoutingVehicleDiscovered;
extern char const * const kRoutingPedestrianDiscovered;
extern char const * const kRoutingBicycleDiscovered;
extern char const * const kRoutingTaxiDiscovered;
extern char const * const kEditorAddDiscovered;
extern char const * const kEditorEditDiscovered;
extern char const * const kTrafficDiscovered;

// Events.
extern char const * const kDownloaderMapActionFinished;
extern char const * const kSearchEmitResultsAndCoords;
extern char const * const kBookmarksBookmarkAction;
extern char const * const kPlacepageHotelBook;
extern char const * const kEditorAddStart;
extern char const * const kEditorEditStart;
extern char const * const kDiffSchemeFallback;
extern char const * const kDiffSchemeError;

// Settings.
extern char const * const kFrom;
extern char const * const kType;
extern char const * const kName;
extern char const * const kContent;
extern char const * const kKeyword;
}  // marketing

class MarketingService
{
public:
  using PushWooshSenderFn = function<void(string const & tag, vector<string> const & values)>;
  using MarketingSenderFn = function<void(string const & tag, map<string, string> const & params)>;

  void ProcessFirstLaunch();

  void SetPushWooshSender(PushWooshSenderFn const & fn) { m_pushwooshSender = fn; }
  void SendPushWooshTag(string const & tag);
  void SendPushWooshTag(string const & tag, string const & value);
  void SendPushWooshTag(string const & tag, vector<string> const & values);

  void SetMarketingSender(MarketingSenderFn const & fn) { m_marketingSender = fn; }
  void SendMarketingEvent(string const & tag, map<string, string> const & params);

private:
  /// Callback fucntion for setting PushWoosh tags.
  PushWooshSenderFn m_pushwooshSender;

  /// Callback fucntion for sending marketing events.
  MarketingSenderFn m_marketingSender;
};
