#pragma once

#include "render_policy.hpp"
#include "../geometry/screenbase.hpp"

class TestRenderPolicy : public RenderPolicy
{
private:
  shared_ptr<yg::gl::FrameBuffer> m_primaryFrameBuffer;
  shared_ptr<yg::gl::FrameBuffer> m_auxFrameBuffer;
  shared_ptr<yg::gl::FrameBuffer> m_frameBuffer;
  shared_ptr<yg::gl::RenderBuffer> m_depthBuffer;
  shared_ptr<yg::gl::BaseTexture> m_actualTarget;
  shared_ptr<yg::gl::BaseTexture> m_backBuffer;

  shared_ptr<yg::gl::Screen> m_auxScreen;
  shared_ptr<yg::gl::Screen> m_drawerScreen;

  bool m_hasScreen;
  ScreenBase m_screen;
public:
  TestRenderPolicy(Params const & p);

  void DrawFrame(shared_ptr<PaintEvent> const & pe,
                 ScreenBase const & screenBase);

  m2::RectI const OnSize(int w, int h);
};
