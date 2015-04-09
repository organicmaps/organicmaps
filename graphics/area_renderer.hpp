#pragma once

#include "graphics/blitter.hpp"

namespace graphics
{
  class AreaRenderer : public Blitter
  {
  private:

    unsigned m_trianglesCount;
    unsigned m_areasCount;
    bool m_drawAreas;

  public:

    typedef Blitter base_t;

    struct Params : base_t::Params
    {
      bool m_drawAreas;
      Params();
    };

    AreaRenderer(Params const & params);

    /// drawing triangles list. assuming that each 3 points compose a triangle
    void drawTrianglesList(m2::PointD const * points,
                           size_t pointsCount,
                           uint32_t resID,
                           double depth);

    void drawTrianglesFan(m2::PointF const * points,
                          size_t pointsCount,
                          uint32_t resID,
                          double depth);

    void beginFrame();
    void endFrame();
  };
}
