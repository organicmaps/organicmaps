#pragma once

#include "render_policy.hpp"
#include "../geometry/screenbase.hpp"

class TestRenderPolicy : public RenderPolicy
{
private:
  shared_ptr<graphics::gl::FrameBuffer> m_primaryFrameBuffer;
  shared_ptr<graphics::gl::FrameBuffer> m_auxFrameBuffer;
  shared_ptr<graphics::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<graphics::gl::RenderBuffer> m_depthBuffer;
  shared_ptr<graphics::gl::BaseTexture> m_actualTarget;
  shared_ptr<graphics::gl::BaseTexture> m_backBuffer;

  shared_ptr<graphics::Screen> m_auxScreen;
  shared_ptr<graphics::Screen> m_drawerScreen;

  bool m_hasScreen;
  ScreenBase m_screen;
public:
  TestRenderPolicy(Params const & p);

  void DrawFrame(shared_ptr<PaintEvent> const & pe,
                 ScreenBase const & screenBase);

  m2::RectI const OnSize(int w, int h);
};
