#include "../base/SRC_FIRST.hpp"

#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "tile_renderer.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

CoverageGenerator::CoverageGenerator(
  size_t tileSize,
  size_t scaleEtalonSize,
  TileRenderer * tileRenderer,
  shared_ptr<WindowHandle> const & windowHandle)
  : m_queue(1),
    m_tileRenderer(tileRenderer),
    m_workCoverage(0),
    m_currentCoverage(new ScreenCoverage(tileRenderer, this, tileSize, scaleEtalonSize)),
    m_sequenceID(0),
    m_windowHandle(windowHandle)
{
}

void CoverageGenerator::Initialize()
{
  m_queue.Start();
}

CoverageGenerator::~CoverageGenerator()
{
  delete m_workCoverage;
  delete m_currentCoverage;
}

void CoverageGenerator::Cancel()
{
  m_queue.Cancel();
}

void CoverageGenerator::AddCoverScreenTask(ScreenBase const & screen)
{
  if (screen == m_currentScreen)
    return;

  m_currentScreen = screen;
  m_sequenceID++;

  m_queue.AddCommand(bind(&CoverageGenerator::CoverScreen, this, screen, m_sequenceID));
}

void CoverageGenerator::CoverScreen(ScreenBase const & screen, int sequenceID)
{
  if (sequenceID < m_sequenceID)
    return;

  m_workCoverage = m_currentCoverage->Clone();
  m_workCoverage->SetScreen(screen);

  ScreenCoverage * oldCoverage = m_currentCoverage;

  {
    threads::MutexGuard g(m_mutex);
    m_currentCoverage = m_workCoverage;
  }

  delete oldCoverage;
  m_workCoverage = 0;
}

void CoverageGenerator::AddMergeTileTask(Tiler::RectInfo const & rectInfo)
{
  m_queue.AddCommand(bind(&CoverageGenerator::MergeTile, this, rectInfo));
}

void CoverageGenerator::MergeTile(Tiler::RectInfo const & rectInfo)
{
  m_workCoverage = m_currentCoverage->Clone();

  m_workCoverage->Merge(rectInfo);

  ScreenCoverage * oldCoverage = m_currentCoverage;

  {
    threads::MutexGuard g(m_mutex);
    m_currentCoverage = m_workCoverage;
  }

  delete oldCoverage;
  m_workCoverage = 0;

  m_windowHandle->invalidate();
}

void CoverageGenerator::WaitForEmptyAndFinished()
{
  m_queue.Join();
}

ScreenCoverage & CoverageGenerator::CurrentCoverage()
{
  return *m_currentCoverage;
}

threads::Mutex & CoverageGenerator::Mutex()
{
  return m_mutex;
}
