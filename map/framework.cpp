#include "../base/SRC_FIRST.hpp"

#include "framework.hpp"
#include "draw_processor.hpp"
#include "drawer_yg.hpp"
#include "feature_vec_model.hpp"
#include "benchmark_provider.hpp"
#include "languages.hpp"

#include "../platform/settings.hpp"

#include "../search/engine.hpp"
#include "../search/result.hpp"
#include "../search/categories_holder.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/drawing_rules.hpp"

#include "../base/math.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"
#include "../std/fstream.hpp"

#include "render_policy_st.hpp"

#include "tiling_render_policy_st.hpp"
#include "tiling_render_policy_mt.hpp"

using namespace feature;


template <typename TModel>
void Framework<TModel>::AddMap(string const & file)
{
  // update rect for Show All button
  feature::DataHeader header;
  header.Load(FilesContainerR(GetPlatform().GetReader(file)).GetReader(HEADER_FILE_TAG));
  m_model.AddWorldRect(header.GetBounds());

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
void Framework<TModel>::OnGpsUpdate(location::GpsInfo const & info)
{
  // notify GUI that we received gps position
  if (!(m_locationState & location::State::EGps) && m_locationObserver)
    m_locationObserver();

  m_locationState.UpdateGps(info);
  if (m_centeringMode == ECenterAndScale)
  {
    CenterAndScaleViewport();
    m_centeringMode = ECenterOnly;
  }
  else if (m_centeringMode == ECenterOnly)
    CenterViewport(m_locationState.Position());
  Invalidate();
}

template <typename TModel>
void Framework<TModel>::OnCompassUpdate(location::CompassInfo const & info)
{
  if (info.m_timestamp < location::POSITION_TIMEOUT_SECONDS)
  {
    m_locationState.UpdateCompass(info);
    Invalidate();
  }
}

template <typename TModel>
Framework<TModel>::Framework(shared_ptr<WindowHandle> windowHandle,
          size_t bottomShift)
  : m_windowHandle(windowHandle),
    m_metresMinWidth(20),
    m_minRulerWidth(97),
    m_centeringMode(EDoNothing),
    m_tileSize(GetPlatform().TileSize())
{
//  SetRenderPolicy(make_shared_ptr(new RenderPolicyST(windowHandle, bind(&this_type::DrawModel, this, _1, _2, _3, _4))));
  SetRenderPolicy(make_shared_ptr(new TilingRenderPolicyMT(windowHandle, bind(&this_type::DrawModel, this, _1, _2, _3, _4))));
  m_informationDisplay.setBottomShift(bottomShift);
#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.enableDebugPoints(true);
#endif

#ifdef DEBUG
  m_informationDisplay.enableGlobalRect(true);
#endif

  m_informationDisplay.enableCenter(true);

  m_informationDisplay.enableRuler(true);
  m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth);
  m_navigator.SetMinScreenParams(m_minRulerWidth, m_metresMinWidth);

#ifdef DEBUG
  m_informationDisplay.enableDebugInfo(true);
#endif

  m_informationDisplay.enableLog(GetPlatform().IsVisualLog(), m_windowHandle.get());

  m_informationDisplay.setVisualScale(GetPlatform().VisualScale());

  // initialize gps and compass subsystem
  GetLocationManager().SetGpsObserver(
        bind(&this_type::OnGpsUpdate, this, _1));
  GetLocationManager().SetCompassObserver(
        bind(&this_type::OnCompassUpdate, this, _1));

    // set language priorities
    languages::CodesT langCodes;
    languages::GetCurrentSettings(langCodes);
    languages::SaveSettings(langCodes);
  }

template <typename TModel>
void Framework<TModel>::EnumLocalMaps(maps_list_t & filesList)
{
  Platform & pl = GetPlatform();

  // scan for pre-installed maps in resources
  string const resPath = pl.ResourcesDir();
  Platform::FilesList resFiles;
  pl.GetFilesInDir(resPath, "*" DATA_FILE_EXTENSION, resFiles);

  // scan for probably updated maps in data dir
  string const dataPath = pl.WritableDir();
  Platform::FilesList dataFiles;
  pl.GetFilesInDir(dataPath, "*" DATA_FILE_EXTENSION, dataFiles);

  // wipe out same maps from resources, which have updated
  // downloaded versions in data path
  for (Platform::FilesList::iterator it = resFiles.begin(); it != resFiles.end();)
  {
    Platform::FilesList::iterator found = find(dataFiles.begin(), dataFiles.end(), *it);
    if (found != dataFiles.end())
      it = resFiles.erase(it);
    else
      ++it;
  }

  try
  {
    filesList.clear();
    for_each(resFiles.begin(), resFiles.end(), ReadersAdder(pl, filesList));
    for_each(dataFiles.begin(), dataFiles.end(), ReadersAdder(pl, filesList));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't add map: ", e.what()));
  }
}

template <typename TModel>
void Framework<TModel>::initializeGL(
                  shared_ptr<yg::gl::RenderContext> const & primaryContext,
                  shared_ptr<yg::ResourceManager> const & resourceManager)
{
  m_renderPolicy->Initialize(primaryContext, resourceManager);
}

template <typename TModel>
TModel & Framework<TModel>::get_model()
{
  return m_model;
}

template <typename TModel>
void Framework<TModel>::StartLocationService(LocationRetrievedCallbackT observer)
{
  m_locationObserver = observer;
  m_centeringMode = ECenterAndScale;
  // by default, we always start in accurate mode
  GetLocationManager().StartUpdate(true);
}

template <typename TModel>
void Framework<TModel>::StopLocationService()
{
  // reset callback
  m_locationObserver.clear();
  m_centeringMode = EDoNothing;
  GetLocationManager().StopUpdate();
  m_locationState.TurnOff();
  Invalidate();
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
  if (m_pSearchEngine)
    m_pSearchEngine->StopEverything();
}

template <typename TModel>
void Framework<TModel>::SetMaxWorldRect()
{
  m_navigator.SetFromRect(m_model.GetWorldRect());
}

template <typename TModel>
void Framework<TModel>::Invalidate()
{
  m_windowHandle->invalidate();
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

  m_informationDisplay.setDisplayRect(m2::RectI(m2::PointI(0, 0), m2::PointU(w, h)));

  m_navigator.OnSize(0, 0, w, h);

  m_renderPolicy->OnSize(w, h);
}

template <typename TModel>
bool Framework<TModel>::SetUpdatesEnabled(bool doEnable)
{
  return m_windowHandle->setUpdatesEnabled(doEnable);
}

/// respond to device orientation changes
template <typename TModel>
void Framework<TModel>::SetOrientation(EOrientation orientation)
{
  m_navigator.SetOrientation(orientation);
  m_locationState.SetOrientation(orientation);
  Invalidate();
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

/// Actual rendering function.
template <typename TModel>
void Framework<TModel>::DrawModel(shared_ptr<PaintEvent> e,
               ScreenBase const & screen,
               m2::RectD const & selectRect,
               int scaleLevel)
{
  fwork::DrawProcessor doDraw(selectRect, screen, e, scaleLevel, e->drawer()->screen()->glyphCache());

  try
  {
    threads::MutexGuard lock(m_modelSyn);
    m_model.ForEachFeatureWithScale(selectRect, bind<bool>(ref(doDraw), _1), scaleLevel);
  }
  catch (redraw_operation_cancelled const &)
  {
//    m_renderQueue.renderStatePtr()->m_isEmptyModelCurrent = false;
//    m_renderQueue.renderStatePtr()->m_isEmptyModelActual = false;
  }

  if (m_navigator.Update(m_timer.ElapsedSeconds()))
    Invalidate();
}

/// Function for calling from platform dependent-paint function.
template <typename TModel>
void Framework<TModel>::Paint(shared_ptr<PaintEvent> e)
{
  DrawerYG * pDrawer = e->drawer().get();
  pDrawer->SetVisualScale(GetPlatform().VisualScale());

  m_informationDisplay.setScreen(m_navigator.Screen());

//  m_informationDisplay.setDebugInfo(m_renderQueue.renderState().m_duration, my::rounds(GetCurrentScale()));

  m_informationDisplay.enableRuler(!IsEmptyModel());

  m2::PointD const center = m_navigator.Screen().ClipRect().Center();

  m_informationDisplay.setGlobalRect(m_navigator.Screen().GlobalRect());
  m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

  e->drawer()->screen()->beginFrame();

  m_renderPolicy->DrawFrame(e, m_navigator.Screen());

  m_informationDisplay.doDraw(pDrawer);

  m_locationState.DrawMyPosition(*pDrawer, m_navigator.Screen());

  e->drawer()->screen()->endFrame();
}

template <typename TModel>
void Framework<TModel>::CenterViewport(m2::PointD const & pt)
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

  m_navigator.SetFromRect(rect);
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
    m_navigator.SetFromRect(clipRect);
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
  if (m_navigator.Screen().GlobalRect().IsIntersect(rect))
    Invalidate();
}

/// @name Drag implementation.
///@{
template <typename TModel>
void Framework<TModel>::StartDrag(DragEvent const & e)
{
  m2::PointD pos = m_navigator.OrientPoint(e.Pos());
  m_navigator.StartDrag(pos, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pos);
#endif

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::DoDrag(DragEvent const & e)
{
  m_centeringMode = EDoNothing;

  m2::PointD pos = m_navigator.OrientPoint(e.Pos());
  m_navigator.DoDrag(pos, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pos);
#endif

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::StopDrag(DragEvent const & e)
{
  m2::PointD pos = m_navigator.OrientPoint(e.Pos());

  m_navigator.StopDrag(pos, m_timer.ElapsedSeconds(), true);

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::Move(double azDir, double factor)
{
  m_navigator.Move(azDir, factor);
  //m_tiler.seed(m_navigator.Screen(), m_tileSize);
  Invalidate();
}
//@}

/// @name Scaling.
//@{
template <typename TModel>
void Framework<TModel>::ScaleToPoint(ScaleToPointEvent const & e)
{
  m2::PointD const pt = (m_centeringMode == EDoNothing)
      ? m_navigator.OrientPoint(e.Pt())
      : m_navigator.Screen().PixelRect().Center();

  m_navigator.ScaleToPoint(pt, e.ScaleFactor(), m_timer.ElapsedSeconds());

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
  m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

  m_navigator.StartScale(pt1, pt2, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::DoScale(ScaleEvent const & e)
{
  m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

  m_navigator.DoScale(pt1, pt2, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, pt1);
  m_informationDisplay.setDebugPoint(1, pt2);
#endif

  Invalidate();
}

template <typename TModel>
void Framework<TModel>::StopScale(ScaleEvent const & e)
{
  m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1());
  m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2());

  if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
  {
    m2::PointD ptC = (pt1 + pt2) / 2;
    m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
    pt1 += ptDiff;
    pt2 += ptDiff;
  }

  m_navigator.StopScale(pt1, pt2, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
  m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

  Invalidate();
}

template<typename TModel>
void Framework<TModel>::Search(string const & text, SearchCallbackT callback)
{
  threads::MutexGuard lock(m_modelSyn);

  if (!m_pSearchEngine.get())
  {
    search::CategoriesHolder holder;
    string buffer;
    ReaderT(GetPlatform().GetReader(SEARCH_CATEGORIES_FILE_NAME)).ReadAsString(buffer);
    holder.LoadFromStream(buffer);
    m_pSearchEngine.reset(new search::Engine(&m_model.GetIndex(), holder));
  }

  m_pSearchEngine->Search(text, m_navigator.Screen().GlobalRect(), callback);
}

template <typename TModel>
void Framework<TModel>::SetRenderPolicy(shared_ptr<RenderPolicy> const & renderPolicy)
{
  m_renderPolicy = renderPolicy;
}

template class Framework<model::FeaturesFetcher>;
