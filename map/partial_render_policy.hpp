#pragma once

#include "render_policy_mt.hpp"
#include "../yg/screen.hpp"
#include "../base/threaded_list.hpp"

class WindowHandle;

class PartialRenderPolicy : public RenderPolicyMT
{
private:

  ThreadedList<yg::gl::Renderer::Packet> m_glQueue;
  threads::Condition m_glCondition;

  yg::gl::Renderer::Packet m_currentPacket;
  shared_ptr<yg::gl::Screen::BaseState> m_curState;
  bool m_hasPacket;

  shared_ptr<yg::gl::Renderer::BaseState> m_state;

  void ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue);

public:

  PartialRenderPolicy(VideoTimer * videoTimer,
                      DrawerYG::Params const & params,
                      shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);

  void EndFrame(shared_ptr<PaintEvent> const & paintEvent,
                ScreenBase const & screenBase);

  bool NeedRedraw() const;
};
