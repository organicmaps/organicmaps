#pragma once

#include "render_policy.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/tiler.hpp"
#include "../yg/tile.hpp"
#include "../yg/info_layer.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

class RenderQueue;

/// holds the tile coverage for a specific screen
struct ScreenCoverage
{
  ScreenBase m_screen;
  vector<yg::Tile> m_tiles;
  shared_ptr<yg::InfoLayer> m_infoLayer;

  void Clear();
};

class CoverageGenerator
{
private:

  struct Task
  {
    enum EType
    {
      ECoverageTask,
      EMergeTileTask
    } m_type;

    int m_sequenceID;
    ScreenBase m_screen;
    yg::Tiler::RectInfo m_rectInfo;
    yg::Tile m_tile;
  };

  ThreadedList<Task> m_tasks;

  struct Routine : public threads::IRoutine
  {
    CoverageGenerator * m_parent;
    void Do();
    Routine(CoverageGenerator * parent);
  };

  friend struct Routine;

  Routine * m_routine;
  threads::Thread m_thread;

  threads::Mutex m_mutex;

  yg::Tiler m_tiler;

  size_t m_sequenceID;

  RenderPolicy::TRenderFn m_renderFn;
  RenderQueue * m_renderQueue;

  ScreenCoverage * m_workCoverage;
  ScreenCoverage * m_currentCoverage;

public:

  CoverageGenerator(size_t tileSize,
             size_t scaleEtalonSize,
             RenderPolicy::TRenderFn renderFn,
             RenderQueue * renderQueue);

  void AddCoverageTask(ScreenBase const & screen);

  void AddMergeTileTask(yg::Tiler::RectInfo const & rectInfo, yg::Tile const & tile);

  void Cancel();

  ScreenCoverage * CurrentCoverage();
};
