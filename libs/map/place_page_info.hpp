#pragma once

#include "map/routing_mark.hpp"
#include "map/track.hpp"

#include "storage/storage_defines.hpp"

#include "drape_frontend/frontend_renderer.hpp"
#include "drape_frontend/selection_shape.hpp"

#include "kml/types.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_source.hpp"
#include "indexer/map_object.hpp"

#include "geometry/latlon.hpp"
#include "geometry/point2d.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace place_page
{
std::string_view constexpr kDebugAllTypesSetting = "DebugAllTypes";

enum class OpeningMode
{
  Preview = 0,
  PreviewPlus,
  Details,
  Full
};

// Stable, persisted ids shared by all platforms (Android pref, iOS UserDefaults). NEVER reorder or
// reuse a value; append a new format with the next free id. This order is also the cycle/display
// order on every platform - the single source of order is kDescs in the .cpp.
enum class CoordinatesFormat
{
  LatLonDMS = 0,      // Degrees-minutes-seconds, space separated
  LatLonDecimal = 1,  // Decimal degrees, comma separated
  OLCFull = 2,        // Open Location Code, long format
  OSMLink = 3,        // Link to osm.org
  UTM = 4,            // Universal Transverse Mercator
  MGRS = 5,           // Military Grid Reference System
  OSGB = 6,           // British National Grid (OS Grid), Great Britain and the Isle of Man only
  IrishGrid = 7,      // Irish Grid (letter reference), Northern Ireland and the Republic of Ireland
  ITM = 8             // Irish Transverse Mercator (numeric), Northern Ireland and the Republic of Ireland
};

// The coordinate formats in cycle/display order (currently == ascending id). Single source of order.
std::vector<CoordinatesFormat> const & AllCoordinateFormats();

// Bare coordinate value for the format, e.g. "51.507400, -0.127800", "SW 7400 4210".
// Empty if the format is unavailable here: UTM/MGRS beyond their valid latitudes (|lat| > 84),
// or OSGB outside the region where it is the official reference (regionId fails IsOSGridRegion).
std::string FormatCoordinateValue(CoordinatesFormat format, ms::LatLon ll, std::string_view regionId);

// Display string: "<label>: <value>" for labelled formats (UTM/MGRS/OSGB), else the bare value.
// Empty when the format is unavailable here (same condition as FormatCoordinateValue).
std::string FormatCoordinateDisplay(CoordinatesFormat format, ms::LatLon ll, std::string_view regionId);

// One coordinate format resolved at a point: its stable id and both string forms.
struct CoordinateFormatEntry
{
  CoordinatesFormat m_format;
  std::string m_display;  // Labelled form for the UI row, e.g. "OSGB: SW 7400 4210".
  std::string m_value;    // Bare value for copying, e.g. "SW 7400 4210".
};

// The formats available at this point, in display order, resolved in a single pass. Never empty: the
// decimal formats apply everywhere. This is the one primitive a place page needs - it resolves the
// region once and returns every format's strings, so the platform picks/cycles over the list instead
// of re-resolving per format, per refresh or per tap.
std::vector<CoordinateFormatEntry> GetAvailableCoordinateFormats(ms::LatLon ll, std::string_view regionId);

// Selection over a list from GetAvailableCoordinateFormats plus a saved stable id. The "effective"
// format is the saved one if it is available here, else the first available; "next" is the one after
// it (wrapping). Neither changes the saved preference, so it is restored once the user returns to a
// region where it applies. entries must be non-empty.
CoordinatesFormat EffectiveCoordinateFormat(std::vector<CoordinateFormatEntry> const & entries, int savedId);
CoordinatesFormat NextCoordinateFormat(std::vector<CoordinateFormatEntry> const & entries, int savedId);

struct BuildInfo
{
  enum class Source : uint8_t
  {
    User,
    Search,
    Other
  };

  enum class Match : uint8_t
  {
    Everything = 0,
    FeatureOnly,
    UserMarkOnly,
    TrackOnly,
    Nothing
  };

  BuildInfo() = default;

  explicit BuildInfo(df::TapInfo const & info)
    : m_source(Source::User)
    , m_mercator(info.m_mercator)
    , m_isLongTap(info.m_isLong)
    , m_isMyPosition(info.m_isMyPositionTapped)
    , m_featureId(info.m_featureTapped)
    , m_userMarkId(info.m_markTapped)
  {}

  bool IsFeatureMatchingEnabled() const { return m_match == Match::Everything || m_match == Match::FeatureOnly; }

  bool IsUserMarkMatchingEnabled() const { return m_match == Match::Everything || m_match == Match::UserMarkOnly; }

  bool IsTrackMatchingEnabled() const { return m_match == Match::Everything || m_match == Match::TrackOnly; }

  Source m_source = Source::Other;
  m2::PointD m_mercator;
  bool m_isLongTap = false;
  bool m_isMyPosition = false;
  FeatureID m_featureId;
  Match m_match = Match::Everything;
  kml::MarkId m_userMarkId = kml::kInvalidMarkId;
  kml::TrackId m_trackId = kml::kInvalidTrackId;
  bool m_isGeometrySelectionAllowed = false;
  bool m_needAnimationOnSelection = true;
};

class Info : public osm::MapObject
{
public:
  void SetBuildInfo(place_page::BuildInfo const & info) { m_buildInfo = info; }
  place_page::BuildInfo const & GetBuildInfo() const { return m_buildInfo; }

  /// Place traits
  bool IsFeature() const { return m_featureID.IsValid(); }
  bool IsBookmark() const;
  bool IsTrack() const { return m_trackId != kml::kInvalidTrackId; }
  bool IsRelationTrack() const { return m_trackId == kml::kTempRelationTrackId; }
  bool IsMyPosition() const { return m_selectedObject == df::SelectionShape::ESelectedObject::OBJECT_MY_POSITION; }
  bool IsRoutePoint() const { return m_isRoutePoint; }
  bool IsRoadType() const { return m_roadType != RoadWarningMarkType::Count; }
  bool HasMetadata() const { return m_hasMetadata; }

  /// Edit and add
  bool ShouldShowAddPlace() const;
  bool ShouldShowAddBusiness() const { return IsBuilding(); }
  bool ShouldShowEditPlace() const;

  bool CanEditPlace() const { return m_canEditOrAdd; }

  /// @returns true if Back API button should be displayed.
  bool HasApiUrl() const { return !m_apiUrl.empty(); }
  /// TODO: Support all possible Internet types in UI. @See MapObject::GetInternet().
  bool HasWifi() const { return GetInternet() == feature::Internet::Wlan; }
  /// Should be used by UI code to generate cool name for new bookmarks.
  // TODO: Tune new bookmark name. May be add address or some other data.
  kml::LocalizableString FormatNewBookmarkName() const;

  /// For showing in UI
  std::string const & GetTitle() const { return m_uiTitle; }
  /// Convenient wrapper for secondary feature name.
  std::string const & GetSecondaryTitle() const { return m_uiSecondaryTitle; }
  /// Convenient wrapper for type, cuisines, elevation, stars, wifi etc.
  std::string const & GetSubtitle() const { return m_uiSubtitle; }
  std::string const & GetSecondarySubtitle() const
  {
    return !m_uiTrackStatistics.empty() ? m_uiTrackStatistics : m_uiAddress;
  }
  /// Used in desktop PP (instead of GetSecondarySubtitle).
  std::string const & GetAddress() const { return m_address; }

  std::string const & GetWikiDescription() const { return m_wikiDescription; }
  std::string const & GetOSMDescription() const { return m_osmDescription; }
  /// @returns the display string for the format (see FormatCoordinateDisplay), or empty if it is
  /// unavailable at this location. Used by the iOS place page (which keeps the array id-indexed).
  std::string GetFormattedCoordinate(CoordinatesFormat format) const;

  /// UI setters
  void SetCustomName(std::string const & name);
  void SetTitlesForBookmark();
  void SetTitlesForTrack(Track const & track);
  void SetCustomNames(std::string const & title, std::string const & subtitle);
  void SetCustomNameWithCoordinates(m2::PointD const & mercator, std::string const & name);
  void SetAddress(std::string && address) { m_address = std::move(address); }
  void SetCanEditOrAdd(bool canEditOrAdd) { m_canEditOrAdd = canEditOrAdd; }

  /// Bookmark
  void SetFromBookmarkProperties(kml::Properties const & p);
  void SetBookmarkId(kml::MarkId bookmarkId);
  kml::MarkId GetBookmarkId() const { return m_bookmarkId; }
  void SetBookmarkCategoryId(kml::MarkGroupId markGroupId) { m_markGroupId = markGroupId; }
  kml::MarkGroupId GetBookmarkCategoryId() const { return m_markGroupId; }
  std::string const & GetBookmarkCategoryName() const { return m_bookmarkCategoryName; }
  void SetBookmarkCategoryName(std::string const & name) { m_bookmarkCategoryName = name; }
  void SetBookmarkData(kml::BookmarkData const & data) { m_bookmarkData = data; }
  kml::BookmarkData const & GetBookmarkData() const { return m_bookmarkData; }

  /// Track
  void SetTrackId(kml::TrackId trackId) { m_trackId = trackId; }
  kml::TrackId GetTrackId() const { return m_trackId; }
  void SetTrackRelationId(RelationID const & relationId) { m_trackRelationId = relationId; }
  RelationID const & GetTrackRelationId() const { return m_trackRelationId; }

  void SetTrackCandidates(std::vector<Track::TrackSelectionInfo> candidates);
  // Returns only actionable candidates: empty when there is no real choice, otherwise 2 or more tracks.
  std::vector<Track::TrackSelectionInfo> const & GetTrackCandidates() const { return m_trackSelectionCandidates; }

  /// Api
  void SetApiId(std::string const & apiId) { m_apiId = apiId; }
  void SetApiUrl(std::string const & url) { m_apiUrl = url; }
  std::string const & GetApiUrl() const { return m_apiUrl; }

  void SetOpeningMode(OpeningMode openingMode) { m_openingMode = openingMode; }
  OpeningMode GetOpeningMode() const { return m_openingMode; }

  /// Feature status
  void SetFeatureStatus(FeatureStatus const status) { m_featureStatus = status; }
  FeatureStatus GetFeatureStatus() const { return m_featureStatus; }

  /// Routing
  void SetRouteMarkType(RouteMarkType type) { m_routeMarkType = type; }
  RouteMarkType GetRouteMarkType() const { return m_routeMarkType; }
  void SetIntermediateIndex(size_t index) { m_intermediateIndex = index; }
  size_t GetIntermediateIndex() const { return m_intermediateIndex; }
  void SetIsRoutePoint() { m_isRoutePoint = true; }

  /// Road type
  void SetRoadType(FeatureType & ft, RoadWarningMarkType type, std::string const & localizedType,
                   std::string const & distance);
  void SetRoadType(RoadWarningMarkType type, std::string const & localizedType, std::string const & distance);
  RoadWarningMarkType GetRoadType() const { return m_roadType; }

  /// CountryId
  /// Which mwm this MapObject is in.
  /// Exception: for a country-name point it will be set to the topmost node for the mwm.
  /// TODO(@a): use m_topmostCountryIds in exceptional case.
  void SetCountryId(storage::CountryId const & countryId) { m_countryId = countryId; }
  storage::CountryId const & GetCountryId() const { return m_countryId; }
  template <typename Countries>
  void SetTopmostCountryIds(Countries && ids)
  {
    m_topmostCountryIds = std::forward<Countries>(ids);
  }
  storage::CountriesVec const & GetTopmostCountryIds() const { return m_topmostCountryIds; }

  /// MapObject
  void SetFromFeatureType(FeatureType & ft);

  void SetWikiDescription(std::string && description) { m_wikiDescription = std::move(description); }
  void SetOSMDescription(std::string && description) { m_osmDescription = std::move(description); }

  void SetMercator(m2::PointD const & mercator);
  std::vector<std::string> GetRawTypes() const { return m_types.ToObjectNames(); }

  bool IsHotel() const { return m_isHotel; }

  // void SetPopularity(uint8_t popularity) { m_popularity = popularity; }
  // uint8_t GetPopularity() const { return m_popularity; }
  std::string const & GetPrimaryFeatureName() const { return m_primaryFeatureName; }

  void SetSelectedObject(df::SelectionShape::ESelectedObject selectedObject) { m_selectedObject = selectedObject; }
  df::SelectionShape::ESelectedObject GetSelectedObject() const { return m_selectedObject; }

  std::string FormatRouteRefs() const;

  struct RouteRef
  {
    std::string m_ref, m_from, m_to;
    int m_iRef;  // used for int-like sort
    uint32_t m_relID;
    feature::RouteRelationBase::Type m_type;
    dp::Color m_color;  // kEmptyColor if the relation has no color tag

    RouteRef(uint32_t relID, feature::RouteRelationBase const & rel);
  };

  std::vector<RouteRef> const & GetRoutes() const { return m_routes; }

private:
  std::string FormatSubtitle(bool withTypes, bool withMainType) const;
  std::string GetBookmarkName();

  place_page::BuildInfo m_buildInfo;

  /// UI
  std::string m_uiTitle;
  std::string m_uiSubtitle;
  std::string m_uiSecondaryTitle;
  std::string m_uiAddress;
  std::string m_uiTrackStatistics;
  std::string m_wikiDescription;
  std::string m_osmDescription;
  /// Booking rating string
  std::string m_localizedRatingString;

  /// CountryId
  storage::CountryId m_countryId = storage::kInvalidCountryId;
  /// The topmost downloader nodes this MapObject is in, i.e.
  /// the country name for an object whose mwm represents only
  /// one part of the country (or several countries for disputed territories).
  storage::CountriesVec m_topmostCountryIds;

  /// Comes from API, shared links etc.
  std::string m_customName;

  /// Bookmark or track
  kml::MarkGroupId m_markGroupId = kml::kInvalidMarkGroupId;
  /// If not invalid, bookmark is bound to this place page.
  kml::MarkId m_bookmarkId = kml::kInvalidMarkId;
  /// Bookmark category name. Empty, if it's not bookmark; used only on iOS now.
  std::string m_bookmarkCategoryName;
  kml::BookmarkData m_bookmarkData;
  /// If not invalid, track is bound to this place page.
  kml::TrackId m_trackId = kml::kInvalidTrackId;
  /// If valid, relation track is bound to this place page.
  RelationID m_trackRelationId;
  std::vector<Track::TrackSelectionInfo> m_trackSelectionCandidates;
  /// Whether to treat it as plain feature.
  bool m_hasMetadata = false;

  /// Api ID passed for the selected object. It's automatically included in api url below.
  std::string m_apiId;
  /// [Deep] link to open when "Back" button is pressed in a Place Page.
  std::string m_apiUrl;
  /// Formatted feature address for inner using.
  std::string m_address;

  /// Routing
  RouteMarkType m_routeMarkType = RouteMarkType::Start;
  size_t m_intermediateIndex = 0;
  bool m_isRoutePoint = false;

  /// Road type
  RoadWarningMarkType m_roadType = RoadWarningMarkType::Count;

  /// Editor
  /// True if editing of a selected point is allowed by basic logic.
  /// See initialization in framework.
  bool m_canEditOrAdd = false;

  /// Feature status
  FeatureStatus m_featureStatus = FeatureStatus::Untouched;

  bool m_isHotel = false;

  // uint8_t m_popularity = 0;

  std::string m_primaryFeatureName;

  OpeningMode m_openingMode = OpeningMode::Preview;

  df::SelectionShape::ESelectedObject m_selectedObject = df::SelectionShape::ESelectedObject::OBJECT_EMPTY;

  std::vector<RouteRef> m_routes;
};
}  // namespace place_page
