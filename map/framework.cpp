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
                    GetPlatform().MaxTilesCount(),
                    GetPlatform().CpuCores(),
                    m_bgColor),
      m_isRedrawEnabled(true),
      m_metresMinWidth(20),
      m_minRulerWidth(97),
      m_centeringMode(EDoNothing),
      m_maxDuration(0),
      m_tileSize(GetPlatform().TileSize())
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

    if (GetPlatform().IsMultiThreadedRendering())
      m_renderQueue.InitializeGL(primaryContext, m_resourceManager, GetPlatform().VisualScale());
    else
    {
      /// render single tile on the same thread
      shared_ptr<yg::gl::FrameBuffer> frameBuffer(new yg::gl::FrameBuffer());

      unsigned tileWidth = m_resourceManager->tileTextureWidth();
      unsigned tileHeight = m_resourceManager->tileTextureHeight();

      shared_ptr<yg::gl::RenderBuffer> depthBuffer(new yg::gl::RenderBuffer(tileWidth, tileHeight, true));
      frameBuffer->setDepthBuffer(depthBuffer);

      DrawerYG::params_t params;

      shared_ptr<yg::InfoLayer> infoLayer(new yg::InfoLayer());

      params.m_resourceManager = m_resourceManager;
      params.m_frameBuffer = frameBuffer;
      params.m_infoLayer = infoLayer;
      params.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
      params.m_useOverlay = true;
      params.m_threadID = 0;

      m_tileDrawer = make_shared_ptr(new DrawerYG(GetPlatform().SkinName(), params));
      m_tileDrawer->onSize(tileWidth, tileHeight);

      m_tileDrawer->SetVisualScale(GetPlatform().VisualScale());

      m2::RectI renderRect(1, 1, tileWidth - 1, tileWidth - 1);
      m_tileScreen.OnSize(renderRect);
    }
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
      m_model.ForEachFeatureWithScale(selectRect, bind<bool>(ref(doDraw), _1), scaleLevel);
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

    m_infoLayer.clear();

    m_tiler.seed(currentScreen,
                 currentScreen.GlobalRect().Center(),
                 m_tileSize,
                 GetPlatform().ScaleEtalonSize());

    while (m_tiler.hasTile())
    {
      yg::Tiler::RectInfo ri = m_tiler.nextTile();

      m_renderQueue.TileCache().lock();

      if (m_renderQueue.TileCache().hasTile(ri))
      {
        m_renderQueue.TileCache().touchTile(ri);
        yg::Tile tile = m_renderQueue.TileCache().getTile(ri);
        m_renderQueue.TileCache().unlock();

        size_t tileWidth = tile.m_renderTarget->width();
        size_t tileHeight = tile.m_renderTarget->height();

        pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true,
                                yg::Color(),
                                m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                                m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));

        m_infoLayer.merge(*tile.m_infoLayer.get(), tile.m_tileScreen.PtoGMatrix() * currentScreen.GtoPMatrix());
      }
      else
      {
        if (GetPlatform().IsMultiThreadedRendering())
        {
          m_renderQueue.TileCache().unlock();
          m_renderQueue.AddCommand(bind(&this_type::PaintImpl, this, _1, _2, _3, _4), ri, m_tiler.seqNum());
        }
        else
        {
          m_renderQueue.TileCache().unlock();
          shared_ptr<PaintEvent> paintEvent(new PaintEvent(m_tileDrawer));
          shared_ptr<yg::gl::BaseTexture> tileTarget = m_resourceManager->renderTargets().Front(true);

          shared_ptr<yg::InfoLayer> tileInfoLayer(new yg::InfoLayer());

          m_tileDrawer->screen()->setRenderTarget(tileTarget);
          m_tileDrawer->screen()->setInfoLayer(tileInfoLayer);

          m_tileDrawer->beginFrame();

          m_tileDrawer->clear(yg::Color(m_bgColor.r, m_bgColor.g, m_bgColor.b, 0));
          m2::RectI renderRect(1, 1, m_resourceManager->tileTextureWidth() - 1, m_resourceManager->tileTextureHeight() - 1);
          m_tileDrawer->screen()->setClipRect(renderRect);
          m_tileDrawer->clear(m_bgColor);

          m_tileScreen.SetFromRect(ri.m_rect);

          m2::RectD selectionRect;

          double inflationSize = 24 * GetPlatform().VisualScale();

          m_tileScreen.PtoG(m2::Inflate(m2::RectD(renderRect), inflationSize, inflationSize), selectionRect);

          PaintImpl(paintEvent,
                    m_tileScreen,
                    selectionRect,
                    ri.m_drawScale);

          m_tileDrawer->endFrame();
          m_tileDrawer->screen()->resetInfoLayer();

          yg::Tile tile(tileTarget, tileInfoLayer, m_tileScreen, ri, 0);
          m_renderQueue.TileCache().lock();
          m_renderQueue.TileCache().addTile(ri, yg::TileCache::Entry(tile, m_resourceManager));
          m_renderQueue.TileCache().unlock();

          m_renderQueue.TileCache().touchTile(ri);
          tile = m_renderQueue.TileCache().getTile(ri);
          m_renderQueue.TileCache().unlock();

          size_t tileWidth = tile.m_renderTarget->width();
          size_t tileHeight = tile.m_renderTarget->height();

          pDrawer->screen()->blit(tile.m_renderTarget, tile.m_tileScreen, currentScreen, true,
                                  yg::Color(),
                                  m2::RectI(0, 0, tileWidth - 2, tileHeight - 2),
                                  m2::RectU(1, 1, tileWidth - 1, tileHeight - 1));
        }
      }
    }

    m_infoLayer.draw(pDrawer->screen().get(),
                     math::Identity<double, 3>());

    m_informationDisplay.doDraw(pDrawer);

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
