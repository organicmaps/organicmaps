#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "render_queue.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"

#include "../defines.hpp"

#include "../search/search_processor.hpp"

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
#include "../std/function.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/target_os.hpp"
#include "../std/fstream.hpp"

//#define DRAW_TOUCH_POINTS

namespace di { class DrawInfo; }
namespace drule { class BaseRule; }

class redraw_operation_cancelled {};

struct BenchmarkRectProvider;

typedef function<void (search::Result const &)> SearchCallbackT;

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
    yg::GlyphCache * m_glyphCache;

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
                  shared_ptr<yg::gl::RenderState> const & renderState,
                  yg::GlyphCache * glyphCache);

    bool operator() (FeatureType const & f);
  };
}


typedef function<void (void)> LocationRetrievedCallbackT;

template
<
  class TModel
>
class FrameWork
{
  typedef TModel model_t;

  typedef FrameWork<model_t> this_type;

  model_t m_model;
  Navigator m_navigator;
  shared_ptr<WindowHandle> m_windowHandle;

  bool m_isBenchmarking;
  bool m_isBenchmarkInitialized;

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

  mutable threads::Mutex m_modelSyn;

  void AddRedrawCommandSure();
  void AddRedrawCommand();

  double m_maxDuration;
  m2::RectD m_maxDurationRect;
  m2::RectD m_curBenchmarkRect;

  struct BenchmarkResult
  {
    string m_name;
    m2::RectD m_rect;
    double m_time;
  };

  vector<BenchmarkResult> m_benchmarkResults;
  my::Timer m_benchmarksTimer;
  string m_startTime;

  struct Benchmark
  {
    shared_ptr<BenchmarkRectProvider> m_provider;
    string m_name;
  };

  vector<Benchmark> m_benchmarks;
  size_t m_curBenchmark;

  void BenchmarkCommandFinished();
  void NextBenchmarkCommand();
  void SaveBenchmarkResults();
  void SendBenchmarkResults();

  void AddMap(string const & datFile);
  void RemoveMap(string const & datFile);

  void OnGpsUpdate(location::GpsInfo const & info);

  void OnCompassUpdate(location::CompassInfo const & info);

public:
  FrameWork(shared_ptr<WindowHandle> windowHandle,
            size_t bottomShift);

  void InitBenchmark();

  void initializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager);

  model_t & get_model();

  void EnumLocalMaps(Platform::FilesList & filesList);
  void EnumBenchmarkMaps(Platform::FilesList & filesList);

  /// Initialization.
  template <class TStorage>
  void InitStorage(TStorage & storage)
  {
    m_model.InitClassificator();

    typename TStorage::TEnumMapsFunction enumMapsFn;
    if (m_isBenchmarking)
      enumMapsFn = bind(&FrameWork::EnumBenchmarkMaps, this, _1);
    else
      enumMapsFn = bind(&FrameWork::EnumLocalMaps, this, _1);

    LOG(LINFO, ("Initializing storage"));
    // initializes model with locally downloaded maps
    storage.Init(bind(&FrameWork::AddMap, this, _1),
                 bind(&FrameWork::RemoveMap, this, _1),
                 bind(&FrameWork::RepaintRect, this, _1),
                 enumMapsFn);
    LOG(LINFO, ("Storage initialized"));
  };

  void StartLocationService(LocationRetrievedCallbackT observer);
  void StopLocationService();

  bool IsEmptyModel();

  // Cleanup.
  void Clean();

public:

  /// @note Stop search if text is empty
  void Search(string const & text, SearchCallbackT callback) const;

  void SetMaxWorldRect();
  void UpdateNow();
  void Invalidate();
  void SaveState();
  bool LoadState();

  /// Resize event from window.
  void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  /// enabling/disabling AddRedrawCommand
  void SetRedrawEnabled(bool isRedrawEnabled);

  /// respond to device orientation changes
  void SetOrientation(EOrientation orientation);

  int GetCurrentScale() const;

  /// Actual rendering function.
  /// Called, as the renderQueue processes RenderCommand
  /// Usually it happens in the separate thread.
  void PaintImpl(shared_ptr<PaintEvent> e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 int scaleLevel
                 );

  /// Function for calling from platform dependent-paint function.
  void Paint(shared_ptr<PaintEvent> e);

  void CenterViewport(m2::PointD const & pt);

  void ShowRect(m2::RectD const & rect);

  void MemoryWarning();

  void EnterBackground();

  void EnterForeground();

  /// @TODO refactor to accept point and min visible length
  void CenterAndScaleViewport();

  /// Show all model by it's world rect.
  void ShowAll();

  void Repaint();

  void RepaintRect(m2::RectD const & rect);

  /// @name Drag implementation.
  //@{
  void StartDrag(DragEvent const & e);
  void DoDrag(DragEvent const & e);
  void StopDrag(DragEvent const & e);
  void Move(double azDir, double factor);
  //@}

  /// @name Scaling.
  //@{
  void ScaleToPoint(ScaleToPointEvent const & e);
  void ScaleDefault(bool enlarge);
  void Scale(double scale);
  void StartScale(ScaleEvent const & e);
  void DoScale(ScaleEvent const & e);
  void StopScale(ScaleEvent const & e);
  //@}
};
