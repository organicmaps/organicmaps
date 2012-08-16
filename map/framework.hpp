#pragma once

#include "events.hpp"
#include "render_policy.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"
#include "feature_vec_model.hpp"
#include "bookmark.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"

#include "../storage/storage.hpp"

#include "../platform/location.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/color.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/strings_bundle.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/target_os.hpp"


//#define DRAW_TOUCH_POINTS

namespace search { class Result; }
namespace gui { class Controller; }
namespace anim { class Controller; }

class CountryStatusDisplay;
class BenchmarkEngine;

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

  } m_fixedPos;
#endif

protected:
  friend class BenchmarkEngine;

  StringsBundle m_stringsBundle;

  mutable scoped_ptr<search::Engine> m_pSearchEngine;
  model::FeaturesFetcher m_model;
  Navigator m_navigator;

  vector<BookmarkCategory *> m_bookmarks;

  scoped_ptr<RenderPolicy> m_renderPolicy;

  double m_StartForegroundTime;

  /// @todo Need deep analyzing in future.
  /// Now it's like a replacement of "m_hasPendingXXX" stuff.
  int m_etalonSize;

  //bool m_hasPendingInvalidate, m_doForceUpdate, m_queryMaxScaleMode, m_drawPlacemark, m_hasPendingShowRectFixed;
  bool m_queryMaxScaleMode, m_drawPlacemark;

  //m2::RectD m_pendingFixedRect;
  //m2::AnyRectD m_invalidRect;
  m2::PointD m_placemark;

  double const m_metresMinWidth;
  double const m_metresMaxWidth;
  int const m_minRulerWidth;

  int m_width;
  int m_height;

  location::State m_locationState;
  location::ECompassProcessMode m_dragCompassProcessMode;

  //mutable threads::Mutex m_modelSyn;

  storage::Storage m_storage;
  scoped_ptr<gui::Controller> m_guiController;
  scoped_ptr<anim::Controller> m_animController;
  InformationDisplay m_informationDisplay;

  /// This function is called by m_storage to notify that country downloading is finished.
  /// @param[in] file Country file name (without extensions).
  void UpdateAfterDownload(string const & file);

  //my::Timer m_timer;
  inline double ElapsedSeconds() const
  {
    //return m_timer.ElapsedSeconds();
    return 0.0;
  }

  /// Stores lowest loaded map version
  /// Holds -1 if no maps were added
  /// @see feature::DataHeader::Version
  int m_lowestMapVersion;

  void DrawAdditionalInfo(shared_ptr<PaintEvent> const & e);

  BenchmarkEngine * m_benchmarkEngine;

public:
  Framework();
  virtual ~Framework();

  /// @name Used on iPhone for upgrade from April 1.0.1 version
  //@{
  /// @return true if client should display delete old maps dialog before using downloader
  bool NeedToDeleteOldMaps() const;
  void DeleteOldMaps();
  //@}

  void AddMap(string const & file);
  void RemoveMap(string const & datFile);

  /// @name Process storage connecting/disconnecting.
  //@{
  void AddLocalMaps();
  void RemoveLocalMaps();
  //@}

  /// @return File names without path.
  void GetLocalMaps(vector<string> & outMaps) const;

  /// @name This functions is used by Downloader UI.
  //@{
  void DeleteCountry(storage::TIndex const & index);

  storage::TStatus GetCountryStatus(storage::TIndex const & index) const;

  /// Get country rect from borders (not from mwm file).
  /// @param[in] file Pass country file name without extension as an id.
  m2::RectD GetCountryBounds(string const & file) const;
  m2::RectD GetCountryBounds(storage::TIndex const & index) const;
  //@}

  void AddBookmark(string const & category, Bookmark const & bm);
  inline size_t GetBmCategoriesCount() const { return m_bookmarks.size(); }
  BookmarkCategory * GetBmCategory(size_t index) const;

  /// Find or create new category by name.
  BookmarkCategory * GetBmCategory(string const & name);
  /// Delete bookmarks category with all bookmarks
  /// @return true if category was deleted
  bool DeleteBmCategory(size_t index);

  /// Get bookmark by touch.
  /// @param[in]  pixPt   Coordinates of touch point in pixels.
  /// @return     NULL    If there is no bookmark found
  Bookmark const * GetBookmark(m2::PointD pixPt) const;
  Bookmark const * GetBookmark(m2::PointD pixPt, double visualScale) const;

  void ClearBookmarks();

  inline m2::PointD PtoG(m2::PointD const & p) const { return m_navigator.PtoG(p); }
  inline m2::PointD GtoP(m2::PointD const & p) const { return m_navigator.GtoP(p); }

  storage::Storage & Storage() { return m_storage; }

  /// @name GPS location updates routine.
  //@{
  void SkipLocationCentering();
  void OnLocationStatusChanged(location::TLocationStatus newStatus);
  void OnGpsUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);
  //@}

  void SetRenderPolicy(RenderPolicy * renderPolicy);
  RenderPolicy * GetRenderPolicy() const;

  InformationDisplay & GetInformationDisplay();
  CountryStatusDisplay * GetCountryStatusDisplay() const;

  void PrepareToShutdown();

  void SetupMeasurementSystem();

  RenderPolicy::TRenderFn DrawModelFn();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 m2::RectD const & clipRect,
                 int scaleLevel,
                 bool isTiling);

private:
  search::Engine * GetSearchEngine() const;

  void CheckMinGlobalRect(m2::AnyRectD & r) const;

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

  /// Calculate distance and direction to search result for the given position.
  /// @param[out] distance  Formatted distance string;
  void GetDistanceAndAzimut(search::Result const & res,
                            double lat, double lon, double north,
                            string & distance, double & azimut);

  string GetCountryName(m2::PointD const & pt) const;
  /// @param[in] id Country file name without an extension.
  string GetCountryName(string const & id) const;

  /// @return country code in ISO 3166-1 alpha-2 format (two small letters) or empty string
  string GetCountryCodeByPosition(double lat, double lon) const;

  void SetMaxWorldRect();

  void Invalidate(bool doForceUpdate = false);
  void InvalidateRect(m2::RectD const & rect, bool doForceUpdate = false);

  void SaveState();
  bool LoadState();

  /// Resize event from window.
  virtual void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  //double GetCurrentScale() const;
  int GetDrawScale() const;

  m2::PointD GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  bool SetViewportByURL(string const & url);

  bool NeedRedraw() const;
  void SetNeedRedraw(bool flag);

  inline void XorQueryMaxScaleMode()
  {
    m_queryMaxScaleMode = !m_queryMaxScaleMode;
    Invalidate(true);
  }

  /// Get classificator types for nearest features.
  /// @param[in] pixPt Current touch point in device pixel coordinates.
  void GetFeatureTypes(m2::PointD pixPt, vector<string> & types) const;

  struct AddressInfo
  {
    string m_country, m_city, m_street, m_house, m_name;
    vector<string> m_types;

    string FormatAddress() const;
    string FormatTypes() const;

    void Clear();
  };

  /// Get address information for point on map.
  /// @param[in] pt Point in mercator coordinates.
  void GetAddressInfo(m2::PointD const & pt, AddressInfo & info) const;

  virtual void BeginPaint(shared_ptr<PaintEvent> const & e);
  /// Function for calling from platform dependent-paint function.
  virtual void DoPaint(shared_ptr<PaintEvent> const & e);

  virtual void EndPaint(shared_ptr<PaintEvent> const & e);

  void ShowRect(m2::RectD const & rect);
  void ShowRectFixed(m2::RectD const & rect);
  void ShowRectFixed(m2::AnyRectD const & rect);

  void DrawPlacemark(m2::PointD const & pt);
  void DisablePlacemark();

  void MemoryWarning();
  void EnterBackground();
  void EnterForeground();

  /// @TODO refactor to accept point and min visible length
  //void CenterAndScaleViewport();

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
  Navigator & GetNavigator();

  /// Set the localized strings bundle
  inline void AddString(string const & name, string const & value)
  {
    m_stringsBundle.SetString(name, value);
  }

  bool IsBenchmarking() const;

  /// @{ Dealing with "Like us on a Facebook" dialog
  bool ShouldShowFacebookDialog() const;

  /// values for @param result are as follows
  /// 0 - "OK" pressed
  /// 1 - "Later" pressed
  /// 2 - "Never" pressed
  void SaveFacebookDialogResult(int result);
  /// @}

  /// Checks, whether the country which contains
  /// the specified point is loaded
  bool IsCountryLoaded(m2::PointD const & pt) const;
};
