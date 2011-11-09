#include "partial_render_policy.hpp"
#include "events.hpp"
#include "window_handle.hpp"
#include "drawer_yg.hpp"
#include "../yg/internal/opengl.hpp"
#include "../std/bind.hpp"

PartialRenderPolicy::PartialRenderPolicy(shared_ptr<WindowHandle> const & wh,
                                         RenderPolicy::TRenderFn const & renderFn)
  : RenderPolicyMT(wh, renderFn)
{
  SetNeedSynchronize(true);
}

void PartialRenderPolicy::Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                                     shared_ptr<yg::ResourceManager> const & rm)
{
  m_renderQueue.SetGLQueue(&m_glQueue, &m_glCondition);
  RenderPolicyMT::Initialize(rc, rm);
}

void PartialRenderPolicy::ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue)
{
  if (renderQueue.empty())
  {
    m_hasPacket = false;
    m_glCondition.Signal();
  }
  else
  {
    m_hasPacket = true;
    m_currentPacket = renderQueue.front();
    renderQueue.pop_front();
  }
}

void PartialRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                                    ScreenBase const & screenBase)
{
  /// blitting from the current surface onto screen
//  RenderPolicyMT::DrawFrame(paintEvent, screenBase);

  yg::gl::Screen * screen = paintEvent->drawer()->screen().get();

  if (!m_state)
    m_state = screen->createState();

  screen->getState(m_state.get());

  m_curState = m_state;

  unsigned cmdProcessed = 0;
  unsigned const maxCmdPerFrame = 10;

  while (true)
  {
    m_glQueue.ProcessList(bind(&PartialRenderPolicy::ProcessRenderQueue, this, _1));

    if ((m_hasPacket) && (cmdProcessed < maxCmdPerFrame))
    {
      cmdProcessed++;
      if (m_currentPacket.m_state)
      {
        m_currentPacket.m_state->apply(m_curState.get());
        m_curState = m_currentPacket.m_state;
      }
      m_currentPacket.m_command->perform();
    }
    else
      break;
  }

  /// should we continue drawing commands on the next frame
  if ((cmdProcessed == maxCmdPerFrame) && m_hasPacket)
  {
    LOG(LINFO, ("will continue on the next frame(", cmdProcessed, ")"));
  }
  else
    LOG(LINFO, ("finished sequence of commands(", cmdProcessed, ")"));

  {
    threads::ConditionGuard guard(m_glCondition);
    if (m_glQueue.Empty())
      guard.Signal();
  }

  OGLCHECK(glFinish());

  m_state->apply(m_curState.get());

  OGLCHECK(glFinish());

  /// blitting from the current surface onto screen
  RenderPolicyMT::DrawFrame(paintEvent, screenBase);

  OGLCHECK(glFinish());

}

void PartialRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                                   ScreenBase const & screenBase)
{
  RenderPolicyMT::EndFrame(paintEvent, screenBase);
  LOG(LINFO, ("-------EndFrame-------"));
}

bool PartialRenderPolicy::NeedRedraw() const
{
  return !m_glQueue.Empty();
}
