#pragma once

#include "map/basic_tiling_render_policy.hpp"

namespace graphics
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

  TilingRenderPolicyMT(Params const & p);

  ~TilingRenderPolicyMT();

  void SetRenderFn(TRenderFn const & renderFn);
};
