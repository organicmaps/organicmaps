#pragma once

#include "basic_render_policy.hpp"

class RenderPolicyMT : public BasicRenderPolicy
{
public:

  RenderPolicyMT(VideoTimer * videoTimer,
                 bool useDefaultFB,
                 yg::ResourceManager::Params const & rmParams,
                 shared_ptr<yg::gl::RenderContext> const & primaryRC);


};
