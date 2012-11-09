#pragma once

#include "geometry_batcher.hpp"

namespace graphics
{
  namespace gl
  {
    class AreaRenderer : public GeometryBatcher
    {
    private:

      unsigned m_trianglesCount;
      unsigned m_areasCount;
      bool m_drawAreas;

    public:

      typedef GeometryBatcher base_t;

      struct Params : base_t::Params
      {
        bool m_drawAreas;
        Params();
      };

      AreaRenderer(Params const & params);

      /// drawing triangles list. assuming that each 3 points compose a triangle
      void drawTrianglesList(m2::PointD const * points,
                            size_t pointsCount,
                            uint32_t styleID,
                            double depth);

      void drawTrianglesFan(m2::PointF const * points,
                            size_t pointsCount,
                            uint32_t styleID,
                            double depth);

      void beginFrame();
      void endFrame();
    };
  }
}
