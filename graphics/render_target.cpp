#include "graphics/render_target.hpp"
#include "graphics/coordinates.hpp"
#include "graphics/defines.hpp"

namespace graphics
{
  RenderTarget::~RenderTarget()
  {}

  void RenderTarget::coordMatrix(math::Matrix<float, 4, 4> & m)
  {
    getOrthoMatrix(m, 0, width(), 0, height(), minDepth, maxDepth + 100);
  }
}
