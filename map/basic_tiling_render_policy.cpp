#include "basic_tiling_render_policy.hpp"

#include "../platform/platform.hpp"

#include "../indexer/scales.hpp"

#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "screen_coverage.hpp"
#include "queued_renderer.hpp"

BasicTilingRenderPolicy::BasicTilingRenderPolicy(Params const & p,
                                                 bool doSupportRotation,
                                                 bool doUseQueuedRenderer)
  : RenderPolicy(p, doSupportRotation, GetPlatform().CpuCores()),
    m_DrawScale(0),
    m_IsEmptyModel(false),
    m_DoRecreateCoverage(false),
    m_IsNavigating(false)
{
  /// calculating TileSize based on p.m_screenWidth and p.m_screenHeight
  /// ceiling screen sizes to the nearest power of two, and taking half of it as a tile size
  double const log2 = log(2.0);

  size_t ceiledScreenWidth = static_cast<int>(pow(2.0, ceil(log(double(p.m_screenWidth + 1)) / log2)));
  size_t ceiledScreenHeight = static_cast<int>(pow(2.0, ceil(log(double(p.m_screenHeight + 1)) / log2)));

  size_t ceiledScreenSize = max(ceiledScreenWidth, ceiledScreenHeight);

  m_TileSize = min(max(ceiledScreenSize / 2, (size_t)128), (size_t)1024);

  LOG(LINFO, ("ScreenSize=", p.m_screenWidth, "x", p.m_screenHeight, ", TileSize=", m_TileSize));

  if (doUseQueuedRenderer)
    m_QueuedRenderer.reset(new QueuedRenderer(GetPlatform().CpuCores() + 1));
}

void BasicTilingRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  if (m_QueuedRenderer)
    m_QueuedRenderer->BeginFrame();
}

void BasicTilingRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  if (m_QueuedRenderer)
  {
    m_QueuedRenderer->DrawFrame();
    m_resourceManager->updatePoolState();
  }

  /// checking, whether we should add the CoverScreen command

  bool doForceUpdate = DoForceUpdate();
  bool doIntersectInvalidRect = GetInvalidRect().IsIntersect(s.GlobalRect());

  if (doForceUpdate)
    m_CoverageGenerator->InvalidateTiles(GetInvalidRect(), scales::GetUpperWorldScale() + 1);

  if (!m_IsNavigating)
    m_CoverageGenerator->AddCoverScreenTask(s,
                                            m_DoRecreateCoverage || (doForceUpdate && doIntersectInvalidRect));

  SetForceUpdate(false);
  m_DoRecreateCoverage = false;

  /// rendering current coverage

  DrawerYG * pDrawer = e->drawer();

  pDrawer->beginFrame();

  pDrawer->screen()->clear(m_bgColor);

  m_CoverageGenerator->Mutex().Lock();

  ScreenCoverage * curCvg = &m_CoverageGenerator->CurrentCoverage();

  curCvg->Draw(pDrawer->screen().get(), s);

  m_DrawScale = curCvg->GetDrawScale();

  if (!curCvg->IsEmptyDrawingCoverage() || !curCvg->IsPartialCoverage())
  {
    m_IsEmptyModel = curCvg->IsEmptyDrawingCoverage() && curCvg->IsEmptyModelAtCoverageCenter();
    if (m_IsEmptyModel)
      m_CountryName = curCvg->GetCountryNameAtCoverageCenter();
  }

  pDrawer->endFrame();
}

void BasicTilingRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  m_CoverageGenerator->Mutex().Unlock();

  if (m_QueuedRenderer)
    m_QueuedRenderer->EndFrame();
}

TileRenderer & BasicTilingRenderPolicy::GetTileRenderer()
{
  return *m_TileRenderer.get();
}

void BasicTilingRenderPolicy::StartNavigation()
{
  m_TileRenderer->SetIsPaused(true);
  m_IsNavigating = true;
  m_TileRenderer->CancelCommands();
}

void BasicTilingRenderPolicy::StopNavigation()
{
  m_TileRenderer->SetIsPaused(false);
  m_IsNavigating = false;
  m_DoRecreateCoverage = true;
}

void BasicTilingRenderPolicy::StartScale()
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopScale()
{
  StopNavigation();
  RenderPolicy::StopScale();
}

void BasicTilingRenderPolicy::StartDrag()
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopDrag()
{
  StopNavigation();
  RenderPolicy::StopDrag();
}

void BasicTilingRenderPolicy::StartRotate(double a, double timeInSec)
{
  StartNavigation();
}

void BasicTilingRenderPolicy::StopRotate(double a, double timeInSec)
{
  StopNavigation();
  RenderPolicy::StopRotate(a, timeInSec);
}

bool BasicTilingRenderPolicy::IsTiling() const
{
  return true;
}

int BasicTilingRenderPolicy::GetDrawScale(ScreenBase const & s) const
{
  return m_DrawScale;
}

bool BasicTilingRenderPolicy::IsEmptyModel() const
{
  return m_IsEmptyModel;
}

string const BasicTilingRenderPolicy::GetCountryName() const
{
  return m_CountryName;
}

bool BasicTilingRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  if (m_QueuedRenderer && m_QueuedRenderer->NeedRedraw())
    return true;

  return false;
}

size_t BasicTilingRenderPolicy::ScaleEtalonSize() const
{
  return m_TileSize;
}

size_t BasicTilingRenderPolicy::TileSize() const
{
  return m_TileSize;
}

int BasicTilingRenderPolicy::InsertBenchmarkFence()
{
  return m_CoverageGenerator->InsertBenchmarkFence();
}

void BasicTilingRenderPolicy::JoinBenchmarkFence(int fenceID)
{
  return m_CoverageGenerator->JoinBenchmarkFence(fenceID);
}
