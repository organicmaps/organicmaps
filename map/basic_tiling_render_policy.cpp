#include "map/basic_tiling_render_policy.hpp"

#include "platform/platform.hpp"

#include "indexer/scales.hpp"

#include "map/tile_renderer.hpp"
#include "map/coverage_generator.hpp"
#include "map/queued_renderer.hpp"
#include "map/scales_processor.hpp"


BasicTilingRenderPolicy::BasicTilingRenderPolicy(Params const & p,
                                                 bool doUseQueuedRenderer)
  : RenderPolicy(p, GetPlatform().CpuCores() + 2),
    m_IsEmptyModel(false),
    m_IsNavigating(false),
    m_WasAnimatingLastFrame(false),
    m_DoRecreateCoverage(false)
{
  m_TileSize = ScalesProcessor::CalculateTileSize(p.m_screenWidth, p.m_screenHeight);

  LOG(LDEBUG, ("ScreenSize=", p.m_screenWidth, "x", p.m_screenHeight, ", TileSize=", m_TileSize));

  if (doUseQueuedRenderer)
    m_QueuedRenderer.reset(new QueuedRenderer(GetPlatform().CpuCores() + 1, p.m_primaryRC));
}

void BasicTilingRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  RenderPolicy::BeginFrame(e, s);

  if (m_QueuedRenderer)
    m_QueuedRenderer->BeginFrame();
}

void BasicTilingRenderPolicy::CheckAnimationTransition()
{
  // transition from non-animating to animating,
  // should stop all background work
  if (!m_WasAnimatingLastFrame && IsAnimating())
    PauseBackgroundRendering();

  // transition from animating to non-animating
  // should resume all background work
  if (m_WasAnimatingLastFrame && !IsAnimating())
    ResumeBackgroundRendering();

  m_WasAnimatingLastFrame = IsAnimating();
}

void BasicTilingRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
#ifndef USE_DRAPE
  if (m_QueuedRenderer)
  {
    m_QueuedRenderer->DrawFrame();
    m_resourceManager->updatePoolState();
  }

  CheckAnimationTransition();

  /// checking, whether we should add the CoverScreen command

  bool doForceUpdate = DoForceUpdate();
  bool doIntersectInvalidRect = GetInvalidRect().IsIntersect(s.GlobalRect());

  bool doForceUpdateFromGenerator = m_CoverageGenerator->DoForceUpdate();

  if (doForceUpdate)
    m_CoverageGenerator->InvalidateTiles(GetInvalidRect(), scales::GetUpperWorldScale() + 1);

  if (!m_IsNavigating && (!IsAnimating()))
    m_CoverageGenerator->CoverScreen(s,
                                     doForceUpdateFromGenerator
                                     || m_DoRecreateCoverage
                                     || (doForceUpdate && doIntersectInvalidRect));

  SetForceUpdate(false);
  m_DoRecreateCoverage = false;

  /// rendering current coverage

  graphics::Screen * pScreen = GPUDrawer::GetScreen(e->drawer());

  pScreen->beginFrame();
  pScreen->clear(m_bgColor);

  FrameLock();

  m_CoverageGenerator->Draw(pScreen, s);

  m_IsEmptyModel = m_CoverageGenerator->IsEmptyDrawing();

  pScreen->endFrame();
#endif // USE_DRAPE
}

void BasicTilingRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e, ScreenBase const & s)
{
  FrameUnlock();

  if (m_QueuedRenderer)
    m_QueuedRenderer->EndFrame();

  RenderPolicy::EndFrame(e, s);
}

TileRenderer & BasicTilingRenderPolicy::GetTileRenderer()
{
  return *m_TileRenderer.get();
}

void BasicTilingRenderPolicy::PauseBackgroundRendering()
{
  m_TileRenderer->SetIsPaused(true);
  m_TileRenderer->CancelCommands();
  m_CoverageGenerator->Pause();
  if (m_QueuedRenderer)
    m_QueuedRenderer->SetPartialExecution(GetPlatform().CpuCores(), true);
}

void BasicTilingRenderPolicy::ResumeBackgroundRendering()
{
  m_TileRenderer->SetIsPaused(false);
  m_CoverageGenerator->Resume();
  m_DoRecreateCoverage = true;
  if (m_QueuedRenderer)
    m_QueuedRenderer->SetPartialExecution(GetPlatform().CpuCores(), false);
}

void BasicTilingRenderPolicy::StartNavigation()
{
  m_IsNavigating = true;
  PauseBackgroundRendering();
}

void BasicTilingRenderPolicy::StopNavigation()
{
  m_IsNavigating = false;
  ResumeBackgroundRendering();
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

bool BasicTilingRenderPolicy::IsEmptyModel() const
{
  return m_IsEmptyModel;
}

bool BasicTilingRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  if (m_QueuedRenderer && m_QueuedRenderer->NeedRedraw())
    return true;

  return false;
}

size_t BasicTilingRenderPolicy::TileSize() const
{
  return m_TileSize;
}

void BasicTilingRenderPolicy::FrameLock()
{
  m_CoverageGenerator->Lock();
}

void BasicTilingRenderPolicy::FrameUnlock()
{
  m_CoverageGenerator->Unlock();
}

graphics::Overlay * BasicTilingRenderPolicy::FrameOverlay() const
{
  return m_CoverageGenerator->GetOverlay();
}

int BasicTilingRenderPolicy::InsertBenchmarkFence()
{
  return m_CoverageGenerator->InsertBenchmarkFence();
}

void BasicTilingRenderPolicy::JoinBenchmarkFence(int fenceID)
{
  return m_CoverageGenerator->JoinBenchmarkFence(fenceID);
}
