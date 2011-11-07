#pragma once

#include "render_policy_mt.hpp"
#include "../yg/screen.hpp"
#include "../base/threaded_list.hpp"

class WindowHandle;

class PartialRenderPolicy : public RenderPolicyMT
{
private:

  ThreadedList<yg::gl::Renderer::Packet> m_glQueue;

  yg::gl::Renderer::Packet m_currentPacket;
  bool m_hasPacket;

  shared_ptr<yg::gl::Renderer::BaseState> m_state;

  void ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue);

public:

  PartialRenderPolicy(shared_ptr<WindowHandle> const & wh,
                      RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                  shared_ptr<yg::ResourceManager> const & rm);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);

  bool NeedRedraw() const;
};
