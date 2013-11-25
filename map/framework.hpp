#pragma once

#include "events.hpp"
#include "render_policy.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"
#include "animator.hpp"
#include "feature_vec_model.hpp"
#include "scales_processor.hpp"

#include "bookmark.hpp"
#include "bookmark_manager.hpp"
#include "balloon_manager.hpp"

#include "mwm_url.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"

#include "../storage/storage.hpp"

#include "../platform/location.hpp"

#include "../graphics/defines.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/color.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/strings_bundle.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/target_os.hpp"

#include "track.hpp"

//#define DRAW_TOUCH_POINTS

namespace search
{
  class Result;
  struct AddressInfo;
}

namespace gui { class Controller; }
namespace anim { class Controller; }

class CountryStatusDisplay;
class BenchmarkEngine;


/// Uncomment line to make fixed position settings and
/// build version for screenshots.
//#define FIXED_LOCATION

class Framework
{
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
  friend class BenchmarkEngine;

  StringsBundle m_stringsBundle;

  mutable scoped_ptr<search::Engine> m_pSearchEngine;
  model::FeaturesFetcher m_model;
  ScalesProcessor m_scales;
  Navigator m_navigator;
  Animator m_animator;

  typedef vector<BookmarkCategory *>::iterator CategoryIter;

  scoped_ptr<RenderPolicy> m_renderPolicy;

  double m_StartForegroundTime;

  bool m_queryMaxScaleMode;

  double const m_metresMinWidth;
  double const m_metresMaxWidth;
  int const m_minRulerWidth;

  int m_width;
  int m_height;

  location::ECompassProcessMode m_dragCompassProcessMode;
  location::ELocationProcessMode m_dragLocationProcessMode;

  void StopLocationFollow();

  storage::Storage m_storage;
  scoped_ptr<gui::Controller> m_guiController;
  scoped_ptr<anim::Controller> m_animController;
  InformationDisplay m_informationDisplay;


  /// How many pixels around touch point are used to get bookmark or POI
  static const int TOUCH_PIXEL_RADIUS = 20;

  /// This function is called by m_storage to notify that country downloading is finished.
  /// @param[in] file Country file name (without extensions).
  void UpdateAfterDownload(string const & file);

  //my::Timer m_timer;
  inline double ElapsedSeconds() const
  {
    //return m_timer.ElapsedSeconds();
    return 0.0;
  }

  /// Stores lowest loaded map version or MAX_INT if no maps added.
  /// @see feature::DataHeader::Version
  int m_lowestMapVersion;

  void DrawAdditionalInfo(shared_ptr<PaintEvent> const & e);

  BenchmarkEngine * m_benchmarkEngine;

  BookmarkManager m_bmManager;
  BalloonManager m_balloonManager;

  void ClearAllCaches();

  void RemoveMap(string const & file);

public:
  Framework();
  virtual ~Framework();

  /// @name Process storage connecting/disconnecting.
  //@{
  void AddLocalMaps();
  void RemoveLocalMaps();

  void AddMap(string const & file);
  //@}

  /// @return File names without path.
  void GetLocalMaps(vector<string> & outMaps) const;

  /// @name This functions is used by Downloader UI.
  //@{
  void DeleteCountry(storage::TIndex const & index);

  storage::TStatus GetCountryStatus(storage::TIndex const & index) const;
  string GetCountryName(storage::TIndex const & index) const;

  /// Get country rect from borders (not from mwm file).
  /// @param[in] file Pass country file name without extension as an id.
  m2::RectD GetCountryBounds(string const & file) const;
  m2::RectD GetCountryBounds(storage::TIndex const & index) const;

  void ShowCountry(storage::TIndex const & index);
  //@}

  /// Scans and loads all kml files with bookmarks in WritableDir.
  void LoadBookmarks();

  /// @return Created bookmark index in category.
  size_t AddBookmark(size_t categoryIndex, Bookmark & bm);
  void ReplaceBookmark(size_t catIndex, size_t bmIndex, Bookmark const & bm);
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

  /// @name Get bookmark by touch.
  /// @param[in]  pixPt   Coordinates of touch point in pixels.
  /// @return     NULL    If there is no bookmark found
  //@{
  BookmarkAndCategory GetBookmark(m2::PointD const & pxPoint) const;
  BookmarkAndCategory GetBookmark(m2::PointD const & pxPoint, double visualScale) const;
  //@}

  void ShowBookmark(BookmarkAndCategory bnc);
  void ShowTrack(Track const & track);

  void ClearBookmarks();

  bool AddBookmarksFile(string const & filePath);

  /// @name Additional Layer methods.
  //@{
  void AdditionalPoiLayerSetInvisible();
  void AdditionalPoiLayerSetVisible();
  void AdditionalPoiLayerAddPoi(Bookmark const & bm);
  Bookmark const * AdditionalPoiLayerGetBookmark(size_t index) const;
  Bookmark * AdditionalPoiLayerGetBookmark(size_t index);
  void AdditionalPoiLayerClear();
  bool IsAdditionalLayerPoi(const BookmarkAndCategory & bm) const;
  bool AdditionalLayerIsVisible();
  size_t AdditionalLayerNumberOfPoi();
  //@}

  inline m2::PointD PtoG(m2::PointD const & p) const { return m_navigator.PtoG(p); }
  inline m2::PointD GtoP(m2::PointD const & p) const { return m_navigator.GtoP(p); }

  storage::Storage & Storage() { return m_storage; }

  /// @name GPS location updates routine.
  //@{

  void StartLocation();
  void StopLocation();

  void OnLocationError(location::TLocationError error);
  void OnLocationUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);
  //@}

  void SetRenderPolicy(RenderPolicy * renderPolicy);
  void InitGuiSubsystem();
  RenderPolicy * GetRenderPolicy() const;

  InformationDisplay & GetInformationDisplay();
  CountryStatusDisplay * GetCountryStatusDisplay() const;

  /// Safe function to get current visual scale.
  /// Call it when you need do calculate pixel rect (not matter if m_renderPolicy == 0).
  /// @return 1.0 if m_renderPolicy == 0 (possible for Android).
  double GetVisualScale() const;

  void PrepareToShutdown();

  void SetupMeasurementSystem();

  RenderPolicy::TRenderFn DrawModelFn();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & renderRect,
                 int baseScale, bool isTilingQuery);

private:
  search::Engine * GetSearchEngine() const;
  //void AddBookmarkAndSetViewport(Bookmark & bm, m2::RectD const & viewPort);

public:
  m2::RectD GetCurrentViewport() const;

  /// Call this function before entering search GUI.
  /// While it's loading, we can cache features near user's position.
  /// @param[in] hasPt Are (lat, lon) valid
  /// @param[in] (lat, lon) Current user's position
  void PrepareSearch(bool hasPt, double lat = 0.0, double lon = 0.0);
  bool Search(search::SearchParams const & params);
  bool GetCurrentPosition(double & lat, double & lon) const;
  void ShowSearchResult(search::Result const & res);

  /// Calculate distance and direction to POI for the given position.
  /// @param[in]  point             POI's position;
  /// @param[in]  lat, lon, north   Current position and heading from north;
  /// @param[out] distance          Formatted distance string;
  /// @param[out] azimut            Azimut to point from (lat, lon);
  /// @return true  If the POI is near the current position (distance < 25 km);
  bool GetDistanceAndAzimut(m2::PointD const & point,
                            double lat, double lon, double north,
                            string & distance, double & azimut);

 //@{
  storage::TIndex GetCountryIndex(m2::PointD const & pt) const;

  string GetCountryName(m2::PointD const & pt) const;
  /// @param[in] id Country file name without an extension.
  string GetCountryName(string const & id) const;

  /// @return country code in ISO 3166-1 alpha-2 format (two small letters) or empty string
  string GetCountryCode(m2::PointD const & pt) const;
  //@}

  void SetMaxWorldRect();

  void Invalidate(bool doForceUpdate = false);
  void InvalidateRect(m2::RectD const & rect, bool doForceUpdate = false);

  void SaveState();
  bool LoadState();

  /// Resize event from window.
  virtual void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  int GetDrawScale() const;

  m2::PointD GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  /// Set correct viewport, parse API, show balloon.
  bool ShowMapForURL(string const & url);

  bool NeedRedraw() const;
  void SetNeedRedraw(bool flag);

  inline void XorQueryMaxScaleMode()
  {
    m_queryMaxScaleMode = !m_queryMaxScaleMode;
    Invalidate(true);
  }

  inline void SetQueryMaxScaleMode(bool mode)
  {
    m_queryMaxScaleMode = mode;
    Invalidate(true);
  }

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
  bool GetVisiblePOI(m2::PointD const & pxPoint, m2::PointD & pxPivot, search::AddressInfo & info) const;

  enum BookmarkOrPoi
  {
    NOTHING_FOUND = 0,
    BOOKMARK = 1,
    POI = 2,
    ADDTIONAL_LAYER = 3
  };

  BookmarkOrPoi GetBookmarkOrPoi(m2::PointD const & pxPoint, m2::PointD & pxPivot,
                                 search::AddressInfo & info, BookmarkAndCategory & bmCat);

  virtual void BeginPaint(shared_ptr<PaintEvent> const & e);
  /// Function for calling from platform dependent-paint function.
  virtual void DoPaint(shared_ptr<PaintEvent> const & e);

  virtual void EndPaint(shared_ptr<PaintEvent> const & e);

private:
  /// Always check rect in public function for minimal draw scale.
  void CheckMinGlobalRect(m2::RectD & rect) const;
  /// Check for minimal draw scale and maximal logical scale (country is not loaded).
  void CheckMinMaxVisibleScale(m2::RectD & rect, int maxScale = -1) const;

  m2::AnyRectD ToRotated(m2::RectD const & rect) const;
  void ShowRectFixed(m2::RectD const & rect);

public:
  /// Show rect for point and needed draw scale.
  void ShowRect(double lat, double lon, double zoom);

  /// Set navigator viewport by rect as-is (rect should be in screen viewport).
  void ShowRect(m2::RectD rect);

  /// - Use navigator rotate angle.
  /// - Check for fixed scales (rect scale matched on draw tile scale).
  void ShowRectEx(m2::RectD rect);
  /// - Check minimal visible scale according to downloaded countries.
  void ShowRectExVisibleScale(m2::RectD rect, int maxScale = -1);

  void MemoryWarning();
  void EnterBackground();
  void EnterForeground();

  /// Show all model by it's world rect.
  void ShowAll();

  /// @name Drag implementation.
  //@{
private:
  m2::PointD GetPixelCenter() const;
public:
  void StartDrag(DragEvent const & e);
  void DoDrag(DragEvent const & e);
  void StopDrag(DragEvent const & e);
  void Move(double azDir, double factor);
  //@}

  /// @name Rotation implementation
  //@{
  void StartRotate(RotateEvent const & e);
  void DoRotate(RotateEvent const & e);
  void StopRotate(RotateEvent const & e);
  //@}

  /// @name Scaling.
  //@{
  void ScaleToPoint(ScaleToPointEvent const & e);
  void ScaleDefault(bool enlarge);
  void Scale(double scale);

private:
  void CalcScalePoints(ScaleEvent const & e, m2::PointD & pt1, m2::PointD & pt2) const;
public:
  void StartScale(ScaleEvent const & e);
  void DoScale(ScaleEvent const & e);
  void StopScale(ScaleEvent const & e);
  //@}

  gui::Controller * GetGuiController() const;
  anim::Controller * GetAnimController() const;

  Animator & GetAnimator();
  Navigator & GetNavigator();

  /// Set the localized strings bundle
  inline void AddString(string const & name, string const & value)
  {
    m_stringsBundle.SetString(name, value);
  }

  bool IsBenchmarking() const;

  StringsBundle const & GetStringsBundle();

  BalloonManager & GetBalloonManager() { return m_balloonManager; }

  /// Checks, whether the country which contains
  /// the specified point is loaded
  bool IsCountryLoaded(m2::PointD const & pt) const;

  shared_ptr<location::State> const & GetLocationState() const;

public:
  string CodeGe0url(Bookmark const * bmk, bool addName);
  string CodeGe0url(double lat, double lon, double zoomLevel, string const & name);

  /// @name Api
  //@{
private:
  url_scheme::ParsedMapApi m_ParsedMapApi;
  void DrawMapApiPoints(shared_ptr<PaintEvent> const & e);
  void SetViewPortASync(m2::RectD const & rect);

public:
  bool GetMapApiPoint(m2::PointD const & pxPoint, url_scheme::ResultPoint & point);

  vector<url_scheme::ApiPoint> const & GetMapApiPoints() { return m_ParsedMapApi.GetPoints(); }
  void ClearMapApiPoints() { m_ParsedMapApi.Reset(); }
  int GetMapApiVersion() const { return m_ParsedMapApi.GetApiVersion(); }
  string const & GetMapApiAppTitle() const { return m_ParsedMapApi.GetAppTitle(); }
  string const & GetMapApiBackUrl() const { return m_ParsedMapApi.GetGlobalBackUrl(); }
  m2::RectD GetMapApiViewportRect() const;
  bool IsValidMapApi() const { return m_ParsedMapApi.IsValid(); }
  string GenerateApiBackUrl(url_scheme::ApiPoint const & point);
  bool GoBackOnBalloonClick() const { return m_ParsedMapApi.GoBackOnBalloonClick(); }
  //@}

  /// @name Map updates
  //@{
  bool IsDataVersionUpdated();
  void UpdateSavedDataVersion();
  //@}

  /// @name Guides
  //@{
public:
  guides::GuidesManager & GetGuidesManager() { return m_storage.GetGuideManager(); }
  bool GetGuideInfo(storage::TIndex const & index, guides::GuideInfo & info) const;
  //@}
};
