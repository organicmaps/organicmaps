#pragma once

#include "path_renderer.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/any_rect2d.hpp"

namespace graphics
{
  namespace gl
  {
    class ShapeRenderer : public PathRenderer
    {
    private:
      typedef PathRenderer base_t;
    public:

      ShapeRenderer(base_t::Params const & params);

      void drawConvexPolygon(m2::PointF const * points,
                             size_t pointsCount,
                             graphics::Color const & color,
                             double depth);

      static void approximateArc(m2::PointD const & center,
                                 double startA,
                                 double endA,
                                 double r,
                                 vector<m2::PointD> & pts);

      void drawArc(m2::PointD const & center,
                   double startA,
                   double endA,
                   double r,
                   graphics::Color const & c,
                   double depth);

      void drawSector(m2::PointD const & center,
                      double startA,
                      double endA,
                      double r,
                      graphics::Color const & c,
                      double depth);

      void fillSector(m2::PointD const & center,
                      double startA,
                      double endA,
                      double r,
                      graphics::Color const & c,
                      double depth);

      void drawRectangle(m2::AnyRectD const & r,
                         graphics::Color const & c,
                         double depth);

      void drawRectangle(m2::RectD const & r,
                         graphics::Color const & c,
                         double depth);

      void drawRoundedRectangle(m2::RectD const & r,
                                double rad,
                                graphics::Color const & c,
                                double depth);
    };
  }
}
