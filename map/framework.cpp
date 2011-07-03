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

#include "../version/version.hpp"

#include "../yg/internal/opengl.hpp"
#include "../yg/info_layer.hpp"

using namespace feature;

namespace fwork
{
  namespace
  {
    template <class TSrc> void assign_point(di::DrawInfo * p, TSrc & src)
    {
      p->m_point = src.m_point;
    }
    template <class TSrc> void assign_path(di::DrawInfo * p, TSrc & src)
    {
      p->m_pathes.swap(src.m_points);
    }
    template <class TSrc> void assign_area(di::DrawInfo * p, TSrc & src)
    {
      p->m_areas.swap(src.m_points);

      ASSERT ( !p->m_areas.empty(), () );
      p->m_areas.back().SetCenter(src.GetCenter());
    }
  }

  DrawProcessor::DrawProcessor( m2::RectD const & r,
                                ScreenBase const & convertor,
                                shared_ptr<PaintEvent> const & paintEvent,
                                int scaleLevel,
                                yg::GlyphCache * glyphCache)
    : m_rect(r),
      m_convertor(convertor),
      m_paintEvent(paintEvent),
      m_zoom(scaleLevel),
      m_glyphCache(glyphCache)
#ifdef PROFILER_DRAWING
      , m_drawCount(0)
#endif
  {
    m_keys.reserve(reserve_rules_count);

    GetDrawer()->SetScale(m_zoom);
  }

  namespace
  {
    struct less_depth
    {
      bool operator() (di::DrawRule const & r1, di::DrawRule const & r2) const
      {
        return (r1.m_depth < r2.m_depth);
      }
    };

    struct less_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        if (r1.m_type == r2.m_type)
        {
          // assume that unique algo leaves the first element (with max priority), others - go away
          return (r1.m_priority > r2.m_priority);
        }
        else
          return (r1.m_type < r2.m_type);
      }
    };

    struct equal_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        // many line and area rules - is ok, other rules - one is enough
        if (r1.m_type == drule::line || r1.m_type == drule::area)
          return (r1 == r2);
        else
          return (r1.m_type == r2.m_type);
      }
    };
  }

  void DrawProcessor::PreProcessKeys()
  {
    sort(m_keys.begin(), m_keys.end(), less_key());
    m_keys.erase(unique(m_keys.begin(), m_keys.end(), equal_key()), m_keys.end());
  }

#define GET_POINTS(f, for_each_fun, fun, assign_fun)       \
  {                                                        \
    f.for_each_fun(fun, m_zoom);                           \
    if (fun.IsExist())                                     \
    {                                                      \
      isExist = true;                                      \
      assign_fun(ptr.get(), fun);                          \
    }                                                      \
  }

  bool DrawProcessor::operator()(FeatureType const & f)
  {
    if (m_paintEvent->isCancelled())
      throw redraw_operation_cancelled();

    // get drawing rules
    m_keys.clear();
    string names;       // for debug use only, in release it's empty
    int type = feature::GetDrawRule(f, m_zoom, m_keys, names);

    if (m_keys.empty())
    {
      // Index can pass here invisible features.
      // During indexing, features are placed at first visible scale bucket.
      // At higher scales it can become invisible - it depends on classificator.
      return true;
    }

    // remove duplicating identical drawing keys
    PreProcessKeys();

    // get drawing rules for the m_keys array
    size_t const count = m_keys.size();
#ifdef PROFILER_DRAWING
    m_drawCount += count;
#endif

    buffer_vector<di::DrawRule, reserve_rules_count> rules;
    rules.resize(count);

    int layer = f.GetLayer();
    bool isTransparent = false;
    if (layer == feature::LAYER_TRANSPARENT_TUNNEL)
    {
      layer = 0;
      isTransparent = true;
    }

    for (size_t i = 0; i < count; ++i)
    {
      int depth = m_keys[i].m_priority;
      if (layer != 0)
        depth = (layer * drule::layer_base_priority) + (depth % drule::layer_base_priority);

      rules[i] = di::DrawRule(drule::rules().Find(m_keys[i]), depth, isTransparent);
    }

    sort(rules.begin(), rules.end(), less_depth());

    shared_ptr<di::DrawInfo> ptr(new di::DrawInfo(
      f.GetPreferredDrawableName(languages::GetCurrentPriorities()),
      f.GetRoadNumber(),
      (m_zoom > 5) ? f.GetPopulationDrawRank() : 0.0));

    DrawerYG * pDrawer = GetDrawer();

    using namespace get_pts;

    bool isExist = false;
    switch (type)
    {
    case GEOM_POINT:
    {
      typedef get_pts::one_point functor_t;

      functor_t::params p;
      p.m_convertor = &m_convertor;
      p.m_rect = &m_rect;

      functor_t fun(p);
      GET_POINTS(f, ForEachPointRef, fun, assign_point)
      break;
    }

    case GEOM_AREA:
    {
      typedef filter_screenpts_adapter<area_tess_points> functor_t;

      functor_t::params p;
      p.m_convertor = &m_convertor;
      p.m_rect = &m_rect;

      functor_t fun(p);
      GET_POINTS(f, ForEachTriangleExRef, fun, assign_area)
      {
        // if area feature has any line-drawing-rules, than draw it like line
        for (size_t i = 0; i < m_keys.size(); ++i)
          if (m_keys[i].m_type == drule::line)
            goto draw_line;
        break;
      }
    }
    draw_line:
    case GEOM_LINE:
      {
        typedef filter_screenpts_adapter<path_points> functor_t;
        functor_t::params p;
        p.m_convertor = &m_convertor;
        p.m_rect = &m_rect;

        if (!ptr->m_name.empty())
        {
          double fontSize = 0;
          for (size_t i = 0; i < count; ++i)
          {
            if (pDrawer->filter_text_size(rules[i].m_rule))
              fontSize = max((uint8_t)fontSize, pDrawer->get_pathtext_font_size(rules[i].m_rule));
          }

          if (fontSize != 0)
          {
            double textLength = m_glyphCache->getTextLength(fontSize, ptr->m_name);
            typedef calc_length<base_global> functor_t;
            functor_t::params p1;
            p1.m_convertor = &m_convertor;
            p1.m_rect = &m_rect;
            functor_t fun(p1);

            f.ForEachPointRef(fun, m_zoom);
            if ((fun.IsExist()) && (fun.m_length > textLength))
            {
              textLength += 50;
              p.m_startLength = (fun.m_length - textLength) / 2;
              p.m_endLength = p.m_startLength + textLength;
            }
          }
        }

        functor_t fun(p);

        GET_POINTS(f, ForEachPointRef, fun, assign_path)

        break;
      }
    }

    if (isExist)
      pDrawer->Draw(ptr.get(), rules.data(), count);

    return true;
  }
}

/*template <typename TModel>
void FrameWork<TModel>::AddRedrawCommandSure()
{

}*/

/*  template <typename TModel>
  void FrameWork<TModel>::AddRedrawCommand()
  {
    yg::gl::RenderState const state = m_renderQueue.CopyState();
    if ((state.m_currentScreen != m_navigator.Screen()) && (m_isRedrawEnabled))
      AddRedrawCommandSure();
  }*/

  template <typename TModel>
  void FrameWork<TModel>::AddMap(string const & file)
  {
    // update rect for Show All button
    feature::DataHeader header;
    header.Load(FilesContainerR(GetPlatform().GetReader(file)).GetReader(HEADER_FILE_TAG));
    m_model.AddWorldRect(header.GetBounds());

    threads::MutexGuard lock(m_modelSyn);
    m_model.AddMap(file);
  }

  template <typename TModel>
  void FrameWork<TModel>::RemoveMap(string const & datFile)
  {
    threads::MutexGuard lock(m_modelSyn);
    m_model.RemoveMap(datFile);
  }

  template <typename TModel>
  void FrameWork<TModel>::OnGpsUpdate(location::GpsInfo const & info)
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

  template <typename TModel>
  void FrameWork<TModel>::OnCompassUpdate(location::CompassInfo const & info)
  {
//    if (info.m_timestamp < location::POSITION_TIMEOUT_SECONDS)
//    {
      m_locationState.UpdateCompass(info);
      UpdateNow();
//    }
  }

  template <typename TModel>
  FrameWork<TModel>::FrameWork(shared_ptr<WindowHandle> windowHandle,
            size_t bottomShift)
    : m_windowHandle(windowHandle),
      m_isBenchmarking(GetPlatform().IsBenchmarking()),
      m_isBenchmarkInitialized(false),
      m_bgColor(0xEE, 0xEE, 0xDD, 0xFF),
      m_renderQueue(GetPlatform().SkinName(),
                    GetPlatform().IsBenchmarking(),
                    GetPlatform().ScaleEtalonSize(),
                    m_bgColor),
      m_isRedrawEnabled(true),
      m_metresMinWidth(20),
      m_minRulerWidth(97),
      m_centeringMode(EDoNothing),
      m_maxDuration(0),
      m_tileSize(512)
  {
    m_startTime = my::FormatCurrentTime();

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
  FrameWork<TModel>::~FrameWork()
  {
  }

  template <typename TModel>
  void FrameWork<TModel>::BenchmarkCommandFinished()
  {
/*    double duration = m_renderQueue.renderState().m_duration;
    if (duration > m_maxDuration)
    {
      m_maxDuration = duration;
      m_maxDurationRect = m_curBenchmarkRect;
      m_informationDisplay.addBenchmarkInfo("maxDurationRect: ", m_maxDurationRect, m_maxDuration);
    }

    BenchmarkResult res;
    res.m_name = m_benchmarks[m_curBenchmark].m_name;
    res.m_rect = m_curBenchmarkRect;
    res.m_time = duration;
    m_benchmarkResults.push_back(res);

    if (m_benchmarkResults.size() > 100)
      SaveBenchmarkResults();

//    NextBenchmarkCommand();*/
  }

  template <typename TModel>
  void FrameWork<TModel>::SaveBenchmarkResults()
  {
/*    string resultsPath;
    Settings::Get("BenchmarkResults", resultsPath);
    LOG(LINFO, (resultsPath));
    ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);

    for (size_t i = 0; i < m_benchmarkResults.size(); ++i)
    {
      fout << GetPlatform().DeviceID() << " "
           << VERSION_STRING << " "
           << m_startTime << " "
           << m_benchmarkResults[i].m_name << " "
           << m_benchmarkResults[i].m_rect.minX() << " "
           << m_benchmarkResults[i].m_rect.minY() << " "
           << m_benchmarkResults[i].m_rect.maxX() << " "
           << m_benchmarkResults[i].m_rect.maxY() << " "
           << m_benchmarkResults[i].m_time << endl;
    }

    m_benchmarkResults.clear();*/
  }

  template <typename TModel>
  void FrameWork<TModel>::SendBenchmarkResults()
  {
//    ofstream fout(GetPlatform().WritablePathForFile("benchmarks/results.txt").c_str(), ios::app);
//    fout << "[COMPLETED]";
//    fout.close();
    /// send to server for adding to statistics graphics
    /// and delete results file
  }

  template <typename TModel>
  void FrameWork<TModel>::MarkBenchmarkResultsEnd()
  {
    string resultsPath;
    Settings::Get("BenchmarkResults", resultsPath);
    LOG(LINFO, (resultsPath));
    ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
    fout << "END " << m_startTime << endl;
  }

  template <typename TModel>
  void FrameWork<TModel>::MarkBenchmarkResultsStart()
  {
    string resultsPath;
    Settings::Get("BenchmarkResults", resultsPath);
    LOG(LINFO, (resultsPath));
    ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
    fout << "START " << m_startTime << endl;
  }

/*  template <typename TModel>
  void FrameWork<TModel>::NextBenchmarkCommand()
  {
    if ((m_benchmarks[m_curBenchmark].m_provider->hasRect()) || (++m_curBenchmark < m_benchmarks.size()))
    {
      m_curBenchmarkRect = m_benchmarks[m_curBenchmark].m_provider->nextRect();
      m_navigator.SetFromRect(m_curBenchmarkRect);
      m_renderQueue.AddBenchmarkCommand(bind(&this_type::PaintImpl, this, _1, _2, _3, _4), m_navigator.Screen());
    }
    else
    {
      SaveBenchmarkResults();
      MarkBenchmarkResultsEnd();
      SendBenchmarkResults();
      LOG(LINFO, ("Bechmarks took ", m_benchmarksTimer.ElapsedSeconds(), " seconds to complete"));
    }
  }
*/
  struct PathAppender
  {
    string const & m_path;
    PathAppender(string const & path) : m_path(path) {}
    void operator()(string & elem)
    {
      elem.insert(elem.begin(), m_path.begin(), m_path.end());
    }
  };

  class ReadersAdder
  {
  protected:
    typedef vector<string> maps_list_t;

  private:
    Platform & m_pl;
    maps_list_t & m_lst;

  public:
    ReadersAdder(Platform & pl, maps_list_t & lst) : m_pl(pl), m_lst(lst) {}

    void operator() (string const & name)
    {
      m_lst.push_back(name);
    }
  };

  template <typename TModel>
  void FrameWork<TModel>::EnumLocalMaps(maps_list_t & filesList)
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

  template <class ToDo>
  void ForEachBenchmarkRecord(ToDo & toDo)
  {
    Platform & pl = GetPlatform();

    string buffer;
    try
    {
      string configPath;
      Settings::Get("BenchmarkConfig", configPath);
      ReaderPtr<Reader>(pl.GetReader(configPath)).ReadAsString(buffer);
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Error reading benchmarks: ", e.what()));
      return;
    }

    istringstream stream(buffer);

    string line;
    while (stream.good())
    {
      getline(stream, line);

      vector<string> parts;
      strings::SimpleTokenizer it(line, " ");
      while (it)
      {
        parts.push_back(*it);
        ++it;
      }

      if (!parts.empty())
        toDo(parts);
    }
  }

  class FirstReaderAdder : public ReadersAdder
  {
    typedef ReadersAdder base_type;
  public:
    FirstReaderAdder(maps_list_t & lst) : base_type(GetPlatform(), lst) {}
    void operator() (vector<string> const & v)
    {
      base_type::operator() (v[0]);
    }
  };

  template <typename TModel>
  void FrameWork<TModel>::EnumBenchmarkMaps(maps_list_t & filesList)
  {
    FirstReaderAdder adder(filesList);
    ForEachBenchmarkRecord(adder);
  }

  template <class T> class DoGetBenchmarks
  {
    set<string> m_processed;
    vector<T> & m_benchmarks;
    Platform & m_pl;

  public:
    DoGetBenchmarks(vector<T> & benchmarks)
      : m_benchmarks(benchmarks), m_pl(GetPlatform())
    {
    }

    void operator() (vector<string> const & v)
    {
      T b;
      b.m_name = v[1];

      m2::RectD r;
      if (m_processed.insert(v[0]).second)
      {
        try
        {
          feature::DataHeader header;
          header.Load(FilesContainerR(m_pl.GetReader(v[0])).GetReader(HEADER_FILE_TAG));
          r = header.GetBounds();
        }
        catch (RootException const & e)
        {
          LOG(LINFO, ("Cannot add ", v[0], " file to benchmark: ", e.what()));
          return;
        }
      }

      int lastScale;
      if (v.size() > 3)
      {
        double x0, y0, x1, y1;
        strings::to_double(v[2], x0);
        strings::to_double(v[3], y0);
        strings::to_double(v[4], x1);
        strings::to_double(v[5], y1);
        r = m2::RectD(x0, y0, x1, y1);
        strings::to_int(v[6], lastScale);
      }
      else
        strings::to_int(v[2], lastScale);

      ASSERT ( r != m2::RectD::GetEmptyRect(), (r) );
      b.m_provider.reset(new BenchmarkRectProvider(scales::GetScaleLevel(r), r, lastScale));

      m_benchmarks.push_back(b);
    }
  };

  template <typename TModel>
  void FrameWork<TModel>::InitBenchmark()
  {
    DoGetBenchmarks<Benchmark> doGet(m_benchmarks);
    ForEachBenchmarkRecord(doGet);

    m_curBenchmark = 0;

    m_renderQueue.AddRenderCommandFinishedFn(bind(&this_type::BenchmarkCommandFinished, this));
    m_benchmarksTimer.Reset();

    MarkBenchmarkResultsStart();
//    NextBenchmarkCommand();

    Invalidate();
  }

  template <typename TModel>
  void FrameWork<TModel>::initializeGL(
                    shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager)
  {
    m_resourceManager = resourceManager;
    m_renderQueue.InitializeGL(primaryContext, m_resourceManager, GetPlatform().VisualScale());
  }

  template <typename TModel>
  TModel & FrameWork<TModel>::get_model()
  {
    return m_model;
  }

  template <typename TModel>
  void FrameWork<TModel>::StartLocationService(LocationRetrievedCallbackT observer)
  {
    m_locationObserver = observer;
    m_centeringMode = ECenterAndScale;
    // by default, we always start in accurate mode
    GetLocationManager().StartUpdate(true);
  }

  template <typename TModel>
  void FrameWork<TModel>::StopLocationService()
  {
    // reset callback
    m_locationObserver.clear();
    m_centeringMode = EDoNothing;
    GetLocationManager().StopUpdate();
    m_locationState.TurnOff();
    Invalidate();
  }

  template <typename TModel>
  bool FrameWork<TModel>::IsEmptyModel()
  {
    return m_model.GetWorldRect() == m2::RectD::GetEmptyRect();
  }

  // Cleanup.
  template <typename TModel>
  void FrameWork<TModel>::Clean()
  {
    m_model.Clean();
  }

  template <typename TModel>
  void FrameWork<TModel>::PrepareToShutdown()
  {
    if (m_pSearchEngine)
      m_pSearchEngine->StopEverything();
  }

  template <typename TModel>
  void FrameWork<TModel>::SetMaxWorldRect()
  {
    m_navigator.SetFromRect(m_model.GetWorldRect());
  }

  template <typename TModel>
  void FrameWork<TModel>::UpdateNow()
  {
//    AddRedrawCommand();
    Invalidate();
  }

  template <typename TModel>
  void FrameWork<TModel>::Invalidate()
  {
    m_windowHandle->invalidate();
  }

  template <typename TModel>
  void FrameWork<TModel>::SaveState()
  {
    m_navigator.SaveState();
  }

  template <typename TModel>
  bool FrameWork<TModel>::LoadState()
  {
    if (!m_navigator.LoadState())
      return false;

    return true;
  }
  //@}

  /// Resize event from window.
  template <typename TModel>
  void FrameWork<TModel>::OnSize(int w, int h)
  {
    if (w < 2) w = 2;
    if (h < 2) h = 2;

    m_informationDisplay.setDisplayRect(m2::RectI(m2::PointI(0, 0), m2::PointU(w, h)));

    m_navigator.OnSize(0, 0, w, h);

    if ((m_isBenchmarking) && (!m_isBenchmarkInitialized))
    {
      m_isBenchmarkInitialized = true;
      InitBenchmark();
    }
  }

  template <typename TModel>
  bool FrameWork<TModel>::SetUpdatesEnabled(bool doEnable)
  {
    return m_windowHandle->setUpdatesEnabled(doEnable);
  }

  /// enabling/disabling AddRedrawCommand
  template <typename TModel>
  void FrameWork<TModel>::SetRedrawEnabled(bool isRedrawEnabled)
  {
    m_isRedrawEnabled = isRedrawEnabled;
    Invalidate();
//    AddRedrawCommand();
  }

  /// respond to device orientation changes
  template <typename TModel>
  void FrameWork<TModel>::SetOrientation(EOrientation orientation)
  {
    m_navigator.SetOrientation(orientation);
    m_locationState.SetOrientation(orientation);
    UpdateNow();
  }

  template <typename TModel>
  double FrameWork<TModel>::GetCurrentScale() const
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
  /// Called, as the renderQueue processes RenderCommand
  /// Usually it happens in the separate thread.
  template <typename TModel>
  void FrameWork<TModel>::PaintImpl(shared_ptr<PaintEvent> e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 int scaleLevel)
  {
    fwork::DrawProcessor doDraw(selectRect, screen, e, scaleLevel, e->drawer()->screen()->glyphCache());

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
//      m_renderQueue.renderStatePtr()->m_isEmptyModelCurrent = false;
//      m_renderQueue.renderStatePtr()->m_isEmptyModelActual = false;
    }

    if (m_navigator.Update(m_timer.ElapsedSeconds()))
      Invalidate();
  }

  /// Function for calling from platform dependent-paint function.
  template <typename TModel>
  void FrameWork<TModel>::Paint(shared_ptr<PaintEvent> e)
  {
    // Making a copy of actualFrameInfo to compare without synchronizing.
    //typename yg::gl::RenderState state = m_renderQueue.CopyState();

    DrawerYG * pDrawer = e->drawer().get();

    m_informationDisplay.setScreen(m_navigator.Screen());

//    m_informationDisplay.setDebugInfo(m_renderQueue.renderState().m_duration, my::rounds(GetCurrentScale()));

    m_informationDisplay.enableRuler(!IsEmptyModel());

    m2::PointD const center = m_navigator.Screen().ClipRect().Center();

    m_informationDisplay.setGlobalRect(m_navigator.Screen().GlobalRect());
    m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

/*    if (m_isBenchmarking)
    {
      m2::PointD const center = m_renderQueue.renderState().m_actualScreen.ClipRect().Center();
      m_informationDisplay.setScreen(m_renderQueue.renderState().m_actualScreen);
      m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

      if (!m_isBenchmarkInitialized)
      {
        e->drawer()->screen()->beginFrame();
        e->drawer()->screen()->clear(m_bgColor);
        m_informationDisplay.setDisplayRect(m2::RectI(0, 0, 100, 100));
        m_informationDisplay.enableRuler(false);
        m_informationDisplay.doDraw(e->drawer().get());
        e->drawer()->screen()->endFrame();
      }
    }*/

    e->drawer()->screen()->beginFrame();
    e->drawer()->screen()->clear(m_bgColor);

    ScreenBase currentScreen = m_navigator.Screen();

//    m_informationDisplay.enableEmptyModelMessage(m_renderQueue.renderStatePtr()->m_isEmptyModelActual);

/*    if (m_isBenchmarking)
      currentScreen = m_renderQueue.renderState().m_actualScreen;*/

/*      pDrawer->screen()->blit(m_renderQueue.renderState().m_actualTarget,
                              m_renderQueue.renderState().m_actualScreen,
                              currentScreen);*/

    m_tiler.seed(currentScreen, m_tileSize);

    while (m_tiler.hasTile())
    {
      yg::Tiler::RectInfo ri = m_tiler.nextTile();

      m_renderQueue.TileCache().lock();

      LOG(LINFO, ("Checking TileRect:", ri.m_rect));

      if (m_renderQueue.TileCache().hasTile(ri))
      {
        yg::Tile tile = m_renderQueue.TileCache().getTile(ri);
        m_renderQueue.TileCache().unlock();
        pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true);
      }
      else
      {
        m_renderQueue.TileCache().unlock();
        m_renderQueue.AddCommand(bind(&this_type::PaintImpl, this, _1, _2, _3, _4), ri, m_tiler.seqNum());
      }
    }

    m_informationDisplay.doDraw(pDrawer);

/*    m_renderQueue.renderState().m_actualInfoLayer->draw(
          pDrawer->screen().get(),
          m_renderQueue.renderState().m_actualScreen.PtoGMatrix() * currentScreen.GtoPMatrix());*/

    m_locationState.DrawMyPosition(*pDrawer, m_navigator.Screen());

    e->drawer()->screen()->endFrame();
  }

  template <typename TModel>
  void FrameWork<TModel>::CenterViewport(m2::PointD const & pt)
  {
    m_navigator.CenterViewport(pt);
    UpdateNow();
  }

  int const theMetersFactor = 6;

  template <typename TModel>
  void FrameWork<TModel>::ShowRect(m2::RectD rect)
  {
    double const minSizeX = MercatorBounds::ConvertMetresToX(rect.minX(), theMetersFactor * m_metresMinWidth);
    double const minSizeY = MercatorBounds::ConvertMetresToY(rect.minY(), theMetersFactor * m_metresMinWidth);
    if (rect.SizeX() < minSizeX && rect.SizeY() < minSizeY)
      rect.SetSizes(minSizeX, minSizeY);

    m_navigator.SetFromRect(rect);
    UpdateNow();
  }

  template <typename TModel>
  void FrameWork<TModel>::MemoryWarning()
  {
    // clearing caches on memory warning.
    m_model.ClearCaches();
    LOG(LINFO, ("MemoryWarning"));
  }

  template <typename TModel>
  void FrameWork<TModel>::EnterBackground()
  {
    // clearing caches on entering background.
    m_model.ClearCaches();
  }

  template <typename TModel>
  void FrameWork<TModel>::EnterForeground()
  {
  }

  /// @TODO refactor to accept point and min visible length
  template <typename TModel>
  void FrameWork<TModel>::CenterAndScaleViewport()
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

    UpdateNow();
  }

  /// Show all model by it's world rect.
  template <typename TModel>
  void FrameWork<TModel>::ShowAll()
  {
    SetMaxWorldRect();
    UpdateNow();
  }

  template <typename TModel>
  void FrameWork<TModel>::Repaint()
  {
//    AddRedrawCommandSure();
    Invalidate();
  }

  template <typename TModel>
  void FrameWork<TModel>::RepaintRect(m2::RectD const & rect)
  {
    if (m_navigator.Screen().GlobalRect().IsIntersect(rect))
      Repaint();
  }

  /// @name Drag implementation.
  //@{
  template <typename TModel>
  void FrameWork<TModel>::StartDrag(DragEvent const & e)
  {
    m2::PointD pos = m_navigator.OrientPoint(e.Pos());
    m_navigator.StartDrag(pos, m_timer.ElapsedSeconds());

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, pos);
#endif

    Invalidate();
  }

  template <typename TModel>
  void FrameWork<TModel>::DoDrag(DragEvent const & e)
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
  void FrameWork<TModel>::StopDrag(DragEvent const & e)
  {
    m2::PointD pos = m_navigator.OrientPoint(e.Pos());

    m_navigator.StopDrag(pos, m_timer.ElapsedSeconds(), true);

#ifdef DRAW_TOUCH_POINTS
    m_informationDisplay.setDebugPoint(0, m2::PointD(0, 0));
#endif

    UpdateNow();
  }

  template <typename TModel>
  void FrameWork<TModel>::Move(double azDir, double factor)
  {
    m_navigator.Move(azDir, factor);
    //m_tiler.seed(m_navigator.Screen(), m_tileSize);
    UpdateNow();
  }
  //@}

  /// @name Scaling.
  //@{
  template <typename TModel>
  void FrameWork<TModel>::ScaleToPoint(ScaleToPointEvent const & e)
  {
    m2::PointD const pt = (m_centeringMode == EDoNothing)
        ? m_navigator.OrientPoint(e.Pt())
        : m_navigator.Screen().PixelRect().Center();

    m_navigator.ScaleToPoint(pt, e.ScaleFactor(), m_timer.ElapsedSeconds());

    UpdateNow();
  }

  template <typename TModel>
  void FrameWork<TModel>::ScaleDefault(bool enlarge)
  {
    Scale(enlarge ? 1.5 : 2.0/3.0);
  }

  template <typename TModel>
  void FrameWork<TModel>::Scale(double scale)
  {
    m_navigator.Scale(scale);
    //m_tiler.seed(m_navigator.Screen(), m_tileSize);
    UpdateNow();
  }

  template <typename TModel>
  void FrameWork<TModel>::StartScale(ScaleEvent const & e)
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
  void FrameWork<TModel>::DoScale(ScaleEvent const & e)
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
  void FrameWork<TModel>::StopScale(ScaleEvent const & e)
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

    UpdateNow();
  }

  template<typename TModel>
  void FrameWork<TModel>::Search(string const & text, SearchCallbackT callback)
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

template class FrameWork<model::FeaturesFetcher>;
