#pragma once

#include "graphics/shape_renderer.hpp"
#include "graphics/defines.hpp"

namespace graphics
{
  class ImageRenderer : public ShapeRenderer
  {
  private:
  public:

    typedef ShapeRenderer base_t;

    ImageRenderer(base_t::Params const & p);

    void drawImage(math::Matrix<double, 3, 3> const & m,
                   uint32_t resID,
                   double depth);
  };
}
