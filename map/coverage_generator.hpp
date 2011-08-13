#pragma once

#include "render_policy.hpp"
#include "tiler.hpp"
#include "tile.hpp"

#include "../geometry/screenbase.hpp"

#include "../yg/info_layer.hpp"

#include "../base/thread.hpp"
#include "../base/threaded_list.hpp"
#include "../base/mutex.hpp"

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

class RenderQueue;
class TileCache;

/// holds the tile coverage for a specific screen
struct ScreenCoverage
{
  TileCache * m_tileCache;
  ScreenBase m_screen;
  vector<shared_ptr<Tile const> > m_tiles;
  yg::InfoLayer m_infoLayer;

  bool m_IsRendering; //< while this flag is set - the object shouldn't
                      //be altered, 'cause GUI thread is rendering from it

  ScreenCoverage();

  void Merge(Tiler::RectInfo const & rectInfo, Tile const & tile);
  void Compute(ScreenBase const & screen);
  void Clear();
};

class CoverageGenerator
{
private:

  struct Task
  {
    virtual void execute(CoverageGenerator * generator) = 0;
  };

  struct CoverageTask : Task
  {
    ScreenBase m_screen;
    CoverageTask(ScreenBase const & screen);
    void execute(CoverageGenerator * generator);
  };

  struct MergeTileTask : Task
  {
    shared_ptr<Tile const> m_tile;
    MergeTileTask(shared_ptr<Tile const> const & tile);
    void execute(CoverageGenerator * generator);
  };

  ThreadedList<shared_ptr<Task> > m_tasks;

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

  Tiler m_tiler;

  RenderPolicy::TRenderFn m_renderFn;
  RenderQueue * m_renderQueue;

  ScreenCoverage * m_workCoverage;
  ScreenCoverage * m_mergeCoverage;
  ScreenCoverage * m_currentCoverage;

  ScreenBase m_currentScreen;
  size_t m_sequenceID;

public:

  CoverageGenerator(size_t tileSize,
             size_t scaleEtalonSize,
             RenderPolicy::TRenderFn renderFn,
             RenderQueue * renderQueue);

  ~CoverageGenerator();

  void AddCoverageTask(ScreenBase const & screen);

  void AddMergeTileTask(Tiler::RectInfo const & rectInfo, Tile const &);

  void Cancel();

  void Initialize();

  threads::Mutex & Mutex();

  ScreenCoverage * CurrentCoverage();
};
