#pragma once

#include "map/api_mark_point.hpp"
#include "map/booking_filter_params.hpp"
#include "map/booking_filter_processor.hpp"
#include "map/bookmark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/discovery/discovery_manager.hpp"
#include "map/displacement_mode_manager.hpp"
#include "map/feature_vec_model.hpp"
#include "map/local_ads_manager.hpp"
#include "map/mwm_url.hpp"
#include "map/notifications/notification_manager.hpp"
#include "map/place_page_info.hpp"
#include "map/power_manager/power_manager.hpp"
#include "map/purchase.hpp"
#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"
#include "map/search_api.hpp"
#include "map/search_mark.hpp"
#include "map/tips_api.hpp"
#include "map/track.hpp"
#include "map/traffic_manager.hpp"
#include "map/transit/transit_reader.hpp"
#include "map/user.hpp"

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "drape/drape_global.hpp"
#include "drape/graphics_context_factory.hpp"

#include "kml/type_utils.hpp"

#include "ugc/api.hpp"

#include "editor/new_feature_categories.hpp"
#include "editor/user_stats.hpp"

#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"
#include "indexer/data_source_helpers.hpp"
#include "indexer/map_style.hpp"
#include "indexer/popularity_loader.hpp"

#include "search/city_finder.hpp"
#include "search/displayed_categories.hpp"
#include "search/mode.hpp"
#include "search/query_saver.hpp"
#include "search/result.hpp"

#include "storage/downloading_policy.hpp"
#include "storage/storage.hpp"

#include "tracking/reporter.hpp"

#include "partners_api/banner.hpp"
#include "partners_api/booking_api.hpp"
#include "partners_api/locals_api.hpp"
#include "partners_api/taxi_engine.hpp"
#include "partners_api/viator_api.hpp"

#include "metrics/eye_info.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"
#include "platform/platform.hpp"

#include "routing/router.hpp"
#include "routing/routing_session.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/deferred_task.hpp"
#include "base/macros.hpp"
#include "base/strings_bundle.hpp"
#include "base/thread_checker.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/target_os.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include <boost/optional.hpp>

namespace osm
{
class EditableMapObject;
}

namespace search
{
struct EverywhereSearchParams;
struct ViewportSearchParams;
}

namespace storage
{
class CountryInfoGetter;
struct DownloaderSearchParams;
}

namespace routing { namespace turns{ class Settings; } }

namespace platform
{
class NetworkPolicy;
}

namespace ads
{
class Engine;
}

namespace descriptions
{
class Loader;
}

/// Uncomment line to make fixed position settings and
/// build version for screenshots.
//#define FIXED_LOCATION

struct FrameworkParams
{
  bool m_enableLocalAds = true;
  bool m_enableDiffs = true;

  FrameworkParams() = default;
  FrameworkParams(bool enableLocalAds, bool enableDiffs)
    : m_enableLocalAds(enableLocalAds)
    , m_enableDiffs(enableDiffs)
  {}
};

class Framework : public SearchAPI::Delegate,
                  public RoutingManager::Delegate,
                  public TipsApi::Delegate,
                  public notifications::NotificationManager::Delegate,
                  private PowerManager::Subscriber
{
  DISALLOW_COPY(Framework);

#ifdef FIXED_LOCATION
  class FixedPosition
  {
    pair<double, double> m_latlon;
    double m_dirFromNorth;
    bool m_fixedLatLon, m_fixedDir;

  public:
    FixedPosition();

    void GetLat(double & l) const { if (m_fixedLatLon) l = m_latlon.first; }
    void GetLon(double & l) const { if (m_fixedLatLon) l = m_latlon.second; }

    void GetNorth(double & n) const { if (m_fixedDir) n = m_dirFromNorth; }
    bool HasNorth() const { return m_fixedDir; }

  } m_fixedPos;
#endif
    
private:
  // Must be first member in Framework and must be destroyed first in Framework destructor.
  unique_ptr<Platform::ThreadRunner> m_threadRunner = make_unique<Platform::ThreadRunner>();

protected:
  using TDrapeFunction = function<void (df::DrapeEngine *)>;

  StringsBundle m_stringsBundle;

  model::FeaturesFetcher m_model;

  // The order matters here: DisplayedCategories may be used only
  // after classificator is loaded by |m_model|.
  unique_ptr<search::DisplayedCategories> m_displayedCategories;

  // The order matters here: storage::CountryInfoGetter and
  // m_model::FeaturesFetcher must be initialized before
  // search::Engine and, therefore, destroyed after search::Engine.
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;

  unique_ptr<ugc::Api> m_ugcApi;

  LocalAdsManager m_localAdsManager;

  unique_ptr<SearchAPI> m_searchAPI;

  search::QuerySaver m_searchQuerySaver;

  ScreenBase m_currentModelView;
  m2::RectD m_visibleViewport;

  using TViewportChanged = df::DrapeEngine::TModelViewListenerFn;
  TViewportChanged m_viewportChanged;

  drape_ptr<df::DrapeEngine> m_drapeEngine;

  double m_startForegroundTime = 0.0;
  double m_startBackgroundTime = 0.0;

  StorageDownloadingPolicy m_storageDownloadingPolicy;
  storage::Storage m_storage;
  bool m_enabledDiffs;

  location::TMyPositionModeChanged m_myPositionListener;

  unique_ptr<BookmarkManager> m_bmManager;

  SearchMarks m_searchMarks;

  unique_ptr<booking::Api> m_bookingApi = make_unique<booking::Api>();
  unique_ptr<viator::Api> m_viatorApi = make_unique<viator::Api>();
  unique_ptr<locals::Api> m_localsApi = make_unique<locals::Api>();

  df::DrapeApi m_drapeApi;

  bool m_isRenderingEnabled;

  TransitReadManager m_transitManager;

  // Note. |m_routingManager| should be declared before |m_trafficManager|
  RoutingManager m_routingManager;

  TrafficManager m_trafficManager;

  User m_user;

  booking::filter::FilterProcessor m_bookingFilterProcessor;
  booking::AvailabilityParams m_bookingAvailabilityParams;

  /// This function will be called by m_storage when latest local files
  /// is downloaded.
  void OnCountryFileDownloaded(storage::TCountryId const & countryId, storage::TLocalFilePtr const localFile);

  /// This function will be called by m_storage before latest local files
  /// is deleted.
  bool OnCountryFileDelete(storage::TCountryId const & countryId, storage::TLocalFilePtr const localFile);

  /// This function is called by m_model when the map file is deregistered.
  void OnMapDeregistered(platform::LocalCountryFile const & localFile);

  void ClearAllCaches();

  void StopLocationFollow();

  void OnViewportChanged(ScreenBase const & screen);

  void InitTransliteration();

public:
  Framework(FrameworkParams const & params = {});
  virtual ~Framework();

  /// Get access to booking api helpers
  booking::Api * GetBookingApi(platform::NetworkPolicy const & policy);
  booking::Api const * GetBookingApi(platform::NetworkPolicy const & policy) const;
  viator::Api * GetViatorApi(platform::NetworkPolicy const & policy);
  taxi::Engine * GetTaxiEngine(platform::NetworkPolicy const & policy);
  locals::Api * GetLocalsApi(platform::NetworkPolicy const & policy);
  // NotificationManager::Delegate override.
  ugc::Api * GetUGCApi() override { return m_ugcApi.get(); }
  ugc::Api const * GetUGCApi() const { return m_ugcApi.get(); }

  df::DrapeApi & GetDrapeApi() { return m_drapeApi; }

  User & GetUser() { return m_user; }
  User const & GetUser() const { return m_user; }

  /// Migrate to new version of very different data.
  bool IsEnoughSpaceForMigrate() const;
  storage::TCountryId PreMigrate(ms::LatLon const & position, storage::Storage::TChangeCountryFunction const & change,
                  storage::Storage::TProgressFunction const & progress);
  void Migrate(bool keepDownloaded = true);

  /// \returns true if there're unsaved changes in map with |countryId| and false otherwise.
  /// \note It works for group and leaf node.
  bool HasUnsavedEdits(storage::TCountryId const & countryId);

  /// Registers all local map files in internal indexes.
  void RegisterAllMaps();

  /// Deregisters all registered map files.
  void DeregisterAllMaps();

  /// Registers a local map file in internal indexes.
  pair<MwmSet::MwmId, MwmSet::RegResult> RegisterMap(
      platform::LocalCountryFile const & localFile);
  //@}

  /// Shows group or leaf mwm on the map.
  void ShowNode(storage::TCountryId const & countryId);

  // TipsApi::Delegate override.
  /// Checks, whether the country which contains the specified point is loaded.
  bool IsCountryLoaded(m2::PointD const & pt) const override;
  /// Checks, whether the country is loaded.
  bool IsCountryLoadedByName(string const & name) const;
  //@}

  void InvalidateRect(m2::RectD const & rect);

  /// @name Get any country info by point.
  //@{
  storage::TCountryId GetCountryIndex(m2::PointD const & pt) const;

  string GetCountryName(m2::PointD const & pt) const;
  //@}

  enum class DoAfterUpdate
  {
    Nothing,
    AutoupdateMaps,
    AskForUpdateMaps,
    Migrate
  };

  DoAfterUpdate ToDoAfterUpdate() const;

  storage::Storage & GetStorage() { return m_storage; }
  storage::Storage const & GetStorage() const { return m_storage; }
  search::DisplayedCategories const & GetDisplayedCategories();
  storage::CountryInfoGetter & GetCountryInfoGetter() { return *m_infoGetter; }
  StorageDownloadingPolicy & GetDownloadingPolicy() { return m_storageDownloadingPolicy; }

  DataSource const & GetDataSource() const { return m_model.GetDataSource(); }

  SearchAPI & GetSearchAPI();
  SearchAPI const & GetSearchAPI() const;

  /// @name Bookmarks, Tracks and other UserMarks
  //@{
  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();

  /// @return Created bookmark category id.
  kml::MarkGroupId AddCategory(string const & categoryName);

  kml::MarkGroupId LastEditedBMCategory() { return GetBookmarkManager().LastEditedBMCategory(); }
  kml::PredefinedColor LastEditedBMColor() const { return GetBookmarkManager().LastEditedBMColor(); }

  void ShowBookmark(kml::MarkId id);
  void ShowBookmark(Bookmark const * bookmark);
  void ShowTrack(kml::TrackId trackId);
  void ShowFeatureByMercator(m2::PointD const & pt);
  void ShowBookmarkCategory(kml::MarkGroupId categoryId);

  void AddBookmarksFile(string const & filePath, bool isTemporaryFile);

  BookmarkManager & GetBookmarkManager();
  BookmarkManager const & GetBookmarkManager() const;

  // Utilities
  void VisualizeRoadsInRect(m2::RectD const & rect);
  void VisualizeCityBoundariesInRect(m2::RectD const & rect);
  void VisualizeCityRoadsInRect(m2::RectD const & rect);

  ads::Engine const & GetAdsEngine() const;
  void DisableAdProvider(ads::Banner::Type const type, ads::Banner::Place const place);
  bool HasRuTaxiCategoryBanner();

public:
  // SearchAPI::Delegate overrides:
  void RunUITask(function<void()> fn) override;
  void SetSearchDisplacementModeEnabled(bool enabled) override;
  void ShowViewportSearchResults(search::Results::ConstIter begin,
                                 search::Results::ConstIter end, bool clear) override;
  void ShowViewportSearchResults(search::Results::ConstIter begin,
                                 search::Results::ConstIter end, bool clear,
                                 booking::filter::Types types) override;
  void ClearViewportSearchResults() override;
  // SearchApi::Delegate and TipsApi::Delegate override.
  boost::optional<m2::PointD> GetCurrentPosition() const override;
  bool ParseSearchQueryCommand(search::SearchParams const & params) override;
  search::ProductInfo GetProductInfo(search::Result const & result) const override;
  double GetMinDistanceBetweenResults() const override;

private:
  void ActivateMapSelection(bool needAnimation,
                            df::SelectionShape::ESelectedObject selectionType,
                            place_page::Info const & info);
  void InvalidateUserMarks();

public:
  void DeactivateMapSelection(bool notifyUI);
  /// Used to "refresh" UI in some cases (e.g. feature editing).
  void UpdatePlacePageInfoForCurrentSelection();

  /// Called to notify UI that object on a map was selected (UI should show Place Page, for example).
  using TActivateMapSelectionFn = function<void (place_page::Info const &)>;
  /// Called to notify UI that object on a map was deselected (UI should hide Place Page).
  /// If switchFullScreenMode is true, ui can [optionally] enter or exit full screen mode.
  using TDeactivateMapSelectionFn = function<void (bool /*switchFullScreenMode*/)>;
  void SetMapSelectionListeners(TActivateMapSelectionFn const & activator,
                                TDeactivateMapSelectionFn const & deactivator);

  void ResetLastTapEvent();

  void InvalidateRendering();
  void EnableDebugRectRendering(bool enabled);

  void EnableChoosePositionMode(bool enable, bool enableBounds, bool applyPosition, m2::PointD const & position);
  void BlockTapEvents(bool block);

  using TCurrentCountryChanged = function<void(storage::TCountryId const &)>;
  storage::TCountryId const & GetLastReportedCountry() { return m_lastReportedCountry; }
  /// Guarantees that listener is called in the main thread context.
  void SetCurrentCountryChangedListener(TCurrentCountryChanged const & listener);

  vector<MwmSet::MwmId> GetMwmsByRect(m2::RectD const & rect, bool rough) const;
  MwmSet::MwmId GetMwmIdByName(string const & name) const;

  void ReadFeatures(function<void(FeatureType &)> const & reader,
                    vector<FeatureID> const & features);

private:
  struct TapEvent
  {
    enum class Source
    {
      User,
      Search,
      Other
    };

    TapEvent(df::TapInfo const & info, Source source)
      : m_info(info)
      , m_source(source)
    {}

    df::TapInfo const m_info;
    Source const m_source;
  };

  unique_ptr<TapEvent> m_lastTapEvent;
  bool m_isViewportInitialized = false;

  void OnTapEvent(TapEvent const & tapEvent);
  /// outInfo is valid only if return value is not df::SelectionShape::OBJECT_EMPTY.
  df::SelectionShape::ESelectedObject OnTapEventImpl(TapEvent const & tapEvent,
                                                     place_page::Info & outInfo);
  unique_ptr<TapEvent> MakeTapEvent(m2::PointD const & center, FeatureID const & fid,
                                    TapEvent::Source source) const;
  UserMark const * FindUserMarkInTapPosition(df::TapInfo const & tapInfo) const;
  FeatureID FindBuildingAtPoint(m2::PointD const & mercator) const;

  void UpdateMinBuildingsTapZoom();

  int m_minBuildingsTapZoom;

  TActivateMapSelectionFn m_activateMapSelectionFn;
  TDeactivateMapSelectionFn m_deactivateMapSelectionFn;

  /// Here we store last selected feature to get its polygons in case of adding organization.
  mutable FeatureID m_selectedFeature;

private:
  vector<m2::TriangleD> GetSelectedFeatureTriangles() const;

public:

  /// @name GPS location updates routine.
  //@{
  void OnLocationError(location::TLocationError error);
  void OnLocationUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);
  void SwitchMyPositionNextMode();
  /// Should be set before Drape initialization. Guarantees that fn is called in main thread context.
  void SetMyPositionModeListener(location::TMyPositionModeChanged && fn);

private:
  void OnUserPositionChanged(m2::PointD const & position, bool hasPosition);
  //@}

public:
  struct DrapeCreationParams
  {
    dp::ApiVersion m_apiVersion = dp::ApiVersion::OpenGLES2;
    float m_visualScale = 1.0f;
    int m_surfaceWidth = 0;
    int m_surfaceHeight = 0;
    gui::TWidgetsInitInfo m_widgetsInitInfo;

    bool m_hasMyPositionState = false;
    location::EMyPositionMode m_initialMyPositionState = location::PendingPosition;

    bool m_isChoosePositionMode = false;
    df::Hints m_hints;
  };

  void CreateDrapeEngine(ref_ptr<dp::GraphicsContextFactory> contextFactory, DrapeCreationParams && params);
  ref_ptr<df::DrapeEngine> GetDrapeEngine();
  bool IsDrapeEngineCreated() const { return m_drapeEngine != nullptr; }
  void DestroyDrapeEngine();
  /// Called when graphics engine should be temporarily paused and then resumed.
  void SetRenderingEnabled(ref_ptr<dp::GraphicsContextFactory> contextFactory = nullptr);
  void SetRenderingDisabled(bool destroyContext);

  void OnRecoverGLContext(int width, int height);
  void OnDestroyGLContext();

private:
  /// Depends on initialized Drape engine.
  void SaveViewport();
  /// Depends on initialized Drape engine.
  void LoadViewport();

public:
  void ConnectToGpsTracker();
  void DisconnectFromGpsTracker();

  void SetMapStyle(MapStyle mapStyle);
  void MarkMapStyle(MapStyle mapStyle);
  MapStyle GetMapStyle() const;

  void SetupMeasurementSystem();

  void SetWidgetLayout(gui::TWidgetsLayoutInfo && layout);

  void PrepareToShutdown();

  void SetDisplacementMode(DisplacementModeManager::Slot slot, bool show);

private:
  void InitCountryInfoGetter();
  void InitUGC();
  void InitSearchAPI();
  void InitDiscoveryManager();

  DisplacementModeManager m_displacementModeManager;

  bool m_connectToGpsTrack; // need to connect to tracker when Drape is being constructed

  void OnUpdateCurrentCountry(m2::PointD const & pt, int zoomLevel);

  storage::TCountryId m_lastReportedCountry;
  TCurrentCountryChanged m_currentCountryChanged;

  void OnUpdateGpsTrackPointsCallback(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                      pair<size_t, size_t> const & toRemove);

  CachingPopularityLoader m_popularityLoader;

  unique_ptr<descriptions::Loader> m_descriptionsLoader;

public:
  using TSearchRequest = search::QuerySaver::TSearchRequest;

  // When search in viewport is active or delayed, restarts search in
  // viewport. When |forceSearch| is false, request is skipped when it
  // is similar to the previous request in the current
  // search-in-viewport session.
  void PokeSearchInViewport(bool forceSearch = true);

  // Search everywhere.
  bool SearchEverywhere(search::EverywhereSearchParams const & params);

  // Search in the viewport.
  bool SearchInViewport(search::ViewportSearchParams const & params);

  // Search for maps by countries or cities.
  bool SearchInDownloader(storage::DownloaderSearchParams const & params);

  // Search for bookmarks by query string.
  bool SearchInBookmarks(search::BookmarksSearchParams const & params);

  void CancelSearch(search::Mode mode);
  void CancelAllSearches();

  // Moves viewport to the search result and taps on it.
  void SelectSearchResult(search::Result const & res, bool animation);

  // Cancels all searches, stops location follow and then selects
  // search result.
  void ShowSearchResult(search::Result const & res, bool animation = true);

  size_t ShowSearchResults(search::Results const & results);

  using SearchMarkPostProcessing = function<void(SearchMarkPoint & mark)>;

  void FillSearchResultsMarks(bool clear, search::Results const & results);
  void FillSearchResultsMarks(search::Results::ConstIter begin, search::Results::ConstIter end,
                                bool clear, SearchMarkPostProcessing fn = nullptr);
  list<TSearchRequest> const & GetLastSearchQueries() const { return m_searchQuerySaver.Get(); }
  void SaveSearchQuery(TSearchRequest const & query) { m_searchQuerySaver.Add(query); }
  void ClearSearchHistory() { m_searchQuerySaver.Clear(); }

  /// Calculate distance and direction to POI for the given position.
  /// @param[in]  point             POI's position;
  /// @param[in]  lat, lon, north   Current position and heading from north;
  /// @param[out] distance          Formatted distance string;
  /// @param[out] azimut            Azimut to point from (lat, lon);
  /// @return true  If the POI is near the current position (distance < 25 km);
  bool GetDistanceAndAzimut(m2::PointD const & point,
                            double lat, double lon, double north,
                            string & distance, double & azimut);

  /// @name Manipulating with model view
  //@{
  inline m2::PointD PtoG(m2::PointD const & p) const { return m_currentModelView.PtoG(p); }
  inline m2::PointD P3dtoG(m2::PointD const & p) const { return m_currentModelView.PtoG(m_currentModelView.P3dtoP(p)); }
  inline m2::PointD GtoP(m2::PointD const & p) const { return m_currentModelView.GtoP(p); }
  inline m2::PointD GtoP3d(m2::PointD const & p) const { return m_currentModelView.PtoP3d(m_currentModelView.GtoP(p)); }

  /// Show all model by it's world rect.
  void ShowAll();

  m2::PointD GetPixelCenter() const;
  m2::PointD GetVisiblePixelCenter() const;

  m2::PointD const & GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);
  void SetViewportCenter(m2::PointD const & pt, int zoomLevel);

  m2::RectD GetCurrentViewport() const;
  void SetVisibleViewport(m2::RectD const & rect);

  /// - Check minimal visible scale according to downloaded countries.
  void ShowRect(m2::RectD const & rect, int maxScale = -1, bool animation = true);
  void ShowRect(m2::AnyRectD const & rect);

  void GetTouchRect(m2::PointD const & center, uint32_t pxRadius, m2::AnyRectD & rect);

  void SetViewportListener(TViewportChanged const & fn);

  /// Resize event from window.
  void OnSize(int w, int h);

  enum EScaleMode
  {
    SCALE_MAG,
    SCALE_MAG_LIGHT,
    SCALE_MIN,
    SCALE_MIN_LIGHT
  };

  void Scale(EScaleMode mode, bool isAnim);
  void Scale(EScaleMode mode, m2::PointD const & pxPoint, bool isAnim);
  void Scale(double factor, bool isAnim);
  void Scale(double factor, m2::PointD const & pxPoint, bool isAnim);

  void TouchEvent(df::TouchEvent const & touch);
  //@}

  int GetDrawScale() const;

  void RunFirstLaunchAnimation();

  /// Set correct viewport, parse API, show balloon.
  bool ShowMapForURL(string const & url);
  url_scheme::ParsedMapApi::ParsingResult ParseAndSetApiURL(string const & url);

  struct ParsedRoutingData
  {
    ParsedRoutingData(vector<url_scheme::RoutePoint> const & points, routing::RouterType type)
      : m_points(points), m_type(type)
    {
    }
    vector<url_scheme::RoutePoint> m_points;
    routing::RouterType m_type;
  };

  ParsedRoutingData GetParsedRoutingData() const;
  url_scheme::SearchRequest GetParsedSearchRequest() const;

private:
  // TODO(vng): Uncomment when needed.
  //void GetLocality(m2::PointD const & pt, search::AddressInfo & info) const;
  /// @returns true if command was handled by editor.
  bool ParseEditorDebugCommand(search::SearchParams const & params);

  /// @returns true if command was handled by drape.
  bool ParseDrapeDebugCommand(string const & query);

  /// This function can be used for enabling some experimental features for routing.
  bool ParseRoutingDebugCommand(search::SearchParams const & params);

  void FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const;
  /// @param customTitle, if not empty, overrides any other calculated name.
  void FillPointInfo(m2::PointD const & mercator, string const & customTitle, place_page::Info & info) const;
  void FillInfoFromFeatureType(FeatureType & ft, place_page::Info & info) const;
  void FillApiMarkInfo(ApiMarkPoint const & api, place_page::Info & info) const;
  void FillSearchResultInfo(SearchMarkPoint const & smp, place_page::Info & info) const;
  void FillMyPositionInfo(place_page::Info & info, df::TapInfo const & tapInfo) const;
  void FillRouteMarkInfo(RouteMarkPoint const & rmp, place_page::Info & info) const;

public:
  void FillBookmarkInfo(Bookmark const & bmk, place_page::Info & info) const;
  void ResetBookmarkInfo(Bookmark const & bmk, place_page::Info & info) const;

  /// @returns address of nearby building with house number in approx 1km distance.
  search::AddressInfo GetAddressInfoAtPoint(m2::PointD const & pt) const;

  /// @returns Valid street address only if it was specified in OSM for given feature;
  /// @todo This functions are used in desktop app only. Should we really need them?
  //@{
  search::AddressInfo GetFeatureAddressInfo(FeatureType & ft) const;
  search::AddressInfo GetFeatureAddressInfo(FeatureID const & fid) const;
  //@}

  vector<string> GetPrintableFeatureTypes(FeatureType & ft) const;
  /// Get "best for the user" feature at given point even if it's invisible on the screen.
  /// Ignores coastlines and prefers buildings over other area features.
  /// @returns nullptr if no feature was found at the given mercator point.
  unique_ptr<FeatureType> GetFeatureAtPoint(m2::PointD const & mercator) const;
  template <typename TFn>
  void ForEachFeatureAtPoint(TFn && fn, m2::PointD const & mercator) const
  {
    indexer::ForEachFeatureAtPoint(m_model.GetDataSource(), fn, mercator, 0.0);
  }
  /// Set parse to false if you don't need all feature fields ready.
  /// TODO(AlexZ): Refactor code which uses this method to get rid of it.
  /// FeatureType instances should not be used outside ForEach* core methods.
  WARN_UNUSED_RESULT bool GetFeatureByID(FeatureID const & fid, FeatureType & ft) const;

  void MemoryWarning();
  void EnterBackground();
  void EnterForeground();

  /// Set the localized strings bundle
  inline void AddString(string const & name, string const & value)
  {
    m_stringsBundle.SetString(name, value);
  }

  StringsBundle const & GetStringsBundle();

  /// [in] lat, lon - last known location
  /// [out] lat, lon - predicted location
  static void PredictLocation(double & lat, double & lon, double accuracy,
                              double bearing, double speed, double elapsedSeconds);

public:
  string CodeGe0url(Bookmark const * bmk, bool addName);
  string CodeGe0url(double lat, double lon, double zoomLevel, string const & name);

  /// @name Api
  //@{
  string GenerateApiBackUrl(ApiMarkPoint const & point) const;
  url_scheme::ParsedMapApi const & GetApiDataHolder() const { return m_ParsedMapApi; }

private:
  url_scheme::ParsedMapApi m_ParsedMapApi;

public:
  //@}

  /// @name Data versions
  //@{
  bool IsDataVersionUpdated();
  void UpdateSavedDataVersion();
  int64_t GetCurrentDataVersion() const;
  //@}

public:
  void AllowTransliteration(bool allowTranslit);
  bool LoadTransliteration();
  void SaveTransliteration(bool allowTranslit);

  void Allow3dMode(bool allow3d, bool allow3dBuildings);
  void Save3dMode(bool allow3d, bool allow3dBuildings);
  void Load3dMode(bool & allow3d, bool & allow3dBuildings);

  void SetLargeFontsSize(bool isLargeSize);
  void SaveLargeFontsSize(bool isLargeSize);
  bool LoadLargeFontsSize();

  bool LoadAutoZoom();
  void AllowAutoZoom(bool allowAutoZoom);
  void SaveAutoZoom(bool allowAutoZoom);

  TrafficManager & GetTrafficManager();

  LocalAdsManager & GetLocalAdsManager();

  TransitReadManager & GetTransitManager();

  bool LoadTrafficEnabled();
  void SaveTrafficEnabled(bool trafficEnabled);

  bool LoadTrafficSimplifiedColors();
  void SaveTrafficSimplifiedColors(bool simplified);

  bool LoadTransitSchemeEnabled();
  void SaveTransitSchemeEnabled(bool enabled);

#if defined(OMIM_METAL_AVAILABLE)
  bool LoadMetalAllowed();
  void SaveMetalAllowed(bool allowed);
#endif

public:
  template <typename ResultCallback>
  uint32_t Discover(discovery::ClientParams && params, ResultCallback const & onResult,
                    discovery::Manager::ErrorCalback const & onError) const
  {
    CHECK(m_discoveryManager.get(), ());
    return m_discoveryManager->Discover(GetDiscoveryParams(move(params)), onResult, onError);
  }

  discovery::Manager::Params GetDiscoveryParams(discovery::ClientParams && clientParams) const;

  std::string GetDiscoveryViatorUrl() const;
  std::string GetDiscoveryLocalExpertsUrl() const;

private:
  m2::PointD GetDiscoveryViewportCenter() const;

public:
  /// Routing Manager
  RoutingManager & GetRoutingManager() { return m_routingManager; }
  RoutingManager const & GetRoutingManager() const { return m_routingManager; }
protected:
  /// RoutingManager::Delegate
  void OnRouteFollow(routing::RouterType type) override;
  void RegisterCountryFilesOnRoute(shared_ptr<routing::NumMwmIds> ptr) const override;

public:
  /// @name Editor interface.
  //@{
  /// Initializes feature for Create Object UI.
  /// @returns false in case when coordinate is in the ocean or mwm is not downloaded.
  bool CanEditMap() const;

  bool CreateMapObject(m2::PointD const & mercator, uint32_t const featureType, osm::EditableMapObject & emo) const;
  /// @returns false if feature is invalid or can't be edited.
  bool GetEditableMapObject(FeatureID const & fid, osm::EditableMapObject & emo) const;
  osm::Editor::SaveResult SaveEditedMapObject(osm::EditableMapObject emo);
  void DeleteFeature(FeatureID const & fid) const;
  osm::NewFeatureCategories GetEditorCategories() const;
  bool RollBackChanges(FeatureID const & fid);
  void CreateNote(osm::MapObject const & mapObject, osm::Editor::NoteProblemType const type,
                  string const & note);
  //@}

public:
  //@{
  // User statistics.

  editor::UserStats GetUserStats(string const & userName) const
  {
    return m_userStatsLoader.GetStats(userName);
  }

  // Reads user stats from server or gets it from cache calls |fn| on success.
  void UpdateUserStats(string const & userName, editor::UserStatsLoader::UpdatePolicy policy,
                       editor::UserStatsLoader::TOnUpdateCallback fn)
  {
    m_userStatsLoader.Update(userName, policy, fn);
  }

  void DropUserStats(string const & userName) { m_userStatsLoader.DropStats(userName); }

private:
  editor::UserStatsLoader m_userStatsLoader;
  //@}

public:
  storage::TCountriesVec GetTopmostCountries(ms::LatLon const & latlon) const;

private:
  unique_ptr<search::CityFinder> m_cityFinder;
  unique_ptr<ads::Engine> m_adsEngine;
  // The order matters here: storage::CountryInfoGetter and
  // search::CityFinder must be initialized before
  // taxi::Engine and, therefore, destroyed after taxi::Engine.
  unique_ptr<taxi::Engine> m_taxiEngine;

  void InitCityFinder();
  void InitTaxiEngine();

  void SetPlacePageLocation(place_page::Info & info);

  /// Find feature with viator near point, provided in |info|, and inject viator data into |info|.
  void InjectViator(place_page::Info & info);

  void FillLocalExperts(FeatureType & ft, place_page::Info & info) const;

  void FillDescription(FeatureType & ft, place_page::Info & info) const;

public:
  // UGC.
  void UploadUGC(User::CompleteUploadingHandler const & onCompleteUploading);
  void GetUGC(FeatureID const & id, ugc::Api::UGCCallback const & callback);
private:
  // Filters user's reviews.
  ugc::Reviews FilterUGCReviews(ugc::Reviews const & reviews) const;

public:
  void FilterResultsForHotelsQuery(booking::filter::Tasks const & filterTasks,
                                   search::Results const & results, bool inViewport) override;
  void OnBookingFilterParamsUpdate(booking::filter::Tasks const & filterTasks) override;

  booking::AvailabilityParams GetLastBookingAvailabilityParams() const;

private:
  // m_discoveryManager must be bellow m_searchApi, m_viatorApi, m_localsApi
  unique_ptr<discovery::Manager> m_discoveryManager;

public:
  std::unique_ptr<Purchase> const & GetPurchase() const { return m_purchase; }
  std::unique_ptr<Purchase> & GetPurchase() { return m_purchase; }

private:
  std::unique_ptr<Purchase> m_purchase;
  TipsApi m_tipsApi;
  notifications::NotificationManager m_notificationManager;
  PowerManager m_powerManager;

public:
  TipsApi const & GetTipsApi() const;

  // TipsApi::Delegate override.
  bool HaveTransit(m2::PointD const & pt) const override;
  double GetLastBackgroundTime() const override;

  bool MakePlacePageInfo(eye::MapObject const & mapObject, place_page::Info & info) const;

  PowerManager & GetPowerManager() { return m_powerManager; }

  // PowerManager::Subscriber override.
  void OnPowerFacilityChanged(PowerManager::Facility const facility, bool enabled) override;
};
