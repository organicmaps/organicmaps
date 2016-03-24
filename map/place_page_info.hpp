#pragma once

#include "map/bookmark.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/map_object.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace place_page
{
class Info : public osm::MapObject
{
public:
  static char const * kSubtitleSeparator;
  static char const * kStarSymbol;
  static char const * kMountainSymbol;

  bool IsFeature() const;
  bool IsBookmark() const;
  bool IsMyPosition() const;
  /// @returns true if Back API button should be displayed.
  bool HasApiUrl() const;
  /// @returns true if Edit Place button should be displayed.
  bool IsEditable() const;

  /// TODO: Support all possible Internet types in UI. @See MapObject::GetInternet().
  bool HasWifi() const;

  string GetAddress() const { return m_address; }

  /// Should be used by UI code to generate cool name for new bookmarks.
  // TODO: Tune new bookmark name. May be add address or some other data.
  string FormatNewBookmarkName() const;

  /// Convenient wrapper for feature's name and custom name.
  string GetTitle() const;
  /// Convenient wrapper for type, cuisines, elevation, stars, wifi etc.
  string GetSubtitle() const;
  /// @returns empty string or GetStars() count of ★ symbol.
  string FormatStars() const;

  string GetCustomName() const;
  BookmarkAndCategory GetBookmarkAndCategory() const;
  string const & GetApiUrl() const;

  void SetMercator(m2::PointD const & mercator);

  /// Comes from API, shared links etc.
  string m_customName;
  /// If not empty, bookmark is bound to this place page.
  BookmarkAndCategory m_bac = MakeEmptyBookmarkAndCategory();
  /// Api ID passed for the selected object. It's automatically included in api url below.
  string m_apiId;
  /// [Deep] link to open when "Back" button is pressed in a Place Page.
  string m_apiUrl;
  /// Formatted feature address.
  string m_address;

  bool m_isMyPosition = false;
  bool m_isEditable = false;

  // TODO(AlexZ): Temporary solution. It's better to use a wifi icon in UI instead of text.
  string m_localizedWifiString;
};
}  // namespace place_page
