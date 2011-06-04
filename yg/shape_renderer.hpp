#pragma once

#include "path_renderer.hpp"

namespace yg
{
  namespace gl
  {
    class ShapeRenderer : public PathRenderer
    {
    private:
      typedef PathRenderer base_t;
    public:

      ShapeRenderer(base_t::Params const & params);

      static void approximateArc(m2::PointD const & center, double startA, double endA, double r, vector<m2::PointD> & pts);
      void drawArc(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth);
      void drawSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth);
      void fillSector(m2::PointD const & center, double startA, double endA, double r, yg::Color const & c, double depth);

      void drawRectangle(m2::RectD const & r, yg::Color const & c, double depth);
    };
  }
}
