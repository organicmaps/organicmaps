#pragma once

#include "basic_render_policy.hpp"

class RenderPolicyST : public BasicRenderPolicy
{
public:
  RenderPolicyST(VideoTimer * videoTimer,
                 bool useDefaultFB,
                 yg::ResourceManager::Params const & rmParams,
                 shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~RenderPolicyST();
};
