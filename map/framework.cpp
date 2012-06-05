#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "benchmark_provider.hpp"
#include "geourl_process.hpp"
#include "../gui/controller.hpp"
#include "../defines.hpp"

#include "../platform/settings.hpp"
#include "../platform/preferred_languages.hpp"
#include "../platform/location.hpp"

#include "../yg/rendercontext.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"

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

void Framework::OnLocationStatusChanged(location::TLocationStatus newStatus)
{
  switch (newStatus)
  {
  case location::EStarted:
  case location::EFirstEvent:
    // reset centering mode
    m_centeringMode = ECenterAndScale;
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

  if (m_centeringMode == ECenterAndScale)
  {
    int const upperScale = scales::GetUpperWorldScale();
    if (scales::GetScaleLevel(rect) > upperScale && IsEmptyModel(center))
      rect = scales::GetRectForLevel(upperScale, center, 1.0);

    ShowRectFixed(rect);

    m_centeringMode = ECenterOnly;
  }
  else if (m_centeringMode == ECenterOnly)
    SetViewportCenter(center);
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
  : m_hasPendingInvalidate(false),
    m_doForceUpdate(false),
    m_queryMaxScaleMode(false),
    m_drawPlacemark(false),
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
  m_guiController.reset(new gui::Controller());

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

  m_storage.Init(bind(&Framework::AddMap, this, _1),
               bind(&Framework::RemoveMap, this, _1),
               bind(&Framework::InvalidateRect, this, _1, true));
  LOG(LDEBUG, ("Storage initialized"));
}

Framework::~Framework()
{
  ClearBookmarks();
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

void Framework::GetLocalMaps(vector<string> & outMaps)
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
  if (!flag)
    m_doForceUpdate = false;
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
    m_renderPolicy->SetForceUpdate(doForceUpdate);
    m_renderPolicy->SetInvalidRect(m2::AnyRectD(rect));
    m_renderPolicy->GetWindowHandle()->invalidate();
  }
  else
  {
    m_hasPendingInvalidate = true;
    m_doForceUpdate = doForceUpdate;
    m_invalidRect = m2::AnyRectD(rect);
  }
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

bool Framework::IsEmptyModel(m2::PointD const & pt)
{
  // Correct, but slow version (check country polygon).
  string const fName = GetSearchEngine()->GetCountryFile(pt);
  if (fName.empty())
    return false;

  return !m_model.IsLoaded(fName);
  // Fast, but not strict-correct version (just check limit rect).
  // *Upd* Doesn't work in many cases, as there are a lot of countries which has limit rect
  // that even roughly doesn't correspond to the shape of the country.
  //  return !m_model.IsCountryLoaded(pt);
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
        /// @todo Pass different symbol.
        m_informationDisplay.drawPlacemark(pDrawer, "placemark", m_navigator.GtoP(bm->GetOrg()));
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

  CHECK(m_renderPolicy, ("should have renderPolicy here"));

  size_t const sz = m_renderPolicy->ScaleEtalonSize();
  m2::RectD etalonRect(0, 0, sz, sz);
  etalonRect.Offset(-etalonRect.SizeX() / 2, -etalonRect.SizeY());

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
///@{
void Framework::StartDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.StartDrag(pt, ElapsedSeconds());

  if (m_renderPolicy)
    m_renderPolicy->StartDrag();
//  LOG(LINFO, ("StartDrag", e.Pos()));
}

void Framework::DoDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_centeringMode = EDoNothing;

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.DoDrag(pt, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->DoDrag();
//  LOG(LINFO, ("DoDrag", e.Pos()));
}

void Framework::StopDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_navigator.StopDrag(pt, ElapsedSeconds(), true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  if (m_renderPolicy)
    m_renderPolicy->StopDrag();

//  LOG(LINFO, ("StopDrag", e.Pos()));
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
  m2::PointD const pt = (m_centeringMode == EDoNothing)
      ? m_navigator.ShiftPoint(e.Pt()) : m_navigator.Screen().PixelRect().Center();

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

void Framework::StartScale(ScaleEvent const & e)
{
  m2::PointD pt1 = m_navigator.ShiftPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.ShiftPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif

  m_navigator.StartScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StartScale();

//  LOG(LINFO, ("StartScale", e.Pt1(), e.Pt2()));
}

void Framework::DoScale(ScaleEvent const & e)
{
  m2::PointD pt1 = m_navigator.ShiftPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.ShiftPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif

  m_navigator.DoScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->DoScale();
//  LOG(LINFO, ("DoScale", e.Pt1(), e.Pt2()));
}

void Framework::StopScale(ScaleEvent const & e)
{
  m2::PointD pt1 = m_navigator.ShiftPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.ShiftPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  m_navigator.StopScale(pt1, pt2, ElapsedSeconds());
  if (m_renderPolicy)
    m_renderPolicy->StopScale();
//  LOG(LINFO, ("StopScale", e.Pt1(), e.Pt2()));
}

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

void Framework::PrepareSearch(bool hasPt, double lat, double lon)
{
  GetSearchEngine()->PrepareSearch(GetCurrentViewport(), hasPt, lat, lon);
}

void Framework::Search(search::SearchParams const & params)
{
  GetSearchEngine()->Search(params, GetCurrentViewport());
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
    if (!m_model.IsCountryLoaded(c))
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

  if (m_renderPolicy.get())
  {
    gui::Controller::RenderParams rp(renderPolicy->VisualScale(),
                                     bind(&WindowHandle::invalidate, renderPolicy->GetWindowHandle().get()),
                                     renderPolicy->GetGlyphCache());

    m_guiController->SetRenderParams(rp);

    m_renderPolicy->SetCountryNameFn(bind(&Framework::GetCountryName, this, _1));
    m_renderPolicy->SetRenderFn(DrawModelFn());

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

    if ((m_width != 0) && (m_height != 0))
      OnSize(m_width, m_height);

    if (m_hasPendingInvalidate)
    {
      m_renderPolicy->SetForceUpdate(m_doForceUpdate);
      m_renderPolicy->SetInvalidRect(m_invalidRect);
      m_renderPolicy->GetWindowHandle()->invalidate();
      m_hasPendingInvalidate = false;
    }
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
