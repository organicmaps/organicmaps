#include "basic_render_policy.hpp"

#include "queued_renderer.hpp"
#include "render_queue.hpp"
#include "drawer_yg.hpp"
#include "events.hpp"

#include "../yg/render_state.hpp"

#include "../geometry/transformations.hpp"

BasicRenderPolicy::BasicRenderPolicy(Params const & p,
                                     bool doSupportRotation,
                                     size_t idCacheSize,
                                     shared_ptr<QueuedRenderer> const & queuedRenderer)
  : RenderPolicy(p, doSupportRotation, idCacheSize),
    m_QueuedRenderer(queuedRenderer),
    m_DoAddCommand(true)
{
}

void BasicRenderPolicy::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);
  m_RenderQueue->initializeGL(m_primaryRC, m_resourceManager);
}

void BasicRenderPolicy::SetEmptyModelFn(TEmptyModelFn checkFn)
{
  RenderPolicy::SetEmptyModelFn(checkFn);
  m_RenderQueue->SetEmptyModelFn(checkFn);
}

m2::RectI const BasicRenderPolicy::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);

  m_RenderQueue->OnSize(w, h);

  m2::PointU pt = m_RenderQueue->renderState().coordSystemShift();

  return m2::RectI(pt.x, pt.y, pt.x + w, pt.y + h);
}

void BasicRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & e,
                                   ScreenBase const & s)
{
  if (m_QueuedRenderer)
    m_QueuedRenderer->BeginFrame();
}

void BasicRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & e,
                                 ScreenBase const & s)
{
  m_RenderQueue->renderState().m_mutex->Unlock();

  if (m_QueuedRenderer)
    m_QueuedRenderer->EndFrame();
}

void BasicRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  if (m_QueuedRenderer)
  {
    m_QueuedRenderer->DrawFrame();
    m_resourceManager->updatePoolState();
  }

  bool doForceUpdate = DoForceUpdate();
  bool doIntersectInvalidRect = s.GlobalRect().IsIntersect(GetInvalidRect());

  m_RenderQueue->renderStatePtr()->m_doRepaintAll = doForceUpdate;

  if (m_DoAddCommand)
  {
    if (s != m_RenderQueue->renderState().m_actualScreen)
      m_RenderQueue->AddCommand(m_renderFn, s);
    else
      if (doForceUpdate && doIntersectInvalidRect)
        m_RenderQueue->AddCommand(m_renderFn, s);
  }

  SetForceUpdate(false);

  DrawerYG * pDrawer = e->drawer();

  e->drawer()->beginFrame();

  e->drawer()->screen()->clear(m_bgColor);

  m_RenderQueue->renderState().m_mutex->Lock();

  if (m_RenderQueue->renderState().m_actualTarget.get() != 0)
  {
    m2::PointD const ptShift = m_RenderQueue->renderState().coordSystemShift(false);

    math::Matrix<double, 3, 3> m = m_RenderQueue->renderState().m_actualScreen.PtoGMatrix() * s.GtoPMatrix();
    m = math::Shift(m, -ptShift);

    pDrawer->screen()->blit(m_RenderQueue->renderState().m_actualTarget, m);
  }

  e->drawer()->endFrame();
}

void BasicRenderPolicy::StartDrag()
{
  m_DoAddCommand = false;
  RenderPolicy::StartDrag();
}

void BasicRenderPolicy::StopDrag()
{
  m_DoAddCommand = true;
  RenderPolicy::StopDrag();
}

void BasicRenderPolicy::StartScale()
{
  m_DoAddCommand = false;
  RenderPolicy::StartScale();
}

void BasicRenderPolicy::StopScale()
{
  m_DoAddCommand = true;
  RenderPolicy::StopScale();
}

bool BasicRenderPolicy::IsEmptyModel() const
{
  return m_RenderQueue->renderStatePtr()->m_isEmptyModelActual;
}

RenderQueue & BasicRenderPolicy::GetRenderQueue()
{
  return *m_RenderQueue.get();
}

bool BasicRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  if (m_QueuedRenderer && m_QueuedRenderer->NeedRedraw())
    return true;

  return false;
}
