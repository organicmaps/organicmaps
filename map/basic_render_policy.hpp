#pragma once

#include "render_policy.hpp"
#include "drawer_yg.hpp"

class VideoTimer;

class BasicRenderPolicy : public RenderPolicy
{
public:
  BasicRenderPolicy(VideoTimer * videoTimer,
                    bool useDefaultFB,
                    yg::ResourceManager::Params const & rmParams,
                    shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void DrawFrame(shared_ptr<PaintEvent> const & paintEvent,
                 ScreenBase const & screenBase);
};
