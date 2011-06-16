#pragma once

#include "overlay_renderer.hpp"

namespace yg
{
  namespace gl
  {
    class Screen : public OverlayRenderer
    {
    private:
    public:

      typedef OverlayRenderer::Params Params;

      Screen(Params const & params) : OverlayRenderer(params)
      {}
    };
  }
}
