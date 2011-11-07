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
  m_renderQueue.SetGLQueue(&m_glQueue);
  RenderPolicyMT::Initialize(rc, rm);
}

void PartialRenderPolicy::ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue)
{
  if (renderQueue.empty())
    m_hasPacket = false;
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

  shared_ptr<yg::gl::Renderer::BaseState> curState = m_state;

  unsigned cmdProcessed = 0;
  unsigned const maxCmdPerFrame = 1000;

  while (true)
  {
    m_glQueue.ProcessList(bind(&PartialRenderPolicy::ProcessRenderQueue, this, _1));

    if ((m_hasPacket) && (cmdProcessed < maxCmdPerFrame))
    {
      cmdProcessed++;
      if (m_currentPacket.m_state)
      {
        m_currentPacket.m_state->apply(curState.get());
        curState = m_currentPacket.m_state;
      }
      m_currentPacket.m_command->perform();
    }
    else
      break;
  }

  /// should we continue drawing commands on the next frame
  if ((cmdProcessed == maxCmdPerFrame) && m_hasPacket)
  {
    LOG(LINFO, ("will continue on the next frame"));
    windowHandle()->invalidate();
  }
  else
    LOG(LINFO, ("finished sequence of commands"));

//  OGLCHECK(glFinish());

  m_state->apply(curState.get());

//  LOG(LINFO, (cmdProcessed, " commands processed"));

//  OGLCHECK(glFinish());

  /// blitting from the current surface onto screen
  RenderPolicyMT::DrawFrame(paintEvent, screenBase);

//  OGLCHECK(glFinish());

bool PartialRenderPolicy::NeedRedraw() const
{
  return !m_glQueue.Empty();
}
