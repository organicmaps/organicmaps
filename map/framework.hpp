#pragma once

#include "map/api_mark_point.hpp"
#include "map/bookmark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/place_page_info.hpp"
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
#include "indexer/new_feature_categories.hpp"

#include "search/query_saver.hpp"
#include "search/search_engine.hpp"

#include "storage/downloader_search_params.hpp"
#include "storage/downloading_policy.hpp"
#include "storage/storage.hpp"

#include "platform/country_defines.hpp"
#include "platform/location.hpp"

#include "routing/router.hpp"
#include "routing/routing_session.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/deferred_task.hpp"
#include "base/macros.hpp"
#include "base/strings_bundle.hpp"
#include "base/thread_checker.hpp"

#include "std/list.hpp"
#include "std/shared_ptr.hpp"
#include "std/target_os.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"
#include "std/weak_ptr.hpp"

namespace osm
{
class EditableMapObject;
}

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

  StringsBundle m_stringsBundle;

  // The order matters here: storage::CountryInfoGetter and
  // m_model::FeaturesFetcher must be initialized before
  // search::Engine and, therefore, destroyed after search::Engine.

  model::FeaturesFetcher m_model;

  unique_ptr<storage::CountryInfoGetter> m_infoGetter;

  unique_ptr<search::Engine> m_searchEngine;

  search::QuerySaver m_searchQuerySaver;

  ScreenBase m_currentModelView;

  routing::RoutingSession m_routingSession;

  drape_ptr<df::DrapeEngine> m_drapeEngine;
  drape_ptr<df::watch::CPUDrawer> m_cpuDrawer;

  double m_startForegroundTime;
  double m_startBackgroundTime;

  StorageDownloadingPolicy m_storageDownloadingPolicy;
  storage::Storage m_storage;

  location::TMyPositionModeChanged m_myPositionListener;

  BookmarkManager m_bmManager;

  bool m_isRenderingEnabled;

  /// This function will be called by m_storage when latest local files
  /// is downloaded.
  void OnCountryFileDownloaded(storage::TCountryId const & countryId, storage::Storage::TLocalFilePtr const localFile);

  /// This function will be called by m_storage before latest local files
  /// is deleted.
  bool OnCountryFileDelete(storage::TCountryId const & countryId, storage::Storage::TLocalFilePtr const localFile);

  /// This function is called by m_model when the map file is deregistered.
  void OnMapDeregistered(platform::LocalCountryFile const & localFile);

  void ClearAllCaches();

  void StopLocationFollow();

  void CallDrapeFunction(TDrapeFunction const & fn) const;

public:
  Framework();
  virtual ~Framework();

  /// Migrate to new version of very different data.
  bool IsEnoughSpaceForMigrate() const;
  storage::TCountryId PreMigrate(ms::LatLon const & position, storage::Storage::TChangeCountryFunction const & change,
                  storage::Storage::TProgressFunction const & progress);
  void Migrate(bool keepDownloaded = true);

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

  /// Checks, whether the country which contains the specified point is loaded.
  bool IsCountryLoaded(m2::PointD const & pt) const;
  /// Checks, whether the country is loaded.
  bool IsCountryLoadedByName(string const & name) const;
  //@}

  void InvalidateRect(m2::RectD const & rect);

  /// @name Get any country info by point.
  //@{
  storage::TCountryId GetCountryIndex(m2::PointD const & pt) const;

  string GetCountryName(m2::PointD const & pt) const;
  //@}

  storage::Storage & Storage() { return m_storage; }
  storage::Storage const & Storage() const { return m_storage; }
  storage::CountryInfoGetter & CountryInfoGetter() { return *m_infoGetter; }
  StorageDownloadingPolicy & DownloadingPolicy() { return m_storageDownloadingPolicy; }

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

private:
  void ActivateMapSelection(bool needAnimation,
                            df::SelectionShape::ESelectedObject selectionType,
                            place_page::Info const & info) const;
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

  void EnableChoosePositionMode(bool enable, bool enableBounds, bool applyPosition, m2::PointD const & position);
  void BlockTapEvents(bool block);

  using TCurrentCountryChanged = function<void(storage::TCountryId const &)>;
  storage::TCountryId const & GetLastReportedCountry() { return m_lastReportedCountry; }
  /// Guarantees that listener is called in the main thread context.
  void SetCurrentCountryChangedListener(TCurrentCountryChanged const & listener);

private:
  unique_ptr<df::TapInfo> m_lastTapEvent;
#ifdef OMIM_OS_ANDROID
  unique_ptr<location::CompassInfo> m_lastCompassInfo;
  unique_ptr<location::GpsInfo> m_lastGPSInfo;
#endif

  void OnTapEvent(df::TapInfo const & tapInfo);
  /// outInfo is valid only if return value is not df::SelectionShape::OBJECT_EMPTY.
  df::SelectionShape::ESelectedObject OnTapEventImpl(df::TapInfo const & tapInfo,
                                                     place_page::Info & outInfo) const;
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
    location::EMyPositionMode m_initialMyPositionState = location::PendingPosition;

    bool m_isChoosePositionMode = false;

    bool m_isFirstLaunch = false;
  };

  void CreateDrapeEngine(ref_ptr<dp::OGLContextFactory> contextFactory, DrapeCreationParams && params);
  ref_ptr<df::DrapeEngine> GetDrapeEngine();
  void DestroyDrapeEngine();
  /// Called when graphics engine should be temporarily paused and then resumed.
  void SetRenderingEnabled(bool enable);
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

private:
  void InitCountryInfoGetter();
  void InitSearchEngine();

  // Last search query params for the interactive search.
  search::SearchParams m_lastInteractiveSearchParams;
  uint8_t m_fixedSearchResults;

  bool m_connectToGpsTrack; // need to connect to tracker when Drape is being constructed

  void FillSearchResultsMarks(search::Results const & results);

  void OnUpdateCurrentCountry(m2::PointF const & pt, int zoomLevel);

  storage::TCountryId m_lastReportedCountry;
  TCurrentCountryChanged m_currentCountryChanged;

  // Search query params and viewport for the latest search
  // query. These fields are used to check whether a new search query
  // can be skipped. Note that these fields are not guarded by a mutex
  // because we're assuming that they will be accessed only from the
  // UI thread.
  search::SearchParams m_lastQueryParams;
  m2::RectD m_lastQueryViewport;

  // A handle for the latest search query.
  weak_ptr<search::QueryHandle> m_lastQueryHandle;

  // Returns true when |params| and |viewport| are almost the same as
  // the latest search query's params and viewport.
  bool QueryMayBeSkipped(search::SearchParams const & params, m2::RectD const & viewport) const;

  void OnUpdateGpsTrackPointsCallback(vector<pair<size_t, location::GpsTrackInfo>> && toAdd,
                                      pair<size_t, size_t> const & toRemove);
  bool GetGroupCountryIdFromFeature(FeatureType const & ft, string & name) const;

public:
  using TSearchRequest = search::QuerySaver::TSearchRequest;

  void UpdateUserViewportChanged();

  /// Call this function before entering search GUI.
  /// While it's loading, we can cache features in viewport.
  bool Search(search::SearchParams const & params);
  /// Searches for mwm based on |params|.
  /// Calling |params::m_onResults| for returning the result.
  bool SearchInDownloader(storage::DownloaderSearchParams const & params);
  bool GetCurrentPosition(double & lat, double & lon) const;

  void LoadSearchResultMetadata(search::Result & res) const;

  void ShowSearchResult(search::Result const & res);
  size_t ShowSearchResults(search::Results const & results);

  void StartInteractiveSearch(search::SearchParams const & params);
  bool IsInteractiveSearchActive() const { return !m_lastInteractiveSearchParams.m_query.empty(); }
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

  /// Show all model by it's world rect.
  void ShowAll();

  m2::PointD GetPixelCenter() const;

  m2::PointD const & GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);
  void SetViewportCenter(m2::PointD const & pt, int zoomLevel);

  m2::RectD GetCurrentViewport() const;

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

private:
  // TODO(vng): Uncomment when needed.
  //void GetLocality(m2::PointD const & pt, search::AddressInfo & info) const;
  /// @returns true if command was handled by editor.
  bool ParseEditorDebugCommand(search::SearchParams const & params);

  void FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const;
  /// @param customTitle, if not empty, overrides any other calculated name.
  void FillPointInfo(m2::PointD const & mercator, string const & customTitle, place_page::Info & info) const;
  void FillInfoFromFeatureType(FeatureType const & ft, place_page::Info & info) const;
  void FillApiMarkInfo(ApiMarkPoint const & api, place_page::Info & info) const;
  void FillSearchResultInfo(SearchMarkPoint const & smp, place_page::Info & info) const;
  void FillMyPositionInfo(place_page::Info & info) const;

public:
  void FillBookmarkInfo(Bookmark const & bmk, BookmarkAndCategory const & bac, place_page::Info & info) const;

  /// @returns address of nearby building with house number in approx 1km distance.
  search::AddressInfo GetAddressInfoAtPoint(m2::PointD const & pt) const;

  /// @returns Valid street address only if it was specified in OSM for given feature;
  /// @todo This functions are used in desktop app only. Should we really need them?
  //@{
  search::AddressInfo GetFeatureAddressInfo(FeatureType & ft) const;
  search::AddressInfo GetFeatureAddressInfo(FeatureID const & fid) const;
  //@}

  vector<string> GetPrintableFeatureTypes(FeatureType const & ft) const;
  /// Get "best for the user" feature at given point even if it's invisible on the screen.
  /// Ignores coastlines and prefers buildings over other area features.
  /// @returns nullptr if no feature was found at the given mercator point.
  unique_ptr<FeatureType> GetFeatureAtPoint(m2::PointD const & mercator) const;
  using TFeatureTypeFn = function<void(FeatureType &)>;
  void ForEachFeatureAtPoint(TFeatureTypeFn && fn, m2::PointD const & mercator) const;
  /// Set parse to false if you don't need all feature fields ready.
  /// TODO(AlexZ): Refactor code which uses this method to get rid of it.
  /// FeatureType instances shoud not be used outside ForEach* core methods.
  unique_ptr<FeatureType> GetFeatureByID(FeatureID const & fid, bool parse = true) const;

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
  int64_t GetCurrentDataVersion();
  //@}

public:
  using TRouteBuildingCallback = function<void(routing::IRouter::ResultCode,
                                               storage::TCountriesVec const &)>;
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
  /// @TODO(AlexZ): Warning! These two routing callbacks are the only callbacks which are not called in the main thread context.
  /// UI code should take it into an account. This is a result of current implementation, that can be improved:
  /// Drape core calls some RunOnGuiThread with "this" pointers, and it causes crashes on Android, when Drape engine is destroyed
  /// while switching between activities. Current workaround cleans all callbacks when destroying Drape engine
  /// (@see MwmApplication.clearFunctorsOnUiThread on Android). Better soulution can be fair copying of all needed information into
  /// lambdas/functors before calling RunOnGuiThread.
  void SetRouteBuildingListener(TRouteBuildingCallback const & buildingCallback) { m_routingCallback = buildingCallback; }
  /// See warning above.
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

public:
  /// @name Editor interface.
  //@{
  /// Initializes feature for Create Object UI.
  /// @returns false in case when coordinate is in the ocean or mwm is not downloaded.
  bool CreateMapObject(m2::PointD const & mercator, uint32_t const featureType, osm::EditableMapObject & emo) const;
  /// @returns false if feature is invalid or can't be edited.
  bool GetEditableMapObject(FeatureID const & fid, osm:: EditableMapObject & emo) const;
  osm::Editor::SaveResult SaveEditedMapObject(osm::EditableMapObject emo);
  void DeleteFeature(FeatureID const & fid) const;
  osm::NewFeatureCategories GetEditorCategories() const;
  bool RollBackChanges(FeatureID const & fid);
  //@}

private:
  void SetRouterImpl(routing::RouterType type);
  void RemoveRoute(bool deactivateFollowing);

  void InsertRoute(routing::Route const & route);
  void CheckLocationForRouting(location::GpsInfo const & info);
  void CallRouteBuilded(routing::IRouter::ResultCode code,
                        storage::TCountriesVec const & absentCountries);
  void MatchLocationToRoute(location::GpsInfo & info, location::RouteMatchingInfo & routeMatchingInfo) const;
  string GetRoutingErrorMessage(routing::IRouter::ResultCode code);

  TRouteBuildingCallback m_routingCallback;
  TRouteProgressCallback m_progressCallback;
  routing::RouterType m_currentRouterType;
  //@}

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
