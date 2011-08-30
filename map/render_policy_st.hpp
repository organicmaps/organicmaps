#pragma once

#include "render_policy.hpp"

class WindowHandle;

class RenderPolicyST : public RenderPolicy
{
private:
public:
  RenderPolicyST(shared_ptr<WindowHandle> const & wh,
                 RenderPolicy::TRenderFn const & renderFn);

  void Initialize(shared_ptr<yg::gl::RenderContext> const & rc,
                  shared_ptr<yg::ResourceManager> const & rm);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);
};
