#pragma once

#include "geometry_batcher.hpp"
#include "../geometry/point2d.hpp"

namespace yg
{
  namespace gl
  {
    class PathRenderer : public GeometryBatcher
    {
    private:

      void drawSolidPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth);

    public:

      typedef GeometryBatcher base_t;

      PathRenderer(base_t::Params const & params);

      void drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth);
    };
  }
}
