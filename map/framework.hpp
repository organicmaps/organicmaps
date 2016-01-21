#pragma once

#include "map/active_maps_layout.hpp"
#include "map/api_mark_point.hpp"
#include "map/bookmark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/country_tree.hpp"
#include "map/feature_vec_model.hpp"
#include "map/mwm_url.hpp"
#include "map/track.hpp"

#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "drape_frontend/watch/frame_image.hpp"

#include "drape/oglcontextfactory.hpp"

#include "indexer/data_header.hpp"
#include "indexer/map_style.hpp"

#include "search/query_saver.hpp"
#include "search/search_engine.hpp"

#include "storage/storage.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"

#include "routing/router.hpp"
#include "routing/routing_session.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/macros.hpp"
#include "base/strings_bundle.hpp"
#include "base/thread_checker.hpp"

#include "std/list.hpp"
#include "std/shared_ptr.hpp"
#include "std/target_os.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace search
{
  class Result;
  class Results;
  struct AddressInfo;
}

namespace storage
{
class CountryInfoGetter;
}

namespace routing { namespace turns{ class Settings; } }

class StorageBridge;

namespace df
{
  namespace watch
  {
    class CPUDrawer;
  }
}

/// Uncomment line to make fixed position settings and
/// build version for screenshots.
//#define FIXED_LOCATION

class Framework
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

protected:
  using TDrapeFunction = function<void (df::DrapeEngine *)>;
  using TDownloadCountryListener = function<void(storage::TIndex const &, int)>;

  StringsBundle m_stringsBundle;

  // The order matters here: storage::CountryInfoGetter must be
  // initialized before search::Engine.
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  unique_ptr<search::Engine> m_searchEngine;
  search::QuerySaver m_searchQuerySaver;

  model::FeaturesFetcher m_model;
  ScreenBase m_currentModelView;

  routing::RoutingSession m_routingSession;

  drape_ptr<StorageBridge> m_storageBridge;
  drape_ptr<df::DrapeEngine> m_drapeEngine;
  drape_ptr<df::watch::CPUDrawer> m_cpuDrawer;

  double m_startForegroundTime;

  TDownloadCountryListener m_downloadCountryListener;

  storage::Storage m_storage;
  shared_ptr<storage::ActiveMapsLayout> m_activeMaps;
  storage::CountryTree m_globalCntTree;

  location::TMyPositionModeChanged m_myPositionListener;

  BookmarkManager m_bmManager;

  /// This function is called by m_storage when latest local files
  /// were changed.
  void UpdateLatestCountryFile(platform::LocalCountryFile const & localFile);

  /// This function is called by m_model when the map file is deregistered.
  void OnMapDeregistered(platform::LocalCountryFile const & localFile);

  void ClearAllCaches();

  void StopLocationFollow();

  void CallDrapeFunction(TDrapeFunction const & fn);

public:
  Framework();
  virtual ~Framework();

  void InitWatchFrameRenderer(float visualScale);

  /// @param center - map center in Mercator
  /// @param zoomModifier - result zoom calculate like "base zoom" + zoomModifier
  ///                       if we are have search result "base zoom" calculate that my position and search result
  ///                       will be see with some bottom clamp.
  ///                       if we are't have search result "base zoom" == scales::GetUpperComfortScale() - 1
  /// @param pxWidth - result image width.
  ///                  It must be equal render buffer width. For retina it's equal 2.0 * displayWidth
  /// @param pxHeight - result image height.
  ///                   It must be equal render buffer height. For retina it's equal 2.0 * displayHeight
  /// @param symbols - configuration for symbols on the frame
  /// @param image [out] - result image
  void DrawWatchFrame(m2::PointD const & center, int zoomModifier,
                      uint32_t pxWidth, uint32_t pxHeight,
                      df::watch::FrameSymbols const & symbols,
                      df::watch::FrameImage & image);
  void ReleaseWatchFrameRenderer();
  bool IsWatchFrameRendererInited() const;

  /// Registers all local map files in internal indexes.
  void RegisterAllMaps();

  /// Deregisters all registered map files.
  void DeregisterAllMaps();

  /// Registers a local map file in internal indexes.
  pair<MwmSet::MwmId, MwmSet::RegResult> RegisterMap(
      platform::LocalCountryFile const & localFile);
  //@}

  /// Deletes all disk files corresponding to country.
  ///
  /// @name This functions is used by Downloader UI.
  //@{
  /// options - flags that signal about parts of map that must be deleted
  void DeleteCountry(storage::TIndex const & index, MapOptions opt);
  /// options - flags that signal about parts of map that must be downloaded
  void DownloadCountry(storage::TIndex const & index, MapOptions opt);

  void SetDownloadCountryListener(TDownloadCountryListener const & listener);

  storage::TStatus GetCountryStatus(storage::TIndex const & index) const;
  string GetCountryName(storage::TIndex const & index) const;

  /// Get country rect from borders (not from mwm file).
  /// @param[in] file Pass country file name without extension as an id.
  m2::RectD GetCountryBounds(string const & file) const;
  m2::RectD GetCountryBounds(storage::TIndex const & index) const;

  void ShowCountry(storage::TIndex const & index);

  /// Checks, whether the country which contains the specified point is loaded.
  bool IsCountryLoaded(m2::PointD const & pt) const;
  /// Checks, whether the country is loaded.
  bool IsCountryLoadedByName(string const & name) const;
  //@}

  void InvalidateRect(m2::RectD const & rect);

  /// @name Get any country info by point.
  //@{
  storage::TIndex GetCountryIndex(m2::PointD const & pt) const;

  string GetCountryName(m2::PointD const & pt) const;
  /// @param[in] id Country file name without an extension.
  string GetCountryName(string const & id) const;

  /// @return country code in ISO 3166-1 alpha-2 format (two small letters) or empty string
  string GetCountryCode(m2::PointD const & pt) const;
  //@}

  storage::Storage & Storage() { return m_storage; }
  shared_ptr<storage::ActiveMapsLayout> & GetActiveMaps() { return m_activeMaps; }
  storage::CountryTree & GetCountryTree() { return m_globalCntTree; }

  /// @name Bookmarks, Tracks and other UserMarks
  //@{
  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();

  /// @return Created bookmark index in category.
  size_t AddBookmark(size_t categoryIndex, m2::PointD const & ptOrg, BookmarkData & bm);
  /// @return New moved bookmark index in category.
  size_t MoveBookmark(size_t bmIndex, size_t curCatIndex, size_t newCatIndex);
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, BookmarkData const & bm);
  /// @return Created bookmark category index.
  size_t AddCategory(string const & categoryName);

  inline size_t GetBmCategoriesCount() const { return m_bmManager.GetBmCategoriesCount(); }
  /// @returns 0 if category is not found
  BookmarkCategory * GetBmCategory(size_t index) const;

  size_t LastEditedBMCategory() { return m_bmManager.LastEditedBMCategory(); }
  string LastEditedBMType() const { return m_bmManager.LastEditedBMType(); }

  /// Delete bookmarks category with all bookmarks.
  /// @return true if category was deleted
  bool DeleteBmCategory(size_t index);

  void ShowBookmark(BookmarkAndCategory const & bnc);
  void ShowTrack(Track const & track);

  void ClearBookmarks();

  bool AddBookmarksFile(string const & filePath);

  BookmarkAndCategory FindBookmark(UserMark const * mark) const;
  BookmarkManager & GetBookmarkManager() { return m_bmManager; }

  void ActivateUserMark(UserMark const * mark, bool needAnim);
  void DeactivateUserMark();
  bool HasActiveUserMark();
  void InvalidateUserMarks();
  PoiMarkPoint * GetAddressMark(m2::PointD const & globalPoint) const;

  using TActivateCallbackFn = function<void (unique_ptr<UserMarkCopy> mark)>;
  void SetUserMarkActivationListener(TActivateCallbackFn const & fn) { m_activateUserMarkFn = fn; }

  void ResetLastTapEvent();

  void InvalidateRendering();

private:
  struct TapEventData
  {
    m2::PointD m_pxPoint;
    bool m_isLong;
    bool m_isMyPosition;
    FeatureID m_feature;
  };
  unique_ptr<TapEventData> m_lastTapEvent;
#ifdef OMIM_OS_ANDROID
  unique_ptr<location::CompassInfo> m_lastCompassInfo;
  unique_ptr<location::GpsInfo> m_lastGPSInfo;
#endif

  void OnTapEvent(m2::PointD pxPoint, bool isLong, bool isMyPosition, FeatureID const & feature);
  UserMark const * OnTapEventImpl(m2::PointD pxPoint, bool isLong, bool isMyPosition, FeatureID const & feature);
  //@}

  TActivateCallbackFn m_activateUserMarkFn;

public:

  /// @name GPS location updates routine.
  //@{
  void OnLocationError(location::TLocationError error);
  void OnLocationUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);
  void SwitchMyPositionNextMode();
  void InvalidateMyPosition();
  void SetMyPositionModeListener(location::TMyPositionModeChanged const & fn);

private:
  void OnUserPositionChanged(m2::PointD const & position);
  //@}

public:
  struct DrapeCreationParams
  {
    float m_visualScale = 1.0f;
    int m_surfaceWidth = 0;
    int m_surfaceHeight = 0;
    gui::TWidgetsInitInfo m_widgetsInitInfo;

    bool m_hasMyPositionState = false;
    location::EMyPositionMode m_initialMyPositionState = location::MODE_UNKNOWN_POSITION;
  };

  void CreateDrapeEngine(ref_ptr<dp::OGLContextFactory> contextFactory, DrapeCreationParams && params);
  ref_ptr<df::DrapeEngine> GetDrapeEngine();
  void DestroyDrapeEngine();

  void ConnectToGpsTracker();
  void DisconnectFromGpsTracker();

  void SetMapStyle(MapStyle mapStyle);
  void MarkMapStyle(MapStyle mapStyle);
  MapStyle GetMapStyle() const;

  void SetupMeasurementSystem();

  void SetWidgetLayout(gui::TWidgetsLayoutInfo && layout);
  const gui::TWidgetsSizeInfo & GetWidgetSizes();

  void PrepareToShutdown();

private:
  void InitCountryInfoGetter();
  void InitSearchEngine();

  search::SearchParams m_lastSearch;
  uint8_t m_fixedSearchResults;

  bool m_connectToGpsTrack; // need to connect to tracker when Drape is being constructed

  void FillSearchResultsMarks(search::Results const & results);

  void OnDownloadMapCallback(storage::TIndex const & countryIndex);
  void OnDownloadMapRoutingCallback(storage::TIndex const & countryIndex);
  void OnDownloadRetryCallback(storage::TIndex const & countryIndex);

  void OnUpdateCountryIndex(storage::TIndex const & currentIndex, m2::PointF const & pt);
  void UpdateCountryInfo(storage::TIndex const & countryIndex, bool isCurrentCountry);

  void OnUpdateGpsTrackPointsCallback(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                      pair<size_t, size_t> const & toRemove);

public:
  using TSearchRequest = search::QuerySaver::TSearchRequest;

  void UpdateUserViewportChanged();

  /// Call this function before entering search GUI.
  /// While it's loading, we can cache features in viewport.
  void PrepareSearch();
  bool Search(search::SearchParams const & params);
  bool GetCurrentPosition(double & lat, double & lon) const;

  void LoadSearchResultMetadata(search::Result & res) const;
  void ShowSearchResult(search::Result const & res);
  size_t ShowAllSearchResults(search::Results const & results);

  void StartInteractiveSearch(search::SearchParams const & params);
  bool IsISActive() const { return !m_lastSearch.m_query.empty(); }
  void CancelInteractiveSearch();

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
  inline m2::PointD GtoP(m2::PointD const & p) const { return m_currentModelView.GtoP(p); }
  inline m2::PointD GtoP3d(m2::PointD const & p) const { return m_currentModelView.PtoP3d(m_currentModelView.GtoP(p)); }

  void SaveState();
  void LoadState();

  /// Show all model by it's world rect.
  void ShowAll();

  m2::PointD GetPixelCenter() const;

  m2::PointD const & GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  m2::RectD GetCurrentViewport() const;

  /// Show rect for point and needed draw scale.
  void ShowRect(double lat, double lon, double zoom);
  /// - Check minimal visible scale according to downloaded countries.
  void ShowRect(m2::RectD const & rect, int maxScale = -1);
  void ShowRect(m2::AnyRectD const & rect);

  void GetTouchRect(m2::PointD const & center, uint32_t pxRadius, m2::AnyRectD & rect);

  using TViewportChanged = df::DrapeEngine::TModelViewListenerFn;
  int AddViewportListener(TViewportChanged const & fn);
  void RemoveViewportListener(int slotID);

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

  /// Set correct viewport, parse API, show balloon.
  bool ShowMapForURL(string const & url);

  /// Get classificator types for nearest features.
  /// @param[in] pxPoint Current touch point in device pixel coordinates.
  void GetFeatureTypes(m2::PointD const & pxPoint, vector<string> & types) const;

  /// Get address information for point on map.
  inline void GetAddressInfoForPixelPoint(m2::PointD const & pxPoint, search::AddressInfo & info) const
  {
    GetAddressInfoForGlobalPoint(PtoG(pxPoint), info);
  }
  void GetAddressInfoForGlobalPoint(m2::PointD const & pt, search::AddressInfo & info) const;

private:
  void GetAddressInfo(FeatureType const & ft, m2::PointD const & pt, search::AddressInfo & info) const;
  void GetLocality(m2::PointD const & pt, search::AddressInfo & info) const;

public:
  bool GetVisiblePOI(m2::PointD const & glbPoint, search::AddressInfo & info, feature::Metadata & metadata) const;
  m2::PointD GetVisiblePOI(FeatureID const & id, search::AddressInfo & info, feature::Metadata & metadata) const;
  void FindClosestPOIMetadata(m2::PointD const & pt, feature::Metadata & metadata) const;

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
  string GenerateApiBackUrl(ApiMarkPoint const & point);
  url_scheme::ParsedMapApi const & GetApiDataHolder() const { return m_ParsedMapApi; }

private:
  url_scheme::ParsedMapApi m_ParsedMapApi;

public:
  //@}

  /// @name Data versions
  //@{
  bool IsDataVersionUpdated();
  void UpdateSavedDataVersion();
  int64_t GetCurrentDataVersion();
  //@}

public:
  using TRouteBuildingCallback = function<void(routing::IRouter::ResultCode,
                                               vector<storage::TIndex> const &,
                                               vector<storage::TIndex> const &)>;
  using TRouteProgressCallback = function<void(float)>;

  /// @name Routing mode
  //@{
  void SetRouter(routing::RouterType type);
  routing::RouterType GetRouter() const;
  bool IsRoutingActive() const { return m_routingSession.IsActive(); }
  bool IsRouteBuilt() const { return m_routingSession.IsBuilt(); }
  bool IsRouteBuilding() const { return m_routingSession.IsBuilding(); }
  bool IsOnRoute() const { return m_routingSession.IsOnRoute(); }
  bool IsRouteNavigable() const { return m_routingSession.IsNavigable(); }
  void BuildRoute(m2::PointD const & finish, uint32_t timeoutSec);
  void BuildRoute(m2::PointD const & start, m2::PointD const & finish, uint32_t timeoutSec);
  // FollowRoute has a bug where the router follows the route even if the method hads't been called.
  // This method was added because we do not want to break the behaviour that is familiar to our users.
  bool DisableFollowMode();
  void SetRouteBuildingListener(TRouteBuildingCallback const & buildingCallback) { m_routingCallback = buildingCallback; }
  void SetRouteProgressListener(TRouteProgressCallback const & progressCallback) { m_progressCallback = progressCallback; }
  void FollowRoute();
  void CloseRouting();
  void GetRouteFollowingInfo(location::FollowingInfo & info) const { m_routingSession.GetRouteFollowingInfo(info); }
  m2::PointD GetRouteEndPoint() const { return m_routingSession.GetEndPoint(); }
  routing::RouterType GetLastUsedRouter() const;
  void SetLastUsedRouter(routing::RouterType type);
  /// Returns the most situable router engine type. Bases on distance and the last used router.
  routing::RouterType GetBestRouter(m2::PointD const & startPoint, m2::PointD const & finalPoint);
  // Sound notifications for turn instructions.
  inline void EnableTurnNotifications(bool enable) { m_routingSession.EnableTurnNotifications(enable); }
  inline bool AreTurnNotificationsEnabled() const { return m_routingSession.AreTurnNotificationsEnabled(); }
  /// \brief Sets a locale for TTS.
  /// \param locale is a string with locale code. For example "en", "ru", "zh-Hant" and so on.
  /// \note See sound/tts/languages.txt for the full list of available locales.
  inline void SetTurnNotificationsLocale(string const & locale) { m_routingSession.SetTurnNotificationsLocale(locale); }
  /// @return current TTS locale. For example "en", "ru", "zh-Hant" and so on.
  /// In case of error returns an empty string.
  /// \note The method returns correct locale after SetTurnNotificationsLocale has been called.
  /// If not, it returns an empty string.
  inline string GetTurnNotificationsLocale() const { return m_routingSession.GetTurnNotificationsLocale(); }
  /// \brief When an end user is going to a turn he gets sound turn instructions.
  /// If C++ part wants the client to pronounce an instruction GenerateTurnNotifications (in
  /// turnNotifications) returns
  /// an array of one of more strings. C++ part assumes that all these strings shall be pronounced
  /// by the client's TTS.
  /// For example if C++ part wants the client to pronounce "Make a right turn." this method returns
  /// an array with one string "Make a right turn.". The next call of the method returns nothing.
  /// GenerateTurnNotifications shall be called by the client when a new position is available.
  inline void GenerateTurnNotifications(vector<string> & turnNotifications)
  {
    return m_routingSession.GenerateTurnNotifications(turnNotifications);
  }

  void SetRouteStartPoint(m2::PointD const & pt, bool isValid);
  void SetRouteFinishPoint(m2::PointD const & pt, bool isValid);

  void Allow3dMode(bool allow3d, bool allow3dBuildings);
  void Save3dMode(bool allow3d, bool allow3dBuildings);
  void Load3dMode(bool & allow3d, bool & allow3dBuildings);

private:
  void SetRouterImpl(routing::RouterType type);
  void RemoveRoute(bool deactivateFollowing);

  void InsertRoute(routing::Route const & route);
  void CheckLocationForRouting(location::GpsInfo const & info);
  void CallRouteBuilded(routing::IRouter::ResultCode code,
                        vector<storage::TIndex> const & absentCountries,
                        vector<storage::TIndex> const & absentRoutingFiles);
  void MatchLocationToRoute(location::GpsInfo & info, location::RouteMatchingInfo & routeMatchingInfo) const;
  string GetRoutingErrorMessage(routing::IRouter::ResultCode code);

  TRouteBuildingCallback m_routingCallback;
  TRouteProgressCallback m_progressCallback;
  routing::RouterType m_currentRouterType;
  //@}

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
