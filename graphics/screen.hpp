#pragma once

#include "overlay_renderer.hpp"

namespace graphics
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
