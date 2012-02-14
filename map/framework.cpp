#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "benchmark_provider.hpp"

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
#include "../indexer/classificator.hpp"

#include "../base/math.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/target_os.hpp"
#include "../std/vector.hpp"


void Framework::AddMap(string const & file)
{
  LOG(LDEBUG, ("Loading map:", file));

  threads::MutexGuard lock(m_modelSyn);
  int const version = m_model.AddMap(file);
  if (m_lowestMapVersion == -1 || (version != -1 && m_lowestMapVersion > version))
    m_lowestMapVersion = version;
}

void Framework::RemoveMap(string const & datFile)
{
  threads::MutexGuard lock(m_modelSyn);
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
  m_locationState.UpdateGps(info);
  if (m_centeringMode == ECenterAndScale)
  {
    CenterAndScaleViewport();
    /// calling function twice to eliminate scaling
    /// and rounding errors when positioning from 2-3 scale into 16-17
    CenterAndScaleViewport();
    m_centeringMode = ECenterOnly;
  }
  else if (m_centeringMode == ECenterOnly)
    SetViewportCenter(m_locationState.Position());
  Invalidate();
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

static void GetResourcesMaps(vector<string> & outMaps)
{
  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.ResourcesDir(), "*" DATA_FILE_EXTENSION, outMaps);
}

Framework::Framework()
  : m_hasPendingInvalidate(false),
    m_doForceUpdate(false),
    m_queryMaxScaleMode(false),
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
    m_lowestMapVersion(-1)
{
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  char const s [] = "Nothing found. Have you tried\n"\
                    "downloading maps of the countries?\n"\
                    "Just click the downloader button \n"\
                    "at the bottom of the screen.";

  m_informationDisplay.setEmptyModelMessage(s);

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
  try
  {
    for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't add map: ", e.what()));
  }


  m_storage.Init(bind(&Framework::AddMap, this, _1),
               bind(&Framework::RemoveMap, this, _1),
               bind(&Framework::InvalidateRect, this, _1, true));
  LOG(LDEBUG, ("Storage initialized"));
}

Framework::~Framework()
{}

void Framework::AddLocalMaps()
{
  // initializes model with locally downloaded maps
  LOG(LDEBUG, ("Initializing storage"));
  // add maps to the model
  Platform::FilesList maps;
  GetLocalMaps(maps);
  try
  {
    for_each(maps.begin(), maps.end(), bind(&Framework::AddMap, this, _1));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't add map: ", e.what()));
  }
}

void Framework::RemoveLocalMaps()
{
  m_model.RemoveAllCountries();
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
  return m_renderPolicy->NeedRedraw();
}

void Framework::SetNeedRedraw(bool flag)
{
  m_renderPolicy->GetWindowHandle()->setNeedRedraw(flag);
  if (!flag)
    m_doForceUpdate = false;
}

void Framework::Invalidate(bool doForceUpdate)
{
  if (m_renderPolicy)
  {
    m_renderPolicy->SetForceUpdate(doForceUpdate);
    m_renderPolicy->GetWindowHandle()->invalidate();
  }
  else
  {
    m_hasPendingInvalidate = true;
    m_doForceUpdate = doForceUpdate;
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

  {
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

}

bool Framework::SetUpdatesEnabled(bool doEnable)
{
  threads::MutexGuard g(m_renderMutex);
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

/*double Framework::GetCurrentScale() const
{
  m2::PointD textureCenter(m_navigator.Screen().PixelRect().Center());
  m2::RectD glbRect;

  unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
  m_navigator.Screen().PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                                      textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                            glbRect);
  return scales::GetScaleLevelD(glbRect);
}*/

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
    int const scale = (m_queryMaxScaleMode ? 17 : scaleLevel);

    //threads::MutexGuard lock(m_modelSyn);
    if (isTiling)
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, scale);
    else
      m_model.ForEachFeature(selectRect, doDraw, scale);
  }
  catch (redraw_operation_cancelled const &)
  {
    if (e->drawer()->screen()->renderState())
    {
      e->drawer()->screen()->renderState()->m_isEmptyModelCurrent = false;
      e->drawer()->screen()->renderState()->m_isEmptyModelActual = false;
    }
  }

  e->setIsEmptyDrawing(doDraw.IsEmptyDrawing());

  if (m_navigator.Update(m_timer.ElapsedSeconds()))
    Invalidate();
}

bool Framework::IsEmptyModel(m2::PointD const & pt)
{
  string const fName = GetSearchEngine()->GetCountryFile(pt);
  if (fName.empty())
    return false;

  return !m_model.IsLoaded(fName);
}

void Framework::BeginPaint(shared_ptr<PaintEvent> const & e)
{
  m_renderMutex.Lock();
  if (m_renderPolicy)
    m_renderPolicy->BeginFrame(e, m_navigator.Screen());
}

void Framework::EndPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->EndFrame(e, m_navigator.Screen());
  m_renderMutex.Unlock();
}

void Framework::DrawAdditionalInfo(shared_ptr<PaintEvent> const & e)
{
  e->drawer()->screen()->beginFrame();

  /// m_informationDisplay is set and drawn after the m_renderPolicy

  m2::PointD const center = m_navigator.Screen().GlobalRect().GlobalCenter();

  m_informationDisplay.setScreen(m_navigator.Screen());

  m_informationDisplay.enableEmptyModelMessage(m_renderPolicy->IsEmptyModel());

  m_informationDisplay.setDebugInfo(0/*m_renderQueue.renderState().m_duration*/,
                                    GetDrawScale());
  m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

  m_informationDisplay.enableRuler(true/*!IsEmptyModel()*/);

  m_informationDisplay.doDraw(e->drawer());

  m_locationState.DrawMyPosition(*e->drawer(), m_navigator.Screen());

  e->drawer()->screen()->endFrame();
}

/// Function for calling from platform dependent-paint function.
void Framework::DoPaint(shared_ptr<PaintEvent> const & e)
{
  if (m_renderPolicy)
    m_renderPolicy->DrawFrame(e, m_navigator.Screen());

  DrawAdditionalInfo(e);
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

void Framework::ShowRect(m2::RectD rect)
{
  double const minSizeX = MercatorBounds::ConvertMetresToX(rect.minX(), theMetersFactor * m_metresMinWidth);
  double const minSizeY = MercatorBounds::ConvertMetresToY(rect.minY(), theMetersFactor * m_metresMinWidth);

  if (rect.SizeX() < minSizeX && rect.SizeY() < minSizeY)
    rect.SetSizes(minSizeX, minSizeY);

  m_navigator.SetFromRect(m2::AnyRectD(rect));
  Invalidate();
}

void Framework::MemoryWarning()
{
  // clearing caches on memory warning.
  m_model.ClearCaches();
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

/// @TODO refactor to accept point and min visible length
void Framework::CenterAndScaleViewport()
{
  m2::PointD const pt = m_locationState.Position();
  m_navigator.CenterViewport(pt);

  m2::RectD clipRect = m_navigator.Screen().ClipRect();

  double const xMinSize = theMetersFactor * max(m_locationState.ErrorRadius(),
                            MercatorBounds::ConvertMetresToX(pt.x, m_metresMinWidth));
  double const yMinSize = theMetersFactor * max(m_locationState.ErrorRadius(),
                            MercatorBounds::ConvertMetresToY(pt.y, m_metresMinWidth));

  bool needToScale = false;

  if (clipRect.SizeX() < clipRect.SizeY())
    needToScale = clipRect.SizeX() > xMinSize * 3;
  else
    needToScale = clipRect.SizeY() > yMinSize * 3;

  //if ((ClipRect.SizeX() < 3 * errorRadius) || (ClipRect.SizeY() < 3 * errorRadius))
  //  needToScale = true;

  if (needToScale)
  {
    double const k = max(xMinSize / clipRect.SizeX(),
                         yMinSize / clipRect.SizeY());

    clipRect.Scale(k);
    m_navigator.SetFromRect(m2::AnyRectD(clipRect));
  }

  Invalidate();
}

/// Show all model by it's world rect.
void Framework::ShowAll()
{
  SetMaxWorldRect();
  Invalidate();
}

void Framework::InvalidateRect(m2::RectD const & rect, bool doForceUpdate)
{
  if (m_navigator.Screen().GlobalRect().IsIntersect(m2::AnyRectD(rect)))
    Invalidate(doForceUpdate);
}

/// @name Drag implementation.
///@{
void Framework::StartDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.StartDrag(pt, 0./*m_timer.ElapsedSeconds()*/);

  threads::MutexGuard g(m_renderMutex);
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

  m_navigator.DoDrag(pt, 0./*m_timer.ElapsedSeconds()*/);
  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy)
    m_renderPolicy->DoDrag();
//  LOG(LINFO, ("DoDrag", e.Pos()));
}

void Framework::StopDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_navigator.StopDrag(pt, 0./*m_timer.ElapsedSeconds()*/, true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy)
    m_renderPolicy->StopDrag();

//  LOG(LINFO, ("StopDrag", e.Pos()));
}

void Framework::StartRotate(RotateEvent const & e)
{
  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StartRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->StartRotate(e.Angle(), m_timer.ElapsedSeconds());
  }
}

void Framework::DoRotate(RotateEvent const & e)
{
  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.DoRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->DoRotate(e.Angle(), m_timer.ElapsedSeconds());
  }
}

void Framework::StopRotate(RotateEvent const & e)
{
  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy && m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StopRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->StopRotate(e.Angle(), m_timer.ElapsedSeconds());
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

  m_navigator.ScaleToPoint(pt, e.ScaleFactor(), 0./*m_timer.ElapsedSeconds()*/);

  Invalidate();
}

void Framework::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

void Framework::Scale(double scale)
{
  m_navigator.Scale(scale);
  //m_tiler.seed(m_navigator.Screen(), m_tileSize);

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

  m_navigator.StartScale(pt1, pt2, 0./*m_timer.ElapsedSeconds()*/);
  threads::MutexGuard g(m_renderMutex);
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

  m_navigator.DoScale(pt1, pt2, 0./*m_timer.ElapsedSeconds()*/);
  threads::MutexGuard g(m_renderMutex);
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

  m_navigator.StopScale(pt1, pt2, 0./*m_timer.ElapsedSeconds()*/);
  threads::MutexGuard g(m_renderMutex);
  if (m_renderPolicy)
    m_renderPolicy->StopScale();
//  LOG(LINFO, ("StopScale", e.Pt1(), e.Pt2()));
}

search::Engine * Framework::GetSearchEngine()
{
  // Classical "double check" synchronization pattern.
  if (!m_pSearchEngine)
  {
    threads::MutexGuard lock(m_modelSyn);
    if (!m_pSearchEngine)
    {
      Platform & pl = GetPlatform();

      scoped_ptr<Reader> pReader(pl.GetReader(SEARCH_CATEGORIES_FILE_NAME));
      m_pSearchEngine.reset(
            new search::Engine(&m_model.GetIndex(), new CategoriesHolder(*pReader),
                               pl.GetReader(PACKED_POLYGONS_FILE),
                               pl.GetReader(COUNTRIES_FILE),
                               languages::CurrentLanguage()));
    }
  }
  return m_pSearchEngine.get();
}

void Framework::PrepareSearch(bool nearMe, double lat, double lon)
{
  GetSearchEngine()->PrepareSearch(m_navigator.Screen().ClipRect(), nearMe, lat, lon);
}

void Framework::Search(search::SearchParams const & params)
{
  GetSearchEngine()->Search(params);
}

bool Framework::GetCurrentPosition(double & lat, double & lon)
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

void Framework::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  threads::MutexGuard g(m_renderMutex);

  if (renderPolicy)
  {
    bool isVisualLogEnabled = false;
    Settings::Get("VisualLog", isVisualLogEnabled);
    m_informationDisplay.enableLog(isVisualLogEnabled, renderPolicy->GetWindowHandle().get());
    m_informationDisplay.setVisualScale(GetPlatform().VisualScale());

    m_navigator.SetMinScreenParams(static_cast<unsigned>(m_minRulerWidth * GetPlatform().VisualScale()),
                                   m_metresMinWidth);

    yg::gl::RenderContext::initParams();
  }

  m_renderPolicy.reset();
  m_renderPolicy.reset(renderPolicy);

  if (m_renderPolicy.get())
  {
    m_renderPolicy->SetRenderFn(DrawModelFn());
    m_renderPolicy->SetEmptyModelFn(bind(&Framework::IsEmptyModel, this, _1));

    m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

    if ((m_width != 0) && (m_height != 0))
      OnSize(m_width, m_height);

    if (m_hasPendingInvalidate)
    {
      m_renderPolicy->SetForceUpdate(m_doForceUpdate);
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

namespace
{
  class DoGetFeatureTypes
  {
    typedef vector<string> TypesC;

    class DistanceT
    {
      double m_dist;

    public:
      DistanceT(double d, TypesC & types) : m_dist(d)
      {
        m_types.swap(types);
      }

      bool operator<(DistanceT const & rhs) const
      {
        return (m_dist < rhs.m_dist);
      }

      TypesC m_types;
    };

    class DoParseTypes
    {
      Classificator const & m_c;

    public:
      DoParseTypes() : m_c(classif()) {}

      void operator() (uint32_t t)
      {
        m_types.push_back(m_c.GetFullObjectName(t));
      }

      TypesC m_types;
    };

    double GetCompareEpsilon(feature::EGeomType type) const
    {
      using namespace feature;
      switch (type)
      {
      case GEOM_POINT: return 0.0 * m_eps;
      case GEOM_LINE: return 1.0 * m_eps;
      case GEOM_AREA: return 2.0 * m_eps;
      default:
        ASSERT ( false, () );
        return numeric_limits<double>::max();
      }
    }

    m2::PointD m_pt;
    double m_eps;
    int m_scale;

    vector<DistanceT> m_cont;

  public:
    DoGetFeatureTypes(m2::PointD const & pt, double eps, int scale)
      : m_pt(pt), m_eps(eps), m_scale(scale)
    {
    }

    void operator() (FeatureType const & f)
    {
      double const d = f.GetDistance(m_pt, m_scale);
      ASSERT_GREATER_OR_EQUAL(d, 0.0, ());

      if (d <= m_eps)
      {
        DoParseTypes doParse;
        f.ForEachTypeRef(doParse);

        m_cont.push_back(DistanceT(d + GetCompareEpsilon(f.GetFeatureType()), doParse.m_types));
      }
    }

    void GetFeatureTypes(size_t count, TypesC & types)
    {
      sort(m_cont.begin(), m_cont.end());

      for (size_t i = 0; i < min(count, m_cont.size()); ++i)
        types.insert(types.end(), m_cont[i].m_types.begin(), m_cont[i].m_types.end());
    }
  };
}

void Framework::GetFeatureTypes(m2::PointD pt, vector<string> & types) const
{
  pt = m_navigator.ShiftPoint(pt);

  int const sm = 20;
  m2::RectD pixR(m2::PointD(pt.x - sm, pt.y - sm), m2::PointD(pt.x + sm, pt.y + sm));

  m2::RectD glbR;
  m_navigator.Screen().PtoG(pixR, glbR);

  int const scale = GetDrawScale();
  DoGetFeatureTypes getTypes(m_navigator.Screen().PtoG(pt),
                             max(glbR.SizeX() / 2.0, glbR.SizeY() / 2.0),
                             scale);

  m_model.ForEachFeature(glbR, getTypes, scale);

  getTypes.GetFeatureTypes(5, types);
}

string Framework::GetCountryCodeByPosition(double lat, double lon) const
{
  // @TODO add valid implementation
  return "by";
}
