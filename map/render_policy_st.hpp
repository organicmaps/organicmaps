#pragma once

#include "render_policy.hpp"
#include "drawer_yg.hpp"

class VideoTimer;

class RenderPolicyST : public RenderPolicy
{
public:
  RenderPolicyST(VideoTimer * videoTimer,
                 DrawerYG::Params const & params,
                 shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);
};
