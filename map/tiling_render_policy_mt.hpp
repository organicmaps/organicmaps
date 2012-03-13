#pragma once

#include "basic_tiling_render_policy.hpp"

namespace yg
{
  namespace gl
  {
    class RenderContext;
  }
  class ResourceManager;
}

class WindowHandle;

class TilingRenderPolicyMT : public BasicTilingRenderPolicy
{
public:

  TilingRenderPolicyMT(VideoTimer * videoTimer,
                       bool useDefaultFB,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~TilingRenderPolicyMT();

  void SetRenderFn(TRenderFn renderFn);
};
