#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "render_queue.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"

#include "../defines.hpp"

#include "../indexer/drawing_rule_def.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/data_header.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/feature.hpp"

#include "../platform/platform.hpp"
#include "../platform/location.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/color.hpp"
#include "../yg/render_state.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/profiler.hpp"
#include "../base/mutex.hpp"

#include "../std/bind.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/target_os.hpp"
#include "../std/fstream.hpp"

//#define DRAW_TOUCH_POINTS

namespace di { class DrawInfo; }
namespace drule { class BaseRule; }

class redraw_operation_cancelled {};

namespace fwork
{
  class DrawProcessor
  {
    m2::RectD m_rect;

    ScreenBase const & m_convertor;

    shared_ptr<PaintEvent> m_paintEvent;
    vector<drule::Key> m_keys;

    int m_zoom;
    shared_ptr<yg::gl::RenderState> m_renderState;

#ifdef PROFILER_DRAWING
    size_t m_drawCount;
#endif

    inline DrawerYG * GetDrawer() const { return m_paintEvent->drawer().get(); }

    void PreProcessKeys();

    static const int reserve_rules_count = 16;

  public:

    DrawProcessor(m2::RectD const & r,
                  ScreenBase const & convertor,
                  shared_ptr<PaintEvent> const & paintEvent,
                  int scaleLevel,
                  shared_ptr<yg::gl::RenderState> const & renderState);

    bool operator() (FeatureType const & f);
  };
}


typedef boost::function<void (void)> LocationRetrievedCallbackT;

template
<
  class TModel,
  class TNavigator
>
class FrameWork
{
  typedef TModel model_t;
  typedef TNavigator navigator_t;

  typedef FrameWork<model_t, navigator_t> this_type;

  model_t m_model;
  navigator_t m_navigator;
  shared_ptr<WindowHandle> m_windowHandle;

  bool m_isBenchmarking;
  bool m_isBenchmarkInitialized;
  vector<pair<string, m2::RectD> > m_benchmarks;
  unsigned m_currentBenchmark;

  yg::Color m_bgColor;

  RenderQueue m_renderQueue;
  shared_ptr<yg::ResourceManager> m_resourceManager;
  InformationDisplay m_informationDisplay;

  /// is AddRedrawCommand enabled?
  bool m_isRedrawEnabled;
  double m_metresMinWidth;
  int m_minRulerWidth;


  enum TGpsCenteringMode
  {
    EDoNothing,
    ECenterAndScale,
    ECenterOnly
  };
  TGpsCenteringMode m_centeringMode;
  LocationRetrievedCallbackT m_locationObserver;
  location::State m_locationState;

  void AddRedrawCommandSure()
  {
    m_renderQueue.AddCommand(bind(&this_type::PaintImpl, this, _1, _2, _3, _4), m_navigator.Screen());
  }

  void AddRedrawCommand()
  {
    yg::gl::RenderState const state = m_renderQueue.CopyState();
    if ((state.m_currentScreen != m_navigator.Screen()) && (m_isRedrawEnabled))
      AddRedrawCommandSure();
  }

  threads::Mutex m_modelSyn;

  void AddMap(string const & datFile)
  {
    // update rect for Show All button
    feature::DataHeader header;
    header.Load(FilesContainerR(datFile).GetReader(HEADER_FILE_TAG));

    m2::RectD bounds = header.GetBounds();

    m_model.AddWorldRect(bounds);
    {
      threads::MutexGuard lock(m_modelSyn);
      m_model.AddMap(datFile);
    }
  }

  void RemoveMap(string const & datFile)
  {
    threads::MutexGuard lock(m_modelSyn);
    m_model.RemoveMap(datFile);
  }

  void OnGpsUpdate(location::GpsInfo const & info)
  {
    if (info.m_timestamp < location::POSITION_TIMEOUT_SECONDS)
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
      UpdateNow();
    }
  }

  void OnCompassUpdate(location::CompassInfo const & info)
  {
    if (info.m_timestamp < location::POSITION_TIMEOUT_SECONDS)
    {
      m_locationState.UpdateCompass(info);
      UpdateNow();
    }
  }

public:
  FrameWork(shared_ptr<WindowHandle> windowHandle,
            size_t bottomShift)
    : m_windowHandle(windowHandle),
      m_isBenchmarking(GetPlatform().IsBenchmarking()),
      m_isBenchmarkInitialized(false),
      m_currentBenchmark(0),
      m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
      m_renderQueue(GetPlatform().SkinName(),
                    GetPlatform().IsMultiSampled(),
                    GetPlatform().DoPeriodicalUpdate(),
                    GetPlatform().PeriodicalUpdateInterval(),
                    GetPlatform().IsBenchmarking(),
                    GetPlatform().ScaleEtalonSize(),
                    m_bgColor),
      m_isRedrawEnabled(true),
      m_metresMinWidth(20),
      m_minRulerWidth(97),
      m_centeringMode(EDoNothing)
  {
    m_informationDisplay.setBottomShift(bottomShift);
#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.enableDebugPoints(true);
#endif

#ifdef DEBUG
    m_informationDisplay.enableGlobalRect(!m_isBenchmarking);
#endif

    m_informationDisplay.enableCenter(true);

    m_informationDisplay.enableRuler(true);
    m_informationDisplay.setRulerParams(m_minRulerWidth, m_metresMinWidth);
    m_navigator.SetMinScreenParams(m_minRulerWidth, m_metresMinWidth);

#ifdef DEBUG
    m_informationDisplay.enableDebugInfo(true);
#endif

    m_informationDisplay.enableLog(GetPlatform().IsVisualLog(), m_windowHandle.get());

    m_informationDisplay.enableBenchmarkInfo(m_isBenchmarking);

    m_informationDisplay.setVisualScale(GetPlatform().VisualScale());
    m_renderQueue.AddWindowHandle(m_windowHandle);

    // initialize gps and compass subsystem
    GetLocationService().SetGpsObserver(
          boost::bind(&this_type::OnGpsUpdate, this, _1));
    GetLocationService().SetCompassObserver(
          boost::bind(&this_type::OnCompassUpdate, this, _1));
  }

  void InitBenchmark()
  {
    ifstream fin(GetPlatform().WritablePathForFile("benchmark.txt").c_str());
    while (true)
    {
      string name;
      double x0, y0, x1, y1;
      fin >> name >> x0 >> y0 >> x1 >> y1;
      if (!fin)
        break;
      m_navigator.SetFromRect(m2::RectD(x0, y0, x1, y1));
      m_renderQueue.AddBenchmarkCommand(bind(&this_type::PaintImpl, this, _1, _2, _3, _4), m_navigator.Screen());
      m_benchmarks.push_back(make_pair(name, m2::RectD(x0, y0, x1, y1)));
    }
    Invalidate();
  }

  void initializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager)
  {
    m_resourceManager = resourceManager;
    m_renderQueue.initializeGL(primaryContext, m_resourceManager, GetPlatform().VisualScale());
  }

  model_t & get_model() { return m_model; }

  /// Initialization.
  template <class TStorage>
  void InitStorage(TStorage & storage)
  {
    m_model.InitClassificator();

    LOG(LINFO, ("Initializing storage"));
    // initializes model with locally downloaded maps
    storage.Init(bind(&FrameWork::AddMap, this, _1),
                 bind(&FrameWork::RemoveMap, this, _1),
                 bind(&FrameWork::RepaintRect, this, _1));
    LOG(LINFO, ("Storage initialized"));
  }

  void StartLocationService(LocationRetrievedCallbackT observer)
  {
    m_locationObserver = observer;
    m_centeringMode = ECenterAndScale;
    // by default, we always start in accurate mode
    GetLocationService().StartUpdate(true);
  }

  void StopLocationService()
  {
    // reset callback
    m_locationObserver.clear();
    m_centeringMode = EDoNothing;
    GetLocationService().StopUpdate();
    m_locationState.TurnOff();
    Invalidate();
  }

  bool IsEmptyModel()
  {
    return m_model.GetWorldRect() == m2::RectD::GetEmptyRect();
  }

  // Cleanup.
  void Clean()
  {
    m_model.Clean();
  }

  /// Save and load framework state to ini file between sessions.
  //@{
public:

  void SetMaxWorldRect()
  {
    m_navigator.SetFromRect(m_model.GetWorldRect());
  }

  void UpdateNow()
  {
    AddRedrawCommand();
    Invalidate();
  }

  void Invalidate()
  {
    m_windowHandle->invalidate();
  }

  void SaveState()
  {
    m_navigator.SaveState();
  }

  bool LoadState()
  {
    if (!m_navigator.LoadState())
      return false;

    return true;
  }
  //@}

  /// Resize event from window.
  void OnSize(int w, int h)
  {
    if (w < 2) w = 2;
    if (h < 2) h = 2;

    m_renderQueue.OnSize(w, h);

    m2::PointU ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m_informationDisplay.setDisplayRect(m2::RectI(ptShift, ptShift + m2::PointU(w, h)));

    m_navigator.OnSize(ptShift.x, ptShift.y, w, h);

    if ((m_isBenchmarking) && (!m_isBenchmarkInitialized))
    {
      m_isBenchmarkInitialized = true;
      InitBenchmark();
    }
  }

  bool SetUpdatesEnabled(bool doEnable)
  {
    return m_windowHandle->setUpdatesEnabled(doEnable);
  }

  /// enabling/disabling AddRedrawCommand
  void SetRedrawEnabled(bool isRedrawEnabled)
  {
    m_isRedrawEnabled = isRedrawEnabled;
    AddRedrawCommand();
  }

  /// respond to device orientation changes
  void SetOrientation(EOrientation orientation)
  {
    m_navigator.SetOrientation(orientation);
    m_locationState.SetOrientation(orientation);
    UpdateNow();
  }

  int GetCurrentScale() const
  {
    m2::PointD textureCenter(m_renderQueue.renderState().m_textureWidth / 2,
                             m_renderQueue.renderState().m_textureHeight / 2);
    m2::RectD glbRect;

    unsigned scaleEtalonSize = GetPlatform().ScaleEtalonSize();
    m_navigator.Screen().PtoG(m2::RectD(textureCenter - m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2),
                                        textureCenter + m2::PointD(scaleEtalonSize / 2, scaleEtalonSize / 2)),
                              glbRect);
    return scales::GetScaleLevel(glbRect);
  }

  /// Actual rendering function.
  /// Called, as the renderQueue processes RenderCommand
  /// Usually it happens in the separate thread.
  void PaintImpl(shared_ptr<PaintEvent> e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 int scaleLevel
                 )
  {
    fwork::DrawProcessor doDraw(selectRect, screen, e, scaleLevel, m_renderQueue.renderStatePtr());
    m_renderQueue.renderStatePtr()->m_isEmptyModelCurrent = true;

    try
    {
      threads::MutexGuard lock(m_modelSyn);

#ifdef PROFILER_DRAWING
      using namespace prof;

      start<for_each_feature>();
      reset<feature_count>();
#endif

      m_model.ForEachFeatureWithScale(selectRect, bind<bool>(ref(doDraw), _1), scaleLevel);

#ifdef PROFILER_DRAWING
      end<for_each_feature>();
      LOG(LPROF, ("ForEachFeature=", metric<for_each_feature>(),
                  "FeatureCount=", metric<feature_count>(),
                  "TextureUpload= ", metric<yg_upload_data>()));
#endif
    }
    catch (redraw_operation_cancelled const &)
    {
      m_renderQueue.renderStatePtr()->m_isEmptyModelCurrent = false;
      m_renderQueue.renderStatePtr()->m_isEmptyModelActual = false;
    }

    if (m_navigator.Update(GetPlatform().TimeInSec()))
      Invalidate();
  }

  /// Function for calling from platform dependent-paint function.
  void Paint(shared_ptr<PaintEvent> e)
  {
    /// Making a copy of actualFrameInfo to compare without synchronizing.
//    typename yg::gl::RenderState state = m_renderQueue.CopyState();

    DrawerYG * pDrawer = e->drawer().get();

    m_informationDisplay.setScreen(m_navigator.Screen());

    m_informationDisplay.setDebugInfo(m_renderQueue.renderState().m_duration, GetCurrentScale());

    m_informationDisplay.enableRuler(!IsEmptyModel());

    m2::PointD const center = m_navigator.Screen().ClipRect().Center();

    m_informationDisplay.setGlobalRect(m_navigator.Screen().GlobalRect());
    m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

    {
      threads::MutexGuard guard(*m_renderQueue.renderState().m_mutex.get());

      if (m_isBenchmarking)
      {
        m2::PointD const center = m_renderQueue.renderState().m_actualScreen.ClipRect().Center();
        m_informationDisplay.setScreen(m_renderQueue.renderState().m_actualScreen);
        m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

        if (m_benchmarks.empty())
        {
          e->drawer()->screen()->beginFrame();
          e->drawer()->screen()->clear(m_bgColor);
          m_informationDisplay.setDisplayRect(m2::RectI(0, 0, 100, 100));
          m_informationDisplay.enableRuler(false);
          m_informationDisplay.doDraw(e->drawer().get());
          e->drawer()->screen()->endFrame();
        }
      }

      if (m_renderQueue.renderState().m_actualTarget.get() != 0)
      {
        if (m_isBenchmarking
         && !m_benchmarks.empty()
         && m_informationDisplay.addBenchmarkInfo(m_benchmarks[m_currentBenchmark].first,
                                                  m_benchmarks[m_currentBenchmark].second,
                                                  m_renderQueue.renderState().m_duration))
            m_currentBenchmark++;

        e->drawer()->screen()->beginFrame();
        e->drawer()->screen()->clear(m_bgColor);

        m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(false);

        OGLCHECK(glMatrixMode(GL_MODELVIEW));
        OGLCHECK(glPushMatrix());
        OGLCHECK(glTranslatef(-ptShift.x, -ptShift.y, 0));

        ScreenBase currentScreen = m_navigator.Screen();

        m_informationDisplay.enableEmptyModelMessage(m_renderQueue.renderStatePtr()->m_isEmptyModelActual);

        if (m_isBenchmarking)
          currentScreen = m_renderQueue.renderState().m_actualScreen;

        pDrawer->screen()->blit(m_renderQueue.renderState().m_actualTarget,
                                m_renderQueue.renderState().m_actualScreen,
                                currentScreen);

        m_informationDisplay.doDraw(pDrawer);

        m_locationState.DrawMyPosition(*pDrawer, m_navigator.Screen());

        e->drawer()->screen()->endFrame();

        OGLCHECK(glPopMatrix());
      }
    }
  }

  void CenterViewport(m2::PointD const & pt)
  {
    m_navigator.CenterViewport(pt);
    UpdateNow();
  }

  void ShowRect(m2::RectD const & rect)
  {
    m_navigator.SetFromRect(rect);
    UpdateNow();
  }

  void MemoryWarning()
  {
    // clearing caches on memory warning.
    m_model.ClearCaches();
    LOG(LINFO, ("MemoryWarning"));
  }

  void EnterBackground()
  {
    // clearing caches on entering background.
    m_model.ClearCaches();
  }

  void EnterForeground()
  {
  }

  /// @TODO refactor to accept point and min visible length
  void CenterAndScaleViewport()
  {
    m2::PointD const pt = m_locationState.Position();
    m_navigator.CenterViewport(pt);

    m2::RectD clipRect = m_navigator.Screen().ClipRect();

    double const xMinSize = 6 * max(m_locationState.ErrorRadius(),
                              MercatorBounds::ConvertMetresToX(pt.x, m_metresMinWidth));
    double const yMinSize = 6 * max(m_locationState.ErrorRadius(),
                              MercatorBounds::ConvertMetresToY(pt.y, m_metresMinWidth));

    bool needToScale = false;

    if (clipRect.SizeX() < clipRect.SizeY())
      needToScale = clipRect.SizeX() > xMinSize * 3;
    else
      needToScale = clipRect.SizeY() > yMinSize * 3;

/*    if ((ClipRect.SizeX() < 3 * errorRadius) || (ClipRect.SizeY() < 3 * errorRadius))
      needToScale = true;*/

    if (needToScale)
    {
      double const k = max(xMinSize / clipRect.SizeX(),
                     yMinSize / clipRect.SizeY());

      clipRect.Scale(k);
      m_navigator.SetFromRect(clipRect);
    }

    UpdateNow();
  }

  /// Show all model by it's world rect.
  void ShowAll()
  {
    SetMaxWorldRect();
    UpdateNow();
  }

  void Repaint()
  {
    m_renderQueue.SetRedrawAll();
    AddRedrawCommandSure();
    Invalidate();
  }

  void RepaintRect(m2::RectD const & rect)
  {
    threads::MutexGuard lock(*m_renderQueue.renderState().m_mutex.get());
    m2::RectD pxRect(0, 0, m_renderQueue.renderState().m_surfaceWidth, m_renderQueue.renderState().m_surfaceHeight);
    m2::RectD glbRect;
    m_navigator.Screen().PtoG(pxRect, glbRect);
    if (glbRect.Intersect(rect))
      Repaint();
  }

  /// @name Drag implementation.
  //@{
  void StartDrag(DragEvent const & e)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);
    m2::PointD pos = m_navigator.OrientPoint(e.Pos()) + ptShift;
    m_navigator.StartDrag(pos,
                          GetPlatform().TimeInSec());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, pos);
#endif

    Invalidate();
  }
  void DoDrag(DragEvent const & e)
  {
    m_centeringMode = EDoNothing;

    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m2::PointD pos = m_navigator.OrientPoint(e.Pos()) + ptShift;
    m_navigator.DoDrag(pos, GetPlatform().TimeInSec());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, pos);
#endif

    Invalidate();
  }

  void StopDrag(DragEvent const & e)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m2::PointD pos = m_navigator.OrientPoint(e.Pos()) + ptShift;

    m_navigator.StopDrag(pos,
                         GetPlatform().TimeInSec(),
                         true);

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

    UpdateNow();
  }

  void Move(double azDir, double factor)
  {
    m_navigator.Move(azDir, factor);
    UpdateNow();
  }
  //@}

  /// @name Scaling.
  //@{
  void ScaleToPoint(ScaleToPointEvent const & e)
  {
    m2::PointD const pt = (m_centeringMode == EDoNothing)
        ? m_navigator.OrientPoint(e.Pt()) + m_renderQueue.renderState().coordSystemShift(true)
        : m_navigator.Screen().PixelRect().Center();

    m_navigator.ScaleToPoint(pt, e.ScaleFactor(), GetPlatform().TimeInSec());

    UpdateNow();
  }

  void ScaleDefault(bool enlarge)
  {
    Scale(enlarge ? 1.5 : 2.0/3.0);
  }

  void Scale(double scale)
  {
    m_navigator.Scale(scale);
    UpdateNow();
  }

  void StartScale(ScaleEvent const & e)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1()) + ptShift;
    m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2()) + ptShift;

    if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
    {
      m2::PointD ptC = (pt1 + pt2) / 2;
      m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
      pt1 += ptDiff;
      pt2 += ptDiff;
    }

    m_navigator.StartScale(pt1, pt2, GetPlatform().TimeInSec());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, pt1);
    m_informationDisplay.setDebugPoint(1, pt2);
#endif

    Invalidate();
  }

  void DoScale(ScaleEvent const & e)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1()) + ptShift;
    m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2()) + ptShift;

    if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
    {
      m2::PointD ptC = (pt1 + pt2) / 2;
      m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
      pt1 += ptDiff;
      pt2 += ptDiff;
    }

    m_navigator.DoScale(pt1, pt2, GetPlatform().TimeInSec());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, pt1);
    m_informationDisplay.setDebugPoint(1, pt2);
#endif

    Invalidate();
  }

  void StopScale(ScaleEvent const & e)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(true);

    m2::PointD pt1 = m_navigator.OrientPoint(e.Pt1()) + ptShift;
    m2::PointD pt2 = m_navigator.OrientPoint(e.Pt2()) + ptShift;

    if ((m_locationState & location::State::EGps) && (m_centeringMode == ECenterOnly))
    {
      m2::PointD ptC = (pt1 + pt2) / 2;
      m2::PointD ptDiff = m_navigator.Screen().PixelRect().Center() - ptC;
      pt1 += ptDiff;
      pt2 += ptDiff;
    }

    m_navigator.StopScale(pt1, pt2, GetPlatform().TimeInSec());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
    m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

    UpdateNow();
  }
  //@}
};
