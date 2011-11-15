#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "feature_vec_model.hpp"
#include "benchmark_provider.hpp"

#include "../defines.hpp"

#include "../platform/languages.hpp"
#include "../platform/settings.hpp"

#include "../yg/rendercontext.hpp"

#include "../search/search_engine.hpp"
#include "../search/result.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../base/math.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/fstream.hpp"
#include "../std/target_os.hpp"

using namespace feature;

template <typename TModel>
void Framework<TModel>::AddMap(string const & file)
{
  LOG(LDEBUG, ("Loading map:", file));

  threads::MutexGuard lock(m_modelSyn);
  m_model.AddMap(file);
}

template <typename TModel>
void Framework<TModel>::RemoveMap(string const & datFile)
{
  threads::MutexGuard lock(m_modelSyn);
  m_model.RemoveMap(datFile);
}

template <typename TModel>
void Framework<TModel>::OnLocationStatusChanged(location::TLocationStatus newStatus)
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

template <typename TModel>
void Framework<TModel>::OnGpsUpdate(location::GpsInfo const & info)
{
  m_locationState.UpdateGps(info);
  if (m_centeringMode == ECenterAndScale)
  {
    CenterAndScaleViewport();
    m_centeringMode = ECenterOnly;
  }
  else if (m_centeringMode == ECenterOnly)
    SetViewportCenter(m_locationState.Position());
  Invalidate();
}

template <typename TModel>
void Framework<TModel>::OnCompassUpdate(location::CompassInfo const & info)
{
  m_locationState.UpdateCompass(info);
  Invalidate();
}

template <typename TModel>
Framework<TModel>::Framework()
  : m_hasPendingInvalidate(false),
    m_metresMinWidth(20),
    m_metresMaxWidth(1000000),
#if defined(OMIM_OS_MAC) || defined(OMIM_OS_WINDOWS) || defined(OMIM_OS_LINUX)
    m_minRulerWidth(97),
#else
    m_minRulerWidth(60),
#endif
    m_width(0),
    m_height(0),
    m_centeringMode(EDoNothing)
//    m_tileSize(GetPlatform().TileSize())
{
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

  m_informationDisplay.enableCenter(true);
  m_informationDisplay.enableRuler(true);
  m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth, m_metresMaxWidth);

  double const visScale = GetPlatform().VisualScale();
  m_navigator.SetMinScreenParams(
        static_cast<unsigned>(m_minRulerWidth * visScale), m_metresMinWidth);

#ifdef DEBUG
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_model.InitClassificator();

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

  m_storage.Init(bind(&Framework::AddMap, this, _1),
               bind(&Framework::RemoveMap, this, _1),
               bind(&Framework::InvalidateRect, this, _1));
  LOG(LDEBUG, ("Storage initialized"));

  // set language priorities
  languages::CodesT langCodes;
  languages::GetCurrentSettings(langCodes);
  languages::SaveSettings(langCodes);
}

template <typename TModel>
Framework<TModel>::~Framework()
{}

template <typename TModel>
void Framework<TModel>::GetLocalMaps(vector<string> & outMaps)
{
  outMaps.clear();

  Platform & pl = GetPlatform();
  pl.GetFilesInDir(pl.ResourcesDir(), "*" DATA_FILE_EXTENSION, outMaps);
  pl.GetFilesInDir(pl.WritableDir(), "*" DATA_FILE_EXTENSION, outMaps);
  outMaps.resize(unique(outMaps.begin(), outMaps.end()) - outMaps.begin());
  sort(outMaps.begin(), outMaps.end());
}

template <typename TModel>
TModel & Framework<TModel>::get_model()
{
  return m_model;
}

template <typename TModel>
bool Framework<TModel>::IsEmptyModel()
{
  return m_model.GetWorldRect() == m2::RectD::GetEmptyRect();
}

// Cleanup.
template <typename TModel>
void Framework<TModel>::Clean()
{
  m_model.Clean();
}

template <typename TModel>
void Framework<TModel>::PrepareToShutdown()
{
}

template <typename TModel>
void Framework<TModel>::SetMaxWorldRect()
{
  m_navigator.SetFromRect(m2::AnyRectD(m_model.GetWorldRect()));
}

template <typename TModel>
bool Framework<TModel>::NeedRedraw() const
{
  return m_renderPolicy->NeedRedraw();
}

template <typename TModel>
void Framework<TModel>::SetNeedRedraw(bool flag)
{
  m_renderPolicy->GetWindowHandle()->setNeedRedraw(false);
}

template <typename TModel>
void Framework<TModel>::Invalidate()
{
  if (m_renderPolicy)
    m_renderPolicy->GetWindowHandle()->invalidate();
  else
    m_hasPendingInvalidate = true;
}

template <typename TModel>
void Framework<TModel>::SaveState()
{
  m_navigator.SaveState();
}

template <typename TModel>
bool Framework<TModel>::LoadState()
{
  if (!m_navigator.LoadState())
    return false;

  return true;
}
//@}

/// Resize event from window.
template <typename TModel>
void Framework<TModel>::OnSize(int w, int h)
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

template <typename TModel>
bool Framework<TModel>::SetUpdatesEnabled(bool doEnable)
{
  return m_renderPolicy->GetWindowHandle()->setUpdatesEnabled(doEnable);
}

template <typename TModel>
double Framework<TModel>::GetCurrentScale() const
{
  m2::PointD textureCenter(m_navigator.Screen().PixelRect().Center());
  m2::RectD glbRect;

  unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
  m_navigator.Screen().PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                                      textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                            glbRect);
  return scales::GetScaleLevelD(glbRect);
}

template <typename TModel>
RenderPolicy::TRenderFn Framework<TModel>::DrawModelFn()
{
  return bind(&Framework<TModel>::DrawModel, this, _1, _2, _3, _4, _5);
}

/// Actual rendering function.
template <typename TModel>
void Framework<TModel>::DrawModel(shared_ptr<PaintEvent> const & e,
                                  ScreenBase const & screen,
                                  m2::RectD const & selectRect,
                                  m2::RectD const & clipRect,
                                  int scaleLevel)
{
  fwork::DrawProcessor doDraw(clipRect, screen, e, scaleLevel);

  try
  {
    //threads::MutexGuard lock(m_modelSyn);
    if (m_renderPolicy->IsTiling())
      m_model.ForEachFeature_TileDrawing(selectRect, doDraw, scaleLevel);
    else
      m_model.ForEachFeature(selectRect, doDraw, scaleLevel);
  }
  catch (redraw_operation_cancelled const &)
  {
    //m_renderQueue.renderStatePtr()->m_isEmptyModelCurrent = false;
    //m_renderQueue.renderStatePtr()->m_isEmptyModelActual = false;
  }

  if (m_navigator.Update(m_timer.ElapsedSeconds()))
    Invalidate();
}

template <typename TModel>
void Framework<TModel>::BeginPaint(shared_ptr<PaintEvent> const & e)
{
  m_renderPolicy->BeginFrame(e, m_navigator.Screen());
}

template <typename TModel>
void Framework<TModel>::EndPaint(shared_ptr<PaintEvent> const & e)
{
  m_renderPolicy->EndFrame(e, m_navigator.Screen());
}

/// Function for calling from platform dependent-paint function.
template <typename TModel>
void Framework<TModel>::DoPaint(shared_ptr<PaintEvent> const & e)
{
  DrawerYG * pDrawer = e->drawer();

  m_informationDisplay.setScreen(m_navigator.Screen());

  m_informationDisplay.setDebugInfo(0/*m_renderQueue.renderState().m_duration*/, my::rounds(GetCurrentScale()));

  m_informationDisplay.enableRuler(!IsEmptyModel());

  m2::PointD const center = m_navigator.Screen().GlobalRect().GlobalCenter();

  m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

  e->drawer()->screen()->beginFrame();

  m_renderPolicy->DrawFrame(e, m_navigator.Screen());

  m_informationDisplay.doDraw(pDrawer);

  m_locationState.DrawMyPosition(*pDrawer, m_navigator.Screen());

  e->drawer()->screen()->endFrame();
}

template <typename TModel>
m2::PointD Framework<TModel>::GetViewportCenter() const
{
  return m_navigator.Screen().GlobalRect().GlobalCenter();
}

template <typename TModel>
void Framework<TModel>::SetViewportCenter(m2::PointD const & pt)
{
  m_navigator.CenterViewport(pt);
  Invalidate();
 }

int const theMetersFactor = 6;

template <typename TModel>
void Framework<TModel>::ShowRect(m2::RectD rect)
{
  double const minSizeX = MercatorBounds::ConvertMetresToX(rect.minX(), theMetersFactor * m_metresMinWidth);
  double const minSizeY = MercatorBounds::ConvertMetresToY(rect.minY(), theMetersFactor * m_metresMinWidth);

  if (rect.SizeX() < minSizeX && rect.SizeY() < minSizeY)
    rect.SetSizes(minSizeX, minSizeY);

  m_navigator.SetFromRect(m2::AnyRectD(rect));
  Invalidate();
}

template <typename TModel>
void Framework<TModel>::MemoryWarning()
{
  // clearing caches on memory warning.
  m_model.ClearCaches();
  LOG(LINFO, ("MemoryWarning"));
}

template <typename TModel>
void Framework<TModel>::EnterBackground()
{
  // clearing caches on entering background.
  m_model.ClearCaches();
}

template <typename TModel>
void Framework<TModel>::EnterForeground()
{
}

/// @TODO refactor to accept point and min visible length
template <typename TModel>
void Framework<TModel>::CenterAndScaleViewport()
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
template <typename TModel>
void Framework<TModel>::ShowAll()
{
  SetMaxWorldRect();
  Invalidate();
}

template <typename TModel>
void Framework<TModel>::InvalidateRect(m2::RectD const & rect)
{
  if (m_navigator.Screen().GlobalRect().IsIntersect(m2::AnyRectD(rect)))
    Invalidate();
}

/// @name Drag implementation.
///@{
template <typename TModel>
void Framework<TModel>::StartDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.StartDrag(pt, 0./*m_timer.ElapsedSeconds()*/);
  m_renderPolicy->StartDrag();
}

template <typename TModel>
void Framework<TModel>::DoDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_centeringMode = EDoNothing;

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt);
#endif

  m_navigator.DoDrag(pt, 0./*m_timer.ElapsedSeconds()*/);
  m_renderPolicy->DoDrag();
}

template <typename TModel>
void Framework<TModel>::StopDrag(DragEvent const & e)
{
  m2::PointD const pt = m_navigator.ShiftPoint(e.Pos());

  m_navigator.StopDrag(pt, 0./*m_timer.ElapsedSeconds()*/, true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  m_renderPolicy->StopDrag();
}

template <typename TModel>
void Framework<TModel>::StartRotate(RotateEvent const & e)
{
  if (m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StartRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->StartRotate(e.Angle(), m_timer.ElapsedSeconds());
  }
}

template <typename TModel>
void Framework<TModel>::DoRotate(RotateEvent const & e)
{
  if (m_renderPolicy->DoSupportRotation())
  {
    m_navigator.DoRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->DoRotate(e.Angle(), m_timer.ElapsedSeconds());
  }
}

template <typename TModel>
void Framework<TModel>::StopRotate(RotateEvent const & e)
{
  if (m_renderPolicy->DoSupportRotation())
  {
    m_navigator.StopRotate(e.Angle(), m_timer.ElapsedSeconds());
    m_renderPolicy->StopRotate(e.Angle(), m_timer.ElapsedSeconds());
  }
}


template <typename TModel>
void Framework<TModel>::Move(double azDir, double factor)
{
  m_navigator.Move(azDir, factor);

  Invalidate();
}
//@}

/// @name Scaling.
//@{
template <typename TModel>
void Framework<TModel>::ScaleToPoint(ScaleToPointEvent const & e)
{
  m2::PointD const pt = (m_centeringMode == EDoNothing)
      ? m_navigator.ShiftPoint(e.Pt()) : m_navigator.Screen().PixelRect().Center();

  m_navigator.ScaleToPoint(pt, e.ScaleFactor(), 0./*m_timer.ElapsedSeconds()*/);

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::ScaleDefault(bool enlarge)
{
  Scale(enlarge ? 1.5 : 2.0/3.0);
}

template <typename TModel>
void Framework<TModel>::Scale(double scale)
{
  m_navigator.Scale(scale);
  //m_tiler.seed(m_navigator.Screen(), m_tileSize);

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::StartScale(ScaleEvent const & e)
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
  m_renderPolicy->StartScale();
}

template <typename TModel>
void Framework<TModel>::DoScale(ScaleEvent const & e)
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
  m_renderPolicy->DoScale();
}

template <typename TModel>
void Framework<TModel>::StopScale(ScaleEvent const & e)
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
  m_renderPolicy->StopScale();
}

template<typename TModel>
search::Engine * Framework<TModel>::GetSearchEngine()
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
                               pl.GetReader(COUNTRIES_FILE)));
    }
  }
  return m_pSearchEngine.get();
}

template<typename TModel>
void Framework<TModel>::Search(string const & text, SearchCallbackT callback)
{
  search::Engine * pSearchEngine = GetSearchEngine();
  pSearchEngine->SetViewport(m_navigator.Screen().ClipRect());
  pSearchEngine->Search(text, callback);
}

template <typename TModel>
void Framework<TModel>::SetRenderPolicy(RenderPolicy * renderPolicy)
{
  bool isVisualLogEnabled = false;
  Settings::Get("VisualLog", isVisualLogEnabled);
  m_informationDisplay.enableLog(isVisualLogEnabled, renderPolicy->GetWindowHandle().get());
  m_informationDisplay.setVisualScale(GetPlatform().VisualScale());

  yg::gl::RenderContext::initParams();
  m_renderPolicy.reset(renderPolicy);

  m_renderPolicy->SetRenderFn(DrawModelFn());

  m_navigator.SetSupportRotation(m_renderPolicy->DoSupportRotation());

  if ((m_width != 0) && (m_height != 0))
    OnSize(m_width, m_height);

  if (m_hasPendingInvalidate)
  {
    m_renderPolicy->GetWindowHandle()->invalidate();
    m_hasPendingInvalidate = false;
  }
}

template <typename TModel>
RenderPolicy * Framework<TModel>::GetRenderPolicy() const
{
  return m_renderPolicy.get();
}

template <typename TModel>
void Framework<TModel>::SetupMeasurementSystem()
{
  m_informationDisplay.setupRuler();
  Invalidate();
}

template class Framework<model::FeaturesFetcher>;
