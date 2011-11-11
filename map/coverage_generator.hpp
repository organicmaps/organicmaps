#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile.hpp"
#include "window_handle.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/info_layer.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"
#include "../base/commands_queue.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

class TileRenderer;
class TileCache;
class ScreenCoverage;

class CoverageGenerator
{
private:

  core::CommandsQueue m_queue;

  TileRenderer * m_tileRenderer;

  shared_ptr<yg::ResourceManager> m_resourceManager;
  shared_ptr<yg::gl::RenderContext> m_renderContext;

  ScreenCoverage * m_workCoverage;
  ScreenCoverage * m_currentCoverage;

  shared_ptr<yg::StylesCache> m_workStylesCache;
  shared_ptr<yg::StylesCache> m_currentStylesCache;

  ScreenBase m_currentScreen;
  int m_sequenceID;

  shared_ptr<WindowHandle> m_windowHandle;

  threads::Mutex m_mutex;

public:

  CoverageGenerator(size_t tileSize,
                    size_t scaleEtalonSize,
                    TileRenderer * tileRenderer,
                    shared_ptr<WindowHandle> const & windowHandle,
                    shared_ptr<yg::gl::RenderContext> const & primaryRC,
                    shared_ptr<yg::ResourceManager> const & rm);

  ~CoverageGenerator();

  void InitializeThreadGL();
  void FinalizeThreadGL();

  void AddCoverScreenTask(ScreenBase const & screen);
  void AddMergeTileTask(Tiler::RectInfo const & rectInfo);

  void CoverScreen(ScreenBase const & screen, int sequenceID);
  void MergeTile(Tiler::RectInfo const & rectInfo);

  void Cancel();

  void WaitForEmptyAndFinished();

  ScreenCoverage & CurrentCoverage();

  threads::Mutex & Mutex();

  shared_ptr<yg::ResourceManager> const & resourceManager() const;
};
