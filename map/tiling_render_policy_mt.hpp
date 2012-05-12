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

  TilingRenderPolicyMT(Params const & p);

  ~TilingRenderPolicyMT();

  void SetRenderFn(TRenderFn renderFn);
};
