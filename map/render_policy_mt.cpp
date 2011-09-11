#include "../base/SRC_FIRST.hpp"

#include "render_policy_mt.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "../yg/render_state.hpp"

#include "../geometry/transformations.hpp"

#include "../base/mutex.hpp"

#include "../platform/platform.hpp"

RenderPolicyMT::RenderPolicyMT(shared_ptr<WindowHandle> const & wh,
                               RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicy(wh, renderFn),
    m_renderQueue(GetPlatform().SkinName(),
                  false,
                  true,
                  0.1,
                  false,
                  GetPlatform().ScaleEtalonSize(),
                  GetPlatform().VisualScale(),
                  bgColor()),
    m_DoAddCommand(true)
{
  m_renderQueue.AddWindowHandle(wh);
}

void RenderPolicyMT::Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                                shared_ptr<yg::ResourceManager> const & rm)
{
  m_renderQueue.initializeGL(rc, rm);
  RenderPolicy::Initialize(rc, rm);
}

m2::RectI const RenderPolicyMT::OnSize(int w, int h)
{
  m_renderQueue.OnSize(w, h);

  m2::PointU pt = m_renderQueue.renderState().coordSystemShift();

  return m2::RectI(pt.x, pt.y, pt.x + w, pt.y + h);
}

void RenderPolicyMT::BeginFrame()
{
}

void RenderPolicyMT::EndFrame()
{
  m_renderQueue.renderState().m_mutex->Unlock();
}

void RenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  if (m_DoAddCommand && (s != m_renderQueue.renderState().m_actualScreen))
    m_renderQueue.AddCommand(renderFn(), s);

  DrawerYG * pDrawer = e->drawer();

  e->drawer()->screen()->clear(bgColor());

  m_renderQueue.renderState().m_mutex->Lock();

  if (m_renderQueue.renderState().m_actualTarget.get() != 0)
  {
    m2::PointD ptShift = m_renderQueue.renderState().coordSystemShift(false);

//    OGLCHECK(glMatrixMode(GL_MODELVIEW));
//    OGLCHECK(glPushMatrix());
//    OGLCHECK(glTranslatef(-ptShift.x, -ptShift.y, 0));

    math::Matrix<double, 3, 3> m = m_renderQueue.renderState().m_actualScreen.PtoGMatrix() * s.GtoPMatrix();
    m = math::Shift(m, -ptShift);

    pDrawer->screen()->blit(m_renderQueue.renderState().m_actualTarget,
                            m);
  }
}

void RenderPolicyMT::StartDrag(m2::PointD const & pt, double timeInSec)
{
  m_DoAddCommand = false;
  RenderPolicy::StartDrag(pt, timeInSec);
}

void RenderPolicyMT::StopDrag(m2::PointD const & pt, double timeInSec)
{
  m_DoAddCommand = true;
  RenderPolicy::StopDrag(pt, timeInSec);
}

void RenderPolicyMT::StartScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec)
{
  m_DoAddCommand = false;
  RenderPolicy::StartScale(pt1, pt2, timeInSec);
}

void RenderPolicyMT::StopScale(m2::PointD const & pt1, m2::PointD const & pt2, double timeInSec)
{
  m_DoAddCommand = true;
  RenderPolicy::StartScale(pt1, pt2, timeInSec);
}

RenderQueue & RenderPolicyMT::GetRenderQueue()
{
  return m_renderQueue;
}
