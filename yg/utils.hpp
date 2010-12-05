#pragma once

#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"
#include "color.hpp"
#include "texture.hpp"

namespace yg
{
  namespace gl
  {
    namespace utils
    {
      void setupCoordinates(size_t width, size_t height, bool doSwap = false);
    }
  }
}
