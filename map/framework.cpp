#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "benchmark_provider.hpp"
#include "geourl_process.hpp"

#include "../defines.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"

#include "../gui/controller.hpp"

#include "../platform/settings.hpp"
#include "../platform/preferred_languages.hpp"
#include "../platform/platform.hpp"

#include "../yg/rendercontext.hpp"
#include "../yg/render_state.hpp"

#include "../base/math.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/vector.hpp"


void Framework::AddMap(string const & file)
{
  LOG(LDEBUG, ("Loading map:", file));

  //threads::MutexGuard lock(m_modelSyn);
  int const version = m_model.AddMap(file);
  if (m_lowestMapVersion == -1 || (version != -1 && m_lowestMapVersion > version))
    m_lowestMapVersion = version;
}

void Framework::RemoveMap(string const & datFile)
{
  //threads::MutexGuard lock(m_modelSyn);
  m_model.RemoveMap(datFile);
}

void Framework::SkipLocationCentering()
{
  m_centeringMode = ESkipLocationCentering;
}

void Framework::OnLocationStatusChanged(location::TLocationStatus newStatus)
{
  switch (newStatus)
  {
  case location::EStarted:
  case location::EFirstEvent:
    if (m_centeringMode != ESkipLocationCentering)
    {
      // set centering mode for the first location
      m_centeringMode = ECenterAndScale;
    }
    break;

  default:
    m_centeringMode = EDoNothing;
    m_locationState.TurnOff();
    Invalidate();
  }
}

void Framework::OnGpsUpdate(location::GpsInfo const & info)
{
  m2::RectD rect = MercatorBounds::MetresToXY(
        info.m_longitude, info.m_latitude, info.m_horizontalAccuracy);
  m2::PointD const center = rect.Center();

  m_locationState.UpdateGps(rect);

  switch (m_centeringMode)
  {
  case ECenterAndScale:
  {
    int const rectScale = scales::GetScaleLevel(rect);
    int setScale = -1;

    // correct rect scale if country isn't downloaded
    int const upperScale = scales::GetUpperWorldScale();
    if (rectScale > upperScale && IsCountryLoaded(center))
    {
      setScale = upperScale;
    }
    else
    {
      // correct rect scale for best user experience
      int const bestScale = scales::GetUpperScale() - 1;
      if (rectScale > bestScale)
        setScale = bestScale;
    }

    if (setScale != -1)
      rect = scales::GetRectForLevel(setScale, center, 1.0);

    ShowRectFixed(rect);

    m_centeringMode = ECenterOnly;
    break;
  }

  case ECenterOnly:
    SetViewportCenter(center);
    break;

  case ESkipLocationCentering:
    m_centeringMode = EDoNothing;
    break;
  }
}

void Framework::OnCompassUpdate(location::CompassInfo const & info)
{
  m_locationState.UpdateCompass(info);
  Invalidate();
}

InformationDisplay & Framework::GetInformationDisplay()
{
  return m_informationDisplay;
}

CountryStatusDisplay * Framework::GetCountryStatusDisplay() const
{
  return m_informationDisplay.countryStatusDisplay().get();
}

static void GetResourcesMaps(vector<string> & outMaps)
{
  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.ResourcesDir(), "*" DATA_FILE_EXTENSION, outMaps);
}

Framework::Framework()
  : //m_hasPendingInvalidate(false),
    //m_doForceUpdate(false),
    m_etalonSize(GetPlatform().ScaleEtalonSize()),
    m_queryMaxScaleMode(false),
    m_drawPlacemark(false),
    //m_hasPendingShowRectFixed(false),
    m_metresMinWidth(10),
    m_metresMaxWidth(1000000),
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_WINDOWS) || defined(OMIM_OS_LINUX)
    m_minRulerWidth(97),
#else
    m_minRulerWidth(60),
#endif
    m_width(0),
    m_height(0),
    m_centeringMode(EDoNothing),
    m_informationDisplay(&m_storage),
    m_lowestMapVersion(-1)
{
  // Init strings bundle.
  m_stringsBundle.SetDefaultString("country_status_added_to_queue", "%is added to the\ndownloading queue.");
  m_stringsBundle.SetDefaultString("country_status_downloading", "Downloading%(%\\%)");
  m_stringsBundle.SetDefaultString("country_status_download", "Download%");
  m_stringsBundle.SetDefaultString("country_status_download_failed", "Downloading%\nhas failed");
  m_stringsBundle.SetDefaultString("try_again", "Try Again");

  // Init GUI controller.
  m_guiController.reset(new gui::Controller());
  m_guiController->SetStringsBundle(&m_stringsBundle);

  // Init information display.
  m_informationDisplay.setController(m_guiController.get());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  m_informationDisplay.enableCenter(true);
  m_informationDisplay.enableRuler(true);
  m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth, m_metresMaxWidth);

#ifndef OMIM_PRODUCTION
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_model.InitClassificator();

  vector<string> maps;
  GetResourcesMaps(maps);
#ifndef OMIM_OS_ANDROID
  // On Android, local maps are added and removed when external storage
  // is connected/disconnected
  GetLocalMaps(maps);
#endif

  // Remove duplicate maps if they're both present in resources and in WritableDir
  sort(maps.begin(), maps.end());
  maps.erase(unique(maps.begin(), maps.end()), maps.end());

  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));

  m_storage.Init(bind(&Framework::UpdateAfterDownload, this, _1));
  LOG(LDEBUG, ("Storage initialized"));
}

Framework::~Framework()
{
  ClearBookmarks();
}

void Framework::DeleteMap(storage::TIndex const & index)
{
  if (!m_storage.DeleteFromDownloader(index))
  {
    string const & file = m_storage.CountryByIndex(index).GetFile().m_fileName;
    if (m_model.DeleteMap(file + DATA_FILE_EXTENSION))
      InvalidateRect(GetCountryBounds(file), true);
  }

  m_storage.NotifyStatusChanged(index);
}

storage::TStatus Framework::GetCountryStatus(storage::TIndex const & index) const
{
  using namespace storage;

  storage::TStatus res = m_storage.CountryStatus(index);

  if (res == EUnknown)
  {
    Country const & c = m_storage.CountryByIndex(index);
    LocalAndRemoteSizeT const size = c.Size();

    if (size.first == 0)
      return ENotDownloaded;

    if (size.second == 0)
      return EUnknown;

    res = EOnDisk;
    if (size.first != size.second)
    {
      /// @todo Do better version check, not just size comparison.

      // Additional check for .ready file.
      // Use EOnDisk status if it's good, or EOnDiskOutOfDate otherwise.
      Platform const & pl = GetPlatform();
      string const fName = pl.WritablePathForFile(c.GetFile().GetFileWithExt() + READY_FILE_EXTENSION);

      uint64_t sz = 0;
      if (!pl.GetFileSizeByFullPath(fName, sz) || sz != size.second)
        res = EOnDiskOutOfDate;
    }
  }

  return res;
}

m2::RectD Framework::GetCountryBounds(string const & file)
{
  m2::RectD const r = GetSearchEngine()->GetCountryBounds(file);
  ASSERT ( r.IsValid(), () );
  return r;
}

void Framework::UpdateAfterDownload(string const & file)
{
  m2::RectD rect;
  if (m_model.UpdateMap(file, rect))
    InvalidateRect(rect, true);
}

void Framework::AddLocalMaps()
{
  // initializes model with locally downloaded maps
  LOG(LDEBUG, ("Initializing storage"));

  // add maps to the model
  Platform::FilesList maps;
  GetLocalMaps(maps);

  for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));
}

void Framework::RemoveLocalMaps()
{
  m_model.RemoveAllCountries();
}

void Framework::AddBookmark(string const & category, Bookmark const & bm)
{
  GetBmCategory(category)->AddBookmark(bm);
}

namespace
{
  class EqualCategoryName
  {
    string const & m_name;
  public:
    EqualCategoryName(string const & name) : m_name(name) {}
    bool operator() (BookmarkCategory const * cat) const
    {
      return (cat->GetName() == m_name);
    }
  };
}

BookmarkCategory * Framework::GetBmCategory(size_t index) const
{
  return (index < m_bookmarks.size() ? m_bookmarks[index] : 0);
}

BookmarkCategory * Framework::GetBmCategory(string const & name)
{
  vector<BookmarkCategory *>::iterator i =
      find_if(m_bookmarks.begin(), m_bookmarks.end(), EqualCategoryName(name));

  if (i != m_bookmarks.end())
    return (*i);
  else
  {
    BookmarkCategory * cat = new BookmarkCategory(name);
    m_bookmarks.push_back(cat);
    return cat;
  }
}

bool Framework::DeleteBmCategory(size_t index)
{
  if (index < m_bookmarks.size())
  {
    delete m_bookmarks[index];
    m_bookmarks.erase(m_bookmarks.begin() + index);
    return true;
  }
  else return false;
}

Bookmark const * Framework::GetBookmark(m2::PointD pt) const
{
  if (m_renderPolicy == 0)
    return 0;
  return GetBookmark(pt, m_renderPolicy->VisualScale());
}

Bookmark const * Framework::GetBookmark(m2::PointD pt, double visualScale) const
{
  // Get the global rect of touching area.
  int const sm = 20 * visualScale;
  m2::RectD rect(PtoG(m2::PointD(pt.x - sm, pt.y - sm)), PtoG(m2::PointD(pt.x + sm, pt.y + sm)));

  Bookmark const * ret = 0;
  double minD = numeric_limits<double>::max();

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
  {
    size_t const count = m_bookmarks[i]->GetBookmarksCount();
    for (size_t j = 0; j < count; ++j)
    {
      Bookmark const * bm = m_bookmarks[i]->GetBookmark(j);
      m2::PointD const pt = bm->GetOrg();

      if (rect.IsPointInside(pt))
      {
        double const d = rect.Center().SquareLength(pt);
        if (d < minD)
        {
          ret = bm;
          minD = d;
        }
      }
    }
  }

  return ret;
}

void Framework::ClearBookmarks()
{
  for_each(m_bookmarks.begin(), m_bookmarks.end(), DeleteFunctor());
  m_bookmarks.clear();
}

void Framework::GetLocalMaps(vector<string> & outMaps) const
{
  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.WritableDir(), "*" DATA_FILE_EXTENSION, outMaps);
}

void Framework::PrepareToShutdown()
{
  SetRenderPolicy(0);
}

void Framework::SetMaxWorldRect()
{
  m_navigator.SetFromRect(m2::AnyRectD(m_model.GetWorldRect()));
}

bool Framework::NeedRedraw() const
{
  // Checking this here allows to avoid many dummy "IsInitialized" flags in client code.
  return (m_renderPolicy && m_renderPolicy->NeedRedraw());
}

void Framework::SetNeedRedraw(bool flag)
{
  m_renderPolicy->GetWindowHandle()->setNeedRedraw(flag);
  //if (!flag)
  //  m_doForceUpdate = false;
}

void Framework::Invalidate(bool doForceUpdate)
{
  InvalidateRect(m2::RectD(MercatorBounds::minX, MercatorBounds::minY,
                           MercatorBounds::maxX, MercatorBounds::maxY), doForceUpdate);
}

void Framework::InvalidateRect(m2::RectD const & rect, bool doForceUpdate)
{
  if (m_renderPolicy)
  {
    ASSERT ( rect.IsValid(), () );
    m_renderPolicy->SetForceUpdate(doForceUpdate);
    m_renderPolicy->SetInvalidRect(m2::AnyRectD(rect));
    m_renderPolicy->GetWindowHandle()->invalidate();
  }
  /*
  else
  {
    m_hasPendingInvalidate = true;
    m_doForceUpdate = doForceUpdate;
    m_invalidRect = m2::AnyRectD(rect);
  }
  */
}

void Framework::SaveState()
{
  m_navigator.SaveState();
}

bool Framework::LoadState()
{
  return m_navigator.LoadState();
}
//@}

/// Resize event from window.
void Framework::OnSize(int w, int h)
{
  if (w < 2) w = 2;
  if (h < 2) h = 2;

  if (m_renderPolicy)
  {
    m_informationDisplay.setDisplayRect(m2::RectI(m2::PointI(0, 0), m2::PointU(w, h)));

    m2::RectI const & viewPort = m_renderPolicy->OnSize(w, h);

    m_navigator.OnSize(
          viewPort.minX(),
          viewPort.minY(),
          viewPort.SizeX(),
          viewPort.SizeY());
  }

  m_width = w;
  m_height = h;
}

bool Framework::SetUpdatesEnabled(bool doEnable)
{
  if (m_renderPolicy)
    return m_renderPolicy->GetWindowHandle()->setUpdatesEnabled(doEnable);
  else
    return false;
}

int Framework::GetDrawScale() const
{
  if (m_renderPolicy)
    return m_renderPolicy->GetDrawScale(m_navigator.Screen());
  else
    return 0;
}

/*
double Framework::GetCurrentScale() const
{
  m2::PointD textureCenter(m_navigator.Screen().PixelRect().Center());
  m2::RectD glbRect;

  unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
  m_navigator.Screen().PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                                      textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                            glbRect);
  return scales::GetScaleLevelD(glbRect);
}
*/

RenderPolicy::TRenderFn Framework::DrawModelFn()
{
  return bind(&Framework::DrawModel, this, _1, _2, _3, _4, _5, _6);
}

/// Actual rendering function.
void Framework::DrawModel(shared_ptr<PaintEvent> const & e,
                          ScreenBase const & screen,
                          m2::RectD const & selectRect,
                          m2::RectD const & clipRect,
                          int scaleLevel,
                          bool isTiling)
{
  fwork::DrawProcessor doDraw(clipRect, screen, e, scaleLevel);

  try
  {
    int const scale = (m_queryMaxScaleMode ? scales::GetUpperScale() : scaleLevel);

    //threads::MutexGuard lock(m_modelSyn);
    if (isTiling)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, scale);
    else
      m_model.ForEachFeature(selectRect, doDraw, scale);
  }
  catch (redraw_operation_cancelled const &)
  {
    shared_ptr<yg::gl::RenderState> pState = e->drawer()->screen()->renderState();
    if (pState)
    {
      pState->m_isEmptyModelCurrent = false;
      pState->m_isEmptyModelActual = false;
    }
  }

  e->setIsEmptyDrawing(doDraw.IsEmptyDrawing());

  if (m_navigator.Update(ElapsedSeconds()))
    Invalidate();
}

bool Framework::IsCountryLoaded(m2::PointD const & pt)
{
  // Correct, but slow version (check country polygon).
  string const fName = GetSearchEngine()->GetCountryFile(pt);
  if (fName.empty())
    return true;

  return m_model.IsLoaded(fName);
}

void Framework::BeginPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->BeginFrame(e, m_navigator.Screen());
}

void Framework::EndPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->EndFrame(e, m_navigator.Screen());
}

void Framework::DrawAdditionalInfo(shared_ptr<PaintEvent> const & e)
{
  ASSERT ( m_renderPolicy, () );

  DrawerYG * pDrawer = e->drawer();

  pDrawer->screen()->beginFrame();

  /// m_informationDisplay is set and drawn after the m_renderPolicy

  m2::PointD const center = m_navigator.Screen().GlobalRect().GlobalCenter();

  bool isEmptyModel = m_renderPolicy->IsEmptyModel();

  if (isEmptyModel)
    m_informationDisplay.setEmptyCountryName(m_renderPolicy->GetCountryName());

  m_informationDisplay.enableCountryStatusDisplay(isEmptyModel);

  m_informationDisplay.setScreen(m_navigator.Screen());

  m_informationDisplay.setDebugInfo(0/*m_renderQueue.renderState().m_duration*/, GetDrawScale());

  m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x),
                                            MercatorBounds::YToLat(center.y)));

  m_informationDisplay.enableRuler(true);

  m_informationDisplay.doDraw(pDrawer);

  m_locationState.DrawMyPosition(*pDrawer, m_navigator);

  if (m_drawPlacemark)
    m_informationDisplay.drawPlacemark(pDrawer, "placemark", m_navigator.GtoP(m_placemark));

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
    if (m_bookmarks[i]->IsVisible())
    {
      size_t const count = m_bookmarks[i]->GetBookmarksCount();
      for (size_t j = 0; j < count; ++j)
      {
        Bookmark const * bm = m_bookmarks[i]->GetBookmark(j);
        m_informationDisplay.drawPlacemark(pDrawer, bm->GetType().c_str(), m_navigator.GtoP(bm->GetOrg()));
      }
    }

  pDrawer->screen()->endFrame();

  m_guiController->DrawFrame(pDrawer->screen().get());
}

/// Function for calling from platform dependent-paint function.
void Framework::DoPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
  {
    m_renderPolicy->DrawFrame(e, m_navigator.Screen());

    DrawAdditionalInfo(e);
  }
}

m2::PointD Framework::GetViewportCenter() const
{
  return m_navigator.Screen().GlobalRect().GlobalCenter();
}

void Framework::SetViewportCenter(m2::PointD const & pt)
{
  m_navigator.CenterViewport(pt);
  Invalidate();
}

static int const theMetersFactor = 6;

void Framework::CheckMinGlobalRect(m2::RectD & rect) const
{
  m2::RectD const minRect = MercatorBounds::RectByCenterXYAndSizeInMeters(
                                rect.Center(), theMetersFactor * m_metresMinWidth);

  if (minRect.IsRectInside(rect))
    rect = minRect;
}

void Framework::ShowRect(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  m_navigator.SetFromRect(m2::AnyRectD(rect));
  Invalidate();
}

void Framework::ShowRectFixed(m2::RectD rect)
{
  CheckMinGlobalRect(rect);

  /*
  if (!m_renderPolicy)
  {
    m_pendingFixedRect = rect;
    m_hasPendingShowRectFixed = true;
    return;
  }
  */

  //size_t const sz = m_renderPolicy->ScaleEtalonSize();

  /// @todo Get stored value instead of m_renderPolicy call because of invalid render policy here.
  m2::RectD etalonRect(0, 0, m_etalonSize, m_etalonSize);
  etalonRect.Offset(-m_etalonSize / 2, -m_etalonSize);

  m2::PointD const pxCenter = m_navigator.Screen().PixelRect().Center();
  etalonRect.Offset(pxCenter);

  m_navigator.SetFromRects(m2::AnyRectD(rect), etalonRect);
  Invalidate();
}

void Framework::DrawPlacemark(m2::PointD const & pt)
{
  m_drawPlacemark = true;
  m_placemark = pt;
}

void Framework::DisablePlacemark()
{
  m_drawPlacemark = false;
}

void Framework::MemoryWarning()
{
  // clearing caches on memory warning.
  m_model.ClearCaches();

  GetSearchEngine()->ClearCaches();

  LOG(LINFO, ("MemoryWarning"));
}

void Framework::EnterBackground()
{
  // clearing caches on entering background.
  m_model.ClearCaches();
}

void Framework::EnterForeground()
{
}

/*
/// @TODO refactor to accept point and min visible length
void Framework::CenterAndScaleViewport()
{
  m2::PointD const pt = m_locationState.Position();
  m_navigator.CenterViewport(pt);

  m2::RectD const minRect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, m_metresMinWidth);
  double const xMinSize = theMetersFactor * max(m_locationState.ErrorRadius(), minRect.SizeX());
  double const yMinSize = theMetersFactor * max(m_locationState.ErrorRadius(), minRect.SizeY());

  bool needToScale = false;

  m2::RectD clipRect = GetCurrentViewport();
  if (clipRect.SizeX() < clipRect.SizeY())
    needToScale = clipRect.SizeX() > xMinSize * 3;
  else
    needToScale = clipRect.SizeY() > yMinSize * 3;

  if (needToScale)
  {
    double const k = max(xMinSize / clipRect.SizeX(),
                         yMinSize / clipRect.SizeY());

    clipRect.Scale(k);
    m_navigator.SetFromRect(m2::AnyRectD(clipRect));
  }

  Invalidate();
}
*/

/// Show all model by it's world rect.
void Framework::ShowAll()
{
  SetMaxWorldRect();
  Invalidate();
}

/// @name Drag implementation.
//@{
m2::PointD Framework::GetPixelCenter() const
{
  return m_navigator.Screen().PixelRect().Center();
}

void Framework::StartDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.StartDrag(pt, ElapsedSeconds());

  if (m_renderPolicy)
    m_renderPolicy->StartDrag();
}

void Framework::DoDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.DoDrag(pt, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->DoDrag();
}

void Framework::StopDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_navigator.StopDrag(pt, ElapsedSeconds(), true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  if (m_centeringMode != EDoNothing)
  {
    // reset GPS centering mode if we have dragged far from current location
    ScreenBase const & s = m_navigator.Screen();
    if (GetPixelCenter().Length(s.GtoP(m_locationState.Position())) >= s.GetMinPixelRectSize() / 2.0)
      m_centeringMode = EDoNothing;
  }

  if (m_renderPolicy)
    m_renderPolicy->StopDrag();
}

void Framework::StartRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StartRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->StartRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::DoRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.DoRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->DoRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::StopRotate(RotateEvent const & e)
{
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StopRotate(e.Angle(), ElapsedSeconds());
    m_renderPolicy->StopRotate(e.Angle(), ElapsedSeconds());
  }
}

void Framework::Move(double azDir, double factor)
{
  m_navigator.Move(azDir, factor);

  Invalidate();
}
//@}

/// @name Scaling.
//@{
void Framework::ScaleToPoint(ScaleToPointEvent const & e)
{
  m2::PointD const pt = (m_centeringMode == EDoNothing) ?
        m_navigator.ShiftPoint(e.Pt()) : GetPixelCenter();

  m_navigator.ScaleToPoint(pt, e.ScaleFactor(), ElapsedSeconds());

  Invalidate();
}

void Framework::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

void Framework::Scale(double scale)
{
  m_navigator.Scale(scale);

  Invalidate();
}

void Framework::CalcScalePoints(ScaleEvent const & e, m2::PointD & pt1, m2::PointD & pt2) const
{
  pt1 = m_navigator.ShiftPoint(e.Pt1());
  pt2 = m_navigator.ShiftPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD const ptC = (pt1 + pt2) / 2;
    m2::PointD const ptDiff = GetPixelCenter() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif
}

void Framework::StartScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.StartScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StartScale();
}

void Framework::DoScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.DoScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->DoScale();
}

void Framework::StopScale(ScaleEvent const & e)
{
  m2::PointD pt1, pt2;
  CalcScalePoints(e, pt1, pt2);

  m_navigator.StopScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StopScale();
}
//@}

search::Engine * Framework::GetSearchEngine() const
{
  // Classical "double check" synchronization pattern.
  if (!m_pSearchEngine)
  {
    //threads::MutexGuard lock(m_modelSyn);
    if (!m_pSearchEngine)
    {
      Platform & pl = GetPlatform();

      m_pSearchEngine.reset(
            new search::Engine(&m_model.GetIndex(),
                               pl.GetReader(SEARCH_CATEGORIES_FILE_NAME),
                               pl.GetReader(PACKED_POLYGONS_FILE),
                               pl.GetReader(COUNTRIES_FILE),
                               languages::CurrentLanguage()));
    }
  }
  return m_pSearchEngine.get();
}

string Framework::GetCountryName(m2::PointD const & pt) const
{
  return GetSearchEngine()->GetCountryName(pt);
}

string Framework::GetCountryName(string const & id) const
{
  return GetSearchEngine()->GetCountryName(id);
}

void Framework::PrepareSearch(bool hasPt, double lat, double lon)
{
  GetSearchEngine()->PrepareSearch(GetCurrentViewport(), hasPt, lat, lon);
}

bool Framework::Search(search::SearchParams const & params)
{
  return GetSearchEngine()->Search(params, GetCurrentViewport());
}

bool Framework::GetCurrentPosition(double & lat, double & lon) const
{
  if (m_locationState.IsValidPosition())
  {
    m2::PointD const pos = m_locationState.Position();
    lat = MercatorBounds::YToLat(pos.y);
    lon = MercatorBounds::XToLon(pos.x);
    return true;
  }
  else return false;
}

void Framework::ShowSearchResult(search::Result const & res)
{
  m2::RectD r = res.GetFeatureRect();
  if (scales::GetScaleLevel(r) > scales::GetUpperWorldScale())
  {
    m2::PointD const c = r.Center();
    if (!IsCountryLoaded(c))
      r = scales::GetRectForLevel(scales::GetUpperWorldScale(), c, 1.0);
  }

  ShowRectFixed(r);

  DrawPlacemark(res.GetFeatureCenter());
}

void Framework::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  if (renderPolicy)
  {
    m_informationDisplay.setVisualScale(renderPolicy->VisualScale());

    m_navigator.SetMinScreenParams(static_cast<unsigned>(m_minRulerWidth * renderPolicy->VisualScale()),
                                   m_metresMinWidth);

    yg::gl::RenderContext::initParams();
  }

  m_renderPolicy.reset();
  m_guiController->ResetRenderParams();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy)
  {
    m_etalonSize = m_renderPolicy->ScaleEtalonSize();

    gui::Controller::RenderParams rp(m_renderPolicy->VisualScale(),
                                     bind(&WindowHandle::invalidate,
                                          renderPolicy->GetWindowHandle().get()),
                                     m_renderPolicy->GetGlyphCache());

    m_guiController->SetRenderParams(rp);

    string (Framework::*pFn)(m2::PointD const &) const = &Framework::GetCountryName;
    m_renderPolicy->SetCountryNameFn(bind(pFn, this, _1));

    m_renderPolicy->SetRenderFn(DrawModelFn());

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

    if (m_width != 0 && m_height != 0)
      OnSize(m_width, m_height);

    // Do full invalidate instead of any "pending" stuff.
    Invalidate();

    /*
    if (m_hasPendingInvalidate)
    {
      m_renderPolicy->SetForceUpdate(m_doForceUpdate);
      m_renderPolicy->SetInvalidRect(m_invalidRect);
      m_renderPolicy->GetWindowHandle()->invalidate();
      m_hasPendingInvalidate = false;
    }

    if (m_hasPendingShowRectFixed)
    {
      ShowRectFixed(m_pendingFixedRect);
      m_hasPendingShowRectFixed = false;
    }
    */
  }
}

RenderPolicy * Framework::GetRenderPolicy() const
{
  return m_renderPolicy.get();
}

void Framework::SetupMeasurementSystem()
{
  m_informationDisplay.setupRuler();
  Invalidate();
}

// 0 - old April version which we should delete
#define MAXIMUM_VERSION_TO_DELETE 0

bool Framework::NeedToDeleteOldMaps() const
{
  return m_lowestMapVersion == MAXIMUM_VERSION_TO_DELETE;
}

void Framework::DeleteOldMaps()
{
  Platform & p = GetPlatform();
  vector<string> maps;
  p.GetFilesInDir(p.WritableDir(), "*" DATA_FILE_EXTENSION, maps);
  for (vector<string>::iterator it = maps.begin(); it != maps.end(); ++it)
  {
    feature::DataHeader header;
    LoadMapHeader(p.GetReader(*it), header);
    if (header.GetVersion() <= MAXIMUM_VERSION_TO_DELETE)
    {
      LOG(LINFO, ("Deleting old map", *it));
      RemoveMap(*it);
      FileWriter::DeleteFileX(p.WritablePathForFile(*it));
      InvalidateRect(header.GetBounds());
    }
  }
  m_lowestMapVersion = MAXIMUM_VERSION_TO_DELETE + 1;
}

string Framework::GetCountryCodeByPosition(double lat, double lon) const
{
  return GetSearchEngine()->GetCountryCode(m2::PointD(
                           MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat)));
}

gui::Controller * Framework::GetGuiController() const
{
  return m_guiController.get();
}

bool Framework::SetViewportByURL(string const & url)
{
  using namespace url_scheme;

  Info info;
  ParseURL(url, info);

  if (info.IsValid())
  {
    ShowRectFixed(info.GetViewport());
    Invalidate();
    return true;
  }

  return false;
}
