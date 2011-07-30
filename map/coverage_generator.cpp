#include "../base/SRC_FIRST.hpp"

#include "coverage_generator.hpp"
#include "render_queue.hpp"

#include "../std/bind.hpp"

CoverageGenerator::CoverageGenerator(
  size_t tileSize,
  size_t scaleEtalonSize,
  RenderPolicy::TRenderFn renderFn,
  RenderQueue * renderQueue)
  : m_tiler(tileSize, scaleEtalonSize),
    m_sequenceID(0),
    m_renderFn(renderFn),
    m_renderQueue(renderQueue)
{
  m_routine = new Routine(this);
  m_thread.Create(m_routine);
}

void CoverageGenerator::Cancel()
{
  m_thread.Cancel();
}

void CoverageGenerator::AddCoverageTask(ScreenBase const & screen)
{
}

void CoverageGenerator::AddMergeTileTask(yg::Tiler::RectInfo const & rectInfo, yg::Tile const & tile)
{
}

CoverageGenerator::Routine::Routine(CoverageGenerator * parent)
  : m_parent(parent)
{}

void CoverageGenerator::Routine::Do()
{
  while (!IsCancelled())
  {
    ScreenBase screen = m_parent->m_tasks.Front(true).m_screen;

    if (m_parent->m_tasks.IsCancelled())
      break;

    m_parent->m_tiler.seed(screen, screen.GlobalRect().Center());
    m_parent->m_sequenceID++;

    m_parent->m_workCoverage->Clear();

    while (m_parent->m_tiler.hasTile())
    {
      yg::Tiler::RectInfo rectInfo = m_parent->m_tiler.nextTile();

      if (m_parent->m_renderQueue->TileCache().hasTile(rectInfo))

      m_parent->m_renderQueue->AddCommand(
            m_parent->m_renderFn,
            m_parent->m_tiler.nextTile(),
            m_parent->m_sequenceID,
            bind(&CoverageGenerator::AddMergeTileTask, m_parent, _1, _2));
    }

    {
      threads::MutexGuard g(m_parent->m_mutex);
      swap(m_parent->m_currentCoverage, m_parent->m_workCoverage);
    }

    m_parent->m_workCoverage->Clear();
  }
}
