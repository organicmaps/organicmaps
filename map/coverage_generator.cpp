#include "../base/SRC_FIRST.hpp"

#include "coverage_generator.hpp"
#include "render_queue.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

ScreenCoverage::ScreenCoverage()
{}

struct unlock_synchronized
{
  TileCache * m_c;

  unlock_synchronized(TileCache * c) : m_c(c)
  {}

  void operator()(Tile const * t)
  {
    m_c->lock();
    m_c->unlockTile(t->m_rectInfo);
    m_c->unlock();
  }
};

void ScreenCoverage::Clear()
{
  m_tileCache->lock();

  for (unsigned i = 0; i < m_tiles.size(); ++i)
    m_tileCache->unlockTile(m_tiles[i]->m_rectInfo);

  m_tileCache->unlock();

  m_tiles.clear();
  m_infoLayer.clear();
}

CoverageGenerator::CoverageTask::CoverageTask(ScreenBase const & screen)
  : m_screen(screen)
{}

void CoverageGenerator::CoverageTask::execute(CoverageGenerator * generator)
{
  unsigned tilesCount = 0;
  generator->m_tiler.seed(m_screen, m_screen.GlobalRect().Center());
  generator->m_sequenceID++;

  {
    threads::MutexGuard g(generator->m_mutex);
    LOG(LINFO, ("clearing workCoverage"));
    generator->m_workCoverage->Clear();
  }

  generator->m_workCoverage->m_screen = m_screen;

  while (generator->m_tiler.hasTile())
  {
    ++tilesCount;
    Tiler::RectInfo rectInfo = generator->m_tiler.nextTile();

    TileCache & tileCache = generator->m_renderQueue->GetTileCache();

    tileCache.lock();

    if (tileCache.hasTile(rectInfo))
    {
      tileCache.lockTile(rectInfo);

      Tile const * tile = &tileCache.getTile(rectInfo);

      shared_ptr<Tile const> lockedTile = make_shared_ptr(tile, unlock_synchronized(&tileCache));

      generator->m_workCoverage->m_tiles.push_back(lockedTile);
      generator->m_workCoverage->m_infoLayer.merge(*tile->m_infoLayer.get(),
                                                    tile->m_tileScreen.PtoGMatrix() * generator->m_workCoverage->m_screen.GtoPMatrix());

      tileCache.unlock();
    }
    else
    {
      tileCache.unlock();
      generator->m_renderQueue->AddCommand(
            generator->m_renderFn,
            rectInfo,
            generator->m_sequenceID,
            bind(&CoverageGenerator::AddMergeTileTask, generator, _1, _2));
    }
  }

  {
    threads::MutexGuard g(generator->Mutex());
    swap(generator->m_currentCoverage, generator->m_workCoverage);
  }
}

CoverageGenerator::MergeTileTask::MergeTileTask(shared_ptr<Tile const> const & tile)
  : m_tile(tile)
{}

void CoverageGenerator::MergeTileTask::execute(CoverageGenerator * generator)
{
  {
    threads::MutexGuard g(generator->m_mutex);
    LOG(LINFO, ("making a working copy of currentCoverage"));
    *generator->m_mergeCoverage = *generator->m_currentCoverage;
  }

  LOG(LINFO, (generator->m_mergeCoverage->m_tiles.size()));

  generator->m_mergeCoverage->m_tiles.push_back(m_tile);
  generator->m_mergeCoverage->m_infoLayer.merge(*m_tile->m_infoLayer.get(),
                                                 m_tile->m_tileScreen.PtoGMatrix() * generator->m_mergeCoverage->m_screen.GtoPMatrix());

  {
    threads::MutexGuard g(generator->m_mutex);
    LOG(LINFO, ("saving merged coverage into workCoverage"));
    *generator->m_workCoverage = *generator->m_mergeCoverage;
    LOG(LINFO, ("saving merged coverage into currentCoverage"));
    *generator->m_currentCoverage = *generator->m_mergeCoverage;
  }

  generator->m_renderQueue->Invalidate();
}

CoverageGenerator::CoverageGenerator(
  size_t tileSize,
  size_t scaleEtalonSize,
  RenderPolicy::TRenderFn renderFn,
  RenderQueue * renderQueue)
  : m_tiler(tileSize, scaleEtalonSize),
    m_sequenceID(0),
    m_renderFn(renderFn),
    m_renderQueue(renderQueue),
    m_workCoverage(new ScreenCoverage()),
    m_mergeCoverage(new ScreenCoverage()),
    m_currentCoverage(new ScreenCoverage())
{
  m_routine = new Routine(this);
}

void CoverageGenerator::Initialize()
{
  m_thread.Create(m_routine);
}

CoverageGenerator::~CoverageGenerator()
{
  delete m_workCoverage;
  delete m_mergeCoverage;
  delete m_currentCoverage;
}

void CoverageGenerator::Cancel()
{
  m_thread.Cancel();
}

void CoverageGenerator::AddCoverageTask(ScreenBase const & screen)
{
  if (screen == m_currentScreen)
    return;
  m_currentScreen = screen;
  m_tasks.PushBack(make_shared_ptr(new CoverageTask(screen)));
}

void CoverageGenerator::AddMergeTileTask(Tiler::RectInfo const & rectInfo, Tile const &)
{
  TileCache & tileCache = m_renderQueue->GetTileCache();
  tileCache.lock();

  if (tileCache.hasTile(rectInfo))
  {
    tileCache.lockTile(rectInfo);

    Tile const * tile = &tileCache.getTile(rectInfo);

    shared_ptr<Tile const> lockedTile = make_shared_ptr(tile, unlock_synchronized(&tileCache));

    tileCache.unlock();

    m_tasks.PushBack(make_shared_ptr(new MergeTileTask(lockedTile)));
  }
  else
    tileCache.unlock();
}

CoverageGenerator::Routine::Routine(CoverageGenerator * parent)
  : m_parent(parent)
{}

void CoverageGenerator::Routine::Do()
{
  while (!IsCancelled())
  {
    shared_ptr<Task> task = m_parent->m_tasks.Front(true);

    if (m_parent->m_tasks.IsCancelled())
      break;

    task->execute(m_parent);
  }
}

threads::Mutex & CoverageGenerator::Mutex()
{
  return m_mutex;
}

ScreenCoverage * CoverageGenerator::CurrentCoverage()
{
  return m_currentCoverage;
}
