#include "../base/SRC_FIRST.hpp"

#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "tile_renderer.hpp"

#include "../yg/rendercontext.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"

bool g_coverageGeneratorDestroyed = false;

CoverageGenerator::CoverageGenerator(
    size_t tileSize,
    size_t scaleEtalonSize,
    TileRenderer * tileRenderer,
    shared_ptr<WindowHandle> const & windowHandle,
    shared_ptr<yg::gl::RenderContext> const & primaryRC,
    shared_ptr<yg::ResourceManager> const & rm,
    yg::gl::PacketsQueue * glQueue)
  : m_queue(1),
    m_tileRenderer(tileRenderer),
    m_workCoverage(0),
    m_currentCoverage(new ScreenCoverage(tileRenderer, this, tileSize, scaleEtalonSize)),
    m_sequenceID(0),
    m_windowHandle(windowHandle)
{
  g_coverageGeneratorDestroyed = false;

  m_resourceManager = rm;
  m_renderContext = primaryRC->createShared();

  m_currentStylesCache.reset(new yg::StylesCache(rm,
                                                 rm->cacheThreadGlyphCacheID(),
                                                 glQueue));

  m_workStylesCache.reset(new yg::StylesCache(rm,
                                              rm->cacheThreadGlyphCacheID(),
                                              glQueue));

  m_queue.AddInitCommand(bind(&CoverageGenerator::InitializeThreadGL, this));
  m_queue.AddFinCommand(bind(&CoverageGenerator::FinalizeThreadGL, this));

  m_queue.Start();
}

void CoverageGenerator::InitializeThreadGL()
{
  m_renderContext->makeCurrent();
}

void CoverageGenerator::FinalizeThreadGL()
{
  m_renderContext->endThreadDrawing();
}

CoverageGenerator::~CoverageGenerator()
{
  LOG(LINFO, ("cancelling coverage thread"));
  Cancel();

  LOG(LINFO, ("deleting workCoverage"));
  delete m_workCoverage;

  LOG(LINFO, ("deleting currentCoverage"));
  delete m_currentCoverage;

  g_coverageGeneratorDestroyed = true;
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

  m_queue.Clear();
  m_sequenceID++;
  m_queue.CancelCommands();

  m_queue.AddCommand(bind(&CoverageGenerator::CoverScreen, this, screen, m_sequenceID));
}

void CoverageGenerator::CoverScreen(ScreenBase const & screen, int sequenceID)
{
  if (sequenceID < m_sequenceID)
    return;

  ASSERT(m_workCoverage == 0, ());

  m_workCoverage = m_currentCoverage->Clone();
  m_workCoverage->SetStylesCache(m_workStylesCache.get());
  m_workCoverage->SetScreen(screen);
  m_workCoverage->CacheInfoLayer();

  {
    threads::MutexGuard g(m_mutex);

    /// test that m_workCoverage->InfoLayer doesn't have
    /// the same elements as m_currentCoverage->InfoLayer
    ASSERT(!m_workCoverage->GetInfoLayer()->checkHasEquals(m_currentCoverage->GetInfoLayer()), ());
    ASSERT(m_workCoverage->GetInfoLayer()->checkCached(m_workStylesCache.get()), ());

    swap(m_currentCoverage, m_workCoverage);
    swap(m_currentStylesCache, m_workStylesCache);

    ASSERT(m_currentCoverage->GetStylesCache() == m_currentStylesCache.get(), ());
  }

  delete m_workCoverage;
  m_workCoverage = 0;

  m_windowHandle->invalidate();
}

void CoverageGenerator::AddMergeTileTask(Tiler::RectInfo const & rectInfo)
{
  if (g_coverageGeneratorDestroyed)
    return;

  m_queue.AddCommand(bind(&CoverageGenerator::MergeTile, this, rectInfo));
}

void CoverageGenerator::MergeTile(Tiler::RectInfo const & rectInfo)
{
  ASSERT(m_workCoverage == 0, ());

  m_workCoverage = m_currentCoverage->Clone();
  m_workCoverage->SetStylesCache(m_workStylesCache.get());
  m_workCoverage->Merge(rectInfo);
  m_workCoverage->CacheInfoLayer();

  {
    threads::MutexGuard g(m_mutex);

    /// test that m_workCoverage->InfoLayer doesn't have
    /// the same elements as m_currentCoverage->InfoLayer
    ASSERT(!m_workCoverage->GetInfoLayer()->checkHasEquals(m_currentCoverage->GetInfoLayer()), ());
    ASSERT(m_workCoverage->GetInfoLayer()->checkCached(m_workStylesCache.get()), ());

    swap(m_currentCoverage, m_workCoverage);
    swap(m_currentStylesCache, m_workStylesCache);

    ASSERT(m_currentCoverage->GetStylesCache() == m_currentStylesCache.get(), ());
  }

  /// we should delete workCoverage
  delete m_workCoverage;
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

shared_ptr<yg::ResourceManager> const & CoverageGenerator::resourceManager() const
{
  return m_resourceManager;
}
