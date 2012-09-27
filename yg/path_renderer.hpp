#pragma once

#include "area_renderer.hpp"
#include "../geometry/point2d.hpp"

namespace yg
{
  namespace gl
  {
    class PathRenderer : public AreaRenderer
    {
    private:

      unsigned m_pathCount;
      unsigned m_pointsCount;
      bool m_drawPathes;
      bool m_fastSolidPath;

      void drawFastSolidPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth);

    public:

      typedef AreaRenderer base_t;

      struct Params : base_t::Params
      {
        bool m_drawPathes;
        bool m_fastSolidPath;
        Params();
      };

      PathRenderer(Params const & params);

      void drawPath(m2::PointD const * points, size_t pointsCount, double offset, uint32_t styleID, double depth);

      void beginFrame();
      void endFrame();
    };
  }
}
