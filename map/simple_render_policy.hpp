#pragma once

#include "render_policy.hpp"

class SimpleRenderPolicy : public RenderPolicy
{
public:
  SimpleRenderPolicy(VideoTimer * videoTimer,
                   bool useDefaultFB,
                    yg::ResourceManager::Params const & rmParams,
                    shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);
};
